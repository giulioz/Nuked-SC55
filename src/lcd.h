#pragma once

#include <stdint.h>

class LCD {
public:
  int mode = 0;
  int stage = 0;
  bool display_enabled = false;

  bool config_m0 = 0;     // internal/external cg rom
  bool config_m1 = 0;     // d6 correction
  bool config_m2 = 0;     // 8/16 pixels height
  bool config_ws = 0;     // single/dual panel
  bool config_iv = 0;     // invert
  uint8_t config_fx = 0;  // char pixel width
  bool config_wf = 0;     // ac frame wf period
  uint8_t config_fy = 0;  // vertical char size
  uint8_t config_cr = 0;  // bytes per line
  uint8_t config_tcr = 0; // line length
  uint8_t config_lf = 0;  // height in lines
  uint16_t config_ap = 0; // virtual screen horizontal address

  uint16_t scroll_sad1 = 0;
  uint16_t scroll_sad2 = 0;
  uint16_t scroll_sad3 = 0;
  uint16_t scroll_sad4 = 0;
  uint8_t scroll_sl1 = 0; // screen lines
  uint8_t scroll_sl2 = 0;

  uint8_t cursor_dir = 0;
  uint16_t cursor = 0;

  uint8_t memory[0x2000] = {0};

  void writeData(uint8_t value) {
    if (mode == 0x40) {
      // system set
      switch (stage) {
      case 0:
        config_m0 = (value & 1) != 0;
        config_m1 = (value & 2) != 0;
        config_m2 = (value & 4) != 0;
        config_ws = (value & 8) != 0;
        config_iv = (value & 32) != 0;
        break;
      case 1:
        config_fx = value & 0b111;
        config_wf = (value & 128) != 0;
        break;
      case 2:
        config_fy = value & 0b11111;
        break;
      case 3:
        config_cr = value;
        break;
      case 4:
        config_tcr = value;
        break;
      case 5:
        config_lf = value;
        break;
      case 6:
        config_ap = (config_ap & 0xff00) | value;
        break;
      case 7:
        config_ap = (config_ap & 0x00ff) | (value << 8);
        break;
      }
      stage += 1;
      return;
    }

    if (mode == 0x44) {
      // scroll
      switch (stage) {
      case 0:
        scroll_sad1 = (scroll_sad1 & 0xff00) | value;
        break;
      case 1:
        scroll_sad1 = (scroll_sad1 & 0x00ff) | (value << 8);
        break;
      case 2:
        scroll_sl1 = value;
        break;
      case 3:
        scroll_sad2 = (scroll_sad2 & 0xff00) | value;
        break;
      case 4:
        scroll_sad2 = (scroll_sad2 & 0x00ff) | (value << 8);
        break;
      case 5:
        scroll_sl2 = value;
        break;
      case 6:
        scroll_sad3 = (scroll_sad3 & 0xff00) | value;
        break;
      case 7:
        scroll_sad3 = (scroll_sad3 & 0x00ff) | (value << 8);
        break;
      case 8:
        scroll_sad4 = (scroll_sad4 & 0xff00) | value;
        break;
      case 9:
        scroll_sad4 = (scroll_sad4 & 0x00ff) | (value << 8);
        break;
      }
      stage += 1;
      return;
    }

    if (mode == 0x46) {
      // set cursor addr
      switch (stage) {
      case 0:
        cursor = (cursor & 0xff00) | value;
        break;
      case 1:
        cursor = (cursor & 0x00ff) | (value << 8);
        break;
      }
      stage += 1;
      return;
    }

    if (mode == 0x42) {
      // mem write
      memory[cursor & 0x1fff] = value;
      cursor += 1; // hack
      return;
    }
  }

  void writeParam(uint8_t value) {
    if (value == 0x40) {
      // system set
      mode = 0x40;
      stage = 0;
      display_enabled = false;
      return;
    }

    if (value == 0x42) {
      // mem write
      mode = 0x42;
      return;
    }

    if (value == 0x44) {
      // scroll
      mode = 0x44;
      stage = 0;
      return;
    }

    if (value == 0x46) {
      // set cursor addr
      mode = 0x46;
      stage = 0;
      return;
    }

    if (value >= 0x4c && value <= 0x4f) {
      // set cursor dir
      cursor_dir = value & 3;
      return;
    }

    if ((value & 0xfe) == 0x58) {
      // display on/off
      display_enabled = (value & 1) != 0;
      return;
    }

    if (value == 0x5a) {
      // set horiz scroll position
      return;
    }

    if (value == 0x5b) {
      // set display overlay format
      return;
    }

    if (value == 0x5c) {
      // set char gen start address
      return;
    }

    if (value == 0x5d) {
      // set cursor type
      return;
    }
  }
};
