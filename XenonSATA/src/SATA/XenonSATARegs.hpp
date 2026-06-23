//
//  XenonSATARegs.hpp
//  Xbox 360 SATA controller register definitions
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonSATARegs_hpp
#define XenonSATARegs_hpp

//
// Command registers.
//
#define kXenonSATARegData           0x00
#define kXenonSATARegError          0x01
#define kXenonSATARegFeatures       0x01
#define kXenonSATARegSectorCount    0x02
#define kXenonSATARegLbaLow         0x03
#define kXenonSATARegLbaMed         0x04
#define kXenonSATARegLbaHigh        0x05
#define kXenonSATARegDevSelect      0x06
#define kXenonSATARegStatus         0x07
#define kXenonSATARegCommand        0x07

//
// Control registers.
//
#define kXenonSATARegAltStatus      0x0A
#define kXenonSATARegDevControl     0x0A

//
// Status registers.
//
#define kXenonSATARegSStatus        0x10
#define kXenonSATARegSError         0x14
#define kXenonSATARegSControl       0x18
#define kXenonSATARegSActive        0x1C

//
// DMA registers.
//
#define kXenonSATADmaRegCommand     0x00
#define kXenonSATADmaRegStatus      0x02
#define kXenonSATADmaRegTableOffset 0x04

#endif
