#ifndef __TYPES_H__
#define __TYPES_H__

template<unsigned B, unsigned E=B>
unsigned bit_slice(unsigned n) { return (n >> B) & ((1 << (E-B+1))-1); }

//TODO: add types
#ifdef __SIZEOF_INT128__
typedef __int128 uint128;
#endif
typedef unsigned long long uint64, uint33;
typedef long long int64;
typedef unsigned uint17, uint18, uint20, uint21, uint22, uint23, uint24, uint25, uint26, uint27, uint28, uint31, uint32;
typedef unsigned short uint9, uint10, uint11, uint12, uint15, uint16;
typedef unsigned char uint3, uint4, uint6, uint8, uint7, uint1;
typedef signed short int16, int10;
typedef int int21, int27, int31, int32;
typedef signed char int8;


#ifdef CFLEX_PARSER
//just needed if compiled (like 32-bit multiplication with 64-bit output)
#define promote_u64(x) (x)
#define promote_u128(x) (x)
#else
#define promote_u64(x) uint64(x)
#endif


#endif // __TYPES_H__

