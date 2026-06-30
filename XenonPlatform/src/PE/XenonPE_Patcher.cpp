//
//  XenonPE_Patcher.cpp
//  Xbox 360 platform expert patching functions
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include <mach-o/nlist.h>
#include <ppc/proc_reg.h>

#include "XenonPE.hpp"

//
// Kernel map and pmap exported on all versions.
//
extern "C" vm_map_t kernel_map;
extern "C" void *kernel_pmap;

//
// Additional exported functions.
//
extern "C" vm_offset_t pmap_extract(void *pmap, vm_map_offset_t va);
extern "C" kern_return_t vm_protect(vm_map_t map, vm_offset_t start, vm_size_t size, boolean_t set_maximum, vm_prot_t new_protection);
extern "C" unsigned int	ml_phys_read(vm_offset_t paddr);
extern "C" void	ml_phys_write(vm_offset_t paddr, unsigned int data);

// Flags for mapping_make
#define mmFlgBlock		0x80000000	/* This is a block map, use size for number of pages covered */
#define mmFlgUseAttr	0x40000000	/* Use specified attributes */
#define mmFlgPerm		0x20000000	/* Mapping is permanant */
#define mmFlgCInhib		0x00000002	/* Cahching inhibited - use if mapFlgUseAttr set or block */
#define mmFlgGuarded	0x00000001	/* Access guarded - use if mapFlgUseAttr set or block */

XenonPE* XenonPE::_callbackPE = NULL;

//
// Search for and get the pointer to the kernel's Mach-O header.
//
bool XenonPE::findKernelMachHeader(void) {
  struct mach_header      *machHeader;
  UInt8                   *machCommands;
  struct load_command     *loadCommand;
  struct segment_command  *segCommand;
  struct segment_command  *linkedItSegCommand;
  struct symtab_command   *symCommand;
  bool                    foundHeader;

  //
  // Search for a valid Mach-O header.
  // On all PowerPC versions, the kernel is mapped 1:1 with no KASLR. The address of the header will vary between versions.
  //
  foundHeader = false;
  for (UInt32 addr = reinterpret_cast<UInt32>(IOLog) & 0xFFFFF000; addr > 0; addr -= PAGE_SIZE) {
    machHeader = reinterpret_cast<struct mach_header*>(addr);
    if ((machHeader->magic == MH_MAGIC) && (machHeader->cputype == CPU_TYPE_POWERPC) && (machHeader->filetype == MH_EXECUTE)) {
      //
      // Found a valid header, check to see if __TEXT segment is first.
      //
      machCommands = reinterpret_cast<UInt8*>(addr) + sizeof (*machHeader);
      for (UInt32 i = 0; i < machHeader->ncmds; i++) {
        loadCommand = reinterpret_cast<struct load_command*>(machCommands);

        if (loadCommand->cmd == LC_SEGMENT) {
          segCommand = reinterpret_cast<struct segment_command*>(loadCommand);
          if (strncmp(segCommand->segname, "__TEXT", sizeof (segCommand->segname)) == 0) {
            foundHeader = true;
            break;
          }
        }

        machCommands += loadCommand->cmdsize;
      }

      if (foundHeader) {
        XEDBGLOG("Found kernel Mach-O header at 0x%X", addr);
        break;
      }
    }
  }

  if (!foundHeader) {
    return false;
  }

  //
  // Get the symbols.
  //
  linkedItSegCommand  = NULL;
  symCommand          = NULL;
  machCommands = reinterpret_cast<UInt8*>(machHeader) + sizeof (*machHeader);
  for (UInt32 i = 0; i < machHeader->ncmds; i++) {
    loadCommand = reinterpret_cast<struct load_command*>(machCommands);

    if (loadCommand->cmd == LC_SEGMENT) {
      segCommand = reinterpret_cast<struct segment_command*>(loadCommand);
      if (strncmp(segCommand->segname, "__LINKEDIT", sizeof (segCommand->segname)) == 0) {
        linkedItSegCommand = segCommand;
      }
    } else if (loadCommand->cmd == LC_SYMTAB) {
      symCommand = reinterpret_cast<struct symtab_command*>(loadCommand);
    }

    machCommands += loadCommand->cmdsize;
  }

  if ((symCommand == NULL) || (linkedItSegCommand == NULL)) {
    return false;
  }

  _symTab           = reinterpret_cast<UInt8*>(linkedItSegCommand->vmaddr + symCommand->symoff - linkedItSegCommand->fileoff);
  _symTabNumSymbols = symCommand->nsyms;
  _strTab           = reinterpret_cast<UInt8*>(linkedItSegCommand->vmaddr + symCommand->stroff - linkedItSegCommand->fileoff);
  _strTabSize       = symCommand->strsize;

  XEDBGLOG("Symtab: %p, strtab: %p", _symTab, _strTab);
  return true;
}

//
// Resolve a kernel symbol. This function can only be called prior to kernel linker jettison.
//
UInt32 XenonPE::resolveKernelSymbol(const char *symbolName) {
  struct nlist  *nlistSym;
  char          *symStr;

  nlistSym = reinterpret_cast<struct nlist*>(_symTab);
  for (UInt32 i = 0; i < _symTabNumSymbols; i++, nlistSym++) {
    //
    // Get the symbol string for the symbol.
    //
    symStr = reinterpret_cast<char*>(_strTab + nlistSym->n_un.n_strx);
    if (strncmp(symbolName, symStr, strlen(symbolName)) == 0) {
      XEDBGLOG("Found symbol '%s' at 0x%X", symbolName, nlistSym->n_value);
      return nlistSym->n_value;
    }
  }

  XESYSLOG("Failed to locate symbol '%s'", symbolName);
  return 0;
}

void XenonPE::wrapPmapEnter(void* pmap, vm_map_offset_t va, ppnum_t pa, vm_prot_t prot, unsigned int flags, boolean_t wired) {
  //
  // Invoke original function.
  //
  reinterpret_cast<void(*)(void*, vm_map_offset_t, ppnum_t, vm_prot_t, unsigned int, boolean_t)>(_callbackPE->_orgPmapEnter)(pmap, va, pa, prot, flags, wired);

  //
  // Perform instruction patching in userspace physical pages.
  //
  if ((pmap != NULL) && (pmap != kernel_pmap) && (prot & VM_PROT_EXECUTE)) {
    vm_offset_t currPhys = pa << PAGE_SHIFT;

    bool changed = false;
    for (UInt32 i = 0; i < 1024; i++) {
      UInt32 data = ml_phys_read(currPhys);
      if ((data & 0xFF0007FF) == 0x7C0007EC) {
        data &= 0x03FFFFFF;
        ml_phys_write(currPhys, data);
        changed = true;
      }

      currPhys += 4;
    }

    if (changed) {
      _callbackPE->_pmapSyncPhysFunc(pa);
    }
  }
}

void XenonPE::wrapDebugger(const char *str) {
  reinterpret_cast<void(*)(const char*)>(_callbackPE->_orgDebugger)(str);
  _callbackPE->refreshFramebuffer();
}

int XenonPE::wrapGradeBinary(cpu_type_t exectype, cpu_subtype_t execsubtype) {
  switch (execsubtype) {
    case CPU_SUBTYPE_POWERPC_970:
    case CPU_SUBTYPE_POWERPC_7450:
    case CPU_SUBTYPE_POWERPC_7400:
      return 0;
  }

  return reinterpret_cast<int(*)(cpu_type_t, cpu_subtype_t)>(_callbackPE->_orgGradeBinary)(exectype, execsubtype);
}

int XenonPE::wrapGradeCpuSubtype(cpu_subtype_t cpu_subtype) {
  switch (cpu_subtype) {
    case CPU_SUBTYPE_POWERPC_970:
    case CPU_SUBTYPE_POWERPC_7450:
    case CPU_SUBTYPE_POWERPC_7400:
      return 0;
  }

  return reinterpret_cast<int(*)(cpu_subtype_t)>(_callbackPE->_orgGradeCpuSubtype)(cpu_subtype);
}

UInt32 XenonPE::createTrampoline(UInt32 from) {
  UInt32 *fromPtr;
  UInt32 *trampolinePtr;
  UInt32 trampolineBytes[8];

  fromPtr = (UInt32*) from;
  trampolinePtr = &_tmpExecMemory[_tmpExecMemoryOffset];
  _tmpExecMemoryOffset += ARRSIZE(trampolineBytes);

  trampolineBytes[0] = 0x3C000000 | ((from + 16) >> 16);      // lis r0, (from high bytes)
  trampolineBytes[1] = 0x60000000 | ((from + 16) & 0xFFFF);   // ori r0, r0, (from low bytes)
  trampolineBytes[2] = 0x7C0903A6;                            // mtctr r0
  trampolineBytes[3] = fromPtr[0];
  trampolineBytes[4] = fromPtr[1];
  trampolineBytes[5] = fromPtr[2];
  trampolineBytes[6] = fromPtr[3];
  trampolineBytes[7] = 0x4E800420;                            // bctr

  bcopy(trampolineBytes, trampolinePtr, sizeof (trampolineBytes));
  invalidate_icache((vm_offset_t) trampolinePtr, sizeof (trampolineBytes), 0);

  return (UInt32) trampolinePtr;
}

IOReturn XenonPE::routeFunction(UInt32 from, UInt32 to, UInt32 *trampoline) {
  UInt32 *fromPtr;
  UInt32 targetBytes[4];
  boolean_t ints;

  XEDBGLOG("Routing function 0x%X to 0x%X", from, to);

  *trampoline = createTrampoline(from);
  if (*trampoline == 0) {
    XESYSLOG("Failed to creat trampoline for function 0x%X", from);
    return kIOReturnInternalError;
  }
  XEDBGLOG("Trampoline at 0x%X", *trampoline);

  //
  // Build the jump code replacing the prologue of the target function.
  //
  targetBytes[0] = 0x3C000000 | (to >> 16);     // lis r0, (to high bytes)
  targetBytes[1] = 0x60000000 | (to & 0xFFFF);  // ori r0, r0, (from low bytes)
  targetBytes[2] = 0x7C0903A6;                  // mtctr r0
  targetBytes[3] = 0x4E800420;                  // bctr

  //
  // Disable interrupts, write the function bytes, and re-enable.
  //
  ints = ml_set_interrupts_enabled(false);

  fromPtr = (UInt32*) from;
  for (size_t i = 0; i < ARRSIZE(targetBytes); i++) {
    ml_phys_write(pmap_extract(kernel_pmap, (vm_map_offset_t) &fromPtr[i]), targetBytes[i]);
  }
  invalidate_icache((vm_offset_t) fromPtr, sizeof (targetBytes), 0);

  ml_set_interrupts_enabled(ints);

  XEDBGLOG("Routed function 0x%X", from);
  return kIOReturnSuccess;
}

bool XenonPE::initPatcher(void) {
  kern_return_t result;

  _callbackPE = this;

  //
  // Get the kernel header and resolve required non-exported functions.
  //
  if (!findKernelMachHeader()) {
    return false;
  }

  _kmemAllocPageableFunc = (xnu_kmem_alloc_pageable_func) resolveKernelSymbol("_kmem_alloc_pageable");
  _mappingMakeFunc = (xnu_mapping_make_func) resolveKernelSymbol("_mapping_make");
  if (getKernelVersion() >= kKernelVersionTiger) {
    _pmapSyncPhysFunc = (xnu_pmap_sync_phys) resolveKernelSymbol("_pmap_sync_page_data_phys");
  } else {
    _pmapSyncPhysFunc = (xnu_pmap_sync_phys) resolveKernelSymbol("_pmap_sync_caches_phys");
  }

  if ((_kmemAllocPageableFunc == NULL) || (_mappingMakeFunc == NULL) || (_pmapSyncPhysFunc == NULL)) {
    XESYSLOG("Failed to resolve one or more non-exported functions");
    return false;
  }

  result = vm_protect(kernel_map, (vm_address_t) _tmpExecMemory, sizeof(_tmpExecMemory), TRUE, VM_PROT_ALL);
  XEDBGLOG("Adjusted max perms 0x%X", result);
  result = vm_protect(kernel_map, (vm_address_t) _tmpExecMemory, sizeof(_tmpExecMemory), FALSE, VM_PROT_ALL);
  XEDBGLOG("Adjusted new perms 0x%X", result);

  routeFunction(resolveKernelSymbol("_Debugger"), (UInt32)&XenonPE::wrapDebugger, &_orgDebugger);
  routeFunction(resolveKernelSymbol("_pmap_enter"), (UInt32)&XenonPE::wrapPmapEnter, &_orgPmapEnter);

  if (getKernelVersion() >= kKernelVersionTiger) {
    routeFunction(resolveKernelSymbol("_grade_binary"), (UInt32)&XenonPE::wrapGradeBinary, &_orgGradeBinary);
  } else {
    routeFunction(resolveKernelSymbol("_grade_cpu_subtype"), (UInt32)&XenonPE::wrapGradeCpuSubtype, &_orgGradeCpuSubtype);
  }

  return true;
}

//
// Maps the requested physical address.
// XNU is unable to deal with 64-bit physical addresses normally, hack function to directly map.
//
IOReturn XenonPE::mapMemory(UInt64 physAddr, UInt32 length, void **virtBase) {
  vm_offset_t	  start;
  kern_return_t result;

  //
  // Similar function to io_map in XNU.
  //
  result = _kmemAllocPageableFunc(kernel_map, &start, length);
  if (result != KERN_SUCCESS) {
    return kIOReturnIOError;
  }

  result = _mappingMakeFunc(kernel_pmap, (addr64_t)start, (ppnum_t)(physAddr >> 12),
    (mmFlgBlock | mmFlgUseAttr | mmFlgCInhib | mmFlgGuarded), length >> 12, VM_PROT_READ | VM_PROT_WRITE);
  if (result != KERN_SUCCESS) {
    return kIOReturnIOError;
  }

  *virtBase = (void*)start;
  XEDBGLOG("Mapped 0x%llX to virt 0x%X", physAddr, start);
  return kIOReturnSuccess;
}
