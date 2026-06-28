//
//  XenonSMC.hpp
//  Xbox 360 system management controller driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonSMC_hpp
#define XenonSMC_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/IOSyncer.h>
#include <IOKit/pci/IOPCIDevice.h>

#include "XenonCommon.hpp"
#include "XenonSMCRegs.hpp"

//
// Represents the Xbox 360 system management controller driver.
//
class XenonSMC : public IOService {
  OSDeclareDefaultStructors(XenonSMC);
  XenonDeclareLogFunctions("smc");
  typedef IOService super;

public:
  // IOService overrides.
  bool start(IOService *provider);
  void free(void);
  IOReturn callPlatformFunction(const OSSymbol *functionName, bool waitForFunction,
                                void *param1, void *param2, void *param3, void *param4);

private:
  IOPCIDevice                   *_pciParent;
  IOMemoryMap                   *_mmioMap;
  volatile void                 *_mmioMem;
  IOSimpleLock                  *_lock;
  IOFilterInterruptEventSource  *_intEventSource;
  static XenonSMC               *_gXenonSMC;

  // Async states.
  bool      _powerButtonPressed;
  bool      _syncButtonPressed;
  bool      _tiltState;
  bool      _tiltStatePresent;

  IOLock              *_rtcLock;
  IOSyncer            *_rtcSyncer;
  UInt64              _rtcValue;
  bool                _rtcValuePresent;
  XenonSMCTempStatus  _tempStatus;
  bool                _tempStatusPresent;

  // Register read/writes.
  inline void writeLittleReg32(UInt32 offset, UInt32 data) {
    OSWriteLittleInt32(_mmioMem, offset, data);
  }
  inline UInt32 readLittleReg32(UInt32 offset) {
    return OSReadLittleInt32(_mmioMem, offset);
  }
  inline void writeBigReg32(UInt32 offset, UInt32 data) {
    OSWriteBigInt32(_mmioMem, offset, data);
  }
  inline UInt32 readBigReg32(UInt32 offset) {
    return OSReadBigInt32(_mmioMem, offset);
  }

  // Interrupt handlers.
  bool filterInterrupt(IOFilterInterruptEventSource *intEventSource);
  void handleInterrupt(IOInterruptEventSource *intEventSource, int count);

  // Internal functions.
  IOReturn writeMessage(XenonSMCMessage *message);
  IOReturn readMessage(XenonSMCMessage *message);
  IOReturn getRTC(UInt64 *value);
  IOReturn setLEDState(bool override, UInt8 value);
  IOReturn startLEDBootAnimation(void);
  IOReturn rebootPowerOff(bool reboot);
  IOReturn cancelPowerOff(void);
  IOReturn ejectTray(void);
  IOReturn muteAudio(bool mute);

  // Platform functions.
  static int peHaltRestart(unsigned int type);
  static int peReadWriteTimeDay(unsigned int options, long *secs);
};

#endif
