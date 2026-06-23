//
//  XenonICRegs.hpp
//  Xbox 360 platform interrupt controller registers
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonICRegs_hpp
#define XenonICRegs_hpp

#include "XenonCommon.hpp"

#define kXenonICBase                0x20000050000ULL
#define kXenonICLength              0x10000

#define kXenonICCoreRegOffset       0x1000
#define kXenonICVectorCount         128

//
// Per-core registers.
//
// Logical processor ID
#define kXenonICRegLogicalID                  0x00
// Current interrupt priority level
#define kXenonICRegPriority                   0x08
//
#define kXenonICRegIPIGeneration              0x10
// On read, gets the highest priority pending interrupt and acks it
#define kXenonICRegInterruptAck               0x50
#define kXenonICRegInterruptAckMask           0x7F
// ?
#define kXenonICRegInterruptAckAutoUpd        0x58
// On write, signal EOI for first interrupt in ack queue
#define kXenonICRegEndOfInterrupt             0x60
// On write, signal EOI for first interrupt in ack queue, and update current interrupt priority
#define kXenonICRegEndOfInterruptAutoUpd      0x68
// Vector to use for spurious interrupt?
#define kXenonICRegSpuriousVector             0x70

//
// PCI bridge interrupt registers.
//
#define kXenonPCIBridgeVectorCount            16
#define kXenonPCIBridgeRegIntBase             0x10
#define kXenonPCIBridgeRegIntEnabled          BIT23
#define kXenonPCIBridgeRegIntLatched          BIT21
#define kXenonPCIBridgeRegIntTargetCPUShift   8
#define kXenonPCIBridgeRegIntTargetCPUMask    BITRange(8, 13)
#define kXenonPCIBridgeRegIntCPUIRQShift      2
#define kXenonPCIBridgeRegIntCPUMask          BITRange(0, 5)

//
// Device interrupts.
//
#define kXenonVectorIPI4            0x08
#define kXenonVectorIPI3            0x10
#define kXenonVectorSMC             0x14
#define kXenonVectorFlash           0x18
#define kXenonVectorSATAHDD         0x20
#define kXenonVectorSATACD          0x24
#define kXenonVectorOHCI0           0x2C
#define kXenonVectorEHCI0           0x30
#define kXenonVectorOHCI1           0x34
#define kXenonVectorEHCI1           0x38
#define kXenonVectorXMA             0x40
#define kXenonVectorAudio           0x44
#define kXenonVectorEthernet        0x4C
#define kXenonVectorXPS             0x54
#define kXenonVectorXenos           0x58
#define kXenonVectorProfiler        0x60
#define kXenonVectorBIU             0x64
#define kXenonVectorIOC             0x68
#define kXenonVectorFSB             0x6C
#define kXenonVectorIPI2            0x70
#define kXenonVectorClock           0x74
#define kXenonVectorIPI1            0x78
#define kXenonVectorNone            0x7C
#define kXenonVectorInvalid         0xFF



#endif
