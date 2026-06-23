//
//  XenosRegs.hpp
//  Xbox 360 graphics framebuffer registers
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenosRegs_hpp
#define XenosRegs_hpp

#define kXenosMmioAddress									0xEC800000
#define kXenosMmioLength									0x10000

#define kXenosFramebufferLength           0x800000
#define kXenosRingbufferLength            (0x8000/4)

#define kXenosRegConfigControl            0x00E0
#define kXenosRegConfigXStrap             0x00E4
#define kXenosRegConfigXStrap2            0x00E8
#define kXenosRegRbbmControl              0x00EC
#define kXenosRegRbbmSoftReset            0x00F0
#define kXenosRegRbbmSkewControl          0xF004

// CP_RB_BASE
#define kXenosRegCpRbBase                 0x0700
// CP_RB_CNTL
#define kXenosRegCpRbControl              		0x0704
#define kXenosRegCpRbControlBufferSizeMask		BITRange(0, 5)
#define kXenosRegCpRbControlBlockSizeMask			BITRange(8, 13)
#define kXenosRegCpRbControlBlockSizeShift		8
#define kXenosRegCpRbControlNoUpdate					BIT27
#define kXenosRegCpRbControlWritePtrPollEn		BIT28
// CP_RB_RPTR_ADDR
#define kXenosRegCpRbReadPtrAddr          		0x070C

// CP_RB_RPTR
#define kXenosRegCpRbReadPtr              		0x0710
// CP_RB_WPTR
#define kXenosRegCpRbWritePtr             		0x0714
// CP_RB_WPTR_DELAY
#define kXenosRegCpRbWritePtrDelay        		0x0718
// CP_RB_RPTR_WR
#define kXenosRegCpRbReadPtrWrite        	 		0x071C
// CP_DMA_SRC_ADDR
#define kXenosRegCpDmaSourceAddr          		0x0720
// CP_DMA_DST_ADDR
#define kXenosRegCpDmaDestAddr            		0x0724
// CP_DMA_COMMAND
#define kXenosRegCpDmaCommand             		0x0728

#define kXenosRegScratchUMask             0x0770
#define kXenosRegScratchAddr              0x0774
#define kXenosRegScratchCompareHi         0x0778
#define kXenosRegScratchCompareLo         0x077C
#define kXenosRegCpIntControl             0x07C8
#define kXenosRegCpIntStatus              0x07CC
#define kXenosRegCpIntAck                 0x07D0
#define kXenosRegCpPerfMonControl         0x07D4
#define kXenosRegCpMeControl              0x07D8
#define kXenosRegCpMeStatus               0x07DC
#define kXenosRegCpMeRamWriteAddr         0x07E0
#define kXenosRegCpMeRamReadAddr          0x07E4
#define kXenosRegCpMeRamData              0x07E8
#define kXenosRegCpSnoopControl           0x07EC
#define kXenosRegCpDebug                  0x07F0

#define kXenosRegCpPfpFirmwareAddr        0x117C
#define kXenosRegCpPfpFirmwareData        0x1180

#define kXenosRegScratch0                 0x15E0
#define kXenosRegScratch1                 0x15E4
#define kXenosRegScratch2                 0x15E8
#define kXenosRegScratch3                 0x15EC
#define kXenosRegScratch4                 0x15F0
#define kXenosRegScratch5                 0x15F4
#define kXenosRegScratch6                 0x15F8
#define kXenosRegScratch7                 0x15FC

#define kXenosRegRbbmStatus               0x1740

// Primary graphics enable.
#define kXenosRegD1GrphEnable							0x6100
// Primary graphics control.
#define kXenosRegD1GrphControl						0x6104
// Primary graphics primary framebuffer surface address.
#define kXenosRegD1GrphPriSurfaceAddr			0x6110
// Primary graphics secondary framebuffer surface address.
#define kXenosRegD1GrphSecSurfaceAddr			0x6118

#define kXenosRegVgtEventInitiator        0x87E4

#endif
