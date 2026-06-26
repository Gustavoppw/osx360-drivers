//
//  XenonEthernet_TransmitReceive.cpp
//  Xbox 360 Ethernet controller driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonEthernet.hpp"

#define TX_COUNT_MASK         (kXenonEthernetTxDescCount - 1)
#define TX_NEXT(index)        (((index) + 1) & TX_COUNT_MASK)
#define TX_USED(read, write)  (((write) - (read)) & TX_COUNT_MASK)
#define TX_FREE(read, write)  (kXenonEthernetTxDescCount - TX_USED((read), (write)) - 1)

#define RX_COUNT_MASK         (kXenonEthernetRxDescCount - 1)
#define RX_NEXT(index)        (((index) + 1) & RX_COUNT_MASK)
#define RX_USED(read, write)  (((write) - (read)) & RX_COUNT_MASK)
#define RX_FREE(read, write)  (kXenonEthernetRxDescCount - RX_USED((read), (write)) - 1)

//
// Allocates the transmit and receive descriptor buffers.
//
bool XenonEthernet::allocateTxRxBuffers(void) {
  IOByteCount length;

  // Create descriptor buffers.
  _txDescBufferDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, kXenonEthernetTxBufferLength, PAGE_SIZE);
  _rxDescBufferDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, kXenonEthernetRxBufferLength, PAGE_SIZE);
  if ((_txDescBufferDesc == NULL) || (_rxDescBufferDesc == NULL)) {
    XESYSLOG("Failed to allocate TX/RX descriptor buffers");
    return false;
  }

  _txDesc = (XenonEthernetDescriptor*) _txDescBufferDesc->getBytesNoCopy();
  _rxDesc = (XenonEthernetDescriptor*) _rxDescBufferDesc->getBytesNoCopy();
  _txDescPhysAddr = _txDescBufferDesc->getPhysicalSegment(0, &length);
  _rxDescPhysAddr = _rxDescBufferDesc->getPhysicalSegment(0, &length);

  XEDBGLOG("%u TX descriptors at %p phys 0x%X", kXenonEthernetTxDescCount, _txDesc, _txDescPhysAddr);
  XEDBGLOG("%u RX descriptors at %p phys 0x%X", kXenonEthernetRxDescCount, _rxDesc, _rxDescPhysAddr);

  // Create memory cursors. Controller requires little-endian addresses.
  _txCursor = IOMbufLittleMemoryCursor::withSpecification(kXenonEthernetMaxPacketLength, 1);
  _rxCursor = IOMbufLittleMemoryCursor::withSpecification(kXenonEthernetMaxPacketLength, 1);
  if ((_txCursor == NULL) || (_rxCursor == NULL)) {
    XESYSLOG("Failed to allocate TX/RX cursors");
    return false;
  }

  return true;
}

//
// Sets the physical location of a packet on the specified receive descriptor.
//
bool XenonEthernet::setRxDescriptorPacket(XenonEthernetDescriptor *desc, mbuf_t packet) {
  IOPhysicalSegment segment;
  UInt32            segmentCount;

  // Get the segment for the packet.
  segmentCount = _rxCursor->getPhysicalSegmentsWithCoalesce(packet, &segment, kXenonEthernetRxMaxSegCount);
  if (segmentCount != kXenonEthernetRxMaxSegCount) {
    return false;
  }

  // Memory cursor outputs in little endian format.
  desc->address = segment.location;
  desc->length  = segment.length;
  return true;
}

bool XenonEthernet::initTransmitReceiveBuffers(void) {
  bzero(_txDesc, kXenonEthernetTxBufferLength);
  bzero(_rxDesc, kXenonEthernetRxBufferLength);

  _txReadIndex  = 0;
  _txWriteIndex = 0;
  _rxIndex      = 0;

  // Initialize transmit buffers.
  _txDesc[kXenonEthernetTxDescCount - 1].length = OSSwapHostToLittleInt32(kXenonEthernetDescLengthEnd);

  // Initialize receive buffers.
  for (UInt32 i = 0; i < kXenonEthernetRxDescCount; i++) {
    _rxPackets[i] = allocatePacket(kXenonEthernetMaxPacketLength);
    if (_rxPackets[i] == NULL) {
      return false;
    }

    _rxDesc[i].flags = OSSwapHostToLittleInt32(kXenonEthernetRxFlagsInterrupt | kXenonEthernetRxFlagsOwner);
    if (!setRxDescriptorPacket(&_rxDesc[i], _rxPackets[i])) {
      return false;
    }
  }
  _rxDesc[kXenonEthernetRxDescCount - 1].length |= OSSwapHostToLittleInt32(kXenonEthernetDescLengthEnd);

  // Set descriptor physical locations.
  writeReg32(kXenonEthernetRegTxDescAddr, _txDescPhysAddr);
  writeReg32(kXenonEthernetRegRxDescAddr, _rxDescPhysAddr);

  return true;
}

UInt32 XenonEthernet::sendTxPacket(mbuf_t packet) {
  IOPhysicalSegment         segments[kXenonEthernetTxMaxSegCount];
  UInt32                    segmentCount;
  UInt32                    packetLength;
  UInt32                    checksumReqs;
  UInt32                    flags;

  UInt32                    index;
  XenonEthernetDescriptor   *descStart;
  XenonEthernetDescriptor   *desc;

  // Ensure there is at least the minimum descriptors available.
  if (TX_FREE(_txReadIndex, _txWriteIndex) < kXenonEthernetTxMaxSegCount) {
    return kIOReturnOutputStall;
  }

  segmentCount = _txCursor->getPhysicalSegmentsWithCoalesce(packet, segments, kXenonEthernetTxMaxSegCount);
  if (segmentCount == 0) {
    freePacket(packet);
    XEDBGLOG("Failed to get segments for TX packet");
    return kIOReturnOutputDropped;
  }

  // Common flags.
  flags = kXenonEthernetTxFlagsPad | kXenonEthernetTxFlagsCrc | kXenonEthernetTxFlagsDeferred;

  // Get checksum requirements.
  getChecksumDemand(packet, kChecksumFamilyTCPIP, &checksumReqs);
  if (checksumReqs & kChecksumIP) {
    flags |= kXenonEthernetTxFlagsIpChecksum;
  }
  if (checksumReqs & kChecksumTCP) {
    flags |= kXenonEthernetTxFlagsTcpChecksum;
  }
  if (checksumReqs & kChecksumUDP) {
    flags |= kXenonEthernetTxFlagsUdpChecksum;
  }

  // Fill first descriptor.
  // Segments are in little endian format already.
  index     = _txWriteIndex;
  descStart = &_txDesc[index];

  descStart->flags   = 0;
  descStart->address = segments[0].location;
  descStart->length  = segments[0].length;

  packetLength = OSSwapLittleToHostInt32(descStart->length);

  if (_txWriteIndex < 10)
  XEDBGLOG("Packet[%u] %p, segments: %u, seg[0]: 0x%X (length 0x%X)",
    index, packet, segmentCount, OSSwapLittleToHostInt32(segments[0].location), OSSwapLittleToHostInt32(segments[0].length));

  // Fill remaining descriptors.
  for (UInt32 i = 1; i < segmentCount; i++) {
    index = TX_NEXT(index);
    desc  = &_txDesc[index];

    desc->packetLength = 0;
    desc->flags        = OSSwapHostToLittleInt32(kXenonEthernetTxFlagsOwner);
    desc->address      = segments[i].location;
    desc->length       = segments[i].length;

    if (_txWriteIndex < 10)
    XEDBGLOG("Packet[%u] %p, segments: %u, seg[%u]: 0x%X (length 0x%X)",
      index, packet, segmentCount, i, OSSwapLittleToHostInt32(segments[i].location), OSSwapLittleToHostInt32(segments[i].length));

    packetLength += OSSwapLittleToHostInt32(desc->length);
  }

  _txDescStates[_txWriteIndex].packet = packet;
  _txDescStates[_txWriteIndex].segmentCount = segmentCount;

  descStart->packetLength = OSSwapHostToLittleInt32(packetLength);

    if (_txWriteIndex < 10) XEDBGLOG("Total packet length 0x%X", packetLength);

  if (_txWriteIndex == (kXenonEthernetTxDescCount - 1)) {
    desc->length |= OSSwapHostToLittleInt32(kXenonEthernetDescLengthEnd);
  }
  _txWriteIndex = TX_NEXT(index);

  // Hand off ownership and signal the hardware of the new packet.
  descStart->flags  = OSSwapHostToLittleInt32(flags);
  descStart->flags |= OSSwapHostToLittleInt32(kXenonEthernetTxFlagsOwner | kXenonEthernetTxFlagsInterrupt);
  writeReg32(kXenonEthernetRegTxControl, readReg32(kXenonEthernetRegTxControl) | kXenonEthernetRegTxControlPoll);

  return kIOReturnOutputSuccess;
}

void XenonEthernet::handleTxInterrupt(void) {
  UInt32                  segmentCount;

if (_txWriteIndex < 10)
  XEDBGLOG("Read index %u write index %u", _txReadIndex, _txWriteIndex);

  // Process any newly completed descriptors.
  while (_txReadIndex != _txWriteIndex) {
    if (_txWriteIndex < 10)
    XEDBGLOG("Sent packet %u stat 0x%X flags 0x%X length 0x%X", _txReadIndex,
      OSSwapLittleToHostInt32(_txDesc[_txReadIndex].status), OSSwapLittleToHostInt32(_txDesc[_txReadIndex].flags), OSSwapLittleToHostInt32(_txDesc[_txReadIndex].length));

    // Ensure descriptor is actually done.
    // Only the first one in a chain will be updated by the controller.
    if (_txDesc[_txReadIndex].flags & OSSwapHostToLittleInt32(kXenonEthernetTxFlagsOwner)) {
      break;
    }

    // Free the packet.
    if (_txDescStates[_txReadIndex].packet != NULL) {
      freePacket(_txDescStates[_txReadIndex].packet);
      _txDescStates[_txReadIndex].packet = NULL;
    }

    // Clear out the descriptors.
    segmentCount = _txDescStates[_txReadIndex].segmentCount;
    _txDescStates[_txReadIndex].segmentCount = 0;
    for (UInt32 i = 0; i < segmentCount; i++) {
      _txDesc[_txReadIndex].flags = 0;
      _txReadIndex = TX_NEXT(_txReadIndex);
    }
  }

  _txQueue->service(IOBasicOutputQueue::kServiceAsync);
}

//
// Handles the receive interrupt and processes any new packets.
//
void XenonEthernet::handleRxInterrupt(void) {
  XenonEthernetDescriptor *desc;
  UInt16  packetLength;
  mbuf_t  packet;
  mbuf_t  packetReceived;
  bool    replaced;

  while (true) {
    // Process any descriptors up until the first untouched one.
    desc = &_rxDesc[_rxIndex];
    if (desc->flags & OSSwapHostToLittleInt32(kXenonEthernetTxFlagsOwner)) {
      break;
    }

    // XEDBGLOG("Received packet %u stat 0x%X flags 0x%X length 0x%X", _rxIndex,
    //   OSSwapLittleToHostInt32(desc->status), OSSwapLittleToHostInt32(desc->flags), OSSwapLittleToHostInt32(desc->length));

    packet = _rxPackets[_rxIndex];
    packetLength = OSSwapLittleToHostInt32(desc->packetLength) & kXenonEthernetRxLengthMask;
    packetReceived = replaceOrCopyPacket(&packet, packetLength, &replaced);
    if (packetReceived == NULL) {
      XESYSLOG("Failed to get the packet?");
    }

    if (replaced) {
      _rxPackets[_rxIndex] = packet;
    }

    if (!setRxDescriptorPacket(desc, packet)) {
      freePacket(packet);
      packetReceived = NULL;
      XESYSLOG("Failed to set the packet?");
    }

    desc->flags = OSSwapHostToLittleInt32(kXenonEthernetTxFlagsInterrupt | kXenonEthernetTxFlagsOwner);

    _ethInterface->inputPacket(packetReceived, packetLength, IONetworkInterface::kInputOptionQueuePacket);
    _rxIndex = RX_NEXT(_rxIndex);
  }

  _ethInterface->flushInputQueue();
}
