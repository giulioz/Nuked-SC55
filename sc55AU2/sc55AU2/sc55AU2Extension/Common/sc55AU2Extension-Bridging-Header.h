//
//  sc55AU2Extension-Bridging-Header.h
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

#import "sc55AU2ExtensionAudioUnit.h"
#import "sc55AU2ExtensionParameterAddresses.h"

enum {
    // SC55
    MCU_BUTTON_POWER = 0,
    MCU_BUTTON_INST_L = 3,
    MCU_BUTTON_INST_R = 4,
    MCU_BUTTON_INST_MUTE = 5,
    MCU_BUTTON_INST_ALL = 6,
    MCU_BUTTON_MIDI_CH_L = 7,
    MCU_BUTTON_MIDI_CH_R = 8,
    MCU_BUTTON_CHORUS_L = 9,
    MCU_BUTTON_CHORUS_R = 10,
    MCU_BUTTON_PAN_L = 11,
    MCU_BUTTON_PAN_R = 12,
    MCU_BUTTON_PART_R = 13,
    MCU_BUTTON_KEY_SHIFT_L = 14,
    MCU_BUTTON_KEY_SHIFT_R = 15,
    MCU_BUTTON_REVERB_L = 16,
    MCU_BUTTON_REVERB_R = 17,
    MCU_BUTTON_LEVEL_L = 18,
    MCU_BUTTON_LEVEL_R = 19,
    MCU_BUTTON_PART_L = 20,

    // JV880
    MCU_BUTTON_CURSOR_L = 0,
    MCU_BUTTON_CURSOR_R = 1,
    MCU_BUTTON_TONE_SELECT = 2,
    MCU_BUTTON_MUTE = 3,
    MCU_BUTTON_DATA = 4,
    MCU_BUTTON_MONITOR = 5,
    MCU_BUTTON_COMPARE = 6,
    MCU_BUTTON_ENTER = 7,
    MCU_BUTTON_UTILITY = 8,
    MCU_BUTTON_PREVIEW = 9,
    MCU_BUTTON_PATCH_PERFORM = 10,
    MCU_BUTTON_EDIT = 11,
    MCU_BUTTON_SYSTEM = 12,
    MCU_BUTTON_RHYTHM = 13,
};
