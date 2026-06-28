//
//  XenonPE.hpp
//  Xbox 360 platform expert
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonPE_hpp
#define XenonPE_hpp

#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IOTimerEventSource.h>
#include <mach-o/loader.h>

#include "XenonCommon.hpp"

#define kXenonPETempExecMemoryInsCount    256

typedef kern_return_t (*xnu_kmem_alloc_pageable_func)(vm_map_t map, vm_offset_t *addrp, vm_size_t size);
typedef addr64_t (*xnu_mapping_make_func)(void *pmap, addr64_t va, ppnum_t pa, unsigned int flags, unsigned int size, vm_prot_t prot);
typedef void (*xnu_pmap_sync_phys)(ppnum_t pa);

//
// Represents the Xbox 360 platform expert.
//
class XenonPE : public IODTPlatformExpert {
  OSDeclareDefaultStructors(XenonPE);
  XenonDeclareLogFunctions("pe");
  typedef IODTPlatformExpert super;

public:
  // IOService overrides.
  bool start(IOService *provider);

  // IOPlatformExpert overrides.
  IOReturn callPlatformFunction(const OSSymbol *functionName, bool waitForFunction,
                                void *param1, void *param2, void *param3, void *param4);
  const char *deleteList(void);
  const char *excludeList(void);
  bool getMachineName(char *name, int maxLength);
  long getGMTTimeOfDay(void);

  // GPU functions.
  void prepGraphicsFramebuffer(void);
  void refreshFramebuffer(void);
  void startFramebuffer(void);
  void stopFramebuffer(void);

  // Patcher functions.
  UInt32 resolveKernelSymbol(const char *symbolName);
  UInt32 createTrampoline(UInt32 from);
  IOReturn routeFunction(UInt32 from, UInt32 to, UInt32 *trampoline);
  IOReturn mapMemory(UInt64 physAddr, UInt32 length, void **virtBase);

private:
  // GPU.
  IODeviceMemory      *_gpuMmioDeviceMemory;
  IOMemoryMap         *_gpuMmioMap;
  volatile void       *_gpuMmioMem;

  IOPhysicalAddress   _gpuPhysAddr;
  IOPhysicalAddress   _fbPhysAddr;
  IODeviceMemory      *_gpuDeviceMemory;
  IOMemoryMap         *_gpuMap;
  volatile void       *_gpuMem;
  IODeviceMemory      *_fbDeviceMemory;
  IOMemoryMap         *_fbMap;
  volatile void       *_fbMem;

  IOWorkLoop          *_gpuTimerWorkLoop;
  IOTimerEventSource  *_gpuTimerEventSource;

  // Patching/symbol lookups.
  UInt8     *_symTab;
  UInt32    _symTabNumSymbols;
  UInt8     *_strTab;
  UInt32    _strTabSize;
  UInt32    _tmpExecMemory[kXenonPETempExecMemoryInsCount];
  UInt32    _tmpExecMemoryOffset;

  xnu_kmem_alloc_pageable_func  _kmemAllocPageableFunc;
  xnu_mapping_make_func         _mappingMakeFunc;
  xnu_pmap_sync_phys            _pmapSyncPhysFunc;

  // Function hooks.
  static XenonPE    *_callbackPE;
  UInt32            orgDebugger;
  UInt32            orgKmodCreateInternal;
  UInt32            orgVmFault;
  UInt32            orgTrap;
  UInt32            orgChangePowerStateTo;
  UInt32            orgVmFaultPage;
  UInt32            orgPmapEnter;
  UInt32            orgVmPageCopy;

  // GPU.
  void handleFramebufferTimer(IOTimerEventSource *sender);
  bool mapGPU(void);

  // Patching/symbol lookups.
  bool findKernelMachHeader(void);
  bool initPatcher(void);

  // Function hooks.
  static void wrapDebugger(const char *str);
  static kern_return_t wrapKmodCreateInternal(kmod_info_t *kmod, kmod_t *id);
  static void wrapPmapEnter(void* pmap, vm_map_offset_t va, ppnum_t pa, vm_prot_t prot,
                            unsigned int flags, boolean_t wired);
};

#endif
