//
//  util.h
//  Daodan
//
//  Created by Sam Marshall on 2/18/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef Daodan_util_h
#define Daodan_util_h

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define ATR_PACK __attribute__ ((packed))
#define ATR_FUNC(name) __attribute__ ((ifunc(name)))
#define ATR_UNUSED __attribute__ ((unused))

#ifndef __clang__
#define __has_feature(x) /* empty */
#endif

#if !__has_feature(objc_arc)
#define __unsafe_unretained /* empty */
#endif

#define GetDigitsOfNumber(num) (num > 0 ? (int)log10(num)+0x1 : 0x1)

struct Range {
	uint64_t offset;
	uint64_t length;
} ATR_PACK;

typedef struct Range CoreRange;

#define CoreRangeCreate(offset, length) (CoreRange){offset, length}
#define CoreRangeContainsValue(range, value) (value >= range.offset && value < (range.offet + range.length))

typedef uintptr_t* Pointer;
typedef uintptr_t* (*FunctionPointer)();

#define k32BitMask 0xffffffff
#define k64BitMask 0xffffffffffffffff
#define k64BitMaskHigh 0xffffffff00000000
#define k64BitMaskLow 0x00000000ffffffff
#define PtrCastSmallPointer(a) (*(Pointer)&(a))
#define PtrHighPointer(a) (a & k64BitMaskHigh)
#define PtrLowPointer(a) (a & k64BitMaskLow)

#define Ptr(ptr) PtrCast(ptr,char *)
#define PtrCast(ptr, cast) ((cast)ptr)
#define PtrAdd(ptr, add) (Ptr(ptr) + (uint64_t)add)
#define PtrSub(ptr, sub) (Ptr(ptr) - (uint64_t)sub)

#define SDMSwapEndian16(num) ((num<<8) | (num>>8))
#define SDMSwapEndian32(num) (((num>>24)&0xff) | ((num<<8)&0xff0000) | ((num>>8)&0xff00) |((num<<24)&0xff000000))

#endif
