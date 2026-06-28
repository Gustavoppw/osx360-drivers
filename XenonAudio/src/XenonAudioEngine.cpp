//
//  XenonAudioEngine.cpp
//  Xbox 360 audio engine
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonAudioDevice.hpp"
#include "XenonAudioEngine.hpp"
#include "XenonAudio_Regs.hpp"

OSDefineMetaClassAndStructors(XenonAudioEngine, super);

//
// Initializes the audio engine.
//
bool XenonAudioEngine::init(XenonAudioDevice *device, void *buffer, IOByteCount bufferLength, const char *description) {
  XenonCheckDebugArgs();

  if (!super::init(NULL)) {
    return false;
  }

  _audioDevice        = device;
  _sampleBuffer       = buffer;
  _sampleBufferLength = bufferLength;
  _deviceDescription  = description;
  _currentVolume      = kXenonMaxVolume;
  _currentMute        = 0;

  createVolumeLogTable();
  return createControls();
}

//
// Initializes the audio hardware.
// Overrides IOAudioEngine::initHardware().
//
bool XenonAudioEngine::initHardware(IOService *provider) {
  IOAudioStream       *audioStream;
  IOAudioSampleRate   sampleRate;
  IOAudioStreamFormat format = {
    kXenonAudioNumChannels,
    kIOAudioStreamSampleFormatLinearPCM,
    kIOAudioStreamNumericRepresentationSignedInt,
    kXenonAudioBitWidth,
    kXenonAudioBitWidth,
    kIOAudioStreamAlignmentLowByte,
    kIOAudioStreamByteOrderLittleEndian,
    true,
    0
  };

  XEDBGLOG("Initializing audio engine (buffer %p)", _sampleBuffer);

  if (!super::initHardware(provider)) {
    return false;
  }

  // Add description for 10.2 and older only.
  if (getKernelVersion() <= kKernelVersionJaguar) {
    setDescription(_deviceDescription);
  }

  sampleRate.whole    = 48000;
  sampleRate.fraction = 0;

  setSampleRate(&sampleRate);
  setNumSampleFramesPerBuffer(_sampleBufferLength / kXenonAudioBytesPerFrame);
  setSampleLatency(32);
  setSampleOffset(32);

  // Create the output stream. There is no input hardware.
  audioStream = new IOAudioStream;
  if (audioStream == NULL) {
    return false;
  }
  if (!audioStream->initWithAudioEngine(this, kIOAudioStreamDirectionOutput, 1)) {
    audioStream->release();
    return false;
  }

  audioStream->setSampleBuffer(_sampleBuffer, _sampleBufferLength);
  audioStream->addAvailableFormat(&format, &sampleRate, &sampleRate);
  audioStream->setFormat(&format);


  // Add the output stream.
  addAudioStream(audioStream);
  audioStream->release();

  return true;
}

//
// Gets the current frame being processed by the audio hardware.
// Overrides IOAudioEngine::getCurrentSampleFrame().
//
UInt32 XenonAudioEngine::getCurrentSampleFrame() {
 // return (_sampleBufferLength - _audioDevice->getAudioBytesLeft(this)) / kXenonAudioBytesPerFrame;
  return _audioDevice->getAudioPosition(this) / kXenonAudioBytesPerFrame;
}

//
// Starts the audio hardware.
// Overrides IOAudioEngine::performAudioEngineStart().
//
IOReturn XenonAudioEngine::performAudioEngineStart() {
  takeTimeStamp(false);
  return _audioDevice->startAudio(this);
}

//
// Stops the audio hardware.
// Overrides IOAudioEngine::performAudioEngineStop().
//
IOReturn XenonAudioEngine::performAudioEngineStop() {
  return _audioDevice->stopAudio(this);
}

//
// Handles volume changes.
//
IOReturn XenonAudioEngine::handleVolumeChange(IOAudioControl *audioControl, SInt32 oldValue, SInt32 newValue) {
  if (newValue > kXenonMaxVolume) {
    newValue = kXenonMaxVolume;
  }
  if (newValue < kXenonMinVolume) {
    newValue = kXenonMinVolume;
  }

  _currentVolume = newValue;
  XEDBGLOG("Volume changed to %d", _currentVolume);
  return kIOReturnSuccess;
}

//
// Handles mute changes.
//
IOReturn XenonAudioEngine::handleMuteChange(IOAudioControl *audioControl, SInt32 oldValue, SInt32 newValue) {
  _currentMute = newValue;
  XEDBGLOG("Mute changed to %d", _currentMute);
  return kIOReturnSuccess;
}

//
// Creates the volume logarithmic table for volume adjustments.
// Minimum is -60 dB and max is -25 dB.
//
void XenonAudioEngine::createVolumeLogTable(void) {
  _logTable[0] = 1.0E-6;
  _logTable[1] = 1.0843659E-6;
  _logTable[2] = 1.1758488E-6;
  _logTable[3] = 1.275049E-6;
  _logTable[4] = 1.3826173E-6;
  _logTable[5] = 1.499259E-6;
  _logTable[6] = 1.6257394E-6;
  _logTable[7] = 1.7628888E-6;
  _logTable[8] = 1.9116077E-6;
  _logTable[9] = 2.072872E-6;
  _logTable[10] = 2.2477402E-6;
  _logTable[11] = 2.4373607E-6;
  _logTable[12] = 2.642978E-6;
  _logTable[13] = 2.8659409E-6;
  _logTable[14] = 3.1077117E-6;
  _logTable[15] = 3.369866E-6;
  _logTable[16] = 3.654102E-6;
  _logTable[17] = 3.962251E-6;
  _logTable[18] = 4.296292E-6;
  _logTable[19] = 4.658387E-6;
  _logTable[20] = 5.050903E-6;
  _logTable[21] = 5.4764044E-6;
  _logTable[22] = 5.9376833E-6;
  _logTable[23] = 6.437752E-6;
  _logTable[24] = 6.979879E-6;
  _logTable[25] = 7.567605E-6;
  _logTable[26] = 8.204775E-6;
  _logTable[27] = 8.895555E-6;
  _logTable[28] = 9.644463E-6;
  _logTable[29] = 1.0456367E-5;
  _logTable[30] = 1.1336709E-5;
  _logTable[31] = 1.2291253E-5;
  _logTable[32] = 1.3326259E-5;
  _logTable[33] = 1.4448514E-5;
  _logTable[34] = 1.5665375E-5;
  _logTable[35] = 1.698482E-5;
  _logTable[36] = 1.8415497E-5;
  _logTable[37] = 1.9966785E-5;
  _logTable[38] = 2.1648853E-5;
  _logTable[39] = 2.3472722E-5;
  _logTable[40] = 2.5450342E-5;
  _logTable[41] = 2.7594678E-5;
  _logTable[42] = 2.991979E-5;
  _logTable[43] = 3.244092E-5;
  _logTable[44] = 3.517457E-5;
  _logTable[45] = 3.8138602E-5;
  _logTable[46] = 4.135245E-5;
  _logTable[47] = 4.4837264E-5;
  _logTable[48] = 4.861589E-5;
  _logTable[49] = 5.2713027E-5;
  _logTable[50] = 5.715533E-5;
  _logTable[51] = 6.19719E-5;
  _logTable[52] = 6.71943E-5;
  _logTable[53] = 7.285674E-5;
  _logTable[54] = 7.899633E-5;
  _logTable[55] = 8.5653315E-5;
  _logTable[56] = 9.2871355E-5;
  _logTable[57] = 1.00617925E-4;
  _logTable[58] = 1.0913212E-4;
  _logTable[59] = 1.18365356E-4;
  _logTable[60] = 1.2837846E-4;
  _logTable[61] = 1.3923738E-4;
  _logTable[62] = 1.5101368E-4;
  _logTable[63] = 1.6378497E-4;
  _logTable[64] = 1.7763532E-4;
  _logTable[65] = 1.9265598E-4;
  _logTable[66] = 2.089458E-4;
  _logTable[67] = 2.2661217E-4;
  _logTable[68] = 2.457713E-4;
  _logTable[69] = 2.665492E-4;
  _logTable[70] = 2.890826E-4;
  _logTable[71] = 3.135196E-4;
  _logTable[72] = 3.4002114E-4;
  _logTable[73] = 3.687621E-4;
  _logTable[74] = 3.9993165E-4;
  _logTable[75] = 4.337348E-4;
  _logTable[76] = 4.703941E-4;
  _logTable[77] = 5.1015057E-4;
  _logTable[78] = 5.532659E-4;
  _logTable[79] = 6.0002406E-4;
  _logTable[80] = 6.5073257E-4;
  _logTable[81] = 7.0572516E-4;
  _logTable[82] = 7.653629E-4;
  _logTable[83] = 8.300366E-4;
  _logTable[84] = 9.0017177E-4;
  _logTable[85] = 9.762301E-4;
  _logTable[86] = 0.0010587146;
  _logTable[87] = 0.0011481555;
  _logTable[88] = 0.0012451456;
  _logTable[89] = 0.0013503228;
  _logTable[90] = 0.0014643773;
  _logTable[91] = 0.0015880585;
  _logTable[92] = 0.001722179;
  _logTable[93] = 0.00186762;
  _logTable[94] = 0.002025336;
  _logTable[95] = 0.0021963636;
  _logTable[96] = 0.0023818277;
  _logTable[97] = 0.0025829482;
  _logTable[98] = 0.002801046;
  _logTable[99] = 0.0031622776;
}

//
// Creates controls for the audio engine.
//
bool XenonAudioEngine::createControls(void) {
  IOAudioControl *control;

  // Create volume control.
  control = IOAudioLevelControl::createVolumeControl(kXenonMaxVolume, kXenonMinVolume, kXenonMaxVolume, (-40 << 16) + (32768), 0,
                                                     kIOAudioControlChannelIDAll, kIOAudioControlChannelNameAll,
                                                     0, kIOAudioControlUsageOutput);
  if (control == NULL) {
    return false;
  }

  control->setValueChangeHandler(OSMemberFunctionCast(IOAudioControl::IntValueChangeHandler, this, &XenonAudioEngine::handleVolumeChange), this);
  addDefaultAudioControl(control);
  control->release();

  // Create mute control.
  control = IOAudioToggleControl::createMuteControl(false, kIOAudioControlChannelIDAll, kIOAudioControlChannelNameAll, 0, kIOAudioControlUsageOutput);
  if (control == NULL) {
    return false;
  }

  control->setValueChangeHandler(OSMemberFunctionCast(IOAudioControl::IntValueChangeHandler, this, &XenonAudioEngine::handleMuteChange), this);
  addDefaultAudioControl(control);
  control->release();

  return true;
}
