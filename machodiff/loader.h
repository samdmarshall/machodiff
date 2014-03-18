//
//  loader.h
//  loader
//
//  Created by Sam Marshall on 2/17/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef loader_loader_h
#define loader_loader_h

#include "loader_type.h"
#include "eh_frame.h"
#include "objc.h"
#include "symbol.h"

#include <mach-o/loader.h>

struct loader_binary * SDMLoadBinaryWithPath(char *path, uint8_t target_arch);
bool SDMLoadBinaryFromFile(struct loader_binary *binary, char *path, uint8_t target_arch);
bool SDMIsBinaryLoaded(char *path, struct loader_binary *binary);
void SDMReleaseBinary(struct loader_binary *binary);

uint8_t SDMGetFatBinaryEndianness(uint32_t magic);

uint64_t SDMCalculateVMSlide(struct loader_binary *binary);
uint64_t SDMComputeFslide(struct loader_segment_map *segment_map, bool is64Bit);

uint8_t SDMIsBinaryFat(char *path);
bool SDMBinaryIs64Bit(struct loader_generic_header *header);

#endif
