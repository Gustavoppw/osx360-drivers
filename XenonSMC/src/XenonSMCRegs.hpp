//
//  XenonSMCRegs.hpp
//  Xbox 360 system management controller registers
//
//  See https://github.com/wurthless-elektroniks/smc360/blob/main/docs/ipc.md
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonSMCRegs_hpp
#define XenonSMCRegs_hpp

#include "XenonCommon.hpp"

#define kXenonSMCTimeout        10000000

// Seconds since Nov 15, 2001 at 00:00 UTC (original Xbox release date).
#define kXenonSMCRTCBase        1005782400

//
// SMC registers.
//
// UART.
#define kXenonSMCRegUartOutData         0x10
#define kXenonSMCRegUartInData          0x14
#define kXenonSMCRegUartStatus          0x18
#define kXenonSMCRegUartConfig          0x1C

// Interrupts.
#define kXenonSMCRegIntStatus           0x50
#define kXenonSMCRegIntStatusPending    BIT28
#define kXenonSMCRegIntAck              0x58
#define kXenonSMCRegIntEnabled          0x5C
#define kXenonSMCRegIntEnabledBit       0xC

// Clock.
#define kXenonSMCRegClockIntEnabled     0x64
#define kXenonSMCRegClockIntStatus      0x6C
// FIFO.
#define kXenonSMCRegFifoOutData         0x80
#define kXenonSMCRegFifoOutStatus       0x84
#define kXenonSMCRegFifoOutStatusReady  BIT2
#define kXenonSMCRegFifoInData          0x90
#define kXenonSMCRegFifoInStatus        0x94
#define kXenonSMCRegFifoInStatusReady   BIT2

#pragma pack(1)

//
// SMC commands.
//
#define kXenonSMCCommandGetRTC          0x04
#define kXenonSMCCommandGetTemp         0x07
#define kXenonSMCCommandGetTrayStatus   0x0A
#define kXenonSMCCommandGetAVStatus     0x0F
#define kXenonSMCCommandGetVersion      0x12
#define kXenonSMCCommandGetIRAddr       0x16
#define kXenonSMCCommandGetTiltStatus   0x17
#define kXenonSMCCommandSetPower        0x82
#define kXenonSMCCommandAsync           0x83
#define kXenonSMCCommandSetRTC          0x85
#define kXenonSMCCommandOpenCloseTray   0x8B
#define kXenonSMCCommandSetPowerLED     0x8C
#define kXenonSMCCommandSetRingLED      0x99

// Temperature status.
typedef struct {
  UInt8 cpuTempFraction;
  UInt8 cpuTemp;
  UInt8 gpuTempFraction;
  UInt8 gpuTemp;
  UInt8 edramTempFraction;
  UInt8 edramTemp;
  UInt8 chassisTempFraction;
  UInt8 chassisTemp;
  UInt8 fanTargetSpeed;
} XenonSMCTempStatus;

// Tray status.
#define kXenonSMCTrayStatusOpen         0x60
#define kXenonSMCTrayStatusClosed       0x62
#define kXenonSMCTrayStatusOpening      0x63
#define kXenonSMCTrayStatusClosing      0x64
#define kXenonSMCTrayStatusError        0x65

// Tilt switch.
#define kXenonSMCTiltPresent            BIT0

// Power messages.
#define kXenonSMCPowerPowerOff          0x01
#define kXenonSMCPowerReboot            0x04
#define kXenonSMCPowerRebootHard        0x30
#define kXenonSMCPowerRebootSoft        0x31
#define kXenonSMCPowerRebootCancel      0x33

typedef struct {
  UInt8 type;
  UInt8 special;
  UInt8 delay;
  UInt8 flags;
} XenonSMCPowerState;

// Async messages.
#define kXenonSMCAsyncPowerButton       0x11
#define kXenonSMCAsyncSyncButton        0x13
#define kXenonSMCAsyncTiltSwitch        0x14
#define kXenonSMCAsyncInfraredButton    0x20
#define kXenonSMCAsyncSoftResetAck      0x31
#define kXenonSMCAsyncAVChange          0x40
#define kXenonSMCAsyncEject             0x61

//
// SMC combined message.
//
typedef struct {
  union {
    struct {
      // Command byte.
      UInt8 command;

      // Data bytes.
      union {
        UInt8 data[15];

        UInt8 trayStatus;
        UInt8 tiltStatus;

        XenonSMCTempStatus  temp;
        XenonSMCPowerState  power;
      };
    };

    UInt32 bytes32[4];
  };
} XenonSMCMessage;
OSCompileAssert(sizeof(XenonSMCMessage) == 16);

#pragma pack()

#endif
