//
//  XenonAudio_Regs.hpp
//  Xbox 360 audio device registers
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonAudioRegs_hpp
#define XenonAudioRegs_hpp

// 16-bit PCM in stereo.
#define kXenonAudioNumChannels        2
#define kXenonAudioBitWidth           16
#define kXenonAudioBytesPerFrame      (kXenonAudioNumChannels * (kXenonAudioBitWidth / 8))

#define kXenonAudioTimerUpdateUS      200

#define kXenonAudioDescCount          32
#define kXenonAudioDescMask           0x1F
#define kXenonAudioDescBufferLength   (kXenonAudioDescCount * sizeof (UInt32) * 2)
#define kXenonSampleBufferLength      0x10000

//
// Audio registers.
//
// Audio descriptors base address.
#define kXenonAudioRegDescAddr              0x00
// Audio buffer state.
#define kXenonAudioRegState                 0x04
#define kXenonAudioRegStateReadPtrMask      BITRange(0, 4)
#define kXenonAudioRegStateWritePtrShift    8
#define kXenonAudioRegStateWritePtrMask     BITRange(8, 12)
#define kXenonAudioRegStateLengthShift      16
// Audio control.
#define kXenonAudioRegControl               0x08
#define kXenonAudioRegControlRun            BIT24
// Audio format? Analog vs digital.
#define kXenonAudioRegFormat                0x0C

#endif
