//
//  XenonDVDDevice.cpp
//  Xbox 360 DVD drive shim driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonDVDDevice.hpp"

OSDefineMetaClassAndStructors(XenonDVDDevice, super);

//
// Performs driver startup.
// Overrides IOService::start().
//
bool XenonDVDDevice::start(IOService *provider) {
  mach_timespec_t t;

  XenonCheckDebugArgs();
  XEDBGLOG("Starting DVD device");

  t.tv_sec = 30;
  t.tv_nsec = 0;

  _xenonSMC = waitForService(serviceMatching("XenonSMC"), &t);
  if (_xenonSMC != NULL) {
    _xenonSMC->retain();
  } else {
    XESYSLOG("SMC services not available");
  }

  return super::start(provider);
}

//
// Releases driver resources.
// Overrides IOService::free().
//
void XenonDVDDevice::free(void) {
  OSSafeReleaseNULL(_xenonSMC);
  super::free();
}

//
// Ejects the CD/DVD tray.
// On Xbox 360, the tray will not actually eject normally, so instruct the SMC to do so.
// Overrides IOSCSIMultimediaCommandsDevice::EjectTheMedia().
//
IOReturn XenonDVDDevice::EjectTheMedia(void) {
  IOReturn status;

  XEDBGLOG("Ejecting tray");

  status = super::EjectTheMedia();
  if ((status == kIOReturnSuccess) && (_xenonSMC != NULL)) {
    _xenonSMC->callPlatformFunction(kXenonFuncSMCEject, false, NULL, NULL, NULL, NULL);
  }

  return status;
}

//
// Gets the capabilities of the CD/DVD drive.
// Overrides IOSCSIMultimediaCommandsDevice::GetMechanicalCapabilities().
//
IOReturn XenonDVDDevice::GetMechanicalCapabilities(void) {
  IOReturn status = super::GetMechanicalCapabilities();

  // GetMechanicalCapabilities() fails on this particular drive, ensure DVD is supported.
  fSupportedCDFeatures |= kCDFeaturesReadStructuresMask;
  fSupportedDVDFeatures |= (kDVDFeaturesReadStructuresMask | kDVDFeaturesCSSMask);
  return status;
}
