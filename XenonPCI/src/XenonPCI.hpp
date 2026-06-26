//
//  XenonPCI.hpp
//  Xbox 360 PCI host bridge
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonPCI_hpp
#define XenonPCI_hpp

#include <IOKit/pci/IOPCIBridge.h>
#include "XenonCommon.hpp"

//
// Represents the Xbox 360 PCI host bridge.
//
class XenonPCI : public IOPCIBridge {
  OSDeclareDefaultStructors(XenonPCI);
  XenonDeclareLogFunctions("pci");
  typedef IOPCIBridge super;

public:
  // IOService overrides.
  bool start(IOService *provider);
  void free(void);

  // IOPCIBridge overrides.
  bool configure(IOService *provider);
  IODeviceMemory* ioDeviceMemory(void);
  UInt32 configRead32(IOPCIAddressSpace space, UInt8 offset);
  void configWrite32(IOPCIAddressSpace space, UInt8 offset, UInt32 data);
  UInt16 configRead16(IOPCIAddressSpace space, UInt8 offset);
  void configWrite16(IOPCIAddressSpace space, UInt8 offset, UInt16 data);
  UInt8 configRead8(IOPCIAddressSpace space, UInt8 offset);
  void configWrite8(IOPCIAddressSpace space, UInt8 offset, UInt8 data);
  IOPCIAddressSpace getBridgeSpace(void);
  UInt8 firstBusNum(void);
  UInt8 lastBusNum(void);

private:
  IOMemoryMap     *_configMap;
  volatile UInt8  *_configMem;
  IODeviceMemory  *_deviceMemory;
  IOSimpleLock    *_lock;

  inline UInt32 getConfigAddress(IOPCIAddressSpace space) {
    return ((space.es.busNum << 8) | (space.es.deviceNum << 3) | space.es.functionNum) << 12;
  }
};

#endif
