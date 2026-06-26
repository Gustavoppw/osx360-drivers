//
//  XenonEthernet_regs.hpp
//  Xbox 360 Ethernet controller registers
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonEthernetRegs_hpp
#define XenonEthernetRegs_hpp

#include "XenonCommon.hpp"

//
// Strings.
//
#define kXenonEthernetVendorStr           "Microsoft"
#define kXenonEthernetModelStr            "Xbox 360 Ethernet"

#define kXenonEthernetMaxPacketLength     (kIOEthernetMaxPacketSize + 10)
#define kXenonEthernetTxQueueLength       1000

//
// MMIO registers.
//
// Transmit control.
#define kXenonEthernetRegTxControl        0x00
#define kXenonEthernetRegTxControlEnable  BIT0
#define kXenonEthernetRegTxControlPoll    BIT4
// Transmit descriptors physical address.
#define kXenonEthernetRegTxDescAddr       0x04
#define kXenonEthernetRegTxNext           0x0C

// Receive control.
#define kXenonEthernetRegRxControl        0x10
#define kXenonEthernetRegRxControlEnable  BIT0
#define kXenonEthernetRegRxControlPoll    BIT4
// Receive descriptors physical address.
#define kXenonEthernetRegRxDescAddr       0x14
#define kXenonEthernetRegRxNext           0x1C

// Interrupt status.
#define kXenonEthernetRegIntStatus        0x20
// Interrupt mask.
#define kXenonEthernetRegIntMask          0x24
// Interrupt control.
#define kXenonEthernetRegIntControl       0x28
// Interrupt timer.
#define kXenonEthernetRegIntTimer         0x2C

#define kXenonEthernetRegPmControl        0x30
#define kXenonEthernetRegRomControl       0x38
#define kXenonEthernetRegRomInterface     0x3C
#define kXenonEthernetRegStationControl   0x40

// PHY read/write.
#define kXenonEthernetRegPhy              0x44
#define kXenonEthernetRegPhyRequest       BIT4
#define kXenonEthernetRegPhyOpRead        0
#define kXenonEthernetRegPhyOpWrite       BIT5
#define kXenonEthernetRegPhyAddrShift     6
#define kXenonEthernetRegPhyAddrMask      BITRange(6, 10)
#define kXenonEthernetRegPhyRegShift      11
#define kXenonEthernetRegPhyRegMask       BITRange(11, 15)
#define kXenonEthernetRegPhyDataShift     16
#define kXenonEthernetRegPhyDataMask      BITRange(16, 31)

#define kXenonEthernetRegGMacIoCr         0x48
#define kXenonEthernetRegGMacIoControl    0x4C
// Transmit MAC control.
#define kXenonEthernetRegTxMacControl     0x50
#define kXenonEthernetRegTxMacTimeLimit   0x54
#define kXenonEthernetRegRGMiiDelay       0x58

// Receive MAC control.
#define kXenonEthernetRegRxMacControl           0x60
#define kXenonEthernetRegRxMacControlChecksum         BIT2
#define kXenonEthernetRegRxMacControlPad              BIT3
#define kXenonEthernetRegRxMacControlStripFcs         BIT4
#define kXenonEthernetRegRxMacControlStripVlan        BIT5
#define kXenonEthernetRegRxMacControlAcceptAllPhys    BIT8
#define kXenonEthernetRegRxMacControlAcceptSelfPhys   BIT9
#define kXenonEthernetRegRxMacControlAcceptMulticast  BIT10
#define kXenonEthernetRegRxMacControlAcceptBroadcast  BIT11
// MAC address.
#define kXenonEthernetRegRxMacAddress           0x62

#define kXenonEthernetRegRxHashTable1     0x68
#define kXenonEthernetRegRxHashTable2     0x6C
#define kXenonEthernetRegRxWakeOnLan      0x70
#define kXenonEthernetRegRxWakeOnLanData  0x74
#define kXenonEthernetRegRxMpsControl     0x78

#define kXenonEthernetRegRxMacAddress2    0x7A

//
// Interrupt bits.
//
#define kXenonEthernetIntTxHalt           BIT0
#define kXenonEthernetIntRxHalt           BIT1
#define kXenonEthernetIntTxDone           BIT2
#define kXenonEthernetIntTxIdle           BIT3
#define kXenonEthernetIntTxQ1Done         BIT4
#define kXenonEthernetIntTxQ1Idle         BIT5
#define kXenonEthernetIntRxDone           BIT6
#define kXenonEthernetIntRxIdle           BIT7
#define kXenonEthernetIntReset            BIT15
#define kXenonEthernetIntLink             BIT16
#define kXenonEthernetIntWakeFrame        BIT17
#define kXenonEthernetIntMagicFrame       BIT18
#define kXenonEthernetIntPauseFrame       BIT19
#define kXenonEthernetIntTimer            BIT29
#define kXenonEthernetIntSoft             BIT30

//
// PHY registers.
//
#define kXenonEthernetPhyTimeoutUS    100000
#define kXenonEthernetPhyMaxAddr      31

// Basic Mode Control.
#define kXenonEthernetPhyRegControl                 0x00
#define kXenonEthernetPhyRegControlCollisionTest    BIT7
#define kXenonEthernetPhyRegControlFullDuplex       BIT8
#define kXenonEthernetPhyRegControlAutoNegRestart   BIT9
#define kXenonEthernetPhyRegControlIsolate          BIT10
#define kXenonEthernetPhyRegControlPowerDown        BIT11
#define kXenonEthernetPhyRegControlAutoNegEnable    BIT12
#define kXenonEthernetPhyRegControl100Mbps          BIT13
#define kXenonEthernetPhyRegControlLoopback         BIT14
#define kXenonEthernetPhyRegControlReset            BIT15
// Basic Mode Status.
#define kXenonEthernetPhyRegStatus                  0x01
#define kXenonEthernetPhyRegStatusExtended          BIT0
#define kXenonEthernetPhyRegStatusLink              BIT2
#define kXenonEthernetPhyRegStatusAutoNegCapable    BIT3
#define kXenonEthernetPhyRegStatusAutoNegComplete   BIT5
#define kXenonEthernetPhyRegStatusSupp100Half2      BIT9
#define kXenonEthernetPhyRegStatusSupp100Full2      BIT10
#define kXenonEthernetPhyRegStatusSupp10Half        BIT11
#define kXenonEthernetPhyRegStatusSupp10Full        BIT12
#define kXenonEthernetPhyRegStatusSupp100Half       BIT13
#define kXenonEthernetPhyRegStatusSupp100Full       BIT14
#define kXenonEthernetPhyRegStatusSupp100Base4      BIT15
// PHY IDs
#define kXenonEthernetPhyRegId1                     0x02
#define kXenonEthernetPhyRegId2                     0x03
#define kXenonEthernetPhyRegAdvertise               0x04
#define kXenonEthernetPhyRegLpa                     0x05
#define kXenonEthernetPhyRegExpansion               0x06


//
// Transmit/receive.
//
// Descriptor. All fields are little endian.
typedef struct {
  union {
    // Total length of packet, set only on the first descriptor.
    UInt32  packetLength;
    // RX status.
    UInt32  status;
  };
  // Flags.
  UInt32  flags;
  // Physical address of segment.
  UInt32  address;
  // Length of segment. High bit signals this is the final descriptor.
  UInt32  length;
} XenonEthernetDescriptor;

#define kXenonEthernetTxDescCount     256
#define kXenonEthernetRxDescCount     256
#define kXenonEthernetTxBufferLength  (kXenonEthernetTxDescCount * sizeof (XenonEthernetDescriptor))
#define kXenonEthernetRxBufferLength  (kXenonEthernetRxDescCount * sizeof (XenonEthernetDescriptor))
#define kXenonEthernetTxMaxSegCount   8
#define kXenonEthernetRxMaxSegCount   1

// Set to indicate descriptor is the last.
#define kXenonEthernetDescLengthEnd           BIT31

// Transmit descriptor flags.
// VLAN tagging.
#define kXenonEthernetTxFlagsVlanMask         BITRange(0, 15)
// Pad any runts.
#define kXenonEthernetTxFlagsPad              BIT16
// Append CRC to end of packet.
#define kXenonEthernetTxFlagsCrc              BIT17
#define kXenonEthernetTxFlagsCollision        BIT18
#define kXenonEthernetTxFlagsCarrierSense     BIT19
#define kXenonEthernetTxFlagsBackoff          BIT20
#define kXenonEthernetTxFlagsDeferred         BIT21
// Extended descriptor.
#define kXenonEthernetTxFlagsExtended         BIT22
// Specifies burst-mode should be enabled.
#define kXenonEthernetTxFlagsBurst            BIT23
// Specifies UDP checksum calculation should be performed.
#define kXenonEthernetTxFlagsUdpChecksum      BIT24
// Specifies TCP checksum calculation should be performed.
#define kXenonEthernetTxFlagsTcpChecksum      BIT25
// Specifies IP checksum calculation should be performed.
#define kXenonEthernetTxFlagsIpChecksum       BIT26
// Descriptor contains the last segment of a packet.
#define kXenonEthernetTxFlagsLastSeg          BIT27
// Fire the respective interrupt after the descriptor is processed.
#define kXenonEthernetTxFlagsInterrupt        BIT30
// If set, the hardware owns the descriptor and the driver must not change it.
#define kXenonEthernetTxFlagsOwner            BIT31

// Receive descriptor flags.
#define kXenonEthernetRxLengthMask            BITRange(0, 15)

// Fire the respective interrupt after the descriptor is processed.
#define kXenonEthernetRxFlagsInterrupt        BIT30
// If set, the hardware owns the descriptor and the driver must not change it.
#define kXenonEthernetRxFlagsOwner            BIT31

#endif
