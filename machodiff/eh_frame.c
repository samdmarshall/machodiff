//
//  eh_frame.c
//  machodiff
//
//  Created by Sam Marshall on 3/9/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_eh_frame_c
#define machodiff_eh_frame_c

#include "eh_frame.h"
#include <string.h>

#define kExtendedLength (sizeof(uint64_t) + sizeof(uint32_t))
#define kLength (sizeof(uint32_t))

static struct loader_eh_frame *last_cie = NULL;

uint64_t SDMSTParseCIEFrame(struct loader_eh_frame *frame, Pointer frame_offset);

uint64_t SDMSTParseFDEFrame(struct loader_eh_frame *frame, Pointer frame_offset);

struct loader_eh_frame_map* SDMSTParseCallFrame(CoreRange frame, bool is64bit) {
	struct loader_eh_frame_map *map = calloc(1, sizeof(struct loader_eh_frame_map));
	map->frame = calloc(1, sizeof(struct loader_eh_frame));
	map->count = 0;
	
	if (frame.offset != 0 && frame.length != 0) {
		uint64_t position = 0;
		Pointer frame_offset = PtrCast(frame.offset, Pointer);
		while (position < frame.length) {
			uint64_t frame_length = 0;
			
			uint32_t length = 0;
			frame_offset = read_uint32(frame_offset, &length);
			if (length == 0) {
				break;
			}
			else {
				map->frame = realloc(map->frame, sizeof(struct loader_eh_frame)*(map->count+1));
				
				frame_length += sizeof(uint32_t);
				if (length != k32BitMask) {
					map->frame[map->count].length = length;
				}
				else {
					uint64_t extended_length = 0;
					frame_offset = read_uint64(frame_offset, &extended_length);
					map->frame[map->count].extended_length = extended_length;
					frame_length += sizeof(uint64_t);
				}
				
				uint32_t identifier = 0;
				frame_offset = read_uint32(frame_offset, &identifier);
				map->frame[map->count].id = identifier;
				frame_length += sizeof(uint32_t);
				
				if (is64bit) {
					map->frame[map->count].size = loader_eh_frame_64_size;
				}
				else {
					map->frame[map->count].size = loader_eh_frame_32_size;
				}
				
				uint8_t length_size = (frame->length == k32BitMask ? kExtendedLength : kLength);
				
				frame_length -= length_size;
				
				if (map->frame[map->count].id == 0) {
					// CIE record
					map->frame[map->count].type = loader_eh_frame_cie_type;
					frame_length += SDMSTParseCIEFrame(&(map->frame[map->count]), frame_offset);
					frame_length -= length_size;
				}
				else {
					// FDE record
					map->frame[map->count].type = loader_eh_frame_fde_type;
					frame_length += SDMSTParseFDEFrame(&(map->frame[map->count]), frame_offset);
					frame_length -= length_size;
				}
				
				position += frame_length;
				
				frame_offset = (Pointer)PtrAdd(frame_offset, frame_length);
				
				map->count++;
			}
		}
	}
	
	last_cie = NULL;
	
	return map;
}

uint64_t SDMSTParseCIEFrame(struct loader_eh_frame *frame, Pointer frame_offset) {
	uint64_t frame_length = 0;
	
	uint8_t length_size = (frame->length == k32BitMask ? kExtendedLength : kLength);
	
	uint8_t version = 0;
	frame_offset = read_uint8(frame_offset, &version);
	frame->cie.version = version;
	frame_length += sizeof(uint8_t);
	
	frame->cie.aug_string = frame_offset;
	char *data = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_data);
	if (data) {
		frame->cie.aug_string_type |= loader_eh_frame_aug_data;
	}
	
	char *ehdata = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_ehdata);
	if (ehdata) {
		frame->cie.aug_string_type |= loader_eh_frame_aug_ehdata;
	}
	
	
	if ((frame->cie.aug_string_type & loader_eh_frame_aug_data) == loader_eh_frame_aug_data) {
		char *lsda = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_lsda);
		if (lsda) {
			frame->cie.aug_string_type |= loader_eh_frame_aug_lsda;
		}
		
		char *person = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_person);
		if (person) {
			frame->cie.aug_string_type |= loader_eh_frame_aug_person;
		}
		
		char *encode = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_encode);
		if (encode) {
			frame->cie.aug_string_type |= loader_eh_frame_aug_encode;
		}
	}
	
	uint32_t aug_string_length = (uint32_t)strlen(Ptr(frame->cie.aug_string)) + 1;
	frame_length += aug_string_length;
	frame_offset = (Pointer)PtrAdd(frame_offset, aug_string_length);
	
	
	switch (frame->type) {
		case loader_eh_frame_invalid_size: {
			break;
		}
		case loader_eh_frame_32_size: {
			if ((frame->cie.aug_string_type & loader_eh_frame_aug_ehdata) == loader_eh_frame_aug_ehdata) {
				uint32_t eh_data = 0;
				frame_offset = read_uint32(frame_offset, &eh_data);
				frame->cie.eh_data = eh_data;
				frame_length += sizeof(uint32_t);
			}
			break;
		}
		case loader_eh_frame_64_size: {
			if ((frame->cie.aug_string_type & loader_eh_frame_aug_ehdata) == loader_eh_frame_aug_ehdata) {
				uint64_t eh_data = 0;
				frame_offset = read_uint64(frame_offset, &eh_data);
				frame->cie.eh_data = eh_data;
				frame_length += sizeof(uint64_t);
			}
			break;
		}
		default: {
			break;
		}
	}
	
	Pointer old_position = NULL;
	
	uint64_t code_alignment = 0;
	old_position = frame_offset;
	frame_offset = read_uleb128((uint8_t*)old_position, &code_alignment);
	frame->cie.code_alignment = code_alignment;
	frame_length += (uint64_t)((uint64_t)frame_offset - (uint64_t)old_position);
	
	int64_t data_alignment = 0;
	old_position = frame_offset;
	frame_offset = read_sleb128((uint8_t*)old_position, &data_alignment);
	frame->cie.data_alignment = data_alignment;
	frame_length += (uint64_t)((uint64_t)frame_offset - (uint64_t)old_position);
	
	uint8_t return_register = 0;
	frame_offset = read_uint8(frame_offset, &return_register);
	frame->cie.return_register = return_register;
	frame_length += sizeof(uint8_t);
	
	if ((frame->cie.aug_string_type & loader_eh_frame_aug_data) == loader_eh_frame_aug_data) {
		uint64_t aug_length = 0;
		old_position = frame_offset;
		frame_offset = read_uleb128((uint8_t*)old_position, &aug_length);
		frame->cie.aug_length = aug_length;
		frame_length += (uint64_t)((uint64_t)frame_offset - (uint64_t)old_position);
		
		frame->cie.aug_data = frame_offset;
		
		frame_length += frame->cie.aug_length;
	}
	
	frame->cie.initial_instructions = frame_offset;
	frame->cie.instructions_length = (frame->length == k32BitMask ? frame->extended_length : frame->length) - frame_length - length_size;
	frame_length += frame->cie.instructions_length;
	
	last_cie = frame;
	
	return frame_length;
}

uint64_t SDMSTParseFDEFrame(struct loader_eh_frame *frame, Pointer frame_offset) {
	uint64_t frame_length = 0;
	
	uint8_t length_size = (frame->length == k32BitMask ? kExtendedLength : kLength);
	
	switch (frame->type) {
		case loader_eh_frame_invalid_size: {
			break;
		}
		case loader_eh_frame_32_size: {
			uint32_t pc_begin = 0;
			frame_offset = read_uint32(frame_offset, &pc_begin);
			frame->fde.pc_begin = pc_begin;
			frame_length += sizeof(uint32_t);
			
			// SDM: change encoding
			
			uint32_t pc_range = 0;
			frame_offset = read_uint32(frame_offset, &pc_range);
			frame->fde.pc_range = pc_range;
			frame_length += sizeof(uint32_t);
			
			// SDM: change encoding
			
			break;
		}
		case loader_eh_frame_64_size: {
			uint64_t pc_begin = 0;
			frame_offset = read_uint64(frame_offset, &pc_begin);
			frame->fde.pc_begin = pc_begin;
			frame_length += sizeof(uint64_t);
			
			// SDM: change encoding
			
			uint64_t pc_range = 0;
			frame_offset = read_uint64(frame_offset, &pc_range);
			frame->fde.pc_range = pc_range;
			frame_length += sizeof(uint64_t);
			
			// SDM: change encoding
			
			break;
		}
		default: {
			break;
		}
	}
	
	if ((last_cie->cie.aug_string_type & loader_eh_frame_aug_data) == loader_eh_frame_aug_data) {
		Pointer old_position = NULL;
		
		uint64_t aug_length = 0;
		old_position = frame_offset;
		frame_offset = read_uleb128((uint8_t*)old_position, &aug_length);
		frame->fde.aug_length = aug_length;
		frame_length += (uint64_t)(frame_offset - old_position);

		if (frame->fde.aug_length != 0) {
			frame->fde.aug_data = frame_offset;
		}
		
		frame_length += frame->fde.aug_length;
	}
	
	frame->fde.initial_instructions = frame_offset;
	frame->fde.instructions_length = (frame->length == k32BitMask ? frame->extended_length : frame->length) - frame_length - length_size;
	frame_length += frame->fde.instructions_length;
	
	frame->fde.relevant_cie = last_cie;
	
	return frame_length;
}

CoreRange SDMSTEH_FramePointer(struct loader_segment *text, bool is64Bit, uint64_t header_offset) {
	CoreRange result = CoreRangeCreate(0, 0);
	Pointer offset = (Pointer)text;
	uint32_t sections_count = 0;
	if (is64Bit) {
		struct loader_segment_64 *text_segment = PtrCast(text, struct loader_segment_64 *);
		sections_count = text_segment->info.nsects;
		offset = (Pointer)PtrAdd(offset, sizeof(struct loader_segment_64));
		for (uint32_t index = 0; index < sections_count; index++) {
			struct loader_section_64 *text_section = (struct loader_section_64 *)offset;
			if (strncmp(text_section->name.sectname, EH_FRAME, sizeof(char[16])) == 0) {
				Pointer result_offset = PtrCast(PtrCastSmallPointer(text_section->info.offset), Pointer);
				uint64_t addr_offset = text_section->position.addr - text_segment->data.vm_position.addr;
				if (text_section->info.offset != addr_offset) {
					result_offset = PtrCast(addr_offset, Pointer);
				}
				result.offset = (uint64_t)PtrAdd(result_offset, header_offset);
				result.length = (uint64_t)text_section->position.size;
				break;
			}
			offset = (Pointer)PtrAdd(offset, sizeof(struct loader_section_64));
		}
	}
	else {
		struct loader_segment_32 *text_segment = PtrCast(text, struct loader_segment_32 *);
		sections_count = text_segment->info.nsects;
		offset = (Pointer)PtrAdd(offset, sizeof(struct loader_segment_32));
		for (uint32_t index = 0; index < sections_count; index++) {
			struct loader_section_32 *text_section = (struct loader_section_32 *)offset;
			if (strncmp(text_section->name.sectname, EH_FRAME, sizeof(char[16])) == 0) {
				Pointer result_offset = PtrCast(PtrCastSmallPointer(text_section->info.offset), Pointer);
				uint64_t addr_offset = text_section->position.addr - text_segment->data.vm_position.addr;
				if (text_section->info.offset != addr_offset) {
					result_offset = PtrCast(addr_offset, Pointer);
				}
				result.offset = (uint64_t)PtrAdd(result_offset, header_offset);
				break;
			}
			offset = (Pointer)PtrAdd(offset, sizeof(struct loader_section_32));
		}
	}
	if (sections_count == 0) {
		result = CoreRangeCreate(0, 0);
	}
	return result;
}

bool SDMSTTEXTHasEH_Frame(struct loader_segment *text, bool is64Bit, uint64_t header_offset, CoreRange *eh_frame) {
	*eh_frame = SDMSTEH_FramePointer(text, is64Bit, header_offset);
	return ((eh_frame->offset != 0 && eh_frame->length != 0) ? true : false);
}

#endif
