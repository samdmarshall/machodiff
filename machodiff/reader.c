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

uint64_t read_uleb128(Pointer addr, Pointer *new_addr) {
	uint32_t bitCount = 0x0;
	uint64_t offset = 0x0;
	do {
		uint64_t slice = (*addr & 0x7f);
		if (bitCount < 0x40) {
			offset |= (slice << bitCount);
			bitCount += 0x7;
		}
		else {
			break;
		}
	} while ((*addr++ & 0x80) != 0);
	
	*new_addr = addr;
	
	return offset;
}

int64_t read_sleb128(Pointer addr, Pointer *new_addr) {
	uint32_t bitCount = 0x0;
	int64_t offset = 0x0;
	do {
		int64_t slice = (*addr & 0x7f);
		offset |= (slice << bitCount);
		bitCount += 0x7;
	} while ((*addr++ & 0x80) != 0);
	
	if ((*addr & 0x40) != 0) {
		offset |= (-1LL) << bitCount;
	}
	
	*new_addr = addr;
	
	return offset;
}

#endif
