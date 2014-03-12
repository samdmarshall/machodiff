//
//  reader.c
//  machodiff
//
//  Created by Sam Marshall on 3/8/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_reader_c
#define machodiff_reader_c

#include "reader.h"

Pointer read_uleb128(uint8_t *addr, uint64_t *value) {
	uint32_t bitCount = 0;
	uintptr_t offset = 0;
	do {
		uint32_t slice = ((uint8_t)*addr & 0x7f);
		if (bitCount < 0x40) {
			offset |= (slice << bitCount);
			bitCount += 7;
		}
		else {
			break;
		}
	} while ((*addr++ & 0x80) != 0);
	

	*value = offset;
	
	return PtrCast(addr, Pointer);
}

Pointer read_sleb128(uint8_t *addr, int64_t *value) {
	uint32_t bitCount = 0;
	int64_t offset = 0;
	do {
		uint8_t slice = (*addr & 0x7f);
		offset |= (slice << bitCount);
		bitCount += 7;
	} while ((*addr++ & 0x80) != 0);
	
	if ((*addr & 0x40) != 0) {
		offset |= (-1LL) << bitCount;
	}
	
	*value = offset;
	
	return PtrCast(addr, Pointer);
}

Pointer read_uint64(Pointer addr, uint64_t *value) {
	uint64_t read = (uint64_t)addr[0];
	*value = read;
	return (Pointer)PtrAdd(addr, sizeof(uint64_t));
}

Pointer read_int64(Pointer addr, int64_t *value) {
	int64_t read = (int64_t)addr[0];
	*value = read;
	return (Pointer)PtrAdd(addr, sizeof(int64_t));
}

Pointer read_uint32(Pointer addr, uint32_t *value) {
	uint32_t read = (uint32_t)addr[0];
	*value = read;
	return (Pointer)PtrAdd(addr, sizeof(uint32_t));
}

Pointer read_int32(Pointer addr, int32_t *value) {
	int32_t read = (int32_t)addr[0];
	*value = read;
	return (Pointer)PtrAdd(addr, sizeof(int32_t));
}

Pointer read_uint16(Pointer addr, uint16_t *value) {
	uint16_t read = (uint16_t)addr[0];
	*value = read;
	return (Pointer)PtrAdd(addr, sizeof(uint16_t));
}

Pointer read_int16(Pointer addr, int16_t *value) {
	int16_t read = (int16_t)addr[0];
	*value = read;
	return (Pointer)PtrAdd(addr, sizeof(int16_t));
}

Pointer read_uint8(Pointer addr, uint8_t *value) {
	uint8_t read = (uint8_t)addr[0];
	*value = read;
	return (Pointer)PtrAdd(addr, sizeof(uint8_t));
}

Pointer read_int8(Pointer addr, int8_t *value) {
	int8_t read = (int8_t)addr[0];
	*value = read;
	return (Pointer)PtrAdd(addr, sizeof(int8_t));
}

#endif
