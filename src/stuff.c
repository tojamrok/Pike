/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stuff.h"

/* Used by is8bitalnum in pike_macros.h. */
PMOD_EXPORT const char Pike_is8bitalnum_vector[] =
  "0000000000000000"
  "0000000000000000"
  "0000000000000000"
  "1111111111000000"
  "0111111111111111"
  "1111111111100001"
  "0111111111111111"
  "1111111111100000"
  "0000000000000000"
  "0000000000000000"
  "1011110101100010"
  "1011011001101110"
  "1111111111111111"
  "1111111011111111"
  "1111111111111111"
  "1111111011111111";

/* Not all of these are primes, but they should be adequate */
const INT32 hashprimes[32] =
{
  31,        /* ~ 2^0  = 1 */
  31,        /* ~ 2^1  = 2 */
  31,        /* ~ 2^2  = 4 */
  31,        /* ~ 2^3  = 8 */
  31,        /* ~ 2^4  = 16 */
  31,        /* ~ 2^5  = 32 */
  61,        /* ~ 2^6  = 64 */
  127,       /* ~ 2^7  = 128 */
  251,       /* ~ 2^8  = 256 */
  541,       /* ~ 2^9  = 512 */
  1151,      /* ~ 2^10 = 1024 */
  2111,      /* ~ 2^11 = 2048 */
  4327,      /* ~ 2^12 = 4096 */
  8803,      /* ~ 2^13 = 8192 */
  17903,     /* ~ 2^14 = 16384 */
  32321,     /* ~ 2^15 = 32768 */
  65599,     /* ~ 2^16 = 65536 */
  133153,    /* ~ 2^17 = 131072 */
  270001,    /* ~ 2^18 = 264144 */
  547453,    /* ~ 2^19 = 524288 */
  1109891,   /* ~ 2^20 = 1048576 */
  2000143,   /* ~ 2^21 = 2097152 */
  4561877,   /* ~ 2^22 = 4194304 */
  9248339,   /* ~ 2^23 = 8388608 */
  16777215,  /* ~ 2^24 = 16777216 */
  33554431,  /* ~ 2^25 = 33554432 */
  67108863,  /* ~ 2^26 = 67108864 */
  134217727, /* ~ 2^27 = 134217728 */
  268435455, /* ~ 2^28 = 268435456 */
  536870911, /* ~ 2^29 = 536870912 */
  1073741823,/* ~ 2^30 = 1073741824 */
  2147483647,/* ~ 2^31 = 2147483648 */
};

/* same thing as (int)floor(log((double)x) / log(2.0)) */
/* Except a bit quicker :) (hopefully) */

PMOD_EXPORT int my_log2(size_t x)
{
#if defined(HAS___BUILTIN_CLZL)
    if( x == 0 ) return 0;
    return ((sizeof(unsigned long)*8)-__builtin_clzl(x))-1;
#elif defined(HAS___BUILTIN_CLZ)
    /* on arm32 only this one exists. Happily enough sizeof(int) is
     * sizeof(size_t).
     */
    if( x == 0 ) return 0;
    return ((sizeof(int)*8)-__builtin_clz(x))-1;
#else
  static const signed char bit[256] =
  {
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  };
  size_t tmp;
  if( x == 0 ) return 0;
#if SIZEOF_CHAR_P > 4
  if((tmp=(x>>32)))
  {
    if((x=(tmp>>16))) {
      if((tmp=(x>>8))) return bit[tmp]+56;
      return bit[x]+48;
    }
    if((x=(tmp>>8))) return bit[x]+40;
    return bit[tmp]+32;
  }
#endif /* SIZEOF_CHAP_P > 4 */
  if((tmp=(x>>16)))
  {
    if((x=(tmp>>8))) return bit[x]+24;
    return bit[tmp]+16;
  }
  if((tmp=(x>>8))) return bit[tmp]+8;
  return bit[x];
#endif
}


PMOD_EXPORT double my_strtod(char *nptr, char **endptr)
{
  double tmp=strtod(nptr,endptr);
  if(*endptr>nptr)
  {
    if(endptr[0][-1]=='.')
      endptr[0]--;
  }
  return tmp;
}

/*
 * This rounds an integer up to the next power of two. For x a power
 * of two, this will just return the same again.
 */
unsigned INT32 find_next_power(unsigned INT32 x)
{
    if( x == 0 ) return 1;
    return 1<<(my_log2(x-1)+1);
}

static unsigned INT32 RandSeed1 = 0x5c2582a4;
static unsigned INT32 RandSeed2 = 0x64dff8ca;

static unsigned INT32 slow_rand(void)
{
  RandSeed1 = ((RandSeed1 * 13 + 1) ^ (RandSeed1 >> 9)) + RandSeed2;
  RandSeed2 = (RandSeed2 * RandSeed1 + 13) ^ (RandSeed2 >> 13);
  return RandSeed1;
}

static void slow_srand(INT32 seed)
{
  RandSeed1 = (seed - 1) ^ 0xA5B96384UL;
  RandSeed2 = (seed + 1) ^ 0x56F04021UL;
}

#define RNDBUF 250
#define RNDSTEP 7
#define RNDJUMP 103

static unsigned INT32 rndbuf[ RNDBUF ];
static unsigned int rnd_index;

#if HAS___BUILTIN_IA32_RDRAND32_STEP
static int use_rdrnd;
#endif
#define bit_RDRND_2 (1<<30)

#if SIZEOF_CHAR_P == 4
#define __cpuid(level, a, b, c, d)                      \
    __asm__ ("pushl %%ebx      \n\t"                    \
             "cpuid \n\t"                               \
             "movl %%ebx, %1   \n\t"                    \
             "popl %%ebx       \n\t"                    \
             : "=a" (a), "=r" (b), "=c" (c), "=d" (d)   \
             : "a" (level)                              \
             : "cc")
#else
#define __cpuid(level, a, b, c, d)                      \
    __asm__ ("push %%rbx      \n\t"			\
             "cpuid \n\t"                               \
             "movl %%ebx, %1   \n\t"                    \
             "pop %%rbx       \n\t"			\
             : "=a" (a), "=r" (b), "=c" (c), "=d" (d)   \
             : "a" (level)                              \
             : "cc")
#endif

PMOD_EXPORT void my_srand(INT32 seed)
{
#if HAS___BUILTIN_IA32_RDRAND32_STEP
  unsigned int ignore, cpuid_ecx;
  if( !use_rdrnd )
  {
    __cpuid( 0x1, ignore, ignore, cpuid_ecx, ignore );
    if( cpuid_ecx & bit_RDRND_2 )
      use_rdrnd = 1;
  }
  /* We still do the initialization here, since rdrnd might stop
     working if the hardware random unit in the CPU fails (according
     to intel documentation).


     This is likely to be rather rare. But the cost is not exactly
     high.

     Source:

     http://software.intel.com/en-us/articles/intel-digital-random-number-generator-drng-software-implementation-guide
  */
#endif
  {
    int e;
    unsigned INT32 mask;

    slow_srand(seed);

    rnd_index = 0;
    for (e=0;e < RNDBUF; e++) rndbuf[e]=slow_rand();

    mask = (unsigned INT32) -1;

    for (e=0;e< (int)sizeof(INT32)*8 ;e++)
    {
      int d = RNDSTEP * e + 3;
      rndbuf[d % RNDBUF] &= mask;
      mask>>=1;
      rndbuf[d % RNDBUF] |= (mask+1);
    }
  }
}

PMOD_EXPORT unsigned INT32 my_rand(void)
{
#if HAS___BUILTIN_IA32_RDRAND32_STEP
  if( use_rdrnd )
  {
    unsigned int cnt = 0;
    do{
      if( __builtin_ia32_rdrand32_step( &rnd_index ) )
        return rnd_index;
    } while(cnt++ < 100);

    /* hardware random unit most likely not healthy.
       Switch to software random. */
    rnd_index = 0;
    use_rdrnd = 0;
  }
#endif
  if(++rnd_index == RNDBUF) rnd_index=0;
  return rndbuf[rnd_index] += rndbuf[rnd_index+RNDJUMP-(rnd_index<RNDBUF-RNDJUMP?0:RNDBUF)];
}

PMOD_EXPORT unsigned INT64 my_rand64(void)
{
#if HAS___BUILTIN_IA32_RDRAND32_STEP
  if( use_rdrnd )
  {
    unsigned long long rnd;
    unsigned int cnt = 0;
    do{
      /* We blindly trust that _builtin_ia32_rdrand64_step exists when
         the 32 bit version does. */
      if( __builtin_ia32_rdrand64_step( &rnd ) )
        return rnd;
    } while(cnt++ < 100);

    /* hardware random unit most likely not healthy.
       Switch to software random. */
    rnd_index = 0;
    use_rdrnd = 0;
  }
#endif
  return ((INT64)my_rand()<<32) | my_rand();
}
