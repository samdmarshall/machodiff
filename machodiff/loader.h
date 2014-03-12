//
//  loader.h
//  loader
//
//  Created by Sam Marshall on 2/17/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef loader_loader_h
#define loader_loader_h

#pragma mark -
#pragma mark #include
#include "loader_type.h"
#include "eh_frame.h"
#include "objc.h"
#include "capstone.h"
#include "symbol.h"

#pragma mark -
#pragma mark Public Types

#pragma mark -
#pragma mark Functions

struct loader_binary * SDMLoadBinaryWithPath(char *path, uint8_t target_arch);
void SDMReleaseBinary(struct loader_binary *binary);

uint8_t SDMGetFatBinaryEndianness(uint32_t magic);

uint64_t SDMCalculateVMSlide(struct loader_binary *binary);
uint64_t SDMComputeFslide(struct loader_segment_map *segment_map, bool is64Bit);

uint8_t SDMIsBinaryFat(char *path);

bool SDMBinaryIs64Bit(struct loader_generic_header *header);

bool SDMIsBinaryLoaded(char *path, struct loader_binary *binary);

bool SDMLoadBinaryFromFile(struct loader_binary *binary, char *path, uint8_t target_arch);

#endif
