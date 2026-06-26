//
//  XenonSATA.cpp
//  Xbox 360 SATA controller driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include <IOKit/ata/ATADeviceNub.h>
#include <IOKit/ata/IOATABusInfo.h>
#include <IOKit/ata/IOATATypes.h>
#include <IOKit/scsi/SCSICommandOperationCodes.h>
#include <IOKit/storage/IOStorageProtocolCharacteristics.h>

#include "XenonSATA.hpp"
#include "XenonSATARegs.hpp"

OSDefineMetaClassAndStructors(XenonSATA, super);

//
// Performs driver startup.
// Overrides IOService::start().
//
bool XenonSATA::start(IOService *provider) {
  ATADeviceNub *newNub;

  XenonCheckDebugArgs();
  XEDBGLOG("Starting Xenon SATA controller");

  _pciParent = OSDynamicCast(IOPCIDevice, provider);
  if (_pciParent == NULL) {
    XESYSLOG("Provider is not IOPCIDevice");
    return false;
  }
  _pciParent->retain();

  if (!_pciParent->open(this)) {
    return false;
  }

  // System Profiler works off the interconnect for ATA vs SATA.
  setProperty(kIOPropertyPhysicalInterconnectTypeKey, kIOPropertyPhysicalInterconnectTypeSerialATA);

  if (!super::start(provider)) {
    XESYSLOG("super::start() returned false");
    return false;
  }

  _intEventSource = IOInterruptEventSource::interruptEventSource(this,
    OSMemberFunctionCast(IOInterruptEventSource::Action, this, &XenonSATA::handleInterrupt), getProvider(), 0);
  if ((_intEventSource == NULL) || (getWorkLoop()->addEventSource(_intEventSource) != kIOReturnSuccess)) {
    XESYSLOG("Failed to create interrupt");
    return false;
  }
  _intEventSource->enable();

  // Create nubs for attached devices.
  for (UInt32 i = 0; i < 2; i++) {
    if (_devInfo[i].type != kUnknownATADeviceType) {
      XEDBGLOG("Creating nub %u for device type %u", i, _devInfo[i].type);
      newNub = ATADeviceNub::ataDeviceNub(this, (ataUnitID)i, _devInfo[i].type);
      if (newNub != NULL) {
        XEDBGLOG("Attaching nub %u", i);
        newNub->attach(this);

        _nub[i] = newNub;
        newNub->registerService();

        XEDBGLOG("Registered nub %u", i);
        OSSafeReleaseNULL(newNub);
      }
    }
  }

  XEDBGLOG("Started Xenon SATA controller");
  return true;
}

//
// Releases driver resources.
// Overrides IOService::free().
//
void XenonSATA::free(void) {
  OSSafeReleaseNULL(_intEventSource);
  OSSafeReleaseNULL(_mmioMap);
  OSSafeReleaseNULL(_bmdmaMap);
  OSSafeReleaseNULL(_pciParent);

  super::free();
}

//
// Creates the work loop if not already created.
// Overrides IOService::getWorkLoop().
//
IOWorkLoop* XenonSATA::getWorkLoop(void) const {
  IOWorkLoop *workLoop = _workLoop;

  if (workLoop == NULL) {
    workLoop = IOWorkLoop::workLoop();
  }
  return workLoop;
}

//
// Gets ATA controller info.
// Overrides IOATAController::provideBusInfo().
//
IOReturn XenonSATA::provideBusInfo(IOATABusInfo *infoOut) {
  UInt8 units = 0;

  if (infoOut == NULL) {
    return -1;
  }

  infoOut->zeroData();
  infoOut->setSocketType(kInternalSATA);
  infoOut->setPIOModes(0x1F);
  infoOut->setDMAModes(0x7);
  infoOut->setUltraModes(0x007F);
  infoOut->setExtendedLBA(true);
  infoOut->setMaxBlocksExtended(256);

  for (UInt32 i = 0; i < ARRSIZE(_devInfo); i++) {
    if(_devInfo[i].type != kUnknownATADeviceType) {
      units++;
    }
  }
  infoOut->setUnits( units);

  return kATANoErr;
}

//
// Overrides IOATAController::selectConfig().
//
IOReturn XenonSATA::selectConfig(IOATADevConfig *configRequest, UInt32 unitNumber) {
  XEDBGLOG("start");

  _devInfo[unitNumber].packetSend = configRequest->getPacketConfig();
  return kATANoErr;
}

//
// Overrides IOATAController::getConfig().
//
IOReturn XenonSATA::getConfig(IOATADevConfig *configRequest, UInt32 unitNumber) {
  if ((configRequest == NULL) || (unitNumber > 1)) {
    return -1;
  }

  XEDBGLOG("start");

  // grab the info from our internal data.
  configRequest->setPIOMode( busTimings[unitNumber].ataPIOSpeedMode);
  configRequest->setDMAMode(busTimings[unitNumber].ataMultiDMASpeed);
  configRequest->setPIOCycleTime(busTimings[unitNumber].ataPIOCycleTime );
  configRequest->setDMACycleTime(busTimings[unitNumber].ataMultiCycleTime);
  configRequest->setPacketConfig( _devInfo[unitNumber].packetSend );
  configRequest->setUltraMode(busTimings[unitNumber].ataUltraDMASpeedMode);

  return kATANoErr;
}

//
// Submits a command to the queue.
// Overrides IOATAController::executeCommand().
//
IOReturn XenonSATA::executeCommand(IOATADevice *nub, IOATABusCommand *cmd) {
  UInt8 *packetData;

  // INQUIRY command needs to have control bits set.
  if ((cmd->getOpcode() == kATAPIFnExecIO) && (cmd->getPacketSize() >= 6)) {
    packetData = (UInt8*) cmd->getPacketData();
    if ((packetData[0] == kSCSICmd_INQUIRY) && (packetData[4] == 0x24)) {
      packetData[5] = 0xC0;
    }
  }

  return super::executeCommand(nub, cmd);
}

//
// Configures/initializes taskfile registers. Called during super::start().
// Overrides IOATAController::configureTFPointers().
//
bool XenonSATA::configureTFPointers(void) {
  // Map in command/control registers.
  _mmioMap = _pciParent->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
  if (_mmioMap == NULL) {
    XESYSLOG("Failed to map SATA command/control registers");
    return false;
  }

  _mmioMem = (volatile UInt8*) _mmioMap->getVirtualAddress();
  XEDBGLOG("Mapped SATA command/control registers at 0x%X length 0x%X to %p", _mmioMap->getPhysicalAddress(),
    _mmioMap->getLength(), _mmioMem);

  // Configure command/control registers.
  _tfDataReg      = (volatile UInt16*)(_mmioMem + kXenonSATARegData);
  _tfFeatureReg   = _mmioMem + kXenonSATARegFeatures;
  _tfSCountReg    = _mmioMem + kXenonSATARegSectorCount;
  _tfSectorNReg   = _mmioMem + kXenonSATARegLbaLow;
  _tfCylLoReg     = _mmioMem + kXenonSATARegLbaMed;
  _tfCylHiReg     = _mmioMem + kXenonSATARegLbaHigh;
  _tfSDHReg       = _mmioMem + kXenonSATARegDevSelect;
  _tfStatusCmdReg = _mmioMem + kXenonSATARegStatus;
  _tfAltSDevCReg  = _mmioMem + kXenonSATARegAltStatus;

  // Map BMDMA registers.
  _bmdmaMap = _pciParent->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress1);
  if (_bmdmaMap == NULL) {
    XESYSLOG("Failed to map SATA BMDMA registers");
    return false;
  }

  _bmdmaMem = (volatile UInt8*) _bmdmaMap->getVirtualAddress();
  XEDBGLOG("Mapped SATA BMDMA registers at 0x%X length 0x%X to %p", _bmdmaMap->getPhysicalAddress(),
    _bmdmaMap->getLength(), _bmdmaMem);

  // Configure BMDMA registers.
  _bmCommandReg   = _bmdmaMem + kXenonSATADmaRegCommand;
  _bmStatusReg    = _bmdmaMem + kXenonSATADmaRegStatus;
  _bmPRDAddresReg = (volatile UInt32*)(_bmdmaMem + kXenonSATADmaRegTableOffset);

  return true;
}

//
// Interrupt handler.
//
void XenonSATA::handleInterrupt(IOInterruptEventSource *intEventSource, int count) {
  handleDeviceInterrupt();
}
