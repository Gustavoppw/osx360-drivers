//
//  XenonDVDDevice.hpp
//  Xbox 360 DVD drive shim driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonDVDDevice_hpp
#define XenonDVDDevice_hpp

#include <IOKit/scsi/IOSCSIPeripheralDeviceType05.h>
#include "XenonCommon.hpp"

//
// Represents the Xbox 360 DVD drive shim driver.
//
class XenonDVDDevice : public IOSCSIPeripheralDeviceType05 {
  OSDeclareDefaultStructors(XenonDVDDevice);
  XenonDeclareLogFunctions("dvd");
  typedef IOSCSIPeripheralDeviceType05 super;

public:
  // IOService overrides.
  bool start(IOService *provider);
  void free(void);

  // IOSCSIMultimediaCommandsDevice overrides.
  IOReturn EjectTheMedia(void);

protected:
  // IOSCSIMultimediaCommandsDevice overrides.
  IOReturn GetMechanicalCapabilities(void);

private:
  IOService *_xenonSMC;
};

#endif
