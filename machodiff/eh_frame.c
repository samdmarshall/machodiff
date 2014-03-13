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

#define kLength (sizeof(uint32_t))
#define kExtendedLength (sizeof(uint64_t) + kLength)

uint64_t SDMSTParseCIEFrame(struct loader_eh_frame *frame, Pointer frame_offset);

uint64_t SDMSTParseFDEFrame(struct loader_eh_frame *frame, Pointer frame_offset);

void SDMSTAppendAugDataOrdering(struct loader_eh_frame_aug_array *array, uint8_t type);

struct loader_eh_frame_map* SDMSTParseCallFrame(CoreRange frame, bool is64bit) {
	struct loader_eh_frame_map *map = calloc(1, sizeof(struct loader_eh_frame_map));
	map->frame = calloc(1, sizeof(struct loader_eh_frame));
	map->count = 0;
	
	uint32_t cie_counter = 0;
	
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
					map->frame[map->count].extended_length = 0;
				}
				else {
					map->frame[map->count].length = k32BitMask;
					uint64_t extended_length = 0;
					frame_offset = read_uint64(frame_offset, &extended_length);
					map->frame[map->count].extended_length = extended_length;
					frame_length += sizeof(uint64_t);
				}
				
				map->frame[map->count].identifier_ptr = frame_offset;

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
					
					cie_counter = map->count;
				}
				else {
					// FDE record
					map->frame[map->count].type = loader_eh_frame_fde_type;
					map->frame[map->count].fde.relevant_cie = &(map->frame[cie_counter]);
					frame_length += SDMSTParseFDEFrame(&(map->frame[map->count]), frame_offset);
					frame_length -= length_size;
				}
				
				position += frame_length;
				
				frame_offset = (Pointer)PtrAdd(frame_offset, frame_length);
				
				map->count++;
			}
		}
	}
	
	return map;
}

void SDMSTAppendAugDataOrdering(struct loader_eh_frame_aug_array *array, uint8_t type) {
	if (array->aug_order_length == 0) {
		array->aug_order = calloc(array->aug_order_length+1, sizeof(uint8_t));
	} else {
		array->aug_order = realloc(array->aug_order, (array->aug_order_length+1)*sizeof(uint8_t));
	}
	array->aug_order[array->aug_order_length] = type;
	array->aug_order_length++;
}

uint64_t SDMSTParseCIEFrame(struct loader_eh_frame *frame, Pointer frame_offset) {
	uint64_t frame_length = 0;
	
	uint8_t length_size = (frame->length == k32BitMask ? kExtendedLength : kLength);
	
	frame->cie.order_array.aug_order_length = 0;
	
	uint8_t version = 0;
	frame_offset = read_uint8(frame_offset, &version);
	frame->cie.version = version;
	frame_length += sizeof(uint8_t);
	
	frame->cie.aug_string = Ptr(frame_offset);
	char *data = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_data);
	if (data) {
		frame->cie.aug_string_type |= loader_eh_frame_aug_data;
	}

	char *ehdata = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_ehdata);
	if (ehdata) {
		frame->cie.aug_string_type |= loader_eh_frame_aug_ehdata;
		SDMSTAppendAugDataOrdering(&frame->cie.order_array, loader_eh_frame_aug_order_ehdata);
	}
	else {
		frame->cie.order_array.aug_order = NULL;
		frame->cie.order_array.aug_order_length = 0;
	}
	
	if ((frame->cie.aug_string_type & loader_eh_frame_aug_data) == loader_eh_frame_aug_data) {
		char *lsda = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_lsda);
		if (lsda) {
			frame->cie.aug_string_type |= loader_eh_frame_aug_lsda;
			SDMSTAppendAugDataOrdering(&frame->cie.order_array, loader_eh_frame_aug_order_ldsa);
		}
		else {
			frame->cie.lsda_pointer_encoding = 0;
		}
		
		char *person = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_person);
		if (person) {
			frame->cie.aug_string_type |= loader_eh_frame_aug_person;
			SDMSTAppendAugDataOrdering(&frame->cie.order_array, loader_eh_frame_aug_order_person);
		}
		else {
			frame->cie.personality_pointer = 0;
			frame->cie.personality_encoding = 0;
			frame->cie.personality_routine = 0;
		}
		
		char *encode = strstr(Ptr(frame->cie.aug_string), kLoader_EH_FRAME_aug_encode);
		if (encode) {
			frame->cie.aug_string_type |= loader_eh_frame_aug_encode;
			SDMSTAppendAugDataOrdering(&frame->cie.order_array, loader_eh_frame_aug_order_encode);
		}
		else {
			frame->cie.fde_pointer_encoding = 0;
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
		
		Pointer aug_ptr = frame->cie.aug_data;
		for (uint8_t index = 0; index < frame->cie.order_array.aug_order_length; index++) {
			switch (frame->cie.order_array.aug_order[index]) {
				case loader_eh_frame_aug_order_ehdata: {
					break;
				}
				case loader_eh_frame_aug_order_person: {
					uint8_t person_encoding = 0;
					aug_ptr = read_uint8(aug_ptr, &person_encoding);
					frame->cie.personality_encoding = person_encoding;
					
					uint32_t person_routine = 0;
					aug_ptr = read_uint32(aug_ptr, &person_routine);
					frame->cie.personality_routine = person_routine;
					
					break;
				}
				case loader_eh_frame_aug_order_ldsa: {
					uint8_t ldsa_encoding = 0;
					aug_ptr = read_uint8(aug_ptr, &ldsa_encoding);
					frame->cie.lsda_pointer_encoding = ldsa_encoding;
					break;
				}
				case loader_eh_frame_aug_order_encode: {
					uint8_t fde_encoding = 0;
					aug_ptr = read_uint8(aug_ptr, &fde_encoding);
					frame->cie.fde_pointer_encoding = fde_encoding;
					break;
				}
				default: {
					break;
				}
			}
		}
		
	}
	
	frame->cie.initial_instructions = frame_offset;
	frame->cie.instructions_length = (frame->length == k32BitMask ? frame->extended_length : frame->length) - frame_length - length_size;
	frame_length += frame->cie.instructions_length;
	
	memset(&(frame->fde), 0, sizeof(struct loader_eh_frame_fde));
	
	return frame_length;
}

uint64_t SDMSTDecodePC_Begin(struct loader_eh_frame *frame) {
	uint64_t pc_begin = frame->fde.pc_begin;
	if ((frame->fde.relevant_cie->cie.aug_string_type & loader_eh_frame_aug_encode) == loader_eh_frame_aug_encode) {
		uint8_t encoding = frame->fde.relevant_cie->cie.fde_pointer_encoding;
		if (encoding == DW_EH_PE_absptr) {
			pc_begin = frame->fde.pc_begin;
		}
		else if ((encoding & DW_EH_PE_pcrel) == DW_EH_PE_pcrel) {
			pc_begin = ((uint64_t)frame->fde.pc_begin_ptr - TwosComp64(frame->fde.pc_begin));
		}
		else if ((encoding & DW_EH_PE_textrel) == DW_EH_PE_textrel) {
			
		}
		else if ((encoding & DW_EH_PE_datarel) == DW_EH_PE_datarel) {
			
		}
		else if ((encoding & DW_EH_PE_funcrel) == DW_EH_PE_funcrel) {
			
		}
		else if ((encoding & DW_EH_PE_aligned) == DW_EH_PE_aligned) {
			
		}
		
		if ((encoding & DW_EH_PE_indirect) == DW_EH_PE_indirect) {
			
		}
	}
	return pc_begin;
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
			frame->fde.pc_begin_ptr = frame_offset;
			frame_offset = read_uint32(frame_offset, &pc_begin);
			frame->fde.pc_begin = pc_begin;
			frame_length += sizeof(uint32_t);
			
			frame->fde.pc_begin = SDMSTDecodePC_Begin(frame);
			
			uint32_t pc_range = 0;
			frame_offset = read_uint32(frame_offset, &pc_range);
			frame->fde.pc_range = pc_range;
			frame_length += sizeof(uint32_t);
			
			break;
		}
		case loader_eh_frame_64_size: {
			uint64_t pc_begin = 0;
			frame->fde.pc_begin_ptr = frame_offset;
			frame_offset = read_uint64(frame_offset, &pc_begin);
			frame->fde.pc_begin = pc_begin;
			frame_length += sizeof(uint64_t);
			
			frame->fde.pc_begin = SDMSTDecodePC_Begin(frame);
			
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
	
	if ((frame->cie.aug_string_type & loader_eh_frame_aug_data) == loader_eh_frame_aug_data) {
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
	
	memset(&(frame->cie), 0, sizeof(struct loader_eh_frame_cie));
	
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
				Pointer result_offset = (Pointer)((uint64_t)text_section->info.offset);
				uint64_t addr_offset = text_section->position.addr - text_segment->data.vm_position.addr;
				if (text_section->info.offset != addr_offset) {
					result_offset = PtrCast(addr_offset, Pointer);
				}
				result.offset = (uint64_t)PtrAdd((uint64_t)result_offset, header_offset);
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
