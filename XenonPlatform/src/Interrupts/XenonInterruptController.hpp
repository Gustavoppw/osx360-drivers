//
//  XenonInterruptController.hpp
//  Xbox 360 interrupt controller
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonInterruptController_hpp
#define XenonInterruptController_hpp

#include <IOKit/IOInterrupts.h>
#include <IOKit/IOInterruptController.h>

#include "XenonCommon.hpp"
#include "XenonICRegs.hpp"

//
// Represents the Xbox 360 interrupt controller.
//
class XenonInterruptController : public IOInterruptController {
  OSDeclareDefaultStructors(XenonInterruptController);
  XenonDeclareLogFunctions("ic");
  typedef IOInterruptController super;

public:
  // IOService overrides.
  bool start(IOService *provider);

  // IOInterruptController overrides.
  IOInterruptAction getInterruptHandlerAddress(void);
  IOReturn handleInterrupt(void *refCon, IOService *nub, int source);
  int getVectorType(IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector);
  void disableVectorHard(IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector);
  void enableVector(IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector);

private:
  volatile void       *_mmioMem;
  IOMemoryMap         *_bridgeMmioMap;
  IOMemoryMap         *_biuMmioMap;
  volatile void       *_bridgeMmioMem;
  volatile void       *_biuMmioMem;

  // Register read/write.
  inline UInt64 readICReg64(UInt32 cpu, UInt32 offset) {
    return ReadFullBigInt64(_mmioMem, (cpu * kXenonICCoreRegOffset) + offset);
  }
  inline void writeICReg64(UInt32 cpu, UInt32 offset, UInt64 data) {
    WriteFullBigInt64(_mmioMem, (cpu * kXenonICCoreRegOffset) + offset, data);
  }
  inline UInt32 readBridgeReg32(UInt32 offset) {
    return OSReadLittleInt32(_bridgeMmioMem, offset);
  }
  inline void writeBridgeReg32(UInt32 offset, UInt32 data) {
    OSWriteLittleInt32(_bridgeMmioMem, offset, data);
  }
};

#endif
