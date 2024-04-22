import SwiftUI

struct SysexSlider: View {
    let address: UInt32
    let text: String
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    @State var value = 0.0
    
    var body: some View {
        VStack {
            Slider(
                value: $value,
                in: 0...1,
                minimumValueLabel: Text("\(0)"),
                maximumValueLabel: Text("\(1)")
            ) {
                EmptyView()
            }.onChange(of: value) { oldValue, newValue in
                audioUnit?.mcu_PostUART(0xf0)
                audioUnit?.mcu_PostUART(0x41)
                audioUnit?.mcu_PostUART(0x10) // unit number
                audioUnit?.mcu_PostUART(0x46)
                audioUnit?.mcu_PostUART(0x12) // command
                let data: [Int] = [
                  Int((address >> 21) & 127), // address MSB
                  Int((address >> 14) & 127), // address
                  Int((address >> 7) & 127),  // address
                  Int((address >> 0) & 127),  // address LSB
                  Int(floor(value * 128.0)),  // data
                ]
                var checksum = 0
                for b in data {
                    checksum += b
                    if (checksum >= 128) {
                        checksum -= 128
                    }
                }
                checksum = 128 - checksum
                for b in data {
                  audioUnit?.mcu_PostUART(UInt8(b))
                }
                audioUnit?.mcu_PostUART(UInt8(checksum))
                audioUnit?.mcu_PostUART(0xf7)
            }
            Text("\(text): \(value)")
        }
        .padding()
    }
}

struct SystemConfig: View {
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    @State var value = 0.0
    
    var body: some View {
        VStack {
            SysexSlider(address: 0x01, text: "Master Tune", audioUnit: audioUnit)
        }
        .padding()
    }
}

#Preview {
    SystemConfig()
}
