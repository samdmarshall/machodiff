//
//  eh_frame.h
//  machodiff
//
//  Created by Sam Marshall on 3/9/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_eh_frame_h
#define machodiff_eh_frame_h

#include "util.h"
#include "reader.h"
#include "loader_type.h"
#include "dwarf.h"

struct loader_eh_frame_map* SDMSTParseCallFrame(CoreRange frame, bool is64bit);

CoreRange SDMSTEH_FramePointer(struct loader_segment *text, bool is64Bit, uint64_t header_offset);

bool SDMSTTEXTHasEH_Frame(struct loader_segment *text, bool is64Bit, uint64_t header_offset, CoreRange *eh_frame);

uint64_t SDMSTDecodePC_Begin(struct loader_eh_frame *frame);

#endif
