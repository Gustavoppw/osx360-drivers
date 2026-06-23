//
//  XenonCommon.hpp
//  Xbox 360 common functions
//
//  Copyright © 2026 John Davis. All rights reserved.
//

#ifndef XenonCommon_hpp
#define XenonCommon_hpp

#include <IOKit/IOLib.h>

#define ARRSIZE(x)    ((sizeof (x) / sizeof ((x)[0])))

#define kHz   1000
#define MHz   (kHz * 1000)
#define kByte 1024

// 1000 uS in one MS
#define kMicrosecondMS     1000

#define BIT0  (1<<0)
#define BIT1  (1<<1)
#define BIT2  (1<<2)
#define BIT3  (1<<3)
#define BIT4  (1<<4)
#define BIT5  (1<<5)
#define BIT6  (1<<6)
#define BIT7  (1<<7)
#define BIT8  (1<<8)
#define BIT9  (1<<9)
#define BIT10 (1<<10)
#define BIT11 (1<<11)
#define BIT12 (1<<12)
#define BIT13 (1<<13)
#define BIT14 (1<<14)
#define BIT15 (1<<15)
#define BIT16 (1<<16)
#define BIT17 (1<<17)
#define BIT18 (1<<18)
#define BIT19 (1<<19)
#define BIT20 (1<<20)
#define BIT21 (1<<21)
#define BIT22 (1<<22)
#define BIT23 (1<<23)
#define BIT24 (1<<24)
#define BIT25 (1<<25)
#define BIT26 (1<<26)
#define BIT27 (1<<27)
#define BIT28 (1<<28)
#define BIT29 (1<<29)
#define BIT30 (1<<30)
#define BIT31 (1<<31)

#define BITRange(start, end) (                \
  ((((UInt32) 0xFFFFFFFF) << (31 - (end))) >> \
  ((31 - (end)) + (start))) <<                \
  (start)                                     \
)

//
// Platform functions.
//
#define kXenonFuncPlatformStartFB       "PlatformStartFB"
#define kXenonFuncPlatformStopFB        "PlatformStopFB"

//
// SMC functions.
//
#define kXenonFuncSMCEject              "SMCEjectTray"

//
// Major kernel version exported from XNU.
//
extern const int version_major;

//
// Kernel verisons.
//
typedef enum {
  kKernelVersionCheetahPumaBase = 1,
  kKernelVersionPumaUpdated     = 5,
  kKernelVersionJaguar          = 6,
  kKernelVersionPanther         = 7,
  kKernelVersionTiger           = 8,
  kKernelVersionLeopard         = 9,
  kKernelVersionSnowLeopard     = 10
} KernelVersion;

//
// Get major kernel version.
//
inline KernelVersion getKernelVersion() {
	return static_cast<KernelVersion>(version_major);
}

inline bool checkKernelArgument(const char *name) {
  int val[16];
  return PE_parse_boot_arg(name, val);
}

//
// 64-bit true read/write functions.
//
inline UInt64 ReadFullBigInt64(volatile void *base, uintptr_t byteOffset) {
  UInt32 hi;
  UInt32 lo;
  volatile UInt64 *addr = (volatile UInt64*)(((uintptr_t)base) + byteOffset);

  asm volatile(
    "ld   %1,0(%2)\n\t"
    "eieio\n\t"
    "srdi %0,%1,32\n\t"
    : "=r"(hi), "=r"(lo)
    : "b"(addr)
    : "memory");

  return ((UInt64)hi << 32) | lo;
}

inline void WriteFullBigInt64(volatile void *base, uintptr_t byteOffset, UInt64 data) {
  UInt32 hi = (UInt32)(data >> 32);
  UInt32 lo = (UInt32)(data);
  UInt32 tmp;
  volatile UInt64 *addr = (volatile UInt64*)(((uintptr_t)base) + byteOffset);

  asm volatile(
    "rldicl %0, %2, 0, 32\n\t"    // Clear any garbage in the upper 32 bits of 'lo', store in 'tmp'
    "rldimi %0, %1, 32, 0\n\t"    // Shift 'hi' left 32 bits and insert it into the upper half of 'tmp'
    "std %0, 0(%3)\n\t"           // Store the combined 64-bit register to memory
    "eieio\n\t"
    : "=&r"(tmp)                  // Early-clobber output for our combined 64-bit value
    : "r"(hi), "r"(lo), "b"(addr) // 'b' constraint ensures a valid base register for the offset
    : "memory"
  );
}

#if DEBUG
//
// Debug logging function.
//
inline void logPrint(const char *className, const char *locationName, const char *funcName, const char *format, va_list va) {
  char tmp[256];
  tmp[0] = '\0';
  vsnprintf(tmp, sizeof (tmp), format, va);
  if (locationName != NULL) {
    IOLog("%s[%s]::%s(): %s\n", className, locationName, funcName, tmp);
  } else {
    IOLog("%s::%s(): %s\n", className, funcName, tmp);
  }
}

//
// Log functions for I/O Kit modules.
//
#define XenonDeclareLogFunctions(a) \
  protected: \
  bool _debugEnabled; \
  const char *_debugLocation; \
  inline void XenonCheckDebugArgs() { \
    _debugEnabled = checkKernelArgument("-xe" a "dbg"); \
    _debugLocation = NULL; \
  } \
  inline void XenonSetDebugLocation(const char *location) { \
    _debugLocation = location; \
  } \
  inline void XEDBGLOG_PRINT(const char *func, const char *str, ...) const { \
    if (this->_debugEnabled) { \
      va_list args; \
      va_start(args, str); \
      logPrint(this->getMetaClass()->getClassName(), _debugLocation, func, str, args); \
      va_end(args); \
    } \
  } \
    \
  inline void XESYSLOG_PRINT(const char *func, const char *str, ...) const { \
    va_list args; \
    va_start(args, str); \
    logPrint(this->getMetaClass()->getClassName(), _debugLocation, func, str, args); \
    va_end(args); \
  } \
  protected:

//
// Common logging macros to inject function name.
//
#define XEDBGLOG(str, args...)     XEDBGLOG_PRINT(__FUNCTION__, str, ##args)
#define XEDATADBGLOG(str, args...) XEDATADBGLOG_PRINT(__FUNCTION__, str, ##args)
#define XESYSLOG(str, args...)     XESYSLOG_PRINT(__FUNCTION__, str, ##args)

#else

//
// Release print function.
//
inline void logPrint(const char *className, const char *locationName, const char *format, va_list va) {
  char tmp[256];
  tmp[0] = '\0';
  vsnprintf(tmp, sizeof (tmp), format, va);
  if (locationName != NULL) {
    IOLog("%s[%s]: %s\n", className, locationName, tmp);
  } else {
    IOLog("%s: %s\n", className, tmp);
  }
}

//
// Log functions for I/O Kit modules.
//
#define XenonDeclareLogFunctions(a) \
  protected: \
  bool _debugEnabled; \
  const char *_debugLocation; \
  inline void XenonCheckDebugArgs() { \
    _debugEnabled = false; \
    _debugLocation = NULL; \
  } \
  inline void XenonSetDebugLocation(const char *location) { \
    _debugLocation = location; \
  } \
  inline void XEDBGLOG(const char *str, ...) const { } \
    \
  inline void XESYSLOG(const char *str, ...) const { \
    va_list args; \
    va_start(args, str); \
    logPrint(this->getMetaClass()->getClassName(), _debugLocation, str, args); \
    va_end(args); \
  } \
  protected:
#endif

//
// Pulled from https://github.com/esneider/div64/blob/master/div64.h.
//
static inline UInt32 Div64_32(UInt64 dividend, UInt32 divisor) {

	UInt32 low      = 0xFFFF & (UInt32)dividend;
	UInt32 mid_low  = 0xFFFF & (UInt32)(dividend >>= 16);
	UInt32 mid_high = 0xFFFF & (UInt32)(dividend >>= 16);
	UInt32 high     =          (UInt32)(dividend >> 16);

	dividend  = high / divisor;
	mid_high += (high % divisor) << 16;
	dividend  = (dividend << 16) + mid_high / divisor;
	mid_low  += (mid_high % divisor) << 16;
	dividend  = (dividend << 16) + mid_low / divisor;
	low      += (mid_low % divisor) << 16;

	return (dividend << 16) + low / divisor;
}

#endif
