# libvr
A cross-platform library for quickly constructing iOS and Android video player, 360 degree, 360 degree 3D VR player etc. 
Support local file playing and http, rtmp, hls(correctly support DISCONTINUITY) etc. protocols.

## Preview
![ScreenShot](https://github.com/sqvcnet/react-native-libvr-demo/raw/master/screenshots/screenshots.png)

## Features
- Common
- remove rarely used ffmpeg components to reduce binary size
- workaround for some buggy online video.


- Android
- platform: API 9~23
- cpu: ARMv7a, ARM64v8a, x86 (ARMv5 is not tested on real devices)
- video-output: OpenGL ES 2.0
- audio-output: AudioTrack, OpenSL ES
- hw-decoder: MediaCodec (API 16+, Android 4.1+)


- iOS
- platform: iOS 7.0~10.3.x
- cpu: armv7, arm64, i386, x86_64, (armv7s is obselete)
- video-output: OpenGL ES 2.0
- audio-output: AudioQueue
- hw-decoder: VideoToolbox (iOS 8+)

## How to use
#### See https://github.com/sqvcnet/react-native-libvr-demo and https://github.com/sqvcnet/react-native-libvr for detailed usages
#### You can also run the simple demo.xcodeproj to take the first glance
#### Notice: If you want to use this project in your own iOS project, remember to add "CoreMedia.framework, AudioToolbox.framework, VideoToolbox.framework, libz.dylib, libc++.dylib, GLKit.framework" in "Link Binary with Libraries" of project "Build Phrases". In order to add libz.dylib and libc++.dylib, you may be need to "Add Other" then press "Shift+Command+G" then input "/usr/lib" then find them, since the *.tbd cannot work.

### For iOS
You should add NSMotionUsageDescription(Privacy - Motion Usage Description) in Info.plist, see https://developer.apple.com/documentation/coremotion

### Important
The OpenGL ES code may run faster or slower in simulator than on an actual device. Always profile and optimize your drawing code on a real device, and never assume that Simulator reflects real-world performance.
See https://developer.apple.com/library/content/documentation/OpenGLES/Conceptual/OpenGLESHardwarePlatformGuide_iOS/OpenGLESiniOSSimulator/OpenGLESiniOSSimulator.html for more details.

