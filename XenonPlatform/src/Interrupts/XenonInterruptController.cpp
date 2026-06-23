//
//  XenonInterruptController.cpp
//  Xbox 360 interrupt controller
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include <ppc/proc_reg.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOPlatformExpert.h>

#include "XenonPE.hpp"
#include "XenonInterruptController.hpp"

OSDefineMetaClassAndStructors(XenonInterruptController, super);

//
// CPU IRQ to PCI bridge IRQ mapping (filled at startup).
// Used for masking/unmasking.
//
static UInt8 sXenonCPUInterruptMap[kXenonICVectorCount];

//
// Overrides IOInterruptController::init().
//
bool XenonInterruptController::init(OSDictionary *dictionary) {
  XenonCheckDebugArgs();

  _debugEnabled = true;

  _mmioMem   = NULL;

  return super::init(dictionary);
}

//
// Overrides IOInterruptController::start().
//
bool XenonInterruptController::start(IOService *provider) {
  XenonPE         *xenonPE;
  OSSymbol        *interruptControllerName;
  bool            vectorLockResult;
  IOReturn        status;

  //
  // Get the Xenon platform expert
  //
  xenonPE = OSDynamicCast(XenonPE, getPlatform());
  if (xenonPE == NULL) {
    XESYSLOG("Current platform is not Xenon");
    return false;
  }

  //
  // Fill out interrupt map table.
  //
  for (UInt32 i = 0; i < kXenonICVectorCount; i++) {
    sXenonCPUInterruptMap[i] = kXenonVectorInvalid;
  }
  sXenonCPUInterruptMap[kXenonVectorClock] = 0;
  sXenonCPUInterruptMap[kXenonVectorSATACD] = 1;
  sXenonCPUInterruptMap[kXenonVectorSATAHDD] = 2;
  sXenonCPUInterruptMap[kXenonVectorSMC] = 3;
  sXenonCPUInterruptMap[kXenonVectorOHCI0] = 4;
  sXenonCPUInterruptMap[kXenonVectorEHCI0] = 5;
  sXenonCPUInterruptMap[kXenonVectorOHCI1] = 6;
  sXenonCPUInterruptMap[kXenonVectorEHCI1] = 7;
  sXenonCPUInterruptMap[kXenonVectorEthernet] = 10;
  sXenonCPUInterruptMap[kXenonVectorXMA] = 11;
  sXenonCPUInterruptMap[kXenonVectorAudio] = 12;
  sXenonCPUInterruptMap[kXenonVectorFlash] = 13;

  if (!super::start(provider)) {
    XESYSLOG("super::start() returned false");
    return false;
  }

  //
  // Get the interrupt controller name.
  //
  interruptControllerName = (OSSymbol *)IODTInterruptControllerName(provider);
  if (interruptControllerName == NULL) {
    XESYSLOG("Failed to get interrupt controller name");
    return false;
  }

  //
  // Map interrupt controller memory.
  // XNU is unable to deal with 64-bit physical MMIO addresses on 32-bit kernels (all PowerPC).
  // Use platform expert hack functions instead.
  //
  status = xenonPE->mapMemory(kXenonICBase, kXenonICLength, (void**) &_mmioMem);
  if (status != kIOReturnSuccess) {
    XESYSLOG("Failed to map interrupt controller memory");
    return false;
  }

  XEDBGLOG("Mapped registers to %p", _mmioMem);

  //
  // Map PCI bridge memory for interrupt routing.
  //
  _bridgeMmioMap = getProvider()->mapDeviceMemoryWithIndex(0);
  if (_bridgeMmioMap == NULL) {
    XESYSLOG("Failed to map bridge memory");
    return false;
  }
  _bridgeMmioMem = (volatile void*) _bridgeMmioMap->getVirtualAddress();
  XEDBGLOG("Mapped bridge memory at 0x%X length 0x%X to %p", _bridgeMmioMap->getPhysicalAddress(),
    _bridgeMmioMap->getLength(), _bridgeMmioMem);

  //
  // Map BIU memory.
  //
  _biuMmioMap = getProvider()->mapDeviceMemoryWithIndex(1);
  if (_biuMmioMap == NULL) {
    XESYSLOG("Failed to map BIU memory");
    return false;
  }
  _biuMmioMem = (volatile void*) _biuMmioMap->getVirtualAddress();
  XEDBGLOG("Mapped BIU memory at 0x%X length 0x%X to %p", _biuMmioMap->getPhysicalAddress(),
    _biuMmioMap->getLength(), _biuMmioMem);

  writeBridgeReg32(0, 0);
  writeBridgeReg32(4, 0x40000000);

  OSWriteLittleInt32(_biuMmioMem, 0x40074, 0x40000000);
  OSWriteLittleInt32(_biuMmioMem, 0x40078, 0xea000050);

  writeBridgeReg32(0xC, 0);
  writeBridgeReg32(0, 0x3);

  //
  // Mask all PCI bridge interrupts if not already.
  //
  for (UInt32 i = 0; i < kXenonPCIBridgeVectorCount; i++) {
    writeBridgeReg32(kXenonPCIBridgeRegIntBase + (i * sizeof(UInt32)), 0);
  }
  eieio();

  //
  // Configure interrupt controller.
  //
  writeICReg64(0, kXenonICRegSpuriousVector, 0x7C);
  writeICReg64(0, kXenonICRegPriority, 0);
  writeICReg64(0, kXenonICRegLogicalID, 1);

  // Purge any pending interrupts.
  while (readICReg64(0, kXenonICRegInterruptAck) != 0x7C) {

  }
 // writeICReg64(0, kXenonICRegEndOfInterruptAutoUpd, 0);
  eieio();

  //
  // Allocate vectors.
  //
  vectors = (IOInterruptVector *)IOMalloc(kXenonICVectorCount * sizeof (IOInterruptVector));
  if (vectors == NULL) {
    XESYSLOG("Failed to allocate vectors");
    return false;
  }
  bzero(vectors, kXenonICVectorCount * sizeof (IOInterruptVector));

  //
  // Allocate vector locks.
  //
  vectorLockResult = true;
  for (int i = 0; i < kXenonICVectorCount; i++) {
    vectors[i].interruptLock = IOLockAlloc();
    if (vectors[i].interruptLock == NULL) {
      vectorLockResult = false;
      break;
    }
  }
  if (!vectorLockResult) {
    XESYSLOG("Failed to allocate vector locks");
    return false;
  }

  registerService();

  //
  // Register this as the platform interrupt controller.
  //
  getPlatform()->setCPUInterruptProperties(provider);
  provider->registerInterrupt(0, this, getInterruptHandlerAddress(), 0);
  provider->enableInterrupt(0);

  getPlatform()->registerInterruptController(interruptControllerName, this);

  XEDBGLOG("Initialized Xenon interrupt controller");
  return true;
}

//
// Overrides IOInterruptController::getInterruptHandlerAddress().
//
// Gets the address of the primary interrupt handler for this controller.
//
IOInterruptAction XenonInterruptController::getInterruptHandlerAddress(void) {
  return OSMemberFunctionCast(IOInterruptAction, this, &XenonInterruptController::handleInterrupt);
}

//
// Overrides IOInterruptController::handleInterrupt().
//
// Handles all incoming interrupts for this controller and forwards to the appropriate vectors.
//
IOReturn XenonInterruptController::handleInterrupt(void *refCon, IOService *nub, int source) {
  IOInterruptVector *vector;
  UInt8             vectorIndex;
  UInt8             pciIrq;

  //
  // Get/ack pending IRQ and reset priority.
  //
  vectorIndex = readICReg64(0, kXenonICRegInterruptAck) & kXenonICRegInterruptAckMask;
  pciIrq = sXenonCPUInterruptMap[vectorIndex];
  writeICReg64(0, kXenonICRegPriority, 0x7C);
  eieio();
  readICReg64(0, kXenonICRegPriority);

  switch (vectorIndex) {
    case kXenonVectorXenos:
      panic("Got Xenons here!\n");
      break;

    case kXenonVectorIOC:
      printf("Got IOC here\n");
      OSWriteLittleInt32(_biuMmioMem, 0x4002c, 0);
      break;

    case kXenonVectorClock:
      printf("Got the clock here\n");
      break;

    default:
      break;
  }

  if (pciIrq == kXenonVectorInvalid) {
    printf("Got another one here 0x%X\n", vectorIndex);
  }

  vector = &vectors[vectorIndex];
  vector->interruptActive = 1;
  sync();
  isync();

  if (!vector->interruptDisabledSoft) {
    isync();

    //
    // Call the handler if it exists.
    //
    if (vector->interruptRegistered) {
      vector->handler(vector->target, vector->refCon, vector->nub, vector->source);
    }

  } else {
    vector->interruptDisabledHard = 1;
    disableVectorHard(vectorIndex, vector);
  }

  vector->interruptActive = 0;

  //
  // EOI it.
  //
  writeICReg64(0, kXenonICRegEndOfInterrupt, 0);
  writeICReg64(0, kXenonICRegEndOfInterruptAutoUpd, 0);
  readICReg64(0, kXenonICRegPriority);
  return kIOReturnSuccess;
}

//
// Overrides IOInterruptController::getVectorType().
//
// Gets the type of vector.
//
int XenonInterruptController::getVectorType(IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector) {
  return kIOInterruptTypeLevel;
}

//
// Overrides IOInterruptController::disableVectorHard().
//
// Masks and disables the specified vector.
//
void XenonInterruptController::disableVectorHard(IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector) {
  UInt8 pciIrq;

  if (vectorNumber >= kXenonICVectorCount) {
    return;
  }

  // Only PCI bridge interrupts can actually be masked.
  pciIrq = sXenonCPUInterruptMap[vectorNumber];
  if (pciIrq == kXenonVectorInvalid) {
    return;
  }

  writeBridgeReg32(kXenonPCIBridgeRegIntBase + (pciIrq * sizeof(UInt32)), 0);
}

//
// Overrides IOInterruptController::enableVector().
//
// Unmasks and enables the specified vector.
//
void XenonInterruptController::enableVector(IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector) {
  UInt8 pciIrq;
  UInt32 irqState;

  if (vectorNumber >= kXenonICVectorCount) {
    return;
  }

  // Only PCI bridge interrupts can actually be unmasked.
  pciIrq = sXenonCPUInterruptMap[vectorNumber];
  if (pciIrq == kXenonVectorInvalid) {
    return;
  }

  // Configure interrupt to route to CPU0 only.
  irqState = kXenonPCIBridgeRegIntEnabled | (1 << kXenonPCIBridgeRegIntTargetCPUShift) | 0x80 | (vectorNumber >> kXenonPCIBridgeRegIntCPUIRQShift);
  writeBridgeReg32(kXenonPCIBridgeRegIntBase + (pciIrq * sizeof(UInt32)), irqState);
}
