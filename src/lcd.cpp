/*
 * Copyright (C) 2021, 2024 nukeykt
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "SDL_mutex.h"
#include "lcd.h"
#include "lcd_font.h"
#include "mcu.h"
#include "submcu.h"
#include "utils/files.h"

static const int lcd_width_max = 2048;
static const int lcd_height_max = 1024;


#define LOG printf
#define logerror printf
#define side_eff true
template <typename T, typename U> constexpr T BIT(T x, U n) noexcept { return (x >> n) & T(1); }


#define INSTRUCTION_SYSTEM_SET      0x40
#define INSTRUCTION_SLEEP_IN        0x53    // unimplemented
#define INSTRUCTION_DISP_ON         0x59
#define INSTRUCTION_DISP_OFF        0x58
#define INSTRUCTION_SCROLL          0x44
#define INSTRUCTION_CSRFORM         0x5d
#define INSTRUCTION_CGRAM_ADR       0x5c
#define INSTRUCTION_CSRDIR_RIGHT    0x4c
#define INSTRUCTION_CSRDIR_LEFT     0x4d
#define INSTRUCTION_CSRDIR_UP       0x4e
#define INSTRUCTION_CSRDIR_DOWN     0x4f
#define INSTRUCTION_HDOT_SCR        0x5a
#define INSTRUCTION_OVLAY           0x5b
#define INSTRUCTION_CSRW            0x46
#define INSTRUCTION_CSRR            0x47
#define INSTRUCTION_MWRITE          0x42
#define INSTRUCTION_MREAD           0x43


#define CSRDIR_RIGHT                0x00
#define CSRDIR_LEFT                 0x01
#define CSRDIR_UP                   0x02
#define CSRDIR_DOWN                 0x03


#define MX_OR                       0x00
#define MX_XOR                      0x01    // unimplemented
#define MX_AND                      0x02    // unimplemented
#define MX_PRIORITY_OR              0x03    // unimplemented


#define FC_OFF                      0x00
#define FC_SOLID                    0x01
#define FC_FLASH_32                 0x02    // unimplemented
#define FC_FLASH_64                 0x03    // unimplemented


#define FP_OFF                      0x00
#define FP_SOLID                    0x01
#define FP_FLASH_32                 0x02    // unimplemented
#define FP_FLASH_4                  0x03    // unimplemented


struct SedState {
    int m_bf;                   // busy flag

	uint8_t m_ir;                 // instruction register
	uint8_t m_dor;                // data output register
	int m_pbc;                  // parameter byte counter

	int m_d;                    // display enabled
	int m_sleep;                // sleep mode

	uint16_t m_sag;               // character generator RAM start address
	int m_m0;                   // character generator ROM (0=internal, 1=external)
	int m_m1;                   // character generator RAM D6 correction (0=no, 1=yes)
	int m_m2;                   // height of character bitmaps (0=8, 1=16 pixels)
	int m_ws;                   // LCD drive method (0=single, 1=dual panel)
	int m_iv;                   // screen origin compensation for inverse display (0=yes, 1=no)
	int m_wf;                   // AC frame drive waveform period (0=16-line, 1=2-frame)

	int m_fx;                   // character width in pixels
	int m_fy;                   // character height in pixels
	int m_cr;                   // visible line width in characters
	int m_tcr;                  // total line width in characters (including horizontal blanking)
	int m_lf;                   // frame height in lines
	uint16_t m_ap;                // virtual screen line width in characters

	uint16_t m_sad1;              // display page 1 start address
	uint16_t m_sad2;              // display page 2 start address
	uint16_t m_sad3;              // display page 3 start address
	uint16_t m_sad4;              // display page 4 start address
	int m_sl1;                  // display block 1 height in lines
	int m_sl2;                  // display block 2 height in lines
	int m_hdotscr;              // horizontal dot scroll in pixels
	int m_fp;                   // display page flash control

	uint16_t m_csr;               // cursor address register
	int m_cd;                   // cursor increment direction
	int m_crx;                  // cursor width
	int m_cry;                  // cursor height or location
	int m_cm;                   // cursor shape (0=underscore, 1=block)
	int m_fc;                   // cursor flash control

	int m_mx;                   // screen layer composition method
	int m_dm;                   // display mode for pages 1, 3
	int m_ov;                   // graphics mode layer composition

    uint8_t vram[0x2000];

    uint8_t readbyte(uint16_t addr)
    {
        // if (addr >= 0x2000)
        // {
        //     logerror("SED1330 Read invalid address %04x\n", addr);
        //     exit(1);
        // }
        return vram[addr & 0x1fff];
    }
    void writebyte(uint16_t addr, uint8_t data)
    {
        // if (addr >= 0x2000)
        // {
        //     logerror("SED1330 Write to invalid address %04x\n", addr);
        //     exit(1);
        // }
        vram[addr & 0x1fff] = data;
    }

    inline void increment_csr()
    {
        switch (m_cd)
        {
        case CSRDIR_RIGHT:
            m_csr++;
            break;

        case CSRDIR_LEFT:
            m_csr--;
            break;

        case CSRDIR_UP:
            m_csr -= m_ap;
            break;

        case CSRDIR_DOWN:
            m_csr += m_ap;
            break;
        }
    }

    uint8_t status_r()
    {
        // if (side_eff)
        // 	LOG("SED1330 Status Read: %s\n", m_bf ? "busy" : "ready");

        return m_bf << 6;
    }


    //-------------------------------------------------
    //  command_w -
    //-------------------------------------------------

    void command_w(uint8_t data)
    {
        m_ir = data;
        m_pbc = 0;

        switch (m_ir)
        {
    #if 0
        case INSTRUCTION_SLEEP_IN:
            break;
    #endif
        case INSTRUCTION_CSRDIR_RIGHT:
        case INSTRUCTION_CSRDIR_LEFT:
        case INSTRUCTION_CSRDIR_UP:
        case INSTRUCTION_CSRDIR_DOWN:
            m_cd = data & 0x03;

            switch (m_cd)
            {
            case CSRDIR_RIGHT:  LOG("SED1330 Cursor Direction: Right\n");  break;
            case CSRDIR_LEFT:   LOG("SED1330 Cursor Direction: Left\n");   break;
            case CSRDIR_UP:     LOG("SED1330 Cursor Direction: Up\n");     break;
            case CSRDIR_DOWN:   LOG("SED1330 Cursor Direction: Down\n");   break;
            }
            break;
        }
    }


    //-------------------------------------------------
    //  data_r -
    //-------------------------------------------------

    uint8_t data_r()
    {
        uint8_t data = 0;

        switch (m_ir)
        {
        case INSTRUCTION_MREAD:
            data = readbyte(m_csr);
            if (side_eff)
            {
                LOG("SED1330 Memory Read %02x from %04x\n", data, m_csr);
                increment_csr();
            }
            break;

        case INSTRUCTION_CSRR:
            switch (m_pbc)
            {
            case 0:
                data = m_csr & 0xff;
                break;

            case 1:
                data = (m_csr & 0xff00) >> 8;
                break;

            default:
                logerror("SED1330 Invalid parameter byte %02x\n", data);
            }
            if (side_eff)
            {
                LOG("SED1330 Cursor Byte %d Read %02x\n", m_pbc, data);
                m_pbc++;
            }
            break;

        default:
            logerror("SED1330 Unsupported instruction %02x\n", m_ir);
            break;
        }

        return data;
    }


    //-------------------------------------------------
    //  data_w -
    //-------------------------------------------------

    void data_w(uint8_t data)
    {
        switch (m_ir)
        {
        case INSTRUCTION_SYSTEM_SET:
            switch (m_pbc)
            {
            case 0:
                m_m0 = BIT(data, 0);
                m_m1 = BIT(data, 1);
                m_m2 = BIT(data, 2);
                m_ws = BIT(data, 3);
                m_iv = BIT(data, 5);

                LOG("SED1330 %s CG ROM\n", BIT(data, 0) ? "External" : "Internal");
                LOG("SED1330 D6 Correction: %s\n", BIT(data, 1) ? "enabled" : "disabled");
                LOG("SED1330 Character Height: %u\n", BIT(data, 2) ? 16 : 8);
                LOG("SED1330 %s Panel Drive\n", BIT(data, 3) ? "Dual" : "Single");
                LOG("SED1330 Screen Top-Line Correction: %s\n", BIT(data, 5) ? "disabled" : "enabled");
                break;

            case 1:
                m_fx = (data & 0x07) + 1;
                m_wf = BIT(data, 7);

                LOG("SED1330 Horizontal Character Size: %u\n", m_fx);
                LOG("SED1330 %s AC Drive\n", BIT(data, 7) ? "2-frame" : "16-line");
                break;

            case 2:
                m_fy = (data & 0x0f) + 1;
                LOG("SED1330 Vertical Character Size: %u\n", m_fy);
                break;

            case 3:
                m_cr = data + 1;
                LOG("SED1330 Visible Characters Per Line: %u\n", m_cr);
                break;

            case 4:
                m_tcr = data + 1;
                LOG("SED1330 Total Characters Per Line: %u\n", m_tcr);
                break;

            case 5:
                m_lf = data + 1;
                LOG("SED1330 Frame Height: %u\n", m_lf);
                if (clock() != 0)
                {
                    // attotime fr = clocks_to_attotime(m_tcr * m_lf * 9);
                    // screen().configure(m_tcr * m_fx, m_lf, screen().visible_area(), fr.as_attoseconds());
                    // LOG("SED1330 Frame Rate: %.1f Hz\n", fr.as_hz());
                }
                break;

            case 6:
                m_ap = (m_ap & 0xff00) | data;
                break;

            case 7:
                m_ap = (data << 8) | (m_ap & 0xff);
                LOG("SED1330 Virtual Screen Width: %u\n", m_ap);
                break;

            default:
                logerror("SED1330 Invalid parameter byte %02x\n", data);
            }
            break;

        case INSTRUCTION_DISP_ON:
        case INSTRUCTION_DISP_OFF:
            m_d = BIT(m_ir, 0);
            m_fc = data & 0x03;
            m_fp = data >> 2;
            LOG("SED1330 Display: %s\n", BIT(m_ir, 0) ? "enabled" : "disabled");

            switch (m_fc)
            {
            case FC_OFF:        LOG("SED1330 Cursor: disabled\n"); break;
            case FC_SOLID:      LOG("SED1330 Cursor: solid\n");    break;
            case FC_FLASH_32:   LOG("SED1330 Cursor: fFR/32\n");   break;
            case FC_FLASH_64:   LOG("SED1330 Cursor: fFR/64\n");   break;
            }

            switch (m_fp & 0x03)
            {
            case FP_OFF:        LOG("SED1330 Display Page 1: disabled\n");     break;
            case FP_SOLID:      LOG("SED1330 Display Page 1: enabled\n");      break;
            case FP_FLASH_32:   LOG("SED1330 Display Page 1: flash fFR/32\n"); break;
            case FP_FLASH_4:    LOG("SED1330 Display Page 1: flash fFR/4\n");  break;
            }

            switch ((m_fp >> 2) & 0x03)
            {
            case FP_OFF:        LOG("SED1330 Display Page 2/4: disabled\n");       break;
            case FP_SOLID:      LOG("SED1330 Display Page 2/4: enabled\n");        break;
            case FP_FLASH_32:   LOG("SED1330 Display Page 2/4: flash fFR/32\n");   break;
            case FP_FLASH_4:    LOG("SED1330 Display Page 2/4: flash fFR/4\n");    break;
            }

            switch ((m_fp >> 4) & 0x03)
            {
            case FP_OFF:        LOG("SED1330 Display Page 3: disabled\n");     break;
            case FP_SOLID:      LOG("SED1330 Display Page 3: enabled\n");      break;
            case FP_FLASH_32:   LOG("SED1330 Display Page 3: flash fFR/32\n"); break;
            case FP_FLASH_4:    LOG("SED1330 Display Page 3: flash fFR/4\n");  break;
            }
            break;

        case INSTRUCTION_SCROLL:
            switch (m_pbc)
            {
            case 0:
                m_sad1 = (m_sad1 & 0xff00) | data;
                break;

            case 1:
                m_sad1 = (data << 8) | (m_sad1 & 0xff);
                LOG("SED1330 Display Page 1 Start Address: %04x\n", m_sad1);
                break;

            case 2:
                m_sl1 = data + 1;
                LOG("SED1330 Display Block 1 Screen Lines: %u\n", m_sl1);
                break;

            case 3:
                m_sad2 = (m_sad2 & 0xff00) | data;
                break;

            case 4:
                m_sad2 = (data << 8) | (m_sad2 & 0xff);
                LOG("SED1330 Display Page 2 Start Address: %04x\n", m_sad2);
                break;

            case 5:
                m_sl2 = data + 1;
                LOG("SED1330 Display Block 2 Screen Lines: %u\n", m_sl2);
                break;

            case 6:
                m_sad3 = (m_sad3 & 0xff00) | data;
                break;

            case 7:
                m_sad3 = (data << 8) | (m_sad3 & 0xff);
                LOG("SED1330 Display Page 3 Start Address: %04x\n", m_sad3);
                break;

            case 8:
                m_sad4 = (m_sad4 & 0xff00) | data;
                break;

            case 9:
                m_sad4 = (data << 8) | (m_sad4 & 0xff);
                LOG("SED1330 Display Page 4 Start Address: %04x\n", m_sad4);
                break;

            default:
                logerror("SED1330 Invalid parameter byte %02x\n", data);
            }
            break;

        case INSTRUCTION_CSRFORM:
            switch (m_pbc)
            {
            case 0:
                m_crx = (data & 0x0f) + 1;
                LOG("SED1330 Horizontal Cursor Size: %u\n", m_crx);
                break;

            case 1:
                m_cry = (data & 0x0f) + 1;
                m_cm = BIT(data, 7);
                LOG("SED1330 Vertical Cursor Location: %u\n", m_cry);
                LOG("SED1330 Cursor Shape: %s\n", BIT(data, 7) ? "Block" : "Underscore");
                break;

            default:
                logerror("SED1330 Invalid parameter byte %02x\n", data);
            }
            break;

        case INSTRUCTION_CGRAM_ADR:
            switch (m_pbc)
            {
            case 0:
                m_sag = (m_sag & 0xff00) | data;
                break;

            case 1:
                m_sag = (data << 8) | (m_sag & 0xff);
                LOG("SED1330 Character Generator RAM Start Address: %04x\n", m_sag);
                break;

            default:
                logerror("SED1330 Invalid parameter byte %02x\n", data);
            }
            break;

        case INSTRUCTION_HDOT_SCR:
            m_hdotscr = data & 0x07;
            LOG("SED1330 Horizontal Dot Scroll: %u\n", m_hdotscr);
            break;

        case INSTRUCTION_OVLAY:
            m_mx = data & 0x03;
            m_dm = (data >> 2) & 0x03;
            m_ov = BIT(data, 4);

            switch (m_mx)
            {
            case MX_OR:             LOG("SED1330 Display Composition Method: OR\n");           break;
            case MX_XOR:            LOG("SED1330 Display Composition Method: Exclusive-OR\n"); break;
            case MX_AND:            LOG("SED1330 Display Composition Method: AND\n");          break;
            case MX_PRIORITY_OR:    LOG("SED1330 Display Composition Method: Priority-OR\n");  break;
            }

            LOG("SED1330 Display Page 1 Mode: %s\n", BIT(data, 2) ? "Graphics" : "Text");
            LOG("SED1330 Display Page 3 Mode: %s\n", BIT(data, 3) ? "Graphics" : "Text");
            LOG("SED1330 Display Composition Layers: %u\n", BIT(data, 4) ? 3 : 2);
            break;

        case INSTRUCTION_CSRW:
            switch (m_pbc)
            {
            case 0:
                m_csr = (m_csr & 0xff00) | data;
                break;

            case 1:
                m_csr = (data << 8) | (m_csr & 0xff);
                LOG("SED1330 Cursor Address %04x\n", m_csr);
                break;

            default:
                logerror("SED1330 Invalid parameter byte %02x\n", data);
            }
            break;
    #if 0
        case INSTRUCTION_CSRR:
            break;
    #endif
        case INSTRUCTION_MWRITE:
            LOG("SED1330 Memory Write %02x %c to %04x (row %u col %u line %u)\n", data, data, m_csr, m_csr/80/8, m_csr%80, m_csr/80);

            writebyte(m_csr, data);

            increment_csr();
            break;
    #if 0
        case INSTRUCTION_MREAD:
            break;
    #endif
        default:
            logerror("SED1330 Unsupported instruction %02x\n", m_ir);
        }

        m_pbc++;
    }


    //-------------------------------------------------
    //  draw_text_scanline -
    //-------------------------------------------------

    void draw_text_scanline(uint32_t lcd_buffer[lcd_height_max][lcd_width_max], int y, int r, uint16_t va, bool cursor)
    {
        uint32_t *p = &lcd_buffer[y][0];

        for (int sx = 0; sx < m_cr; sx++, p += m_fx)
        {
            if (m_m0 && !m_m1)
            {
                uint8_t c = readbyte(va + sx);
                uint8_t data = readbyte(0xf000 | (m_m2 ? std::uint16_t(c) << 4 | r : std::uint16_t(c) << 3 | (r & 7)));
                for (int x = 0; x < m_fx; x++, data <<= 1)
                    if (BIT(data, 7))
                        p[x] |= 1;
            }

            if (cursor && (va + sx) == m_csr)
            {
                if (m_cm)
                {
                    // block cursor
                    if (r < m_cry)
                    {
                        std::fill_n(p, m_crx, 1);
                    }
                }
                else
                {
                    // underscore cursor
                    if (r == m_cry)
                    {
                        std::fill_n(p, m_crx, 1);
                    }
                }
            }
        }
    }


    //-------------------------------------------------
    //  draw_graphics_scanline -
    //-------------------------------------------------

    void draw_graphics_scanline(uint32_t lcd_buffer[lcd_height_max][lcd_width_max], int y, uint16_t va)
    {
        for (int sx = 0; sx < m_cr; sx++)
        {
            uint8_t data = readbyte(va++);

            for (int x = 0; x < m_fx; x++)
            {
                lcd_buffer[y][(sx * m_fx) + x] |= BIT(data, 7);
                data <<= 1;
            }
        }
    }


    //-------------------------------------------------
    //  update_graphics -
    //-------------------------------------------------

    void update_graphics(uint32_t lcd_buffer[lcd_height_max][lcd_width_max])
    {
        for (int y = 0; y < m_lf; y++)
        {
            uint16_t sad2 = m_sad2 + (y * m_ap);
            // draw graphics display page 2 scanline
            draw_graphics_scanline(lcd_buffer, y, sad2);

            uint16_t sad1 = m_sad1 + ((y / m_fy) * m_ap);
            // draw text display page 1 scanline
            draw_text_scanline(lcd_buffer, y, y % m_fy, sad1, !m_ov && m_fc != FC_OFF);
        }
    }


    //-------------------------------------------------
    //  update_text -
    //-------------------------------------------------

    void update_text(uint32_t lcd_buffer[lcd_height_max][lcd_width_max])
    {
        uint8_t attr1 = m_fp & 0x03;
        uint8_t attr2 = (m_fp >> 2) & 0x03;
        uint8_t attr3 = (m_fp >> 4) & 0x03;

        for (int y = 0; y < m_lf; y++)
        {
            if (y >= m_sl1)
            {
                if (attr3 != FP_OFF)
                {
                    uint16_t sad3 = m_sad3 + (((y - m_sl1) / m_fy) * m_ap);

                    // draw text display page 3 scanline
                    draw_text_scanline(lcd_buffer, y, (y - m_sl1) % m_fy, sad3, m_ov && m_fc != FC_OFF);
                }
            }
            else
            {
                if (attr1 != FP_OFF)
                {
                    uint16_t sad1 = m_sad1 + ((y / m_fy) * m_ap);

                    // draw text display page 1 scanline
                    draw_text_scanline(lcd_buffer, y, y % m_fy, sad1, !m_ov && m_fc != FC_OFF);
                }
            }

            if (attr2 != FP_OFF)
            {
                if (m_ws && y >= m_sl2)
                {
                    uint16_t sad4 = m_sad4 + ((y - m_sl2) * m_ap);

                    // draw graphics display page 4 scanline
                    draw_graphics_scanline(lcd_buffer, y, sad4);
                }
                else
                {
                    uint16_t sad2 = m_sad2 + (y * m_ap);

                    // draw graphics display page 2 scanline
                    draw_graphics_scanline(lcd_buffer, y, sad2);
                }
            }
        }
    }


    //-------------------------------------------------
    //  screen_update -
    //-------------------------------------------------

    uint32_t screen_update(uint32_t lcd_buffer[lcd_height_max][lcd_width_max])
    {
        for (size_t i = 0; i < lcd_height_max * lcd_width_max; i++)
        {
            lcd_buffer[i / lcd_width_max][i % lcd_width_max] = 0;
        }
        
        if (m_d)
        {
            if (m_dm)
            {
                update_graphics(lcd_buffer);
            }
            else
            {
                update_text(lcd_buffer);
            }
        }
        return 0;
    }

};

static uint32_t LCD_DL, LCD_N, LCD_F, LCD_D, LCD_C, LCD_B, LCD_ID = 1, LCD_S;
static uint32_t LCD_DD_RAM, LCD_AC, LCD_CG_RAM;
static uint32_t LCD_RAM_MODE = 0;
uint8_t LCD_Data[80];
uint8_t LCD_CG[64];
uint8_t LCD_7SEG[3];

static SedState sed_state;

static uint8_t lcd_enable = 1;
static bool lcd_quit_requested = false;

void LCD_Enable(uint32_t enable)
{
    lcd_enable = enable;
}

bool LCD_QuitRequested()
{
    return lcd_quit_requested;
}

void LCD_Write(uint32_t address, uint8_t data)
{
    if (mcu_jd990)
    {
        if ((address & 0x3) == 0x0) sed_state.data_w(data);
        if ((address & 0x3) == 0x2) sed_state.command_w(data);
        return;
    }

    if (address == 0)
    {
        if ((data & 0xe0) == 0x20)
        {
            LCD_DL = (data & 0x10) != 0;
            LCD_N = (data & 0x8) != 0;
            LCD_F = (data & 0x4) != 0;
        }
        else if ((data & 0xf8) == 0x8)
        {
            LCD_D = (data & 0x4) != 0;
            LCD_C = (data & 0x2) != 0;
            LCD_B = (data & 0x1) != 0;
        }
        else if ((data & 0xff) == 0x01)
        {
            LCD_DD_RAM = 0;
            LCD_ID = 1;
            memset(LCD_Data, 0x20, sizeof(LCD_Data));
        }
        else if ((data & 0xff) == 0x02)
        {
            LCD_DD_RAM = 0;
        }
        else if ((data & 0xfc) == 0x04)
        {
            LCD_ID = (data & 0x2) != 0;
            LCD_S = (data & 0x1) != 0;
        }
        else if ((data & 0xc0) == 0x40)
        {
            LCD_CG_RAM = (data & 0x3f);
            LCD_RAM_MODE = 0;
        }
        else if ((data & 0x80) == 0x80)
        {
            LCD_DD_RAM = (data & 0x7f);
            LCD_RAM_MODE = 1;
        }
        else
        {
            address += 0;
        }
    }
    else
    {
        if (!LCD_RAM_MODE)
        {
            LCD_CG[LCD_CG_RAM] = data & 0x1f;
            if (LCD_ID)
            {
                LCD_CG_RAM++;
            }
            else
            {
                LCD_CG_RAM--;
            }
            LCD_CG_RAM &= 0x3f;
        }
        else
        {
            if (LCD_N)
            {
                if (LCD_DD_RAM & 0x40)
                {
                    if ((LCD_DD_RAM & 0x3f) < 40)
                        LCD_Data[(LCD_DD_RAM & 0x3f) + 40] = data;
                }
                else
                {
                    if ((LCD_DD_RAM & 0x3f) < 40)
                        LCD_Data[LCD_DD_RAM & 0x3f] = data;
                }
            }
            else
            {
                if (LCD_DD_RAM < 80)
                    LCD_Data[LCD_DD_RAM] = data;
            }
            if (LCD_ID)
            {
                LCD_DD_RAM++;
            }
            else
            {
                LCD_DD_RAM--;
            }
            LCD_DD_RAM &= 0x7f;
        }
    }
    //printf("%i %.2x ", address, data);
    // if (data >= 0x20 && data <= 'z')
    //     printf("%c\n", data);
    //else
    //    printf("\n");
}

void LCD_Write_7seg(uint8_t address, uint8_t data)
{
    LCD_7SEG[address] = data;
}

int lcd_width = 741;
int lcd_height = 268;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

static std::string m_back_path = "back.data";

static uint32_t lcd_buffer[lcd_height_max][lcd_width_max];
static uint32_t lcd_background[268][741];

static uint32_t lcd_init = 0;

const int button_map_sc55[][2] =
{
    SDL_SCANCODE_Q, MCU_BUTTON_POWER,
    SDL_SCANCODE_W, MCU_BUTTON_INST_ALL,
    SDL_SCANCODE_E, MCU_BUTTON_INST_MUTE,
    SDL_SCANCODE_R, MCU_BUTTON_PART_L,
    SDL_SCANCODE_T, MCU_BUTTON_PART_R,
    SDL_SCANCODE_Y, MCU_BUTTON_INST_L,
    SDL_SCANCODE_U, MCU_BUTTON_INST_R,
    SDL_SCANCODE_I, MCU_BUTTON_KEY_SHIFT_L,
    SDL_SCANCODE_O, MCU_BUTTON_KEY_SHIFT_R,
    SDL_SCANCODE_P, MCU_BUTTON_LEVEL_L,
    SDL_SCANCODE_LEFTBRACKET, MCU_BUTTON_LEVEL_R,
    SDL_SCANCODE_A, MCU_BUTTON_MIDI_CH_L,
    SDL_SCANCODE_S, MCU_BUTTON_MIDI_CH_R,
    SDL_SCANCODE_D, MCU_BUTTON_PAN_L,
    SDL_SCANCODE_F, MCU_BUTTON_PAN_R,
    SDL_SCANCODE_G, MCU_BUTTON_REVERB_L,
    SDL_SCANCODE_H, MCU_BUTTON_REVERB_R,
    SDL_SCANCODE_J, MCU_BUTTON_CHORUS_L,
    SDL_SCANCODE_K, MCU_BUTTON_CHORUS_R,
    SDL_SCANCODE_LEFT, MCU_BUTTON_PART_L,
    SDL_SCANCODE_RIGHT, MCU_BUTTON_PART_R,
};

const int button_map_sc88[][2] =
{
    SDL_SCANCODE_Q, MCU_BUTTON_POWER,
    SDL_SCANCODE_W, MCU_BUTTON_INST_ALL,
    SDL_SCANCODE_E, MCU_BUTTON_INST_MUTE,
    SDL_SCANCODE_R, MCU_BUTTON_PART_L,
    SDL_SCANCODE_T, MCU_BUTTON_PART_R,
    SDL_SCANCODE_Y, MCU_BUTTON_INST_L,
    SDL_SCANCODE_U, MCU_BUTTON_INST_R,
    SDL_SCANCODE_I, MCU_BUTTON_KEY_SHIFT_L,
    SDL_SCANCODE_O, MCU_BUTTON_KEY_SHIFT_R,
    SDL_SCANCODE_P, MCU_BUTTON_LEVEL_L,
    SDL_SCANCODE_LEFTBRACKET, MCU_BUTTON_LEVEL_R,
    SDL_SCANCODE_A, MCU_BUTTON_MIDI_CH_L,
    SDL_SCANCODE_S, MCU_BUTTON_MIDI_CH_R,
    SDL_SCANCODE_D, MCU_BUTTON_PAN_L,
    SDL_SCANCODE_F, MCU_BUTTON_PAN_R,
    SDL_SCANCODE_G, MCU_BUTTON_REVERB_L,
    SDL_SCANCODE_H, MCU_BUTTON_REVERB_R,
    SDL_SCANCODE_J, MCU_BUTTON_CHORUS_L,
    SDL_SCANCODE_K, MCU_BUTTON_CHORUS_R,
    SDL_SCANCODE_LEFT, MCU_BUTTON_PART_L,
    SDL_SCANCODE_RIGHT, MCU_BUTTON_PART_R,

    SDL_SCANCODE_1, MCU_SC88_BUTTON_EQ,
    SDL_SCANCODE_2, MCU_SC88_BUTTON_INSTMAP,
    SDL_SCANCODE_3, MCU_SC88_BUTTON_USER_INST,
    SDL_SCANCODE_4, MCU_SC88_BUTTON_EDIT,
    SDL_SCANCODE_Z, MCU_SC88_BUTTON_VIB_RATE_L,
    SDL_SCANCODE_X, MCU_SC88_BUTTON_VIB_RATE_R,
    SDL_SCANCODE_C, MCU_SC88_BUTTON_VIB_DEPTH_L,
    SDL_SCANCODE_V, MCU_SC88_BUTTON_VIB_DEPTH_R,
    SDL_SCANCODE_B, MCU_SC88_BUTTON_VIB_DELAY_L,
    SDL_SCANCODE_N, MCU_SC88_BUTTON_VIB_DELAY_R,
    SDL_SCANCODE_TAB, MCU_SC88_BUTTON_PREVIEW,
};

const int button_map_jv880[][2] =
{
    SDL_SCANCODE_P, MCU_JV880_BUTTON_PREVIEW,
    SDL_SCANCODE_LEFT, MCU_JV880_BUTTON_CURSOR_L,
    SDL_SCANCODE_RIGHT, MCU_JV880_BUTTON_CURSOR_R,
    SDL_SCANCODE_TAB, MCU_JV880_BUTTON_DATA,
    SDL_SCANCODE_Q, MCU_JV880_BUTTON_TONE_SELECT,
    SDL_SCANCODE_A, MCU_JV880_BUTTON_PATCH_PERFORM,
    SDL_SCANCODE_W, MCU_JV880_BUTTON_EDIT,
    SDL_SCANCODE_E, MCU_JV880_BUTTON_SYSTEM,
    SDL_SCANCODE_R, MCU_JV880_BUTTON_RHYTHM,
    SDL_SCANCODE_T, MCU_JV880_BUTTON_UTILITY,
    SDL_SCANCODE_S, MCU_JV880_BUTTON_MUTE,
    SDL_SCANCODE_D, MCU_JV880_BUTTON_MONITOR,
    SDL_SCANCODE_F, MCU_JV880_BUTTON_COMPARE,
    SDL_SCANCODE_G, MCU_JV880_BUTTON_ENTER,
};

const int button_map_rd500[][2] =
{
    // TODO
};

const int button_map_ra30[][2] =
{
    SDL_SCANCODE_A, MCU_RA30_BALANCE_ARRANGER,
    SDL_SCANCODE_B, MCU_RA30_BALANCE_TONE,
    SDL_SCANCODE_C, MCU_RA30_TEMPO_MINUS,
    SDL_SCANCODE_D, MCU_RA30_TEMPO_PLUS,
    SDL_SCANCODE_E, MCU_RA30_FN_TUNE,
    SDL_SCANCODE_F, MCU_RA30_FN_DEMO,
    SDL_SCANCODE_G, MCU_RA30_REC_REC,
    SDL_SCANCODE_H, MCU_RA30_REC_PLAY,
    SDL_SCANCODE_I, MCU_RA30_TONE_KEYBOARD,
    SDL_SCANCODE_J, MCU_RA30_NONE_0,
    SDL_SCANCODE_K, MCU_RA30_FILLIN_VARIATION,
    SDL_SCANCODE_L, MCU_RA30_FILLIN_ORIGINAL,
    SDL_SCANCODE_M, MCU_RA30_INTRO_ENDING,
    SDL_SCANCODE_N, MCU_RA30_START_STOP,
    SDL_SCANCODE_O, MCU_RA30_ONETOUCH_KEYBOARD,
    SDL_SCANCODE_P, MCU_RA30_ONETOUCH_ARRANGER,
    SDL_SCANCODE_Q, MCU_RA30_STYLE_A,
    SDL_SCANCODE_R, MCU_RA30_STYLE_B,
    SDL_SCANCODE_S, MCU_RA30_STYLE_C,
    SDL_SCANCODE_T, MCU_RA30_STYLE_D,
    SDL_SCANCODE_U, MCU_RA30_STYLE_E,
    SDL_SCANCODE_V, MCU_RA30_STYLE_F,
    SDL_SCANCODE_W, MCU_RA30_STYLE_G,
    SDL_SCANCODE_X, MCU_RA30_STYLE_H,
    SDL_SCANCODE_Y, MCU_RA30_TONE_1,
    SDL_SCANCODE_Z, MCU_RA30_TONE_2,
    SDL_SCANCODE_0, MCU_RA30_TONE_3,
    SDL_SCANCODE_1, MCU_RA30_TONE_4,
    SDL_SCANCODE_2, MCU_RA30_TONE_5,
    SDL_SCANCODE_3, MCU_RA30_TONE_6,
    SDL_SCANCODE_4, MCU_RA30_TONE_7,
    SDL_SCANCODE_5, MCU_RA30_TONE_8,
    // SDL_SCANCODE_6, MCU_RA30_NONE_1,
    // SDL_SCANCODE_7, MCU_RA30_NONE_2,
    // SDL_SCANCODE_8, MCU_RA30_NONE_3,
    // SDL_SCANCODE_9, MCU_RA30_NONE_4,
    // SDL_SCANCODE_10, MCU_RA30_TONESELECT_U,
    // SDL_SCANCODE_11, MCU_RA30_TONESELECT_L,
    // SDL_SCANCODE_12, MCU_RA30_STYLESELECT_U,
    // SDL_SCANCODE_13, MCU_RA30_STYLESELECT_L,
};

const int button_map_xp10[][2] =
{
    SDL_SCANCODE_I, MCU_XP10_BUTTON_UTILITY,
    SDL_SCANCODE_U, MCU_XP10_BUTTON_EDIT,
    SDL_SCANCODE_Y, MCU_XP10_BUTTON_TRANSPOSE,
    SDL_SCANCODE_R, MCU_XP10_BUTTON_ARPEGGIO,
    SDL_SCANCODE_E, MCU_XP10_BUTTON_XDUAL,
    SDL_SCANCODE_W, MCU_XP10_BUTTON_DUAL,
    SDL_SCANCODE_Q, MCU_XP10_BUTTON_SPLIT,
    SDL_SCANCODE_O, MCU_XP10_BUTTON_SEQCTRL,
    SDL_SCANCODE_T, MCU_XP10_BUTTON_SELECT,
    
    SDL_SCANCODE_D, MCU_XP10_BUTTON_PERFORM,
    SDL_SCANCODE_S, MCU_XP10_BUTTON_PARTP,
    SDL_SCANCODE_A, MCU_XP10_BUTTON_PARTM,
    SDL_SCANCODE_F, MCU_XP10_BUTTON_VARIATION,

    SDL_SCANCODE_X, MCU_XP10_BUTTON_VALUEP,
    SDL_SCANCODE_Z, MCU_XP10_BUTTON_VALUEM,
    SDL_SCANCODE_V, MCU_XP10_BUTTON_ENTER,
    SDL_SCANCODE_C, MCU_XP10_BUTTON_TONE,
    
    SDL_SCANCODE_1, MCU_XP10_BUTTON_1,
    SDL_SCANCODE_2, MCU_XP10_BUTTON_2,
    SDL_SCANCODE_3, MCU_XP10_BUTTON_3,
    SDL_SCANCODE_4, MCU_XP10_BUTTON_4,
    SDL_SCANCODE_5, MCU_XP10_BUTTON_5,
    SDL_SCANCODE_6, MCU_XP10_BUTTON_6,
    SDL_SCANCODE_7, MCU_XP10_BUTTON_7,
    SDL_SCANCODE_8, MCU_XP10_BUTTON_8,
    SDL_SCANCODE_9, MCU_XP10_BUTTON_9,
    SDL_SCANCODE_0, MCU_XP10_BUTTON_0,
};

const int button_map_se70[][2] =
{
    SDL_SCANCODE_Q, MCU_SE70_EXIT,
    SDL_SCANCODE_W, MCU_SE70_WRITE,
    SDL_SCANCODE_A, MCU_SE70_PARAM_L,
    SDL_SCANCODE_S, MCU_SE70_PARAM_R,
    SDL_SCANCODE_Z, MCU_SE70_UTILITY,
    SDL_SCANCODE_X, MCU_SE70_CONTROL1,
    SDL_SCANCODE_C, MCU_SE70_CONTROL2,
    SDL_SCANCODE_V, MCU_SE70_CONTROL3,
    SDL_SCANCODE_TAB, MCU_SE70_ENTER,
};

const int button_map_jd800[][2] =
{
    SDL_SCANCODE_UNKNOWN, 0x00, // PORTAMENTO
    SDL_SCANCODE_UNKNOWN, 0x01, // SOLO
    SDL_SCANCODE_UNKNOWN, 0x02, // KEY TRANSPOSE
    SDL_SCANCODE_UNKNOWN, 0x03, // MULTI SETUP EFFECT
    SDL_SCANCODE_I, 0x04, // PART R
    SDL_SCANCODE_Y, 0x05, // MIDI
    SDL_SCANCODE_T, 0x06, // PART EDIT
    SDL_SCANCODE_UNKNOWN, 0x07, // 
    SDL_SCANCODE_UNKNOWN, 0x08, // BANK 7
    SDL_SCANCODE_UNKNOWN, 0x09, // BANK 5
    SDL_SCANCODE_UNKNOWN, 0x0a, // BANK 4
    SDL_SCANCODE_X, 0x0b, // BANK 2
    SDL_SCANCODE_UNKNOWN, 0x0c, // BANK 8
    SDL_SCANCODE_UNKNOWN, 0x0d, // COPY
    SDL_SCANCODE_Z, 0x0e, // BANK 1
    SDL_SCANCODE_LEFT, 0x0f, // CURSOR L
    SDL_SCANCODE_R, 0x10, // PATCH EDIT EFFECT
    SDL_SCANCODE_E, 0x11, // COMMON
    SDL_SCANCODE_UNKNOWN, 0x12, // LAYER ACTIVE
    SDL_SCANCODE_UNKNOWN, 0x13, // SPECIAL SETUP
    SDL_SCANCODE_U, 0x14, // PART L
    SDL_SCANCODE_W, 0x15, // TUNE/FUNC
    SDL_SCANCODE_UNKNOWN, 0x16, // WG BEND
    SDL_SCANCODE_Q, 0x17, // WG SOURCE
    SDL_SCANCODE_ESCAPE, 0x18, // EXIT
    SDL_SCANCODE_O, 0x19, // INC/YES
    SDL_SCANCODE_UNKNOWN, 0x1a, // TVF MODE
    SDL_SCANCODE_UNKNOWN, 0x1b, // TVF LFO SELECT
    SDL_SCANCODE_UNKNOWN, 0x1c, // TVA BIAS DIRECTION
    SDL_SCANCODE_UNKNOWN, 0x1d, // TVA LFO SELECT
    SDL_SCANCODE_L, 0x1e, // PAGE U
    SDL_SCANCODE_K, 0x1f, // PAGE D
    SDL_SCANCODE_7, 0x20, // NUMBER 7
    SDL_SCANCODE_UNKNOWN, 0x21, // BANK 6
    SDL_SCANCODE_4, 0x22, // NUMBER 4
    SDL_SCANCODE_C, 0x23, // BANK 3
    SDL_SCANCODE_UNKNOWN, 0x24, // COMPARE
    SDL_SCANCODE_UNKNOWN, 0x25, // MANUAL
    SDL_SCANCODE_1, 0x26, // NUMBER 1
    SDL_SCANCODE_RIGHT, 0x27, // CURSOR R
    SDL_SCANCODE_UNKNOWN, 0x28, // TONE D
    SDL_SCANCODE_UNKNOWN, 0x29, // TONE C
    SDL_SCANCODE_UNKNOWN, 0x2a, // TONE B
    SDL_SCANCODE_UNKNOWN, 0x2b, // TONE A
    SDL_SCANCODE_M, 0x2c, // MULTI
    SDL_SCANCODE_UNKNOWN, 0x2d, // SINGLE
    SDL_SCANCODE_UNKNOWN, 0x2e, // LFO1 WAVEFORM
    SDL_SCANCODE_UNKNOWN, 0x2f, // LFO1 OFFSET
    SDL_SCANCODE_P, 0x30, // DEC/NO
    SDL_SCANCODE_UNKNOWN, 0x31, // WG A-TOUCH BEND
    SDL_SCANCODE_UNKNOWN, 0x32, // LFO2 WAVEFORM
    SDL_SCANCODE_UNKNOWN, 0x33, // LFO2 KEY TRIG
    SDL_SCANCODE_UNKNOWN, 0x34, // COMMON VELOCITY CURVE
    SDL_SCANCODE_UNKNOWN, 0x35, // COMMON HOLD CONTROL
    SDL_SCANCODE_UNKNOWN, 0x36, // LFO2 OFFSET
    SDL_SCANCODE_UNKNOWN, 0x37, // LFO1 KEY TRIG
    SDL_SCANCODE_8, 0x38, // NUMBER 8
    SDL_SCANCODE_6, 0x39, // NUMBER 6
    SDL_SCANCODE_5, 0x3a, // NUMBER 5
    SDL_SCANCODE_3, 0x3b, // NUMBER 3
    SDL_SCANCODE_UNKNOWN, 0x3c, // WRITE
    SDL_SCANCODE_UNKNOWN, 0x3d, // DATA TRANSFER
    SDL_SCANCODE_2, 0x3e, // NUMBER 2
    SDL_SCANCODE_UNKNOWN, 0x3f, // INT/CARD
};


void LCD_SetBackPath(const std::string &path)
{
    m_back_path = path;
}

void LCD_Init(void)
{
    FILE *raw;

    if(lcd_init)
        return;

    lcd_quit_requested = false;

    std::string title = "Nuked SC-55: ";

    title += rs_name[romset];

    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, lcd_width, lcd_height, SDL_WINDOW_SHOWN);
    if (!window)
        return;

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        return;

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, lcd_width, lcd_height);

    if (!texture)
        return;

    raw = Files::utf8_fopen(m_back_path.c_str(), "rb");
    if (!raw)
        return;

    fread(lcd_background, 1, sizeof(lcd_background), raw);
    fclose(raw);

    lcd_init = 1;
}

void LCD_UnInit(void)
{
    if(!lcd_init)
        return;
}

uint32_t lcd_col1 = 0x000000;
uint32_t lcd_col2 = 0x0050c8;

void LCD_FontRenderStandard(int32_t x, int32_t y, uint8_t ch, bool overlay = false)
{
    uint8_t* f;
    if (ch >= 16)
        f = &lcd_font[ch - 16][0];
    else
        f = &LCD_CG[(ch & 7) * 8];
    for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            uint32_t col;
            if (f[i] & (1<<(4-j)))
            {
                col = lcd_col1;
            }
            else
            {
                col = lcd_col2;
            }
            int xx = x + i * 6;
            int yy = y + j * 6;
            for (int ii = 0; ii < 5; ii++)
            {
                for (int jj = 0; jj < 5; jj++)
                {
                    if (overlay)
                        lcd_buffer[xx+ii][yy+jj] &= col;
                    else
                        lcd_buffer[xx+ii][yy+jj] = col;
                }
            }
        }
    }
}

void LCD_FontRenderLevel(int32_t x, int32_t y, uint8_t ch, uint8_t width = 5)
{
    uint8_t* f;
    if (ch >= 16)
        f = &lcd_font[ch - 16][0];
    else
        f = &LCD_CG[(ch & 7) * 8];
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < width; j++)
        {
            uint32_t col;
            if (f[i] & (1<<(4-j)))
            {
                col = lcd_col1;
            }
            else
            {
                col = lcd_col2;
            }
            int xx = x + i * 11;
            int yy = y + j * 26;
            for (int ii = 0; ii < 9; ii++)
            {
                for (int jj = 0; jj < 24; jj++)
                {
                    lcd_buffer[xx+ii][yy+jj] = col;
                }
            }
        }
    }
}

static const uint8_t LR[2][12][11] =
{
    {
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,
    },
    {
        1,1,1,1,1,1,1,1,1,0,0,
        1,1,1,1,1,1,1,1,1,1,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,1,1,1,1,1,1,1,1,0,
        1,1,1,1,1,1,1,1,1,0,0,
        1,1,0,0,0,0,0,1,1,0,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,0,0,0,0,0,0,0,1,1,
        1,1,0,0,0,0,0,0,0,1,1,
    }
};

static const int LR_xy[2][2] = {
    { 70, 264 },
    { 232, 264 }
};


void LCD_FontRenderLR(uint8_t ch)
{
    uint8_t* f;
    if (ch >= 16)
        f = &lcd_font[ch - 16][0];
    else
        f = &LCD_CG[(ch & 7) * 8];
    int col;
    if (f[0] & 1)
    {
        col = lcd_col1;
    }
    else
    {
        col = lcd_col2;
    }
    for (int f = 0; f < 2; f++)
    {
        for (int i = 0; i < 12; i++)
        {
            for (int j = 0; j < 11; j++)
            {
                if (LR[f][i][j])
                    lcd_buffer[i+LR_xy[f][0]][j+LR_xy[f][1]] = col;
            }
        }
    }
}

void LCD_HorizontalSegment(int x, int y, int length, int thickness)
{
    for (int i = 0; i < thickness; i++) {
        for (int j = 0; j < length; j++) {
            lcd_buffer[y + i][x + j] = 1;
        }
    }
}

void LCD_VerticalSegment(int x, int y, int length, int thickness)
{
    for (int i = 0; i < length; i++) {
        for (int j = 0; j < thickness; j++) {
            lcd_buffer[y + i][x + j] = 1;
        }
    }
}

void LCD_RenderSegments(int32_t x, int32_t y, uint8_t digit)
{
    int segmentLength = 20;
    int segmentThickness = 3;

    if (digit & 0x01) LCD_HorizontalSegment(x + segmentThickness, y, segmentLength, segmentThickness);
    if (digit & 0x02) LCD_VerticalSegment(x + segmentLength + segmentThickness, y + segmentThickness, segmentLength, segmentThickness);
    if (digit & 0x04) LCD_VerticalSegment(x + segmentLength + segmentThickness, y + 2 * segmentThickness + segmentLength, segmentLength, segmentThickness);
    if (digit & 0x08) LCD_HorizontalSegment(x + segmentThickness, y + 2 * (segmentThickness + segmentLength), segmentLength, segmentThickness);
    if (digit & 0x10) LCD_VerticalSegment(x, y + 2 * segmentThickness + segmentLength, segmentLength, segmentThickness);
    if (digit & 0x20) LCD_VerticalSegment(x, y + segmentThickness, segmentLength, segmentThickness);
    if (digit & 0x40) LCD_HorizontalSegment(x + segmentThickness, y + segmentThickness + segmentLength, segmentLength, segmentThickness);
    if (digit & 0x80) LCD_HorizontalSegment(x + segmentLength + 3 * segmentThickness, y + 2 * (segmentThickness + segmentLength), segmentThickness, segmentThickness);
}

void LCD_Update(void)
{
    if (!lcd_init)
        return;

    if (!mcu_cm300 && !mcu_st && !mcu_scb55)
    {
        MCU_WorkThread_Lock();

        if (!lcd_enable && !mcu_jv880 && !mcu_jd990)
        {
            memset(lcd_buffer, 0, sizeof(lcd_buffer));
        }
        else
        {
            if (mcu_jv880 || mcu_xp10 || mcu_rd500 || mcu_ra30 || mcu_se70 || mcu_jd800 || mcu_jd990)
            {
                uint32_t back_color = 0xFF03be51;
                if (mcu_jd800) back_color = lcd_background[0][0];
                for (size_t i = 0; i < lcd_height; i++) {
                    for (size_t j = 0; j < lcd_width; j++) {
                        lcd_buffer[i][j] = back_color;
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < lcd_height; i++) {
                    for (size_t j = 0; j < lcd_width; j++) {
                        lcd_buffer[i][j] = lcd_background[i][j];
                    }
                }
            }

            if (mcu_rd500 || mcu_ra30)
            {
                LCD_RenderSegments(10 + 40 * 0, 10, LCD_7SEG[0]);
                LCD_RenderSegments(10 + 40 * 1, 10, LCD_7SEG[1]);
                LCD_RenderSegments(10 + 40 * 2, 10, LCD_7SEG[2]);
            }
            else if (mcu_jd990)
            {
                sed_state.screen_update(lcd_buffer);
                for (size_t y = 0; y < lcd_height_max; y++)
                {
                    for (size_t x = 0; x < lcd_width_max; x++)
                    {
                        if (lcd_buffer[y][x] == 0)
                            lcd_buffer[y][x] = 0xFF000000;
                        else
                            lcd_buffer[y][x] = 0xFFFFFFFF;
                    }
                }
                
            }
            else if (mcu_jv880 || mcu_xp10 || mcu_se70 || mcu_jd800)
            {
                int width = mcu_jd800 ? 40 : mcu_jv880 ? 24 : 16;
                for (int i = 0; i < 2; i++)
                {
                    for (int j = 0; j < width; j++)
                    {
                        uint8_t ch = LCD_Data[i * 40 + j];
                        LCD_FontRenderStandard(4 + i * 50, 4 + j * 34, ch);
                    }
                }
                
                // cursor
                int j = LCD_DD_RAM % 0x40;
                int i = LCD_DD_RAM / 0x40;
                if (i < 2 && j < 24 && LCD_C)
                    LCD_FontRenderStandard(4 + i * 50, 4 + j * 34, '_', true);
            }
            else
            {
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = LCD_Data[0 + i];
                    LCD_FontRenderStandard(11, 34 + i * 35, ch);
                }
                for (int i = 0; i < 16; i++)
                {
                    uint8_t ch = LCD_Data[3 + i];
                    LCD_FontRenderStandard(11, 153 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = LCD_Data[40 + i];
                    LCD_FontRenderStandard(75, 34 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = LCD_Data[43 + i];
                    LCD_FontRenderStandard(75, 153 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = LCD_Data[49 + i];
                    LCD_FontRenderStandard(139, 34 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = LCD_Data[46 + i];
                    LCD_FontRenderStandard(139, 153 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = LCD_Data[52 + i];
                    LCD_FontRenderStandard(203, 34 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = LCD_Data[55 + i];
                    LCD_FontRenderStandard(203, 153 + i * 35, ch);
                }

                LCD_FontRenderLR(LCD_Data[58]);

                for (int i = 0; i < 2; i++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        uint8_t ch = LCD_Data[20 + j + i * 40];
                        LCD_FontRenderLevel(71 + i * 88, 293 + j * 130, ch, j == 3 ? 1 : 5);
                    }
                }
            }
        }

        MCU_WorkThread_Unlock();

        SDL_UpdateTexture(texture, NULL, lcd_buffer, lcd_width_max * 4);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event))
    {
        if (sdl_event.type == SDL_KEYDOWN)
        {
            if (sdl_event.key.keysym.scancode == SDL_SCANCODE_COMMA)
                MCU_EncoderTrigger(0);
            if (sdl_event.key.keysym.scancode == SDL_SCANCODE_PERIOD)
                MCU_EncoderTrigger(1);
        }

        switch (sdl_event.type)
        {
            case SDL_QUIT:
                lcd_quit_requested = true;
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                if (sdl_event.key.repeat)
                    continue;
                
                // if (sdl_event.key.keysym.scancode == SDL_SCANCODE_L && sdl_event.type == SDL_KEYDOWN)
                // {
                //     MCU_PostUART(0x90);
                //     MCU_PostUART(0x30);
                //     MCU_PostUART(0x7f);
                // }
                // else if (sdl_event.key.keysym.scancode == SDL_SCANCODE_L && sdl_event.type == SDL_KEYUP)
                // {
                //     MCU_PostUART(0x80);
                //     MCU_PostUART(0x30);
                //     MCU_PostUART(0);
                // }

                if (sdl_event.key.keysym.scancode == SDL_SCANCODE_F4 && sdl_event.type == SDL_KEYDOWN)
                {
                    saveLSP();
                }
                if (sdl_event.key.keysym.scancode == SDL_SCANCODE_F5 && sdl_event.type == SDL_KEYDOWN)
                {
                    saveState();
                }
                if (sdl_event.key.keysym.scancode == SDL_SCANCODE_F6 && sdl_event.type == SDL_KEYDOWN)
                {
                    loadState();
                }
                
                int mask = 0;
                uint32_t button_pressed = (uint32_t)SDL_AtomicGet(&mcu_button_pressed);

                printf("key %s %d\n", SDL_GetScancodeName(sdl_event.key.keysym.scancode), sdl_event.type);

                if (mcu_jd800)
                {
                    for (size_t i = 0; i < sizeof(button_map_jd800) / sizeof(button_map_jd800[0]); i++)
                    {
                        if (button_map_jd800[i][0] == sdl_event.key.keysym.scancode)
                            jd800_btn_down[button_map_jd800[i][1]] = sdl_event.type == SDL_KEYDOWN;
                    }
                }
                else
                {
                    auto button_map =
                        mcu_xp10 ? button_map_xp10 :
                        mcu_rd500 ? button_map_rd500 :
                        mcu_jv880 ? button_map_jv880 :
                        (mcu_sc88 || mcu_sc88pro) ? button_map_sc88 :
                        mcu_ra30 ? button_map_ra30 :
                        mcu_se70 ? button_map_se70 :
                        button_map_sc55;
                    auto button_size = (
                        mcu_xp10 ? sizeof(button_map_xp10) :
                        mcu_rd500 ? sizeof(button_map_rd500) :
                        mcu_jv880 ? sizeof(button_map_jv880) :
                        (mcu_sc88 || mcu_sc88pro) ? sizeof(button_map_sc88) :
                        mcu_ra30 ? sizeof(button_map_ra30) :
                        mcu_se70 ? sizeof(button_map_se70) :
                        sizeof(button_map_sc55)
                    ) / sizeof(button_map_sc55[0]);
                    for (size_t i = 0; i < button_size; i++)
                    {
                        if (button_map[i][0] == sdl_event.key.keysym.scancode)
                            mask |= (1 << button_map[i][1]);
                    }

                    if (sdl_event.type == SDL_KEYDOWN)
                        button_pressed |= mask;
                    else
                        button_pressed &= ~mask;
                }

                SDL_AtomicSet(&mcu_button_pressed, (int)button_pressed);

                // printf("button_pressed %02x\n", button_pressed);

#if 0
                if (sdl_event.key.keysym.scancode >= SDL_SCANCODE_1 && sdl_event.key.keysym.scancode < SDL_SCANCODE_0)
                {
#if 0
                    int kk = sdl_event.key.keysym.scancode - SDL_SCANCODE_1;
                    if (sdl_event.type == SDL_KEYDOWN)
                    {
                        MCU_PostUART(0xc0);
                        MCU_PostUART(118);
                        MCU_PostUART(0x90);
                        MCU_PostUART(0x30 + kk);
                        MCU_PostUART(0x7f);
                    }
                    else
                    {
                        MCU_PostUART(0x90);
                        MCU_PostUART(0x30 + kk);
                        MCU_PostUART(0);
                    }
#endif
                    int kk = sdl_event.key.keysym.scancode - SDL_SCANCODE_1;
                    const int patch = 47;
                    if (sdl_event.type == SDL_KEYDOWN)
                    {
                        static int bend = 0x2000;
                        if (kk == 4)
                        {
                            MCU_PostUART(0x99);
                            MCU_PostUART(0x32);
                            MCU_PostUART(0x7f);
                        }
                        else if (kk == 3)
                        {
                            bend += 0x100;
                            if (bend > 0x3fff)
                                bend = 0x3fff;
                            MCU_PostUART(0xe1);
                            MCU_PostUART(bend & 127);
                            MCU_PostUART((bend >> 7) & 127);
                        }
                        else if (kk == 2)
                        {
                            bend -= 0x100;
                            if (bend < 0)
                                bend = 0;
                            MCU_PostUART(0xe1);
                            MCU_PostUART(bend & 127);
                            MCU_PostUART((bend >> 7) & 127);
                        }
                        else if (kk)
                        {
                            MCU_PostUART(0xc1);
                            MCU_PostUART(patch);
                            MCU_PostUART(0xe1);
                            MCU_PostUART(bend & 127);
                            MCU_PostUART((bend >> 7) & 127);
                            MCU_PostUART(0x91);
                            MCU_PostUART(0x32);
                            MCU_PostUART(0x7f);
                        }
                        else if (kk == 0)
                        {
                            //MCU_PostUART(0xc0);
                            //MCU_PostUART(patch);
                            MCU_PostUART(0xe0);
                            MCU_PostUART(0x00);
                            MCU_PostUART(0x40);
                            MCU_PostUART(0x99);
                            MCU_PostUART(0x37);
                            MCU_PostUART(0x7f);
                        }
                    }
                    else
                    {
                        if (kk == 1)
                        {
                            MCU_PostUART(0x91);
                            MCU_PostUART(0x32);
                            MCU_PostUART(0);
                        }
                        else if (kk == 0)
                        {
                            MCU_PostUART(0x99);
                            MCU_PostUART(0x37);
                            MCU_PostUART(0);
                        }
                        else if (kk == 4)
                        {
                            MCU_PostUART(0x99);
                            MCU_PostUART(0x32);
                            MCU_PostUART(0);
                        }
                    }
                }
#endif
                break;
            }
        }
    }
}
