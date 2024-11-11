#include <stdint.h>
#ifndef AP_COMMON_BYTESWAP_H
#define AP_COMMON_BYTESWAP_H

#if defined(HAVE_BYTESWAP_H) && HAVE_BYTESWAP_H
#include_next <byteswap.h>
#elif defined(__has_include)
  #if __has_include(<byteswap.h>)
    #include <byteswap.h>
  #else
    #include <inttypes.h>

    #if !defined(__bswap_16) && !defined(bswap_16)
    static inline uint16_t bswap_16(uint16_t u)
    {
        return (uint16_t) __builtin_bswap16(u);
    }
    #define __bswap_16(x) bswap_16(x)
    #endif

    #if !defined(__bswap_32) && !defined(bswap_32)
    static inline uint32_t bswap_32(uint32_t u)
    {
        return (uint32_t) __builtin_bswap32(u);
    }
    #define __bswap_32(x) bswap_32(x)
    #endif

    #if !defined(__bswap_64) && !defined(bswap_64)
    static inline uint64_t bswap_64(uint64_t u)
    {
        return (uint64_t) __builtin_bswap64(u);
    }
    #define __bswap_64(x) bswap_64(x)
    #endif
  #endif
#endif

#endif /* AP_COMMON_BYTESWAP_H */