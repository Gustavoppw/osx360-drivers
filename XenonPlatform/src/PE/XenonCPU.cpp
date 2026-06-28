//
//  XenonCPU.cpp
//  Xbox 360 CPU platform device
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include <IOKit/IODeviceTreeSupport.h>
#include "XenonCPU.hpp"
#include "XenonPE.hpp"

OSDefineMetaClassAndStructors(XenonCPU, super);

static IOCPUInterruptController *gCPUIC;

//
// Overrides IOCPU::start()
//
bool XenonCPU::start(IOService *provider) {
  kern_return_t       result;
  IORegistryEntry     *cpusRegEntry;
  OSData              *tmpData;
  UInt32              physCPU;
  ml_processor_info_t processor_info;

  XenonCheckDebugArgs();

  if (OSDynamicCast(XenonPE, getPlatform()) == NULL) {
    XESYSLOG("Current platform is not Xenon");
    return false;
  }

  if (!super::start(provider)) {
    XESYSLOG("super::start() returned false");
    return false;
  }

  // Get total CPU count.
  _numCPUs = 0;
  cpusRegEntry = fromPath("/cpus", gIODTPlane);
  if (cpusRegEntry == NULL) {
    XESYSLOG("Failed to get /cpus from the device tree");
    return false;
  }
  _numCPUs = 1; // TODO: Only handle one CPU.

  // Set physical CPU number from the "reg" property.
  tmpData = OSDynamicCast(OSData, provider->getProperty("reg"));
  if (tmpData == NULL) {
    XESYSLOG("Failed to read reg property");
    return false;
  }
  physCPU = *((UInt32 *)tmpData->getBytesNoCopy());
  setCPUNumber(physCPU);

  // Check if boot CPU.
  _isBootCPU = true;
  tmpData = OSDynamicCast(OSData, provider->getProperty("state"));
  if (tmpData == 0) {
    XESYSLOG("Failed to read state property");
    return false;
  }
  if (strcmp((char *)tmpData->getBytesNoCopy(), "running") == 0) {
    _isBootCPU = true;
  }
  XEDBGLOG("Physical CPU number: %u, boot CPU: %u", physCPU, _isBootCPU);

  // Create the CPU interrupt controller.
  if (_isBootCPU) {
    gCPUIC = new IOCPUInterruptController;
    if (gCPUIC == NULL) {
      XESYSLOG("Failed to create IOCPUInterruptController");
      return false;
    }
    if (gCPUIC->initCPUInterruptController(_numCPUs) != kIOReturnSuccess) {
      XESYSLOG("Failed to initialize IOCPUInterruptController");
      return false;
    }
    gCPUIC->attach(this);
    gCPUIC->registerCPUInterruptController();
  }

  // CPU starts out uninitialized.
  setCPUState(kIOCPUStateUninitalized);

  // Register the CPU with XNU and start it.
  XEDBGLOG("Registering CPU %u with XNU", physCPU);
  if (physCPU < _numCPUs) {
    processor_info.cpu_id           = (cpu_id_t)this;
    processor_info.boot_cpu         = _isBootCPU;
    processor_info.start_paddr      = 0x0100;
    processor_info.l2cr_value       = 0;
    processor_info.supports_nap     = false;
    processor_info.time_base_enable = NULL;

    result = ml_processor_register(&processor_info, &machProcessor, &ipi_handler);
    if (result == KERN_FAILURE) {
      XESYSLOG("Failed to register the CPU with XNU");
      return false;
    }
    processor_start(machProcessor);
  }

  registerService();

  XEDBGLOG("Initialized CPU %u", physCPU);
  return true;
}

//
// Overrides IOCPU::initCPU()
//
void XenonCPU::initCPU(bool boot) {
  if (boot) {
    gCPUIC->enableCPUInterrupt(this);

    // Register and enable IPIs.
	  cpuNub->registerInterrupt(0, this, OSMemberFunctionCast(IOInterruptAction, this, &XenonCPU::ipiHandler), 0);
    cpuNub->enableInterrupt(0);
  }
  else
  {
    // TODO
  }

  //
  // CPU is now running.
  //
  setCPUState(kIOCPUStateRunning);
}

//
// Overrides IOCPU::quiesceCPU()
//
void XenonCPU::quiesceCPU(void) {
  // TODO
}

//
// Overrides IOCPU::startCPU()
//
kern_return_t XenonCPU::startCPU(vm_offset_t start_paddr, vm_offset_t arg_paddr) {
  // TODO
  return KERN_SUCCESS;
}

//
// Overrides IOCPU::haltCPU()
//
void XenonCPU::haltCPU(void) {
  // TODO
}

//
// Overrides IOCPU::getCPUName()
//
const OSSymbol *XenonCPU::getCPUName(void) {
  char tmpStr[256];
  snprintf(tmpStr, sizeof (tmpStr), "Primary%lu", getCPUNumber());
  return OSSymbol::withCString(tmpStr);
}

//
// IPI handler.
//
void XenonCPU::ipiHandler(void *refCon, void *nub, int source) {
  if (ipi_handler != NULL) {
    ipi_handler();
  }
}
