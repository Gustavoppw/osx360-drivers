//
//  XenonPCI.cpp
//  Xbox 360 PCI host bridge
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonPCI.hpp"

OSDefineMetaClassAndStructors(XenonPCI, super);

//
// Performs driver startup.
// Overrides IOService::start().
//
bool XenonPCI::start(IOService *provider) {
  XenonCheckDebugArgs();
  XEDBGLOG("Starting PCI host bridge");

  _lock = IOSimpleLockAlloc();
  if (_lock == NULL) {
    return false;
  }

  _configMap = provider->mapDeviceMemoryWithIndex(0);
  if (_configMap == NULL) {
    XESYSLOG("Failed to map configuration space");
    return false;
  }

  _configMem = (volatile UInt8*) _configMap->getVirtualAddress();
  XEDBGLOG("Mapped configuration space at 0x%X to %p", _configMap->getPhysicalAddress(), _configMem);

  registerService();

  return super::start(provider);
}

//
// Releases driver resources.
// Overrides IOService::free().
//
void XenonPCI::free(void) {
  OSSafeReleaseNULL(_configMap);

  if (_lock != NULL) {
    IOSimpleLockFree(_lock);
    _lock = NULL;
  }

  super::free();
}

//
// Overrides IOPCIBridge::configure().
//
bool XenonPCI::configure(IOService *provider) {
  addBridgeMemoryRange(0xEA000000, 0x1000000, true);
  addBridgeMemoryRange(0xEC000000, 0x1000000, true);

  return super::configure(provider);
}

//
// Overrides IOPCIBridge::ioDeviceMemory().
//
IODeviceMemory* XenonPCI::ioDeviceMemory(void) {
  XEDBGLOG("start");
  return NULL;
}

//
// Reads the specified 32-bit PCI configuration register.
// Overrides IOPCIBridge::configRead32().
//
UInt32 XenonPCI::configRead32(IOPCIAddressSpace space, UInt8 offset) {
  UInt32            data;
  IOInterruptState  ints;

  XEDBGLOG("%u:%u.%u offset 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset);

  ints = IOSimpleLockLockDisableInterrupt(_lock);
  data = OSReadLittleInt32(_configMem, getConfigAddress(space) + offset);
  eieio();
  IOSimpleLockUnlockEnableInterrupt(_lock, ints);

  XEDBGLOG("%u:%u.%u offset 0x%X data 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset, data);

  return data;
}

//
// Writes the specified 32-bit PCI configuration register.
// Overrides IOPCIBridge::configWrite32().
//
void XenonPCI::configWrite32(IOPCIAddressSpace space, UInt8 offset, UInt32 data) {
  IOInterruptState  ints;

  XEDBGLOG("%u:%u.%u offset 0x%X data 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset, data);

  ints = IOSimpleLockLockDisableInterrupt(_lock);
  OSWriteLittleInt32(_configMem, getConfigAddress(space) + offset, data);
  eieio();
  IOSimpleLockUnlockEnableInterrupt(_lock, ints);
}

//
// Reads the specified 16-bit PCI configuration register.
// Overrides IOPCIBridge::configRead16().
//
UInt16 XenonPCI::configRead16(IOPCIAddressSpace space, UInt8 offset) {
  UInt16            data;
  IOInterruptState  ints;

  XEDBGLOG("%u:%u.%u offset 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset);

  ints = IOSimpleLockLockDisableInterrupt(_lock);
  data = OSReadLittleInt16(_configMem, getConfigAddress(space) + offset);
  eieio();
  IOSimpleLockUnlockEnableInterrupt(_lock, ints);

  XEDBGLOG("%u:%u.%u offset 0x%X data 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset, data);

  return data;
}

//
// Writes the specified 16-bit PCI configuration register.
// Overrides IOPCIBridge::configWrite16().
//
void XenonPCI::configWrite16(IOPCIAddressSpace space, UInt8 offset, UInt16 data) {
  IOInterruptState  ints;

  XEDBGLOG("%u:%u.%u offset 0x%X data 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset, data);

  ints = IOSimpleLockLockDisableInterrupt(_lock);
  OSWriteLittleInt16(_configMem, getConfigAddress(space) + offset, data);
  eieio();
  IOSimpleLockUnlockEnableInterrupt(_lock, ints);
}

//
// Reads the specified 8-bit PCI configuration register.
// Overrides IOPCIBridge::configRead8().
//
UInt8 XenonPCI::configRead8(IOPCIAddressSpace space, UInt8 offset) {
  UInt8            data;
  IOInterruptState  ints;

  XEDBGLOG("%u:%u.%u offset 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset);

  ints = IOSimpleLockLockDisableInterrupt(_lock);
  data = _configMem[getConfigAddress(space) + offset];
  eieio();
  IOSimpleLockUnlockEnableInterrupt(_lock, ints);

  XEDBGLOG("%u:%u.%u offset 0x%X data 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset, data);

  return data;
}

//
// Writes the specified 8-bit PCI configuration register.
// Overrides IOPCIBridge::configWrite8().
//
void XenonPCI::configWrite8(IOPCIAddressSpace space, UInt8 offset, UInt8 data) {
  IOInterruptState  ints;

  XEDBGLOG("%u:%u.%u offset 0x%X data 0x%X", space.es.busNum, space.es.deviceNum, space.es.functionNum, offset, data);

  ints = IOSimpleLockLockDisableInterrupt(_lock);
  _configMem[getConfigAddress(space) + offset] = data;
  eieio();
  IOSimpleLockUnlockEnableInterrupt(_lock, ints);
}

//
// Overrides IOPCIBridge::getBridgeSpace().
//
IOPCIAddressSpace XenonPCI::getBridgeSpace(void) {
  IOPCIAddressSpace	space;

  space.bits = 0;
  space.s.deviceNum = 0;

  return space;
}

//
// Gets the first enumerable PCI bus number.
// Overrides IOPCIBridge::firstBusNum().
//
UInt8 XenonPCI::firstBusNum(void) {
  return 0;
}

//
// Gets the last enumerable PCI bus number.
// Overrides IOPCIBridge::lastBusNum().
//
UInt8 XenonPCI::lastBusNum(void) {
  return 1;
}
