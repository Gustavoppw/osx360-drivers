//
//  XenonAudioDevice.cpp
//  Xbox 360 audio device
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonAudioDevice.hpp"
#include "XenonAudioEngine.hpp"
#include "XenonAudio_Regs.hpp"

OSDefineMetaClassAndStructors(XenonAudioDevice, super);

extern "C" vm_offset_t ml_io_map(vm_offset_t phys_addr, vm_size_t size);

//
// Performs driver startup.
// Overrides IOService::start().
//
bool XenonAudioDevice::start(IOService *provider) {
  XenonCheckDebugArgs();

  return super::start(provider);
}

//
// Releases resources used by the driver.
// Overrides IOService::free().
//
void XenonAudioDevice::free(void) {
  OSSafeReleaseNULL(_descBufferDesc);
  OSSafeReleaseNULL(_sampleBufferDesc);
  OSSafeReleaseNULL(_mmioMap);
  OSSafeReleaseNULL(_pciParent);

  super::free();
}

//
// Initializes the audio hardware. Called during super::start().
// Overrides IOAudioDevice::initHardware().
//
bool XenonAudioDevice::initHardware(IOService *provider) {
  XEDBGLOG("Starting Xenon audio controller");

  if (!super::initHardware(provider)) {
    XESYSLOG("super::initHardware() returned false");
    return false;
  }

  _pciParent = OSDynamicCast(IOPCIDevice, provider);
  if (_pciParent == NULL) {
    XESYSLOG("Provider is not IOPCIDevice");
    return false;
  }
  _pciParent->retain();

  // Ensure PCI device is ready.
  _pciParent->setBusMasterEnable(true);
  _pciParent->setMemoryEnable(true);
  _pciParent->setIOEnable(false);

  // Get device properties in order.
  setDeviceName("Built-in Audio");
  setDeviceShortName("Built-in");
  setManufacturerName("Microsoft");
  setProperty(kIOAudioDeviceTransportTypeKey, kIOAudioDeviceTransportTypeBuiltIn, 32);

  // Map in MMIO registers.
  _mmioMap = _pciParent->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
  if (_mmioMap == NULL) {
    XESYSLOG("Failed to map audio registers");
    return false;
  }

  _mmioMem = (volatile UInt8*) _mmioMap->getVirtualAddress();
  XEDBGLOG("Mapped audio registers at 0x%X length 0x%X to %p", _mmioMap->getPhysicalAddress(),
    _mmioMap->getLength(), _mmioMem);

  if (!allocateSampleDescriptors()) {
    XESYSLOG("Failed to allocate sample descriptors");
    return false;
  }
  setupSampleDescriptors();

  _timerEventSource = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &XenonAudioDevice::handleTimer));
  if ((_timerEventSource == NULL) || (workLoop->addEventSource(_timerEventSource) != kIOReturnSuccess)) {
    XESYSLOG("Failed to create timer");
    return false;
  }

  // Create audio engines for outputs.
  _audioOutputEngine = createAudioEngine(_sampleBuffer, kXenonSampleBufferLength, "Xbox 360 A/V");
  if (_audioOutputEngine == NULL) {
    XESYSLOG("Failed to create audio engine");
    return false;
  }
  if (!createAudioPorts(_audioOutputEngine, kIOAudioOutputPortSubTypeExternalSpeaker, "Xbox 360 A/V")) {
    XESYSLOG("Failed to create audio ports");
    return false;
  }

  // Unmute audio. Xell should have done this but make sure.
  setAudioMute(false);

  // Activate the engines.
  activateAudioEngine(_audioOutputEngine);

  XEDBGLOG("Started Xenon audio controller");
  return true;
}

//
// Starts audio playback on the specified engine.
//
IOReturn XenonAudioDevice::startAudio(XenonAudioEngine *audioEngine) {
  UInt32 reg;
  if (audioEngine == _audioOutputEngine) {
    reg = kXenonAudioRegControl;
  } else {
    return false;
  }
  XEDBGLOG("Start register 0x%X", reg);

  writeReg32(reg, readReg32(reg) | kXenonAudioRegControlRun);
  _timerEventSource->enable();
  _timerEventSource->setTimeoutUS(kXenonAudioTimerUpdateUS);

  return kIOReturnSuccess;
}

//
// Stops audio playback on the specified engine.
//
IOReturn XenonAudioDevice::stopAudio(XenonAudioEngine *audioEngine) {
  UInt32 reg;
  if (audioEngine == _audioOutputEngine) {
    reg = kXenonAudioRegControl;
  } else {
    return kIOReturnUnsupported;
  }
  XEDBGLOG("Stop register 0x%X", reg);

  writeReg32(reg, readReg32(reg) & ~(kXenonAudioRegControlRun));
  _timerEventSource->disable();
  _timerEventSource->cancelTimeout();

  return kIOReturnSuccess;
}

//
// Gets the current audio playback position.
//
UInt32 XenonAudioDevice::getAudioPosition(XenonAudioEngine *audioEngine) {
  UInt32  reg;
  UInt32  state;
  UInt32  readPtr;
  UInt16  length;
  UInt32  position;

  if (audioEngine == _audioOutputEngine) {
    reg = kXenonAudioRegControl;
  } else {
    return kIOReturnUnsupported;
  }

  state    = readReg32(reg);
  readPtr  = state & kXenonAudioRegStateReadPtrMask;
  length   = (UInt16)((state >> kXenonAudioRegStateLengthShift) & 0xFFFF);
  position = (readPtr * (kXenonSampleBufferLength / kXenonAudioDescCount)) + ((kXenonSampleBufferLength / kXenonAudioDescCount) - length);

  return position;
}

//
// Timer handler.
// TODO: In the future the interrupt should be used for this, but right now it just creates a storm.
//
void XenonAudioDevice::handleTimer(IOTimerEventSource *sender) {
  // Get new timestamp if the buffer has looped around.
  UInt32 readPtr = readReg32(kXenonAudioRegState) & kXenonAudioRegStateReadPtrMask;
  if (readPtr == 0 && _lastReadPtr != 0) {
    _audioOutputEngine->takeTimeStamp();
  }
  _lastReadPtr = readPtr;

  // Keep the buffer moving.
  readPtr += 10;
  readPtr &= kXenonAudioDescMask;
  writeReg32(kXenonAudioRegState, readPtr << kXenonAudioRegStateWritePtrShift);

  _timerEventSource->setTimeoutUS(kXenonAudioTimerUpdateUS);
}

//
// Mutes or unmutes the console audio.
//
bool XenonAudioDevice::setAudioMute(bool mute) {
  IOService       *xenonSMC;
  mach_timespec_t t;

  t.tv_sec = 30;
  t.tv_nsec = 0;

  xenonSMC = waitForService(serviceMatching("XenonSMC"), &t);
  if (xenonSMC != NULL) {
    return xenonSMC->callPlatformFunction(kXenonFuncSMCMuteAudio, false, (void*)false, NULL, NULL, NULL) == kIOReturnSuccess;
  } else {
    XESYSLOG("SMC services not available");
  }

  return false;
}

//
// Allocates the sample descriptors and sample buffer.
//
bool XenonAudioDevice::allocateSampleDescriptors(void) {
  IOByteCount length;

  // Allocate descriptor buffer. This assumes the buffer is within the first 512MB of memory.
  _descBufferDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, kXenonAudioDescBufferLength, PAGE_SIZE);
  if (_descBufferDesc == NULL) {
    XESYSLOG("Failed to allocate descriptor buffer");
    return false;
  }
  _descBuffer = (UInt32*) _descBufferDesc->getBytesNoCopy();
  _descBufferPhysAddr = _descBufferDesc->getPhysicalSegment(0, &length);

  XEDBGLOG("Allocated descriptor buffer at 0x%X length 0x%X to %p", _descBufferPhysAddr,
    _descBufferDesc->getLength(), _descBuffer);

  // Allocate sample buffer. This assumes the buffer is within the first 512MB of memory.
  _sampleBufferDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, kXenonSampleBufferLength, PAGE_SIZE);
  if (_sampleBufferDesc == NULL) {
    XESYSLOG("Failed to allocate sample buffer");
    return false;
  }

  // Map sample buffer as I/O, audio hardware doesn't seem to be cache coherent.
  // TODO: Is there a better way to do this? Attempting to use IOSetProcessorCache like others doesn't seem to work.
  _sampleBufferPhysAddr = _sampleBufferDesc->getPhysicalSegment(0, &length);
  _sampleBuffer = (UInt8*) ml_io_map(_sampleBufferPhysAddr, kXenonSampleBufferLength);

  XEDBGLOG("Allocated sample buffer at 0x%X length 0x%X to %p", _sampleBufferPhysAddr,
    _sampleBufferDesc->getLength(), _sampleBuffer);

  return true;
}

//
// Fills and configure the sample descriptors.
//
void XenonAudioDevice::setupSampleDescriptors(void) {
  bzero(_sampleBuffer, kXenonSampleBufferLength);

  for (UInt32 i = 0; i < kXenonAudioDescCount; i++) {
    _descBuffer[i * 2]       = OSSwapHostToLittleInt32(_sampleBufferPhysAddr + ((kXenonSampleBufferLength / kXenonAudioDescCount) * i));
    _descBuffer[(i * 2) + 1] = OSSwapHostToLittleInt32(0x80000000 | (kXenonSampleBufferLength / kXenonAudioDescCount));
  }
  flush_dcache((vm_offset_t) _descBuffer, kXenonAudioDescBufferLength, 0);

  writeReg32(kXenonAudioRegControl, 0);
  writeReg32(kXenonAudioRegControl, 0x2000000);
  writeReg32(kXenonAudioRegDescAddr, _descBufferPhysAddr);
  //writeReg32(kXenonAudioRegControl, 0x1c08001c);
  writeReg32(kXenonAudioRegControl, 0x1c00001c);
  writeReg32(kXenonAudioRegFormat, 0x1c);
}

//
// Creates an audio engine.
//
XenonAudioEngine *XenonAudioDevice::createAudioEngine(void *buffer, IOByteCount bufferLength, const char *description) {
  XenonAudioEngine  *audioEngine;
  IOAudioControl    *control;

  // Create a new audio engine with the buffer.
  audioEngine = new XenonAudioEngine;
  if (audioEngine == NULL) {
    return NULL;
  }

  if (!audioEngine->init(this, buffer, bufferLength, description)) {
    OSSafeReleaseNULL(audioEngine);
    return NULL;
  }

  return audioEngine;
}

//
// Creates audio ports for an audio engine.
//
bool XenonAudioDevice::createAudioPorts(XenonAudioEngine *audioEngine, SInt32 type, const char *name) {
  IOAudioPort             *outputPort;
  IOAudioSelectorControl  *outputSelector;
  IOReturn                status;

  outputPort = IOAudioPort::withAttributes(kIOAudioPortTypeOutput, "Output port");
  if (outputPort == NULL) {
    return false;
  }

  outputSelector = IOAudioSelectorControl::createOutputSelector(type, kIOAudioControlChannelIDAll);
  if (outputSelector == NULL) {
    outputPort->release();
    return false;
  }

  // Add selector for nice name in System Preferences.
  audioEngine->addDefaultAudioControl(outputSelector);
  outputSelector->addAvailableSelection(type, name);
  outputSelector->release();

  // Add the port to the engine.
  status = attachAudioPort(outputPort, audioEngine, NULL);
  outputPort->release();

  return status == kIOReturnSuccess;
}
