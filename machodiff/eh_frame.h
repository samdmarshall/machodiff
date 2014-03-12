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

#define EH_FRAME "__eh_frame"

#define kLoader_EH_FRAME_aug_data "z"
#define kLoader_EH_FRAME_aug_ehdata "eh"
#define kLoader_EH_FRAME_aug_lsda "L"
#define kLoader_EH_FRAME_aug_person "P"
#define kLoader_EH_FRAME_aug_encode "R"

enum loader_eh_frame_aug {
	loader_eh_frame_aug_empty = 	    0, //
	loader_eh_frame_aug_data = 		  0x1, // z
	loader_eh_frame_aug_ehdata =     0x10, // eh
	loader_eh_frame_aug_lsda = 	    0x100, // L
	loader_eh_frame_aug_person =   0x1000, // P
	loader_eh_frame_aug_encode =  0x10000, // R
};

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
	
	uint64_t aug_string_type;
	
	uint64_t code_alignment;
	int64_t data_alignment;
	
	uint8_t return_register;
	
	uint64_t aug_length;
	Pointer aug_data;
	
	uint64_t instructions_length;
	Pointer initial_instructions;
	
	uint64_t eh_data;
	
	uint8_t lsda_pointer_encoding;
	
	uint8_t personality_encoding;
	uint64_t personality_pointer;
	
	uint8_t fde_pointer_encoding;
};

struct loader_eh_frame_fde {
	uint64_t pc_begin;
	uint64_t pc_range;
	
	uint64_t aug_length;
	Pointer aug_data;
	
	uint64_t instructions_length;
	Pointer initial_instructions;
	
	struct loader_eh_frame *relevant_cie;
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

CoreRange SDMSTEH_FramePointer(struct loader_segment *text, bool is64Bit, uint64_t header_offset);
bool SDMSTTEXTHasEH_Frame(struct loader_segment *text, bool is64Bit, uint64_t header_offset, CoreRange *eh_frame);



#endif
