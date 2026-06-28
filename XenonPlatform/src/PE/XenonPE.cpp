//
//  XenonPE.cpp
//  Xbox 360 platform expert
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include <ppc/proc_reg.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/platform/ApplePlatformExpert.h>
#include <IOKit/IODeviceTreeSupport.h>

#include "XenonPE.hpp"

OSDefineMetaClassAndStructors(XenonPE, super);

#define kMolStdMachineType	1
#define kChipSetTypeMol		170

//
// Overrides IODTPlatformExpert::start().
//
bool XenonPE::start(IOService *provider) {
  IOReturn        status;
  IOService       *service;
  IOPMrootDomain  *pmRootDomain;

  XenonCheckDebugArgs();
  XEDBGLOG("Initializing Xenon platform expert");

  if (!mapGPU()) {
    return false;
  }
  startFramebuffer();

  status = initPatcher();
  if (status != kIOReturnSuccess) {
    return false;
  }

  setChipSetType(kChipSetTypeMol);
  setMachineType(kMolStdMachineType);
  setBootROMType(kBootROMTypeNewWorld);

  _pePMFeatures     = kStdDesktopPMFeatures;
  _pePrivPMFeatures = kStdDesktopPrivPMFeatures;
  _peNumBatteriesSupported = kStdDesktopNumBatteries;

  if (!super::start(provider)) {
    XESYSLOG("super::start() returned false");
    return false;
  }

  //
  // TODO: Need to implement IONVRAM resources, otherwise XNU waits for 30s for these in IOKitResetTime().
  //
  publishResource("IONVRAM");

  //
  // Prevent sleep/doze, Wii hardware is incapable of sleeping but unsure of doze. Seems to cause issues on Wii U and the GPU.
  //
  service = waitForService(serviceMatching("IOPMrootDomain"));
  pmRootDomain = OSDynamicCast(IOPMrootDomain, service);
  if (pmRootDomain != NULL) {
    pmRootDomain->receivePowerNotification(kIOPMPreventSleep);
  }

  XEDBGLOG("Initialized Xenon platform expert");
  return true;
}

//
// Overrides IODTPlatformExpert::callPlatformFunction().
//
IOReturn XenonPE::callPlatformFunction(const OSSymbol *functionName, bool waitForFunction,
                                       void *param1, void *param2, void *param3, void *param4) {
  if (functionName->isEqualTo(kXenonFuncPlatformStartFB)) {
    startFramebuffer();
    return kIOReturnSuccess;
  } else if (functionName->isEqualTo(kXenonFuncPlatformStopFB)) {
    stopFramebuffer();
    return kIOReturnSuccess;
  }

  return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}

//
// Overrides IODTPlatformExpert::deleteList()
//
const char *XenonPE::deleteList(void) {
  return("('packages', 'psuedo-usb', 'psuedo-hid', 'multiboot', 'rtas')");
}

//
// Overrides IODTPlatformExpert::excludeList()
//
const char *XenonPE::excludeList(void) {
  //
  // List of DT nodes to exclude from enumeration.
  //
  return("('chosen', 'memory', 'openprom', 'AAPL,ROM', 'rom', 'options', 'aliases')");
}

//
// Overrides IODTPlatformExpert::getMachineName()
//
bool XenonPE::getMachineName(char *name, int maxLength) {
  strncpy(name, "Power Macintosh", maxLength);
  return true;
}

//
// Overrides IODTPlatformExpert::getGMTTimeOfDay().
//
long XenonPE::getGMTTimeOfDay(void) {
  mach_timespec_t t;
  long            secs;

  t.tv_sec = 30;
  t.tv_nsec = 0;
  if (waitForService(resourceMatching("IORTC"), &t) != NULL) {
    if (PE_read_write_time_of_day(kPEReadTOD, &secs) == 0) {
      return secs;
    }
  } else {
    XESYSLOG("RTC did not show up");
  }

  return 0;
}
