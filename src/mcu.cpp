#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_opengl.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "imgui_memory_editor.h"
#include "mcu.h"
#include "mcu_interrupt.h"
#include "mcu_opcodes.h"
#include "utf8main.h"
#include "lcd.h"

static const int ROM_SIZE = 0x80000;
static const int RAM_SIZE = 0x800;
static const int SRAM_SIZE = 0x10000;

void MCU_ErrorTrap(void) { printf("ERROR %.2x %.4x\n", mcu.cp, mcu.pc); }

mcu_t mcu;
uint8_t dev_register[0x100];

uint16_t ad_val[4];
uint8_t ad_nibble = 0x00;
int adf_rd = 0;
uint64_t analog_end_time;

void MCU_AnalogSample(int channel) {
  int value = 0x2a0; // battery
  int dest = (channel << 1) & 6;
  dev_register[DEV_ADDRAH + dest] = value >> 2;
  dev_register[DEV_ADDRAL + dest] = (value << 6) & 0xc0;
}

void MCU_UpdateAnalog(uint64_t cycles) {
  int ctrl = dev_register[DEV_ADCSR];
  int isscan = (ctrl & 16) != 0;

  if (ctrl & 0x20) {
    if (analog_end_time == 0)
      analog_end_time = cycles + 200;
    else if (analog_end_time < cycles) {
      if (isscan) {
        int base = ctrl & 4;
        for (int i = 0; i <= (ctrl & 3); i++)
          MCU_AnalogSample(base + i);
        analog_end_time = cycles + 200;
      } else {
        MCU_AnalogSample(ctrl & 7);
        dev_register[DEV_ADCSR] &= ~0x20;
        analog_end_time = 0;
      }
      dev_register[DEV_ADCSR] |= 0x80;
      if (ctrl & 0x40)
        MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ADI, 1);
    }
  } else
    analog_end_time = 0;
}

uint8_t rom[ROM_SIZE];
uint8_t ram[RAM_SIZE];
uint8_t sram[SRAM_SIZE];
uint8_t unk_e8_buf[SRAM_SIZE];
int rom_mask = ROM_SIZE - 1;
uint16_t isp_dr[32] = {0};

uint8_t csp1[0x4000];
uint8_t csp2[0x4000];

LCD lcd;

void dump() {
  FILE *f;

  f = fopen("jd990_ram.bin", "wb");
  fwrite(ram, 1, RAM_SIZE, f);
  fclose(f);

  f = fopen("jd990_sram.bin", "wb");
  fwrite(sram, 1, SRAM_SIZE, f);
  fclose(f);

  f = fopen("jd990_csp1.bin", "wb");
  fwrite(csp1, 1, 0x4000, f);
  fclose(f);

  f = fopen("jd990_csp2.bin", "wb");
  fwrite(csp2, 1, 0x4000, f);
  fclose(f);

  f = fopen("jd990_lcd.bin", "wb");
  fwrite(lcd.memory, 1, 0x2000, f);
  fclose(f);
}

// 0x0f680 -> 0x0fe7f onchip ram
// 0x0fe80 -> 0x0ff7f onchip i/o
// 0x0ff80 -> 0x0ffff 8 bit
// 0xf0000 -> 0xffe7f 8 bit
// 0xffe80 -> 0xfff7f onchip i/o
// 0xfff80 -> 0xfffff 8 bit

uint8_t MCU_Read(uint32_t address, bool code) {
  uint8_t page = address >> 16;
  uint32_t address_low = address & 0xffff;

  // cpu dev
  if ((page == 0x00 || page == 0xff) &&
      (address_low >= 0xfe80 && address_low <= 0xff7f)) {
    int devSlot = address - 0xfe80;

    if (devSlot == DEV_ADCSR) {
      adf_rd = (dev_register[devSlot] & 0x80) != 0;
      return dev_register[devSlot];
    }
    
    else if (address_low == 0xfe91) // P6 (GA?)
      return 0x00;
    else if (address_low == 0xfe94) // P9 (pcm card present?)
      return 0x00;
    else if (address_low == 0xfe96) // P11 (exp board present)
      return 0x00;
    
    else if (address_low >= 0xfec0 && address_low <= 0xfeff) { // ISP dr
      printf("isp read dr %02x\n", (address_low >> 1) & 31);
      if ((address_low & 1) == 1)
        return isp_dr[(address_low - 0xfec0) >> 1] & 0xff;
      else
        return (isp_dr[(address_low - 0xfec0) >> 1] >> 8) & 0xff;
    }
    
    else {
      printf("dev read %06x\n", address);
      return dev_register[devSlot];
    }
  }

  // cpu ram
  else if (address >= 0x0f680 && address <= 0x0fe7f) {
    return ram[address_low - 0xf680];
  }

  // sram
  else if (page == 0x8 || (address >= 0x08000 && address <= 0x0ffff)) {
    return sram[address & 0xffff];
  }

  // rom
  else if (page < 0x8) {
    return rom[address & rom_mask];
  }

  // ep
  else if (page == 0xe && address_low < 0x0080) {
    printf("ep read %06x\n", address);
    return 0x00;
  }

  // tvf
  else if (page == 0xe && address_low < 0x407f) {
    printf("tvf read %06x\n", address);
    return 0x00;
  }

  // unk
  else if (page == 0xe && address_low >= 0x8000) {
    printf("unk_e8 read %06x\n", address);
    return unk_e8_buf[address_low - 0x8000];
  }

  // csp1
  else if (page == 0xf && address_low < 0x4000) {
    printf("csp1 read %06x\n", address);
    return 0x01;
  }

  // csp2
  else if (page == 0xf && address_low < 0x8000) {
    printf("csp2 read %06x\n", address);
    return 0x01;
  }

  // lcd 0
  else if (page == 0xf && address_low == 0xa000) {
    printf("lcd 0 read %06x\n", address);
    return 0x00;
  }
  // lcd 1
  else if (page == 0xf && address_low == 0xa002) {
    printf("lcd 1 read %06x\n", address);
    return 0x00;
  }

  else {
    printf("unknown read %06x\n", address);
  }

  return 0xff;
}

void MCU_Write(uint32_t address, uint8_t value) {
  uint8_t page = address >> 16;
  uint32_t address_low = address & 0xffff;

  // cpu dev
  if ((page == 0x00 || page == 0xff) &&
      (address_low >= 0xfe80 && address_low <= 0xff7f)) {
    int devSlot = address - 0xfe80;
    
    if (devSlot == DEV_ADCSR) {
      dev_register[devSlot] &= ~0x7f;
      dev_register[devSlot] |= value & 0x7f;
      if ((value & 0x80) == 0 && adf_rd) {
        dev_register[devSlot] &= ~0x80;
        MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ADI, 0);
      }
      if ((value & 0x40) == 0)
        MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ADI, 0);
    }
    
    else if (address_low >= 0xfec0 && address_low <= 0xfeff) { // ISP dr
      if ((address_low & 1) == 1)
        isp_dr[(address_low - 0xfec0) >> 1] = (isp_dr[(address_low - 0xfec0) >> 1] & 0xff00) | value;
      else
        isp_dr[(address_low - 0xfec0) >> 1] = (isp_dr[(address_low - 0xfec0) >> 1] & 0x00ff) | (value << 8);
      printf("isp write dr %02x = %04x\n", (address_low >> 1) & 31, isp_dr[(address_low - 0xfec0) >> 1]);
    }
    
    else {
      printf("dev write %06x = %02x\n", address, value);
      dev_register[devSlot] = value;
    }
  }

  // cpu ram
  else if (address >= 0x0f680 && address <= 0x0fe7f) {
    ram[address_low - 0xf680] = value;
  }

  // sram
  else if (page == 0x8 || (address >= 0x08000 && address <= 0x0ffff)) {
    sram[address & 0xffff] = value;
  }

  // ep
  else if (page == 0xe && address_low < 0x0080) {
    printf("ep write %06x = %02x\n", address, value);
  }

  // tvf
  else if (page == 0xe && address_low < 0x407f) {
    printf("tvf write %06x = %02x\n", address, value);
  }

  // unk
  else if (page == 0xe && address_low >= 0x8000) {
    printf("unk_e8 write %06x = %02x\n", address, value);
    unk_e8_buf[address_low - 0x8000] = value;
  }

  // csp1
  else if (page == 0xf && address_low < 0x4000) {
    printf("csp1 write %06x = %02x\n", address, value);
    csp1[address_low & 0x3fff] = value;
  }

  // csp2
  else if (page == 0xf && address_low < 0x8000) {
    printf("csp2 write %06x = %02x\n", address, value);
    csp2[address_low & 0x3fff] = value;
  }

  // lcd data
  else if (page == 0xf && address_low == 0xa000) {
    // printf("lcd data write %06x = %02x %c\n", address, value, value);
    lcd.writeData(value);
  }
  // lcd param
  else if (page == 0xf && address_low == 0xa002) {
    // printf("lcd param write %06x = %02x\n", address, value);
    lcd.writeParam(value);
  }

  else {
    printf("unknown write %06x = %02x\n", address, value);
  }
}

uint16_t MCU_Read16(uint32_t address, bool code) {
  address &= ~1;
  uint8_t b0, b1;
  b0 = MCU_Read(address, code);
  b1 = MCU_Read(address + 1, code);
  return (b0 << 8) + b1;
}

uint32_t MCU_Read32(uint32_t address, bool code) {
  address &= ~3;
  uint8_t b0, b1, b2, b3;
  b0 = MCU_Read(address, code);
  b1 = MCU_Read(address + 1, code);
  b2 = MCU_Read(address + 2, code);
  b3 = MCU_Read(address + 3, code);
  return (b0 << 24) + (b1 << 16) + (b2 << 8) + b3;
}

void MCU_Write16(uint32_t address, uint16_t value) {
  address &= ~1;
  MCU_Write(address, value >> 8);
  MCU_Write(address + 1, value & 0xff);
}

void MCU_ReadInstruction(void) {
  uint8_t operand = MCU_ReadCodeAdvance();

  MCU_Operand_Table[operand](operand);

  if (mcu.sr & STATUS_T) {
    MCU_Interrupt_Exception(EXCEPTION_SOURCE_TRACE);
  }
}

void MCU_Init(void) { memset(&mcu, 0, sizeof(mcu_t)); }

void MCU_Reset(void) {
  mcu.r[0] = 0;
  mcu.r[1] = 0;
  mcu.r[2] = 0;
  mcu.r[3] = 0;
  mcu.r[4] = 0;
  mcu.r[5] = 0;
  mcu.r[6] = 0;
  mcu.r[7] = 0;

  mcu.pc = 0;

  mcu.sr = 0x700;

  mcu.cp = 0;
  mcu.dp = 0;
  mcu.ep = 0;
  mcu.tp = 0;
  mcu.br = 0;

  uint32_t reset_address = MCU_GetVectorAddress(VECTOR_RESET);
  mcu.cp = (reset_address >> 16) & 0xff;
  mcu.pc = reset_address & 0xffff;

  mcu.exception_pending = -1;

  memset(dev_register, 0, sizeof(dev_register));
}

SDL_Window *window = 0;
SDL_GLContext glctx;
void initImgui() {
  // request a GL 3.0+ context
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  window = SDL_CreateWindow(
      "debug ui", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  glctx = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, glctx);
  SDL_GL_SetSwapInterval(1); // vsync

  // imgui core
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui::StyleColorsDark();

  // backends
  ImGui_ImplSDL2_InitForOpenGL(window, glctx);
  ImGui_ImplOpenGL3_Init(
      "#version 150"); // or 150/330 etc depending on platform
}

bool running = true;
static MemoryEditor sram_view, unk_e8_buf_view, lcd_ram_view;

void renderImgui() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    ImGui_ImplSDL2_ProcessEvent(&e);
    if (e.type == SDL_QUIT)
      running = false;
    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE &&
        e.window.windowID == SDL_GetWindowID(window))
      running = false;
  }

  // start imgui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  sram_view.DrawWindow("SRAM", sram, SRAM_SIZE);
  unk_e8_buf_view.DrawWindow("unk_e8_buf", unk_e8_buf, SRAM_SIZE);
  lcd_ram_view.DrawWindow("LCD RAM", lcd.memory, 0x2000);

  ImGui::Begin("LCD Params");
  ImGui::Text("scroll_sad1: %04x", lcd.scroll_sad1);
  ImGui::Text("scroll_sad2: %04x", lcd.scroll_sad2);
  ImGui::Text("scroll_sad3: %04x", lcd.scroll_sad3);
  ImGui::Text("scroll_sad4: %04x", lcd.scroll_sad4);
  ImGui::Text("cgram_adr: %04x", lcd.cgram_adr);
  ImGui::End();

  ImGui::Begin("Control");
  ImGui::Text("PC: %02x:%04x", mcu.cp, mcu.pc);
  if (ImGui::Button("Reset")) {
    memset(sram, 0, SRAM_SIZE);
    memset(ram, 0, RAM_SIZE);
    MCU_Init();
    MCU_Reset();
  }
  if (ImGui::Button("Fire ISF0")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF0, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF0, 0);
  if (ImGui::Button("Fire ISF3")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF3, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF3, 0);
  if (ImGui::Button("Fire ISF4")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF4, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF4, 0);
  if (ImGui::Button("Fire ISF7")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF7, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF7, 0);
  if (ImGui::Button("Fire ISF8")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF8, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF8, 0);
  if (ImGui::Button("Fire ISF9")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF9, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF9, 0);
  if (ImGui::Button("Fire ISF10")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF10, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF10, 0);
  if (ImGui::Button("Fire ISF11")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF11, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF11, 0);
  if (ImGui::Button("Fire ISF15")) MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF15, 1); else MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ISF15, 0);
  ImGui::End();

  // render
  ImGui::Render();
  int w, h;
  SDL_GL_GetDrawableSize(window, &w, &h);
  glViewport(0, 0, w, h);
  glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(window);
}

int main(int argc, char *argv[]) {
  (void)argc;

  memset(&mcu, 0, sizeof(mcu_t));

  // load rom
  FILE *f = fopen("JD990-V1.05.bin_sw", "rb");
  fread(rom, 1, ROM_SIZE, f);
  fclose(f);

  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "FATAL ERROR: Failed to initialize the SDL2: %s.\n",
            SDL_GetError());
    fflush(stderr);
    return 2;
  }
  initImgui();

  MCU_Init();
  MCU_Reset();

  while (running) {
    if (!mcu.ex_ignore)
      MCU_Interrupt_Handle();
    else
      mcu.ex_ignore = 0;

    if (!mcu.sleep)
      MCU_ReadInstruction();

    mcu.cycles += 12; // FIXME: assume 12 cycles per instruction

    // printf("pc %02x%04x\n", mcu.cp, mcu.pc);

    MCU_UpdateAnalog(mcu.cycles);

    if ((mcu.cycles & 0x3fff) == 0) {
      dump();
      renderImgui();
    }
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(glctx);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
