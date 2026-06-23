//
//  XenonSATA.hpp
//  Xbox 360 SATA controller driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonSATA_hpp
#define XenonSATA_hpp

#include <IOKit/ata/IOPCIATA.h>
#include <IOKit/pci/IOPCIDevice.h>

#include "XenonCommon.hpp"

//
// Represents the Xbox 360 SATA controller driver.
//
class XenonSATA : public IOPCIATA {
  OSDeclareDefaultStructors(XenonSATA);
  XenonDeclareLogFunctions("sata");
  typedef IOPCIATA super;

public:
  // Overrides.
  bool init(OSDictionary *dictionary = 0);
  bool start(IOService *provider);
  void free(void);
  IOWorkLoop* getWorkLoop(void) const;

  IOReturn provideBusInfo(IOATABusInfo *infoOut);
  IOReturn selectConfig(IOATADevConfig *configRequest, UInt32 unitNumber);
  IOReturn getConfig(IOATADevConfig *configRequest, UInt32 unitNumber);
  IOReturn executeCommand(IOATADevice *nub, IOATABusCommand *cmd);

protected:
  // Overrides.
  bool configureTFPointers(void);

private:
  IOMemoryMap             *_mmioMap;
  volatile UInt8          *_mmioMem;
  IOMemoryMap             *_bmdmaMap;
  volatile UInt8          *_bmdmaMem;

  IOInterruptEventSource  *_intEventSource;

  struct ATABusTimings {
    UInt8   ataPIOSpeedMode;      // PIO Mode Timing class (bit-significant)
    UInt16  ataPIOCycleTime;      // Cycle time for PIO mode
    UInt8   ataMultiDMASpeed;     // Multiple Word DMA Timing Class (bit-significant)
    UInt16  ataMultiCycleTime;    // Cycle time for Multiword DMA mode
    UInt16  ataUltraDMASpeedMode; // Ultra Timing class (bit-significant)
  };

  ATABusTimings busTimings[2];

  void handleInterrupt(IOInterruptEventSource *intEventSource, int count);
  bool createInterrupt(void);
};

#endif
