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
  // Overrides.
  bool init(OSDictionary *dictionary = 0);
  bool start(IOService *provider);
  IOReturn EjectTheMedia(void);

protected:
  // Overrides.
  IOReturn GetMechanicalCapabilities(void);

private:
  IOService *_xenonSMC;
};

#endif
