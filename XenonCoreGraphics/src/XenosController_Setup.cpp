//
//  XenosController_Setup.cpp
//  Xbox 360 graphics controller
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#include "XenosController.hpp"
#include "adreno_pm4.xml.h"
#include "adreno_common.xml.h"
#include "a2xx.xml.h"

static const float sTexCoords[] = {
  0.0f, 1.0f,
  1.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f,
};

static const float sTriag[] = {
  -1.0f, -1.0f, 0.0f,
  1.0f, -1.0f, 0.0f,
  -1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f
};

static inline uint32_t xy32(int x, int y) {
	return x | (y << 16);
}

//
// Initializes shader buffer descriptors.
//
IOReturn XenosController::initShaders(void) {
  IOByteCount length;

  _texCoordsDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, PAGE_SIZE, PAGE_SIZE);
  if (_texCoordsDesc == NULL) {
    return kIOReturnNoMemory;
  }
  _texCoordsDesc->writeBytes(0, sTexCoords, sizeof (sTexCoords));
  _texCoordsPhys = _texCoordsDesc->getPhysicalSegment(0, &length);

  _triagDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, PAGE_SIZE, PAGE_SIZE);
  if (_triagDesc == NULL) {
    return kIOReturnNoMemory;
  }
  _triagDesc->writeBytes(0, sTriag, sizeof (sTriag));
  _triagPhys = _triagDesc->getPhysicalSegment(0, &length);

  syncData(_texCoordsDesc->getBytesNoCopy(), PAGE_SIZE);
  syncData(_triagDesc->getBytesNoCopy(), PAGE_SIZE);

  return kIOReturnSuccess;
}

void XenosController::initME(void) {
  writeRing(0xc0114800); //PM4_ME_INIT

  /* All fields present (bits 9:0) */
  writeRing(0x000003ff);
  /* Disable/Enable Real-Time Stream processing (present but ignored) */
  writeRing(0x00000000);
  /* Enable (2D <-> 3D) implicit synchronization (present but ignored) */
  writeRing(0x00000000);

  writeRing(0x2000 - 0x2000); //RB_SURFACE_INFO
  writeRing(0x2080 - 0x2000); //PA_SC_WINDOW_OFFSET
  writeRing(0x2100 - 0x2000); //VGT_MAX_VTX_INDX
  writeRing(0x2180 - 0x2000); //SQ_PROGRAM_CNTL
  writeRing(0x2200 - 0x2000); //RB_DEPTHCONTROL
  writeRing(0x2280 - 0x2000); //PA_SU_POINT_SIZE
  writeRing(0x2300 - 0x2000); //PA_SC_LINE_CNTL
  writeRing(0x2380 - 0x2000); //PA_SU_POLY_OFFSET_FRONT_SCALE

  /* Vertex and Pixel Shader Start Addresses in instructions
  * (3 DWORDS per instruction) */
  writeRing(0x80000180);
  /* Maximum Contexts */
  writeRing(0x00000007);
  /* Write Confirm Interval and The CP will wait the
  * wait_interval * 16 clocks between polling  */
  writeRing(0x00000000);
  /* NQ and External Memory Swap */
  writeRing(0x00000000);
  /* protected mode error checking (0x1f2 is REG_AXXX_CP_INT_CNTL) */
  writeRing(0x00000000);
  /* Disable header dumping and Header dump address */
  writeRing(0x00000000);
  /* Header dump size */
  writeRing(0x00000000);
}

IOReturn XenosController::initHardware(void) {
  IOPhysicalAddress ringBufferPhys;
  IOByteCount       segLength;

  _ringBufferDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, kXenosRingbufferLength * 4, kXenosRingbufferLength * 4);
  if (_ringBufferDesc == NULL) {
    return kIOReturnNoResources;
  }
  _ringBuffer = (volatile UInt8*) _ringBufferDesc->getBytesNoCopy();
  ringBufferPhys = _ringBufferDesc->getPhysicalSegment(0, &segLength);
  _ringBufferWritePtr = 0;

  XEDBGLOG("Ring buffer at 0x%X (%p)", ringBufferPhys, _ringBuffer);

  // Configure firmware.
  writeReg32(kXenosRegCpMeControl, (1 << 28));
  loadFirmware();

  writeReg32(kXenosRegCpIntAck, 0xFFFF);
  writeReg32(kXenosRegCpDebug, 0);

  // Reset command processor.
  writeReg32(kXenosRegRbbmSoftReset, 1);
  IODelay(1000);
  writeReg32(kXenosRegRbbmSoftReset, 0);

  // Setup ring buffer.
  writeReg32(kXenosRegCpRbControl, readReg32(kXenosRegCpRbControl) | 0x80000000);
  writeReg32(kXenosRegCpRbWritePtr, 0);
  writeReg32(kXenosRegCpRbControl, readReg32(kXenosRegCpRbControl) & ~0x80000000);

  writeReg32(kXenosRegCpRbControl, 0xC | 0x8020000);
  writeReg32(kXenosRegCpRbBase, ringBufferPhys);

  writeReg32(kXenosRegCpMeControl, 0);

  initME();
  syncRingBuffer();

  return kIOReturnSuccess;
}

void XenosController::initFramebuffer(void) {
  writeRingPacket3(CP_INVALIDATE_STATE, 1);
  writeRing(0x00007fff);

  writeRingPacket0(REG_A2XX_SQ_INST_STORE_MANAGMENT, 1);
  writeRing(0x00000180);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_SQ_VS_CONST));
  writeRing(0x00100020);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_SQ_PS_CONST));
  writeRing(0x000e0120);

  writeRingPacket3(CP_SET_CONSTANT, 3);
  writeRing(CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
  writeRing(0xffffffff);	/* VGT_MAX_VTX_INDX */
  writeRing(0x00000000);	/* VGT_MIN_VTX_INDX */

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_VGT_INDX_OFFSET));
  writeRing(0x00000000);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
  writeRing(0x0000003b);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_SQ_CONTEXT_MISC));
  writeRing(A2XX_SQ_CONTEXT_MISC_SC_SAMPLE_CNTL(CENTERS_ONLY));

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_SQ_INTERPOLATOR_CNTL));
  writeRing(0xffffffff);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_PA_SC_AA_CONFIG));
  writeRing(0x00000000);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_PA_SC_AA_MASK));
  writeRing(0x0000ffff);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_PA_SC_WINDOW_OFFSET));
  writeRing(0x00000000);

  float halfWidth = _fbWidth * 0.5f;
  float halfHeight = _fbHeight * 0.5f;

  writeRingPacket3(CP_SET_CONSTANT, 5);
  writeRing(CP_REG(REG_A2XX_PA_CL_VPORT_XSCALE));
  writeRing(fui(halfWidth));	/* PA_CL_VPORT_XSCALE */
  writeRing(fui(halfWidth));	/* PA_CL_VPORT_XOFFSET */
  writeRing(fui(-halfHeight));	/* PA_CL_VPORT_YSCALE */
  writeRing(fui(halfHeight));	/* PA_CL_VPORT_YOFFSET */

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_PA_CL_CLIP_CNTL));
  writeRing(0x00000000);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_PA_CL_VTE_CNTL));
  writeRing(A2XX_PA_CL_VTE_CNTL_VTX_W0_FMT |
    A2XX_PA_CL_VTE_CNTL_VPORT_X_SCALE_ENA |
    A2XX_PA_CL_VTE_CNTL_VPORT_X_OFFSET_ENA |
    A2XX_PA_CL_VTE_CNTL_VPORT_Y_SCALE_ENA |
    A2XX_PA_CL_VTE_CNTL_VPORT_Y_OFFSET_ENA);

  writeRingPacket3(CP_SET_CONSTANT, 5);
  writeRing(CP_REG(REG_A2XX_PA_CL_GB_VERT_CLIP_ADJ));
  writeRing(fui(1.0));		/* PA_CL_GB_VERT_CLIP_ADJ */
  writeRing(fui(1.0));		/* PA_CL_GB_VERT_DISC_ADJ */
  writeRing(fui(1.0));		/* PA_CL_GB_HORZ_CLIP_ADJ */
  writeRing(fui(1.0));		/* PA_CL_GB_HORZ_DISC_ADJ */

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_PA_SU_VTX_CNTL));
  writeRing(A2XX_PA_SU_VTX_CNTL_PIX_CENTER(PIXCENTER_OGL));

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_PA_SU_SC_MODE_CNTL));
  writeRing(A2XX_PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST |
    A2XX_PA_SU_SC_MODE_CNTL_FRONT_PTYPE(PC_DRAW_TRIANGLES) |
    A2XX_PA_SU_SC_MODE_CNTL_BACK_PTYPE(PC_DRAW_TRIANGLES));

  writeRingPacket3(CP_SET_CONSTANT, 3);
  writeRing(CP_REG(REG_A2XX_PA_SC_WINDOW_SCISSOR_TL));
  writeRing(xy32(0,0));                 /* PA_SC_WINDOW_SCISSOR_TL */
  writeRing(xy32(_fbWidth, _fbHeight)); /* PA_SC_WINDOW_SCISSOR_BR */

  writeRingPacket3(CP_SET_CONSTANT, 3);
  writeRing(CP_REG(REG_A2XX_PA_SC_SCREEN_SCISSOR_TL));
  writeRing(xy32(0,0));                 /* PA_SC_SCREEN_SCISSOR_TL */
  writeRing(xy32(_fbWidth, _fbHeight)); /* PA_SC_SCREEN_SCISSOR_BR */

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_RB_BLEND_CONTROL));
  writeRing(A2XX_RB_BLEND_CONTROL_COLOR_SRCBLEND(FACTOR_ONE) |
    A2XX_RB_BLEND_CONTROL_ALPHA_SRCBLEND(FACTOR_ONE) |
    A2XX_RB_BLEND_CONTROL_COLOR_DESTBLEND(FACTOR_ZERO) |
    A2XX_RB_BLEND_CONTROL_ALPHA_DESTBLEND(FACTOR_ZERO));

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_RB_COLORCONTROL));
  writeRing(A2XX_RB_COLORCONTROL_BLEND_DISABLE);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_RB_COPY_DEST_INFO));
  writeRing(0x1000A80); //copy_dest_swap | k_4_4_4_4

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_RB_ALPHA_REF));
  writeRing(0x00000000);

  writeRingPacket3(CP_WAIT_REG_EQ, 4);
  writeRing(0x000005d0);  /* RBBM_STATUS */
  writeRing(0x00000000);
  writeRing(0x5f601000);  /* bit: 12: VGT_BUSY_NO_DMA */
  writeRing(0x00000001);

  writeRingPacket3(CP_INVALIDATE_STATE, 1);
  writeRing(0x00000300);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_RB_COLOR_MASK));
  writeRing(A2XX_RB_COLOR_MASK_WRITE_RED |
    A2XX_RB_COLOR_MASK_WRITE_GREEN |
    A2XX_RB_COLOR_MASK_WRITE_BLUE |
    A2XX_RB_COLOR_MASK_WRITE_ALPHA);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_RB_DEPTHCONTROL));
  writeRing(0);

  writeRingPacket3(CP_SET_CONSTANT, 3);
  writeRing(CP_REG(REG_A2XX_RB_STENCILREFMASK_BF));
  writeRing(0x00000000);		/* RB_STENCILREFMASK_BF */
  writeRing(0x00000000);		/* REG_RB_STENCILREFMASK */

  writeRingPacket3(CP_SET_CONSTANT, 4);
  writeRing(CP_REG(REG_A2XX_RB_SURFACE_INFO));
  writeRing(ALIGN(_fbWidth, 32));   /* RB_SURFACE_INFO */
  writeRing((k_8_8_8_8 << 16) );    /* RB_COLOR_INFO */
  writeRing(A2XX_RB_DEPTH_INFO_DEPTH_BASE(ALIGN(_fbWidth, 32) * ALIGN(_fbHeight, 32))); /* RB_DEPTH_INFO */

  writeRingPacket3(CP_SET_CONSTANT, 7);
  writeRing((0x1 << 16) | 0x78); // type = fetch (base is 0x4800, val is offset)
  writeRing(_triagPhys | 3); // kVertex
  writeRing(0x2 | 48);			 // 12 dwords | k8in32 endian
  writeRing(_texCoordsPhys | 3); 	 // kVertex
  writeRing(0x2 | 32);			 // 12 dwords | k8in32 endian
  writeRing(0x00000000);
  writeRing(0x00000000);

  writeRingPacket3(CP_SET_CONSTANT, 7);
  writeRing(0x00010000);
  writeRing(
    A2XX_SQ_TEX_0_PITCH(ALIGN(_fbWidth, 32)) |
    A2XX_SQ_TEX_0_CLAMP_X(SQ_TEX_WRAP) |
    A2XX_SQ_TEX_0_CLAMP_Y(SQ_TEX_WRAP));
  writeRing(_fbPhysAddr | FMT_8_8_8_8);
  writeRing(
    A2XX_SQ_TEX_2_HEIGHT(_fbHeight - 1) |
    A2XX_SQ_TEX_2_WIDTH(_fbWidth - 1));
  writeRing(
    A2XX_SQ_TEX_3_SWIZ_X(SQ_TEX_Y) |
    A2XX_SQ_TEX_3_SWIZ_Y(SQ_TEX_Z) |
    A2XX_SQ_TEX_3_SWIZ_Z(SQ_TEX_W) |
    A2XX_SQ_TEX_3_SWIZ_W(SQ_TEX_X) |
    A2XX_SQ_TEX_3_XY_MAG_FILTER(SQ_TEX_FILTER_BILINEAR) |
    A2XX_SQ_TEX_3_XY_MIN_FILTER(SQ_TEX_FILTER_BILINEAR));
  writeRing(0x00000000);
  writeRing(0x00000200);

  writeRingPacket3(CP_SET_CONSTANT, 5);
  writeRing(CP_REG(REG_A2XX_RB_COPY_CONTROL));
  writeRing(0x00100000);                  /* RB_COPY_CONTROL */
  writeRing(_gpuPhysAddr);                /* RB_COPY_DEST_BASE */
  writeRing(xy32(_fbWidth, _fbHeight));   /* RB_COPY_DEST_PITCH */
  writeRing(A2XX_RB_COPY_DEST_INFO_FORMAT(COLORX_S8_8_8_8) |
    A2XX_RB_COPY_DEST_INFO_SWAP(1));      /* RB_COPY_DEST_INFO */

  writeRingPacket3(CP_IM_LOAD_IMMEDIATE, 2 + 21);
  writeRing(0x00000000);
  writeRing(0x00000015);
  writeRing(0x000d2003); writeRing(0x00001000); writeRing(0xc2000000);
  writeRing(0x00001005); writeRing(0x00001000); writeRing(0xc4000000);
  writeRing(0x00001006); writeRing(0x00002000); writeRing(0x00000000);
  writeRing(0x0b482000); writeRing(0x00253b48); writeRing(0x00000002);
  writeRing(0x01481000); writeRing(0x40393a88); writeRing(0x00000003);
  writeRing(0x140f803e); writeRing(0x00000000); writeRing(0xe2010100);
  writeRing(0x140f8000); writeRing(0x00000000); writeRing(0xe2020200);

  writeRingPacket3(CP_SET_CONSTANT, 2);
  writeRing(CP_REG(REG_A2XX_SQ_PROGRAM_CNTL));
  writeRing(0x10030002);

  writeRingPacket3(CP_IM_LOAD_IMMEDIATE, 2 + 12);
  writeRing(0x00000001);
  writeRing(0x0000000c);
  writeRing(0x00031002); writeRing(0x00001000); writeRing(0xc4000000);
  writeRing(0x00001003); writeRing(0x00002000); writeRing(0x00000000);
  writeRing(0x10000001); writeRing(0x1ffff688); writeRing(0x00000000);
  writeRing(0x140f8000); writeRing(0x00000000); writeRing(0xe2000000);
}
