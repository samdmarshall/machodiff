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
					
					frame_length += sizeof(uint64_t);
					
					map->frame[map->count].extended_length = extended_length;
				}
				
				uint32_t identifier = 0;
				frame_offset = read_uint32(frame_offset, &identifier);
				
				frame_length += sizeof(uint32_t);
				
				map->frame[map->count].id = identifier;
				
				if (is64bit) {
					map->frame[map->count].size = loader_eh_frame_64_size;
				}
				else {
					map->frame[map->count].size = loader_eh_frame_32_size;
				}
				
				if (map->frame[map->count].id == 0) {
					// CIE record
					map->frame[map->count].type = loader_eh_frame_cie_type;
					frame_length += SDMSTParseCIEFrame(&(map->frame[map->count]), frame_offset);
				}
				else {
					// FDE record
					map->frame[map->count].type = loader_eh_frame_fde_type;
					frame_length += SDMSTParseFDEFrame(&(map->frame[map->count]), frame_offset);
					
				}
				
				position += frame_length;
				
				map->count++;
			}
		}
	}
	
	last_cie = NULL;
	
	return map;
}

uint64_t SDMSTParseCIEFrame(struct loader_eh_frame *frame, Pointer frame_offset) {
	uint64_t frame_length = 0;
	
	switch (frame->type) {
		case loader_eh_frame_invalid_size: {
			break;
		}
		case loader_eh_frame_32_size: {
			break;
		}
		case loader_eh_frame_64_size: {
			break;
		}
		default: {
			break;
		}
	}
	
	last_cie = frame;
	
	return frame_length;
}

uint64_t SDMSTParseFDEFrame(struct loader_eh_frame *frame, Pointer frame_offset) {
	uint64_t frame_length = 0;
	
	switch (frame->type) {
		case loader_eh_frame_invalid_size: {
			break;
		}
		case loader_eh_frame_32_size: {
			uint32_t pc_begin = 0;
			frame_offset = read_uint32(frame_offset, &pc_begin);
			frame->fde.pc_begin = pc_begin;
			frame_length += sizeof(uint32_t);
			
			uint32_t pc_range = 0;
			frame_offset = read_uint32(frame_offset, &pc_range);
			frame->fde.pc_range = pc_range;
			frame_length += sizeof(uint32_t);
			
			break;
		}
		case loader_eh_frame_64_size: {
			uint64_t pc_begin = 0;
			frame_offset = read_uint64(frame_offset, &pc_begin);
			frame->fde.pc_begin = pc_begin;
			frame_length += sizeof(uint64_t);
			
			uint64_t pc_range = 0;
			frame_offset = read_uint64(frame_offset, &pc_range);
			frame->fde.pc_range = pc_range;
			frame_length += sizeof(uint64_t);
			
			break;
		}
		default: {
			break;
		}
	}
	
	
	
	return frame_length;
}

#endif
