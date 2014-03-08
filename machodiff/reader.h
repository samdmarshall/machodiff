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

uint64_t read_uleb128(Pointer addr, Pointer *new_addr);
int64_t read_sleb128(Pointer addr, Pointer *new_addr);

#endif
