//
//  XenonEthernet_PHY.cpp
//  Xbox 360 Ethernet controller driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonEthernet.hpp"

#define MBPS(x)   (1000000 * (x))

//
// Medium indexes.
//
enum {
  kMediumIndex10Half = 0,
  kMediumIndex10Full,
  kMediumIndex100Half,
  kMediumIndex100Full,
  kMediumIndexAuto,
  kMediumIndexCount
};

//
// Writes a PHY register.
//
bool XenonEthernet::phyWriteWithAddr(UInt32 addr, UInt32 offset, UInt16 data) {
  UInt32 timeout = kXenonEthernetPhyTimeoutUS;

  writeReg32(kXenonEthernetRegPhy, (addr << kXenonEthernetRegPhyAddrShift)
    | (offset << kXenonEthernetRegPhyRegShift)
    | kXenonEthernetRegPhyRequest | kXenonEthernetRegPhyOpWrite
    | ((UInt32)data) << kXenonEthernetRegPhyDataShift);
  do {
    timeout--;
    IODelay(1);
  } while ((readReg32(kXenonEthernetRegPhy) & kXenonEthernetRegPhyRequest) && (timeout != 0));

  if (timeout == 0) {
    XESYSLOG("PHY write timeout");
  }

  return timeout != 0;
}

//
// Reads a PHY register.
//
bool XenonEthernet::phyReadWithAddr(UInt32 addr, UInt32 offset, UInt16 *data) {
  UInt32 timeout = kXenonEthernetPhyTimeoutUS;

  writeReg32(kXenonEthernetRegPhy, (addr << kXenonEthernetRegPhyAddrShift)
    | (offset << kXenonEthernetRegPhyRegShift)
    | kXenonEthernetRegPhyRequest | kXenonEthernetRegPhyOpRead);
  do {
    timeout--;
    IODelay(1);
  } while ((readReg32(kXenonEthernetRegPhy) & kXenonEthernetRegPhyRequest) && (timeout != 0));

  if (timeout == 0) {
    XESYSLOG("PHY read timeout");
    return false;
  }

  *data = (UInt16) (readReg32(kXenonEthernetRegPhy) >> kXenonEthernetRegPhyDataShift);
  return true;
}

//
// Probe and locate the PHY to use.
//
bool XenonEthernet::phyProbe(void) {
  UInt16 phyStatus;
  UInt16 phyId1;
  UInt16 phyId2;

  for (UInt32 addr = 0; addr <= kXenonEthernetPhyMaxAddr; addr++) {
    XEDBGLOG("Probing PHY addr %u", addr);
    if (!phyReadWithAddr(addr, kXenonEthernetPhyRegStatus, &phyStatus)) {
      continue;
    }
    if (!phyReadWithAddr(addr, kXenonEthernetPhyRegStatus, &phyStatus)) {
      continue;
    }

    if ((phyStatus != 0x0000) && (phyStatus != 0xFFFF)) {
      _phyAddr = addr;

      if (!phyRead(kXenonEthernetPhyRegId1, &phyId1) || !phyRead(kXenonEthernetPhyRegId2, &phyId2)) {
        return false;
      }

      XEDBGLOG("PHY %04X:%04X at addr %u", phyId1, phyId2, _phyAddr);
      return true;
    }
  }

  return false;
}

//
// Reset the PHY.
//
bool XenonEthernet::phyReset(void) {
  UInt16 phyControl;
  UInt32 timeout = kXenonEthernetPhyTimeoutUS;

  if (!phyRead(kXenonEthernetPhyRegControl, &phyControl)) {
    return false;
  }
  phyControl |= kXenonEthernetPhyRegControlReset;
  if (!phyWrite(kXenonEthernetPhyRegControl, phyControl)) {
    return false;
  }

  // Wait for bit to clear.
  do {
    timeout--;
    IODelay(1);
    if (!phyRead(kXenonEthernetPhyRegControl, &phyControl)) {
      return false;
    }
  } while ((phyControl & kXenonEthernetPhyRegControlReset) && (timeout != 0));

  return timeout != 0;
}

//
// Adds a medium to the list of supported mediums.
//
bool XenonEthernet::phyAddMedium(IOMediumType type, UInt32 speed, UInt32 index) {
  IONetworkMedium *medium;
  bool            result = false;

  XEDBGLOG("Adding medium type %u speed %u index %u", type, speed, index);
  medium = IONetworkMedium::medium(type, speed, 0, index);
  if (medium != NULL) {
    result = IONetworkMedium::addMedium(_mediumDict, medium);
    medium->release();
  }

  return result;
}

//
// Gets all supported mediums.
//
bool XenonEthernet::phyGetSupportedMediums(void) {
  UInt16 phyStatus;

  if (_mediumDict != NULL) {
    return true;
  }

  // Get the status bits.
  if (!phyRead(kXenonEthernetPhyRegStatus, &phyStatus)) {
    return false;
  }

  _mediumDict = OSDictionary::withCapacity(kMediumIndexCount);
  if (_mediumDict == NULL) {
    return false;
  }

  // Populate the mediums.
  if (phyStatus & kXenonEthernetPhyRegStatusSupp10Half) {
    phyAddMedium(kIOMediumEthernet10BaseT | kIOMediumOptionHalfDuplex, MBPS(10), kMediumIndex10Half);
  }
  if (phyStatus & kXenonEthernetPhyRegStatusSupp10Full) {
    phyAddMedium(kIOMediumEthernet10BaseT | kIOMediumOptionFullDuplex, MBPS(10), kMediumIndex10Full);
  }
  if (phyStatus & kXenonEthernetPhyRegStatusSupp100Half) {
    phyAddMedium(kIOMediumEthernet100BaseTX | kIOMediumOptionHalfDuplex, MBPS(100), kMediumIndex100Half);
  }
  if (phyStatus & kXenonEthernetPhyRegStatusSupp100Full) {
    phyAddMedium(kIOMediumEthernet100BaseTX | kIOMediumOptionHalfDuplex, MBPS(100), kMediumIndex100Full);
  }

  phyAddMedium(kIOMediumEthernetAuto, 0, kMediumIndexAuto);
  return publishMediumDictionary(_mediumDict);
}

void XenonEthernet::phyUpdateLinkStatus(void) {
  UInt32 linkStatus = kIONetworkLinkValid;
  UInt16 phyStatus;

  // Get the status bits.
  if (!phyRead(kXenonEthernetPhyRegStatus, &phyStatus)) {
    return;
  }

  if (phyStatus & kXenonEthernetPhyRegStatusLink) {
    linkStatus |= kIONetworkLinkActive;

    XEDBGLOG("Link up");
  } else {
    XEDBGLOG("Link down");
  }

  setLinkStatus(linkStatus);
}
