//
//  XenonDVDDevice.cpp
//  Xbox 360 DVD drive shim driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonDVDDevice.hpp"

OSDefineMetaClassAndStructors(XenonDVDDevice, super);

//
// Overrides IOSCSIPeripheralDeviceType05::init().
//
bool XenonDVDDevice::init(OSDictionary *dictionary) {
  XenonCheckDebugArgs();

  _xenonSMC = NULL;

  return super::init(dictionary);
}

//
// Overrides IOSCSIPeripheralDeviceType05::start().
//
bool XenonDVDDevice::start(IOService *provider) {
  mach_timespec_t t;

  XEDBGLOG("start");

  t.tv_sec = 30;
  t.tv_nsec = 0;

  _xenonSMC = waitForService(serviceMatching("XenonSMC"), &t);
  if (_xenonSMC == NULL) {
    XESYSLOG("SMC services not available");
  }

  return super::start(provider);
}

//
// Overrides IOSCSIPeripheralDeviceType05::EjectTheMedia().
//
IOReturn XenonDVDDevice::EjectTheMedia(void) {
  IOReturn status;

  XEDBGLOG("start");

  status = super::EjectTheMedia();
  if ((status == kIOReturnSuccess) && (_xenonSMC != NULL)) {
    _xenonSMC->callPlatformFunction(kXenonFuncSMCEject, false, NULL, NULL, NULL, NULL);
  }

  return status;
}

//
// Overrides IOSCSIPeripheralDeviceType05::GetMechanicalCapabilities().
//
IOReturn XenonDVDDevice::GetMechanicalCapabilities(void) {
  IOReturn status = super::GetMechanicalCapabilities();

  XEDBGLOG("start");

  // GetMechanicalCapabilities() fails on this particular drive, ensure DVD is supported.
  fSupportedCDFeatures |= kCDFeaturesReadStructuresMask;
  fSupportedDVDFeatures |= (kDVDFeaturesReadStructuresMask | kDVDFeaturesCSSMask);
  return status;
}
