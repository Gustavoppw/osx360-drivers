//
//  XenosController_Firmware.cpp
//  Xbox 360 graphics controller
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenosController.hpp"
#include "XenosFirmware.hpp"

//
// Loads Xenos firmware onto the card.
//
IOReturn XenosController::loadFirmware(void) {
  writeReg32(kXenosRegCpPfpFirmwareAddr, 0);
  IODelay(100);
  for (UInt32 i = 0; i < ARRSIZE(sXenosFirmware0); i++) {
    writeReg32(kXenosRegCpPfpFirmwareData, sXenosFirmware0[i]);
  }

  writeReg32(kXenosRegCpPfpFirmwareAddr, 0);
  IODelay(100);
  for (UInt32 i = 0; i < ARRSIZE(sXenosFirmware0); i++) {
    readReg32(kXenosRegCpPfpFirmwareData);
  }

  writeReg32(kXenosRegCpMeRamWriteAddr, 0);
  for (UInt32 i = 0; i < ARRSIZE(sXenosFirmware1); i++) {
    writeReg32(kXenosRegCpMeRamData, sXenosFirmware1[i]);
  }

  writeReg32(kXenosRegCpMeRamReadAddr, 0);
  for (UInt32 i = 0; i < ARRSIZE(sXenosFirmware1); i++) {
    if (readReg32(kXenosRegCpMeRamData) != sXenosFirmware1[i]) {
      XESYSLOG("ME firmware invalid");
      return kIOReturnIOError;
    }
  }

  return kIOReturnSuccess;
}