//
//  EmulatorView.swift
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 04.04.24.
//

import SwiftUI

// let width = 741;
// let height = 268;
let lcd_width = 820;
let lcd_height = 100;

extension Image {
    init?(imgData: [UInt8]) {
        let alphaInfo = CGImageAlphaInfo.premultipliedLast
        let bytesPerPixel = 4
        let bytesPerRow = lcd_width * bytesPerPixel

        guard let providerRef = CGDataProvider(data: Data(
            bytes: imgData, count: lcd_height * bytesPerRow
        ) as CFData) else {
            return nil
        }

        guard let cgImage = CGImage(
            width: lcd_width,
            height: lcd_height,
            bitsPerComponent: 8,
            bitsPerPixel: bytesPerPixel * 8,
            bytesPerRow: bytesPerRow,
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: CGBitmapInfo(rawValue: alphaInfo.rawValue),
            provider: providerRef,
            decode: nil,
            shouldInterpolate: true,
            intent: .defaultIntent
        ) else {
            return nil
        }

        self.init(decorative: cgImage, scale: 1.0, orientation: .up)
    }
}

class Screen : ObservableObject {
    @Published
    var toggle: Bool = false
    public var image: Image? {
        didSet {
            DispatchQueue.main.async { [weak self] in
                self?.toggle.toggle()
            }
        }
    }
    
    public private(set) var imgData: [UInt8]
    var audioUnit: sc55AU2ExtensionAudioUnit?
    var timer: Timer?
    
    init() {
        self.imgData = Array(repeating: 0x00, count: lcd_width * lcd_height * 4)
    }
    
    deinit {
        timer?.invalidate()
    }
    
    func start() {
        timer = Timer.scheduledTimer(timeInterval: 0.1, target: self, selector: #selector(timerAction), userInfo: nil, repeats: true)
    }
    
    @objc func timerAction() {
        self.draw()
        self.image = Image(imgData: self.imgData)
    }

    func draw() {
        if let audioUnitR = audioUnit {
            let lcdResult = audioUnitR.lcd_Update();
            if lcdResult != nil {
                for y in 0...(lcd_height-1) {
                    for x in 0...(lcd_width-1) {
                        imgData[(x + y * lcd_width) * 4 + 0] = UInt8((lcdResult![x + y * 1024] >> 0) & 0xff)
                        imgData[(x + y * lcd_width) * 4 + 1] = UInt8((lcdResult![x + y * 1024] >> 8) & 0xff)
                        imgData[(x + y * lcd_width) * 4 + 2] = UInt8((lcdResult![x + y * 1024] >> 16) & 0xff)
                        imgData[(x + y * lcd_width) * 4 + 3] = 255
                    }
                }
             }
        }
    }
}

struct SCButton: View {
    let code: Int
    let text: String
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    var body: some View {
        Button(action: {
            audioUnit?.lcd_SendButton(UInt8(code), 1)
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                audioUnit?.lcd_SendButton(UInt8(code), 0)
            }
        }){
            Text(text)
        }.frame(maxWidth: .infinity)
    }
}

struct EmulatorView: View {
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    @ObservedObject
    var screen = Screen()
    
    var body: some View {
        VStack {
            screen.image?
                .resizable()
                .frame(width: CGFloat(lcd_width)/2, height: CGFloat(lcd_height)/2, alignment: .center)
                .aspectRatio(contentMode: .fit)
            
//            SC55Buttons(audioUnit: audioUnit)
            JV880Buttons(audioUnit: audioUnit)
        }.onAppear(perform: {
            screen.audioUnit = audioUnit
            screen.start()
        })
    }
}

#Preview {
    EmulatorView()
}
