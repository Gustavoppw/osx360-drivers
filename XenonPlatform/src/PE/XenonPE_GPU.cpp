//
//  XenonPE_GPU.cpp
//  Xbox 360 platform expert
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenonPE.hpp"
#include "XenosRegs.hpp"

//
// Handles the framebuffer timer refresh.
//
void XenonPE::handleFramebufferTimer(IOTimerEventSource *sender) {
  refreshFramebuffer();
  _gpuTimerEventSource->setTimeoutMS(500);
}

//
// Prepares any existing framebuffer image for new display.
//
void XenonPE::prepGraphicsFramebuffer(void) {
  PE_Video videoInfo;
  volatile UInt32 *srcFB = (volatile UInt32*)_gpuMem;
  volatile UInt32 *destFB = (volatile UInt32*)_fbMem;

  getConsoleInfo(&videoInfo);

  // Only handle graphics mode.
  if (PE_state.video.v_display) {
    memcpy((void*)_gpuMem, (void*)_fbMem, 0x1000000);

    //
    // Convert each pixel from GPU format to ARGB.
    // Calculation pulled from libxenon.
    //
    for (unsigned long y = 0; y < videoInfo.v_height; y++) {
      for (unsigned long x = 0; x < videoInfo.v_width; x++) {
        UInt32 index = (((y >> 5) * 32 * videoInfo.v_width
          + ((x >> 5) << 10) + (x & 3) + ((y & 1) << 2) + (((x & 31) >> 2) << 3) + (((y & 31) >> 1) << 6)) ^ ((y & 8) << 2));
        destFB[x + (y * (videoInfo.v_rowBytes / sizeof (*srcFB)))] = OSSwapLittleToHostInt32(srcFB[index]);
      }
    }
  }

  refreshFramebuffer();
}

//
// Refreshes the framebuffer image.
//
void XenonPE::refreshFramebuffer(void) {
  PE_Video videoInfo;
  volatile UInt32 *srcFB = (volatile UInt32*)_fbMem;
  volatile UInt32 *destFB = (volatile UInt32*)_gpuMem;

  getConsoleInfo(&videoInfo);

  boolean_t ints = ml_set_interrupts_enabled(false);

  //
  // Convert each pixel over.
  // Calculation pulled from libxenon.
  //
  for (unsigned long y = 0; y < videoInfo.v_height; y++) {
    for (unsigned long x = 0; x < videoInfo.v_width; x++) {
      UInt32 index = (((y >> 5) * 32 * videoInfo.v_width
        + ((x >> 5) << 10) + (x & 3) + ((y & 1) << 2) + (((x & 31) >> 2) << 3) + (((y & 31) >> 1) << 6)) ^ ((y & 8) << 2));
      destFB[index] = OSSwapHostToLittleInt32(srcFB[x + (y * (videoInfo.v_rowBytes / sizeof (*srcFB)))]);
    }
  }

  ml_set_interrupts_enabled(ints);
}

//
// Prepares the GPU and simple copy structures.
//
bool XenonPE::mapGPU(void) {
  _gpuMmioDeviceMemory = IODeviceMemory::withRange(kXenosMmioAddress, kXenosMmioLength);
  if (_gpuMmioDeviceMemory == NULL) {
    XESYSLOG("Failed to get display adapter registers");
    return false;
  }

  _gpuMmioMap = _gpuMmioDeviceMemory->map();
  if (_gpuMmioMap == NULL) {
    XESYSLOG("Failed to map display adapter registers");
    return false;
  }
  _gpuMmioMem = (volatile void*) _gpuMmioMap->getVirtualAddress();

  _fbPhysAddr = OSReadBigInt32(_gpuMmioMem, kXenosRegD1GrphPriSurfaceAddr);
  _fbDeviceMemory = IODeviceMemory::withRange(_fbPhysAddr, kXenosFramebufferLength);
  if (_fbDeviceMemory == NULL) {
    XESYSLOG("Failed to get framebuffer memory");
    return false;
  }

  _fbMap = _fbDeviceMemory->map();
  if (_fbMap == NULL) {
    XESYSLOG("Failed to map framebuffer memory");
    return false;
  }
  _fbMem = (volatile void*) _fbMap->getVirtualAddress();

  _gpuPhysAddr = _fbPhysAddr + kXenosFramebufferLength;
  _gpuDeviceMemory = IODeviceMemory::withRange(_gpuPhysAddr, kXenosFramebufferLength);
  if (_gpuDeviceMemory == NULL) {
    XESYSLOG("Failed to get GPU memory");
    return false;
  }

  _gpuMap = _gpuDeviceMemory->map();
  if (_gpuMap == NULL) {
    XESYSLOG("Failed to map GPU memory");
    return false;
  }
  _gpuMem = (volatile void*) _gpuMap->getVirtualAddress();

  XEDBGLOG("Framebuffer at %p (%ux%ux%u) phys 0%X", _fbMem, PE_state.video.v_width,
    PE_state.video.v_height, PE_state.video.v_depth, _fbPhysAddr);

  _gpuTimerWorkLoop = IOWorkLoop::workLoop();
  if (_gpuTimerWorkLoop == NULL) {
    XESYSLOG("Failed to initialize GPU timer workloop");
    return false;
  }

  _gpuTimerEventSource = IOTimerEventSource::timerEventSource(this,
    OSMemberFunctionCast(IOTimerEventSource::Action, this, &XenonPE::handleFramebufferTimer));
  if (_gpuTimerEventSource == NULL) {
    XESYSLOG("Failed to initialize GPU timer");
    return false;
  }
  _gpuTimerWorkLoop->addEventSource(_gpuTimerEventSource);
  _gpuTimerEventSource->enable();

  prepGraphicsFramebuffer();

  // Switch GPU surface.
  OSWriteBigInt32(_gpuMmioMem, 0x6144, 1);
  OSWriteBigInt32(_gpuMmioMem, kXenosRegD1GrphPriSurfaceAddr, _gpuPhysAddr);
  OSWriteBigInt32(_gpuMmioMem, 0x6144, 0);

  OSWriteBigInt32(_gpuMmioMem, 0x65cc, 1);
  OSWriteBigInt32(_gpuMmioMem, 0x2840, _gpuPhysAddr);
  OSWriteBigInt32(_gpuMmioMem, 0x65cc, 0);

  return true;
}

//
// Starts the simple framebuffer copy timer.
//
void XenonPE::startFramebuffer(void) {
  _gpuTimerEventSource->setTimeoutMS(20);
  _gpuTimerEventSource->enable();
}

//
// Stops the simple framebuffer copy timer.
//
void XenonPE::stopFramebuffer(void) {
  _gpuTimerEventSource->disable();
  _gpuTimerEventSource->cancelTimeout();
}
