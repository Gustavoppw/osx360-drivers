//
//  XenosFB.hpp
//  Xbox 360 graphics controller
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenosController_hpp
#define XenosController_hpp

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOService.h>

#include "XenonCommon.hpp"
#include "XenosRegs.hpp"

static inline UInt32 fui(float f) {
	union {
		float f;
		UInt32 ui;
	} fi;
	fi.f = f;
	return fi.ui;
}

#include "adreno_pm4.xml.h"
#include "adreno_common.xml.h"
#include "a2xx.xml.h"

#define RADEON_CP_PACKET0               0x00000000
#define RADEON_ONE_REG_WR                (1 << 15)

#define CP_REG(reg) ((0x4 << 16) | ((unsigned int)((reg) - (0x2000))))
#define CP_PACKET0( reg, n )                                            \
        (RADEON_CP_PACKET0 | ((n) << 16) | ((reg) >> 2))
#define CP_PACKET0_TABLE( reg, n )                                      \
        (RADEON_CP_PACKET0 | RADEON_ONE_REG_WR | ((n) << 16) | ((reg) >> 2))

//
// Represents the Xbox 360 graphics controller.
// Functionality implemented here to provide core support even if IOGraphicsFamily is not present.
//
class XenosController : public IOService {
  OSDeclareDefaultStructors(XenosController);
  XenonDeclareLogFunctions("xenosctrl");
  typedef IOService super;

public:
  // IOService overrides.
  bool start(IOService *provider);

private:
  // MMIO and framebuffer.
  IOMemoryMap         *_mmioMap;
  volatile void       *_mmioMem;

  IOWorkLoop          *_timerWorkLoop;
  IOTimerEventSource  *_timerEventSource;

  IOPhysicalAddress   _fbPhysAddr;
  IOPhysicalAddress   _gpuPhysAddr;

  // Current display info.
  UInt32              _fbWidth;
  UInt32              _fbHeight;

  // Ring buffer.
  IOBufferMemoryDescriptor  *_ringBufferDesc;
  volatile UInt8            *_ringBuffer;
  UInt32                    _ringBufferWritePtr;

  // Shaders.
  IOBufferMemoryDescriptor  *_texCoordsDesc;
  IOPhysicalAddress         _texCoordsPhys;
  IOBufferMemoryDescriptor  *_triagDesc;
  IOPhysicalAddress         _triagPhys;

  // Register read/write.
  inline void writeReg32(UInt32 offset, UInt32 data) {
    OSWriteBigInt32(_mmioMem, offset, data);
  }
  inline UInt32 readReg32(UInt32 offset) {
    return OSReadBigInt32(_mmioMem, offset);
  }

  // Ring buffer writes.
  inline void writeRing(UInt32 data) {
    *(volatile UInt32*)(_ringBuffer + (_ringBufferWritePtr++ * 4)) = data;
    if (_ringBufferWritePtr == kXenosRingbufferLength) {
      _ringBufferWritePtr = 0;
    }
  }
  inline void writeRingReg32(UInt32 reg, UInt32 data) {
    writeRing(CP_PACKET0(reg, 0));
    writeRing(data);
  }
  inline void writeRingPacket0(UInt16 index, UInt16 length) {
    writeRing(CP_TYPE0_PKT | ((length - 1) << 16) | (index & 0x7FFF));
  }
  inline void writeRingPacket3(UInt8 opcode, UInt16 length) {
    writeRing(CP_TYPE3_PKT | ((length - 1) << 16) | ((opcode & 0xFF) << 8));
  }

  // Firmware.
  IOReturn loadFirmware(void);

  // Setup.
  IOReturn initShaders(void);
  void initME(void);
  IOReturn initHardware(void);
  void initFramebuffer(void);

  // Internal functions.
  void handleTimer(IOTimerEventSource *sender);
  void syncData(volatile void *data, UInt32 length);
  void syncRingBuffer(void);
};

#endif
