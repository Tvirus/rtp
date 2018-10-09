#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_






typedef signed   char       CHAR;
typedef unsigned char       BYTE;

typedef signed   short      SWORD16;
typedef unsigned short       WORD16;

typedef signed   int        SWORD32;
typedef unsigned int         WORD32;

typedef signed   long long  SWORD64;
typedef unsigned long long   WORD64;


#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

extern int Report(const char *pcFormat, ...);
extern BYTE PrintLevel;
#define ERROR_LEVEL  1
#define DEBUG_LEVEL  2
#define DETAIL_LEVEL 3

#if defined(linux) || defined(unix)
    #define Println(level, pFormat, arg...) \
    do { \
        if (unlikely(level <= PrintLevel)) \
        { \
            if (likely(ERROR_LEVEL != level)) \
                printf("[%s] " pFormat "\n", __FUNCTION__, ##arg); \
            else \
                printf("\e[41m\e[5m\e[1m ERROR \e[0m[%s] " pFormat "\n", __FUNCTION__, ##arg); \
        } \
    } while(0)
#else
    #define Println(level, pFormat, arg...) \
    do { \
        if (unlikely(level <= PrintLevel)) \
        { \
            if (likely(ERROR_LEVEL != level)) \
                Report("[%s] " pFormat "\n", __FUNCTION__, ##arg); \
            else \
                Report(" ERROR [%s] " pFormat "\n", __FUNCTION__, ##arg); \
        } \
    } while(0)
#endif



#endif
