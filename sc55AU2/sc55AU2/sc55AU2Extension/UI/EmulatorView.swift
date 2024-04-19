//
//  EmulatorView.swift
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 04.04.24.
//

import SwiftUI

public struct Color {
    public var r, g, b, a: UInt8
    
    public init(r: UInt8, g: UInt8, b: UInt8, a: UInt8 = 255) {
        self.r = r
        self.g = g
        self.b = b
        self.a = a
    }
}

extension Image {
    init?(bitmap: Bitmap) {
        let alphaInfo = CGImageAlphaInfo.premultipliedLast
        let bytesPerPixel = MemoryLayout<Color>.size
        let bytesPerRow = bitmap.width * bytesPerPixel

        guard let providerRef = CGDataProvider(data: Data(
            bytes: bitmap.pixels, count: bitmap.height * bytesPerRow
        ) as CFData) else {
            return nil
        }

        guard let cgImage = CGImage(
            width: bitmap.width,
            height: bitmap.height,
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

public struct Bitmap {
    public private(set) var pixels: [Color]
    public let width: Int
    
    public init(width: Int, pixels: [Color]) {
        self.width  = width
        self.pixels = pixels
    }
    
    var height: Int {
        return pixels.count / width
    }
    
    subscript(x: Int, y: Int) -> Color {
        get { return pixels[y * width + x] }
        set { pixels[y * width + x] = newValue }
    }

    init(width: Int, height: Int, color: Color) {
        self.pixels = Array(repeating: color, count: width * height)
        self.width  = width
    }
}

// let width = 741;
// let height = 268;
let lcd_width = 820;
let lcd_height = 100;

public struct Renderer {
    public private(set) var bitmap: Bitmap
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    public init(width: Int, height: Int) {
        self.bitmap = Bitmap(width: width, height: height, color: Color.init(
            r: 0, g: 0,b: 0, a: 0
        ))
    }
    
    mutating func draw() {
        if let audioUnitR = audioUnit {
            let lcdResult = audioUnitR.lcd_Update();
            if lcdResult != nil {
                for x in 0...(lcd_width-1) {
                    for y in 0...(lcd_height-1) {
                        bitmap[x, y] = Color.init(
                            r: UInt8((lcdResult![x + y * 1024] >> 0) & 0xff),
                            g: UInt8((lcdResult![x + y * 1024] >> 8) & 0xff),
                            b: UInt8((lcdResult![x + y * 1024] >> 16) & 0xff),
                            a: 255
                        )
                    }
                }
             }
        }
    }
}

class Screen : ObservableObject {
    @Published
    var toggle: Bool = false
    var frames: Int64 = -1
    public  var image: Image? {
        didSet {
            DispatchQueue.main.async { [weak self] in
                self?.toggle.toggle()
            }
        }
    }
    
    var renderer: Renderer
    private var displayLink: CVDisplayLink?
    
    let displayCallback: CVDisplayLinkOutputCallback = { displayLink, inNow, inOutputTime, flagsIn, flagsOut, displayLinkContext in
        let screen = unsafeBitCast(displayLinkContext, to: Screen.self)
        screen.frames = screen.frames + 1
        screen.renderer.draw()
        if (screen.frames % 8) == 0 {
            screen.image = Image(bitmap: screen.renderer.bitmap)
        }
        return kCVReturnSuccess
    }
    
    init(width: Int, height: Int) {
        self.renderer = Renderer(width: width, height: height)

        let error = CVDisplayLinkCreateWithActiveCGDisplays(&self.displayLink)
        guard let link = self.displayLink, kCVReturnSuccess == error else {
            NSLog("Display Link created with error: %d", error)
            return
        }
        CVDisplayLinkSetOutputCallback(link, displayCallback, UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque()))
        CVDisplayLinkStart(link)
    }
    
    deinit {
        CVDisplayLinkStop(self.displayLink!)
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
    var screen = Screen(width: lcd_width, height: lcd_height)
    
    var body: some View {
        VStack {
            screen.image?
                .resizable()
                .frame(width: CGFloat(lcd_width)/2, height: CGFloat(lcd_height)/2, alignment: .center)
                .aspectRatio(contentMode: .fit)
                .onAppear(perform: { screen.renderer.audioUnit = audioUnit })
            
//            SC55Buttons(audioUnit: audioUnit)
            JV880Buttons(audioUnit: audioUnit)
        }
    }
}

#Preview {
    EmulatorView()
}
