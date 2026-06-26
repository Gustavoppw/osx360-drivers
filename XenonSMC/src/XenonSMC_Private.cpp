//
//  XenonSMC_Private.cpp
//  Xbox 360 system management controller driver
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonSMC.hpp"

//
// Interrupt filter handler.
// This function runs in the primary interrupt context.
//
bool XenonSMC::filterInterrupt(IOFilterInterruptEventSource *intEventSource) {
  XenonSMCMessage message;
  bool            needSecondary;

  //
  // Handle power button press here, need to notify SMC within a short
  // window otherwise the console will power off.
  //
  readMessage(&message);

  needSecondary = false;
  switch (message.command) {
    // Async events.
    case kXenonSMCCommandAsync:
      switch (message.data[0]) {
        case kXenonSMCAsyncPowerButton:
          cancelPowerOff();
          _powerButtonPressed = true;
          needSecondary = true;
          break;

        case kXenonSMCAsyncSyncButton:
          _syncButtonPressed = true;
          needSecondary = true;
          break;

        case kXenonSMCAsyncTiltSwitch:
          _tiltState = message.tiltStatus != 0;
          _tiltStatePresent = true;
          needSecondary = true;
          break;

        default:
          break;
      }
      break;

    // Get RTC response.
    case kXenonSMCCommandGetRTC:
      _rtcValue = message.data[0] | ((UInt64)message.data[1] << 8) | ((UInt64)message.data[2] << 16)
        | ((UInt64)message.data[3] << 24) | ((UInt64)message.data[4] << 32);
      _rtcValuePresent = true;
      if (_rtcSyncer != NULL) {
        _rtcSyncer->signal();
      }
      break;

    // Get temperature response.
    case kXenonSMCCommandGetTemp:
      memcpy(&_tempStatus, &message.temp, sizeof(_tempStatus));
      _tempStatusPresent = true;
      break;

    default:
      break;
  }

  writeLittleReg32(kXenonSMCRegIntAck, readLittleReg32(kXenonSMCRegIntStatus));

  // Defer to secondary handler.
  if (needSecondary) {
    _intEventSource->signalInterrupt();
  }
  return false;
}

//
// Interrupt handler.
// This function runs in the secondary workloop context.
//
void XenonSMC::handleInterrupt(IOInterruptEventSource *intEventSource, int count) {
  if (_powerButtonPressed) {
    XEDBGLOG("Power button pressed");
    _powerButtonPressed = false;
      rebootPowerOff(true);
  }

  if (_syncButtonPressed) {
    XEDBGLOG("Sync button pressed");
    _syncButtonPressed = false;
  }

  if (_tiltStatePresent) {
    XEDBGLOG("New tilt state: %u", _tiltState);
    _tiltStatePresent = false;
  }
}

//
// Writes a message to the SMC's FIFO buffer.
//
IOReturn XenonSMC::writeMessage(XenonSMCMessage *message) {
  UInt32 timeout = kXenonSMCTimeout;
  IOInterruptState  ints;

  ints = IOSimpleLockLockDisableInterrupt(_lock);

  // Wait for SMC to be ready.
  while ((readLittleReg32(kXenonSMCRegFifoOutStatus) & kXenonSMCRegFifoOutStatusReady) == 0) {
    if (timeout == 0) {
      IOSimpleLockUnlockEnableInterrupt(_lock, ints);
      XEDBGLOG("Timed out waiting for SMC");
      return kIOReturnTimeout;
    }
    timeout--;
    IODelay(1);
  }

  // Signal SMC and write it to the FIFO.
  writeLittleReg32(kXenonSMCRegFifoOutStatus, kXenonSMCRegFifoOutStatusReady);
  for (UInt32 i = 0; i < ARRSIZE(message->bytes32); i++) {
    writeBigReg32(kXenonSMCRegFifoOutData, message->bytes32[i]);
  }
  writeLittleReg32(kXenonSMCRegFifoOutStatus, 0);

  IOSimpleLockUnlockEnableInterrupt(_lock, ints);
  return kIOReturnSuccess;
}

//
// Reads a message from the SMC's FIFO buffer.
//
IOReturn XenonSMC::readMessage(XenonSMCMessage *message) {
  IOInterruptState  ints;

  ints = IOSimpleLockLockDisableInterrupt(_lock);

  // Check if there is a message.
  if ((readLittleReg32(kXenonSMCRegFifoInStatus) & kXenonSMCRegFifoInStatusReady) == 0) {
    IOSimpleLockUnlockEnableInterrupt(_lock, ints);
    return kIOReturnNotReady;
  }

  // Signal SMC and read it from the FIFO.
  writeLittleReg32(kXenonSMCRegFifoInStatus, kXenonSMCRegFifoInStatusReady);
  for (UInt32 i = 0; i < ARRSIZE(message->bytes32); i++) {
    message->bytes32[i] = readBigReg32(kXenonSMCRegFifoInData);
  }
  writeLittleReg32(kXenonSMCRegFifoInStatus, 0);

  IOSimpleLockUnlockEnableInterrupt(_lock, ints);
  return kIOReturnSuccess;
}

//
// Gets the RTC Value.
//
IOReturn XenonSMC::getRTC(UInt64 *value) {
  XenonSMCMessage message;
  IOSyncer        *syncer;
  IOReturn        status;

  bzero(&message, sizeof(message));
  message.command = kXenonSMCCommandGetRTC;

  syncer = IOSyncer::create();
  if (syncer == NULL) {
    return kIOReturnNoResources;
  }

  IOLockLock(_rtcLock);

  _rtcSyncer = syncer;
  status = writeMessage(&message);
  if (status != kIOReturnSuccess) {
    _rtcSyncer = NULL;
    IOLockUnlock(_rtcLock);

    OSSafeReleaseNULL(syncer);
    return status;
  }

  _rtcSyncer->wait();
  status = _rtcValuePresent ? kIOReturnSuccess : kIOReturnIOError;
  if (_rtcValuePresent) {
    *value = _rtcValue;
    _rtcValuePresent = false;
    XEDBGLOG("Current RTC: %llu", *value);
  }

  IOLockUnlock(_rtcLock);

  return status;
}

//
// Sets the LED states.
//
IOReturn XenonSMC::setLEDState(bool override, UInt8 value) {
  XenonSMCMessage message;

  bzero(&message, sizeof(message));
  message.command = kXenonSMCCommandSetRingLED;
  message.data[0] = override ? 0x1 : 0x0;
  message.data[1] = value;

  return writeMessage(&message);
}

//
// Starts the ring LED boot animation.
//
IOReturn XenonSMC::startLEDBootAnimation(void) {
  XenonSMCMessage message;
  IOReturn        status;

  // Disable any ring LED overrides.
  status = setLEDState(false, 0);
  if (status != kIOReturnSuccess) {
    return status;
  }

  // Disable any power LED overrides.
  bzero(&message, sizeof(message));
  message.command = kXenonSMCCommandSetPowerLED;
  message.data[0] = 0x01;
  status = writeMessage(&message);
  if (status != kIOReturnSuccess) {
    return status;
  }

  // Start animation.
  message.data[1] = 0x01;
  return writeMessage(&message);
}

//
// Reboots or powers off the system.
//
IOReturn XenonSMC::rebootPowerOff(bool reboot) {
  XenonSMCMessage message;

  bzero(&message, sizeof(message));
  message.command = kXenonSMCCommandSetPower;
  message.power.type = reboot ? kXenonSMCPowerReboot : kXenonSMCPowerPowerOff;
  message.power.special = reboot ? kXenonSMCPowerRebootSoft : 0;

  XEDBGLOG("Reboot %u", reboot);
  return writeMessage(&message);
}

//
// Cancels the pending power off.
//
IOReturn XenonSMC::cancelPowerOff(void) {
  XenonSMCMessage message;

  bzero(&message, sizeof(message));
  message.command = kXenonSMCCommandSetPower;
  message.power.type = kXenonSMCPowerReboot;
  message.power.special = kXenonSMCPowerRebootCancel;

  return writeMessage(&message);
}

//
// Ejects the disc tray.
//
IOReturn XenonSMC::ejectTray(void) {
  XenonSMCMessage message;

  bzero(&message, sizeof(message));
  message.command = kXenonSMCCommandOpenCloseTray;
  message.data[0] = kXenonSMCTrayStatusOpen;

  return writeMessage(&message);
}
