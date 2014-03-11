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

#define EH_FRAME "__eh_frame"

enum loader_eh_frame_type {
	loader_eh_frame_invalid_type = 0,
	loader_eh_frame_cie_type,
	loader_eh_frame_fde_type
};

enum loader_eh_frame_size {
	loader_eh_frame_invalid_size = 0,
	loader_eh_frame_32_size,
	loader_eh_frame_64_size
};

struct loader_eh_frame_map {
	struct loader_eh_frame *frame;
	uint32_t count;
} ATR_PACK;

struct loader_eh_frame_cie {
	// cie
	uint8_t version;
	Pointer aug_string;
	uint64_t code_alignment;
	uint64_t data_alignment;
	uint64_t aug_length;
	Pointer aug_data;
	Pointer initial_instructions;
};

struct loader_eh_frame_fde {
	uint64_t pc_begin;
	uint64_t pc_range;
	
	uint64_t aug_length;
	Pointer aug_data;
	
	Pointer initial_instructions;
};

struct loader_eh_frame {
	enum loader_eh_frame_type type;
	enum loader_eh_frame_size size;
	// always present
	uint32_t length;
	uint32_t id;
	uint64_t extended_length;
	
	struct loader_eh_frame_cie cie;

	struct loader_eh_frame_fde fde;
};

struct loader_eh_frame_map* SDMSTParseCallFrame(CoreRange frame, bool is64bit);

#endif
