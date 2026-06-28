//
//  XenonAudioEngine.hpp
//  Xbox 360 audio engine
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonAudioEngine_hpp
#define XenonAudioEngine_hpp

#include <IOKit/audio/IOAudioDefines.h>
#include <IOKit/audio/IOAudioEngine.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioStream.h>
#include <IOKit/audio/IOAudioToggleControl.h>

#include "XenonCommon.hpp"

#define kXenonMinVolume   0
#define kXenonMaxVolume   99

class XenonAudioDevice;

//
// Represents an Xbox 360 audio engine.
//
class XenonAudioEngine : public IOAudioEngine {
  OSDeclareDefaultStructors(XenonAudioEngine);
  XenonDeclareLogFunctions("audeng");
  typedef IOAudioEngine super;

public:
  bool init(XenonAudioDevice *device, void *buffer, IOByteCount bufferLength, const char *description);

  // IOAudioEngine overrides.
  bool initHardware(IOService *provider);
  UInt32 getCurrentSampleFrame(void);
  IOReturn performAudioEngineStart(void);
  IOReturn performAudioEngineStop(void);
  IOReturn clipOutputSamples(const void *mixBuf, void *sampleBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames,
                             const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream);

private:
  XenonAudioDevice  *_audioDevice;
  void              *_sampleBuffer;
  IOByteCount       _sampleBufferLength;
  const char        *_deviceDescription;

  // Volume adjustments.
  SInt32  _currentVolume;
  SInt32  _currentMute;
  float   _logTable[kXenonMaxVolume + 1];

  // Internal functions.
  IOReturn handleVolumeChange(IOAudioControl *audioControl, SInt32 oldValue, SInt32 newValue);
  IOReturn handleMuteChange(IOAudioControl *audioControl, SInt32 oldValue, SInt32 newValue);
  void createVolumeLogTable(void);
  bool createControls(void);
};

#endif
