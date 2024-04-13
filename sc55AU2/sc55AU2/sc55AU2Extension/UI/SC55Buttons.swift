//
//  SC55Buttons.swift
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 13.04.24.
//

import SwiftUI

struct SC55Buttons: View {
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    var body: some View {
        VStack {
            SCButton(code: MCU_BUTTON_INST_ALL, text: "ALL", audioUnit: audioUnit)
            SCButton(code: MCU_BUTTON_INST_MUTE, text: "MUTE", audioUnit: audioUnit)
            
            SCButton(code: MCU_BUTTON_POWER, text: "POWER", audioUnit: audioUnit)
            
            Button(action: {
                audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_PART_L), 1)
                audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_PART_R), 1)
                audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 1)
                DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
                    audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_PART_L), 0)
                    audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_PART_R), 0)
                    audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 0)
                }
            }){ Text("DEMO") }

            Button(action: {
                audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_INST_L), 1)
                audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_INST_R), 1)
                audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 1)
                DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
                    audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_INST_L), 0)
                    audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_INST_R), 0)
                    audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 0)
                }
            }){ Text("INIT ALL") }

            Button(action: {
                audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_INST_R), 1)
                audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 1)
                DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
                    audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_INST_R), 0)
                    audioUnit?.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 0)
                }
            }){ Text("GS RESET") }
            
            Button(action: {
                audioUnit?.sc55_Reset()
            }) {
                Text("RESET")
            }
        }.frame(width: 100, alignment: .top)
        
        VStack {
            Text("Part")
            HStack {
                SCButton(code: MCU_BUTTON_PART_L, text: "<", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_PART_R, text: ">", audioUnit: audioUnit)
            }
            Text("Level")
            HStack {
                SCButton(code: MCU_BUTTON_LEVEL_L, text: "<", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_LEVEL_R, text: ">", audioUnit: audioUnit)
            }
            Text("Reverb")
            HStack {
                SCButton(code: MCU_BUTTON_REVERB_L, text: "<", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_REVERB_R, text: ">", audioUnit: audioUnit)
            }
            Text("Key Shift")
            HStack {
                SCButton(code: MCU_BUTTON_KEY_SHIFT_L, text: "<", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_KEY_SHIFT_R, text: ">", audioUnit: audioUnit)
            }
        }.frame(width: 100, alignment: .center)
        
        VStack {
            Text("Instrument")
            HStack {
                SCButton(code: MCU_BUTTON_INST_L, text: "<", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_INST_R, text: ">", audioUnit: audioUnit)
            }
            Text("Pan")
            HStack {
                SCButton(code: MCU_BUTTON_PAN_L, text: "<", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_PAN_R, text: ">", audioUnit: audioUnit)
            }
            Text("Chorus")
            HStack {
                SCButton(code: MCU_BUTTON_CHORUS_L, text: "<", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_CHORUS_R, text: ">", audioUnit: audioUnit)
            }
            Text("Midi CH")
            HStack {
                SCButton(code: MCU_BUTTON_MIDI_CH_L, text: "<", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_MIDI_CH_R, text: ">", audioUnit: audioUnit)
            }
        }.frame(width: 100, alignment: .center)
    }
}

#Preview {
    SC55Buttons()
}
