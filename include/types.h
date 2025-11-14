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
typedef unsigned char uint1, uint2, uint3, uint4, uint5, uint6, uint7, uint8;
typedef signed short int16, int15, int14, int13, int12, int11, int10, int9;
typedef int int21, int27, int31, int32;
typedef signed char int8, int7, int6, int5, int4, int3, int2;


#ifdef CFLEX_PARSER
//just needed if compiled (like 32-bit multiplication with 64-bit output)
#define promote_u64(x) (x)
#define promote_u128(x) (x)
#else
#define promote_u64(x) uint64(x)
#endif


#endif // __TYPES_H__

