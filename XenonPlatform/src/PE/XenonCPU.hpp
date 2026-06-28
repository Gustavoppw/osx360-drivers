//
//  XenonCPU.hpp
//  Xbox 360 CPU platform device
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonCPU_hpp
#define XenonCPU_hpp

#include <IOKit/IOCPU.h>
#include "XenonCommon.hpp"

//
// Represents a Xbox 360 platform CPU.
//
class XenonCPU : public IOCPU {
  OSDeclareDefaultStructors(XenonCPU);
  XenonDeclareLogFunctions("cpu");
  typedef IOCPU super;

public:
  // Overrides.
  bool start(IOService *provider);
  void initCPU(bool boot);
  void quiesceCPU(void);
  kern_return_t startCPU(vm_offset_t start_paddr, vm_offset_t arg_paddr);
  void haltCPU(void);
  const OSSymbol *getCPUName(void);

private:
  bool      _isBootCPU;
  UInt32    _numCPUs;

  void ipiHandler(void *refCon, void *nub, int source);
};

#endif
