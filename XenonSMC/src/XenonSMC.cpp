//
//  XenonSMC.cpp
//  Xbox 360 system management controller driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include <IOKit/IOPlatformExpert.h>
#include "XenonSMC.hpp"

OSDefineMetaClassAndStructors(XenonSMC, super);

//
// Overrides IOService::init().
//
bool XenonSMC::init(OSDictionary *dictionary) {
  XenonCheckDebugArgs();
  _debugEnabled = true;

  _pciParent  = NULL;
  _mmioMem    = NULL;
  _mmioMap    = NULL;

  return super::init(dictionary);
}

//
// Overrides IOService::start().
//
bool XenonSMC::start(IOService *provider) {
  if (!super::start(provider)) {
    XESYSLOG("super::start() returned false");
    return false;
  }

  _pciParent = OSDynamicCast(IOPCIDevice, provider);
  if (_pciParent == NULL) {
    return false;
  }
  _pciParent->retain();

  if (!_pciParent->open(this)) {
    return false;
  }

  _mmioMap = _pciParent->mapDeviceMemoryWithIndex(0);
  if (_mmioMap == NULL) {
    XESYSLOG("Failed to map SMC memory");
    return false;
  }

  _mmioMem = (volatile void*) _mmioMap->getVirtualAddress();
  XEDBGLOG("Mapped SMC registers at 0x%X length 0x%X to %p", _mmioMap->getPhysicalAddress(),
    _mmioMap->getLength(), _mmioMem);

  _lock = IOSimpleLockAlloc();
  if (_lock == NULL) {
    XESYSLOG("Failed to allocate lock");
    return false;
  }
  IOSimpleLockInit(_lock);

  _rtcLock = IOLockAlloc();
  IOLockInit(_rtcLock);

  //
  // Configure interrupt.
  //
  _interruptEventSource = IOFilterInterruptEventSource::filterInterruptEventSource(this,
    OSMemberFunctionCast(IOInterruptEventSource::Action, this, &XenonSMC::handleInterrupt),
    OSMemberFunctionCast(IOFilterInterruptEventSource::Filter, this, &XenonSMC::filterInterrupt),
    provider, 0);
  if (_interruptEventSource == NULL) {
    XESYSLOG("Failed to get interrupt");
    return false;
  }
  getWorkLoop()->addEventSource(_interruptEventSource);
  _interruptEventSource->enable();

  //
  // Clear any pending interrupt and enable it.
  //
  writeLittleReg32(kXenonSMCRegIntAck, readLittleReg32(kXenonSMCRegIntStatus));
  writeLittleReg32(kXenonSMCRegIntEnabled, kXenonSMCRegIntEnabledBit);

  startLEDBootAnimation();

  //
  // Register platform hooks.
  //
  _gXenonSMC = this;
  PE_halt_restart           = XenonSMC::peHaltRestart;
  PE_read_write_time_of_day = XenonSMC::peReadWriteTimeDay;

  publishResource("IORTC", this);
  registerService();

  XEDBGLOG("Started Xenon SMC");
  return true;
}

//
// Overrides IOService::callPlatformFunction().
//
IOReturn XenonSMC::callPlatformFunction(const OSSymbol *functionName, bool waitForFunction,
                                        void *param1, void *param2, void *param3, void *param4) {
  if (functionName->isEqualTo(kXenonFuncSMCEject)) {
    return ejectTray();
  }

  return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}
