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

#pragma mark -
#pragma mark Private Types

#define kStubName "__sdmst_stub_"

#define kSubPrefix "sub_"
#define kSubFormatter "%lx"
#define kSubName "sub_%lx"

#pragma mark -
#pragma mark Public Types

#pragma mark -
#pragma mark Functions

struct loader_binary * SDMLoadBinaryWithPath(char *path, uint8_t target_arch);

uint64_t SDMCalculateVMSlide(struct loader_binary *binary);
uint64_t SDMComputeFslide(struct loader_segment_map *segment_map, bool is64Bit);

void SDMGenerateSymbols(struct loader_binary *binary);

Pointer SDMSTFindFunctionAddress(Pointer *fPointer, struct loader_binary *binary);

uint8_t SDMIsBinaryFat(char *path);

bool SDMBinaryIs64Bit(struct loader_generic_header *header);

void SDMReleaseBinary(struct loader_binary *binary);

#endif
