//
//  JV880Buttons.swift
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 13.04.24.
//

import SwiftUI

struct JV880Buttons: View {
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    var body: some View {
        HStack {
            VStack {
                HStack {
                    Button(action: { audioUnit?.mcu_EncoderTrigger(0) }){ Text("D-") }
                    Button(action: { audioUnit?.mcu_EncoderTrigger(1) }){ Text("D+") }
                }.frame(width: 100, alignment: .center)
                HStack {
                    SCButton(code: MCU_BUTTON_CURSOR_L, text: "C-", audioUnit: audioUnit)
                    SCButton(code: MCU_BUTTON_CURSOR_R, text: "C+", audioUnit: audioUnit)
                }.frame(width: 100, alignment: .center)
            }

            VStack {
                SCButton(code: MCU_BUTTON_PATCH_PERFORM, text: "PA/PF", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_TONE_SELECT, text: "TS", audioUnit: audioUnit)
            }.frame(width: 100, alignment: .center)

            VStack {
                HStack {
                    SCButton(code: MCU_BUTTON_EDIT, text: "EDIT", audioUnit: audioUnit)
                    SCButton(code: MCU_BUTTON_UTILITY, text: "UTILITY", audioUnit: audioUnit)
                    SCButton(code: MCU_BUTTON_SYSTEM, text: "SYSTEM", audioUnit: audioUnit)
                    SCButton(code: MCU_BUTTON_RHYTHM, text: "RHYTHM", audioUnit: audioUnit)
                }

                HStack {
                    SCButton(code: MCU_BUTTON_MUTE, text: "MUTE", audioUnit: audioUnit)
                    SCButton(code: MCU_BUTTON_MONITOR, text: "MONITOR", audioUnit: audioUnit)
                    SCButton(code: MCU_BUTTON_COMPARE, text: "COMPARE", audioUnit: audioUnit)
                    SCButton(code: MCU_BUTTON_ENTER, text: "ENTER", audioUnit: audioUnit)
                }
            }

            SCButton(code: MCU_BUTTON_PREVIEW, text: "PREVIEW", audioUnit: audioUnit)
        }
    }
}

#Preview {
    JV880Buttons()
}
