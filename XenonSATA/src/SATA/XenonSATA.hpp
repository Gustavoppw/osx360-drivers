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
  // IOService overrides.
  bool start(IOService *provider);
  void free(void);
  IOWorkLoop* getWorkLoop(void) const;

  // IOATAController overrides.
  IOReturn provideBusInfo(IOATABusInfo *infoOut);
  IOReturn selectConfig(IOATADevConfig *configRequest, UInt32 unitNumber);
  IOReturn getConfig(IOATADevConfig *configRequest, UInt32 unitNumber);
  IOReturn executeCommand(IOATADevice *nub, IOATABusCommand *cmd);

protected:
  // IOATAController overrides.
  bool configureTFPointers(void);

private:
  IOPCIDevice             *_pciParent;
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
};

#endif
