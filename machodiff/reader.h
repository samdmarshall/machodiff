//
//  reader.h
//  machodiff
//
//  Created by Sam Marshall on 3/8/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_reader_h
#define machodiff_reader_h

#include "util.h"

Pointer read_uleb128(uint8_t *addr, uint64_t *value);
Pointer read_sleb128(uint8_t *addr, int64_t *value);

Pointer read_uint64(Pointer addr, uint64_t *value);
Pointer read_int64(Pointer addr, int64_t *value);
Pointer read_uint32(Pointer addr, uint32_t *value);
Pointer read_int32(Pointer addr, int32_t *value);
Pointer read_uint16(Pointer addr, uint16_t *value);
Pointer read_int16(Pointer addr, int16_t *value);
Pointer read_uint8(Pointer addr, uint8_t *value);
Pointer read_int8(Pointer addr, int8_t *value);

#endif
