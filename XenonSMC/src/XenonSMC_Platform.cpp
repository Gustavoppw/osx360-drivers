//
//  XenonSMC_Platform.cpp
//  Xbox 360 system management controller driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include <IOKit/IOPlatformExpert.h>
#include "XenonSMC.hpp"

//
// Static instance for platform hooks.
//
XenonSMC* XenonSMC::_gXenonSMC = NULL;

//
// PE_halt_restart platform function implementation.
//
int XenonSMC::peHaltRestart(unsigned int type) {
  bool reboot;

  if (_gXenonSMC == NULL) {
    return 1;
  }

  switch (type) {
    case kPERestartCPU:
      reboot = true;
      break;

    case kPEHaltCPU:
      reboot = false;
      break;

    default:
      return 1;
  }

  return (_gXenonSMC->rebootPowerOff(reboot) == kIOReturnSuccess) ? 0 : 1;
}

//
// PE_read_write_time_of_day platform function implementation.
//
int XenonSMC::peReadWriteTimeDay(unsigned int options, long *secs) {
  UInt64    rtcValue;
  IOReturn  status;

  if (_gXenonSMC == NULL) {
    return 1;
  }

  switch (options) {
    case kPEReadTOD:
      status = _gXenonSMC->getRTC(&rtcValue);
      if (status == kIOReturnSuccess) {
        *secs = kXenonSMCRTCBase + Div64_32(rtcValue, 1000);
        return 0;
      }
      break;

    case kPEWriteTOD:
      break;

    default:
      return 1;
  }

  return 1;
}
