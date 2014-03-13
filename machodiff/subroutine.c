//
//  subroutine.c
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_subroutine_c
#define machodiff_subroutine_c

#include "subroutine.h"
#include "link_include.h"
#include "loader.h"

#define Intel_x86_32bit_StackSetupLength 0x3
static uint8_t Intel_x86_32bit_StackSetup[Intel_x86_32bit_StackSetupLength] = {0x55, 0x89, 0xe5};

#define Intel_x86_64bit_StackSetupLength 0x4
static uint8_t Intel_x86_64bit_StackSetup[Intel_x86_64bit_StackSetupLength] = {0x55, 0x48, 0x89, 0xe5};

void SDMSTFindSubroutines(struct loader_binary *binary) {
	bool hasLCFunctionStarts = false;
	binary->map->subroutine_map->subroutine = (struct loader_subroutine *)calloc(1, sizeof(struct loader_subroutine));
	binary->map->subroutine_map->count = 0;
	uint32_t textSections = 0, flags = 0;
	uint64_t pageZero = 0, size = 0, address = 0;
	uintptr_t textSectionOffset = (uintptr_t)(binary->map->segment_map->text);
	
	if (SDMBinaryIs64Bit(binary->header)) {
		textSections = ((struct loader_segment_64 *)(binary->map->segment_map->text))->info.nsects;
		pageZero = ((struct loader_segment_64 *)(binary->map->segment_map->text))->data.vm_position.addr;
	}
	else {
		textSections = ((struct loader_segment_32 *)(binary->map->segment_map->text))->info.nsects;
		pageZero = ((struct loader_segment_32 *)(binary->map->segment_map->text))->data.vm_position.addr;
	}
	
	pageZero += binary->file_offset;
	uint64_t binaryOffset = pageZero;
	
	uint64_t memOffset = 0;
	if (binary->memory_ref) {
		memOffset = (uint64_t)_dyld_get_image_vmaddr_slide(binary->image_index);
	}
	else {
		memOffset = (uint64_t)(binary->header) - binaryOffset;
	}
	
	if (SDMBinaryIs64Bit(binary->header)) {
		flags = ((struct loader_section_64 *)(textSectionOffset))->info.flags;
		size = ((struct loader_section_64 *)(textSectionOffset))->position.size;
		
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_64 *)textSectionOffset)->position.addr);
	}
	else {
		flags = ((struct loader_section_32 *)(textSectionOffset))->info.flags;
		size = ((struct loader_section_32 *)(textSectionOffset))->position.size;
		
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_32 *)textSectionOffset)->position.addr);
	}
	
	if (SDMBinaryIs64Bit(binary->header)) {
		textSectionOffset += sizeof(struct loader_segment_64);
	}
	else {
		textSectionOffset += sizeof(struct loader_segment_32);
	}
	
	if (binary->map->function_start) {
		hasLCFunctionStarts = true;
		uintptr_t functionOffset = (uintptr_t)(binary->map->function_start->position.addr+memOffset+pageZero);
		Pointer functionPointer = (Pointer)functionOffset;
		while ((uintptr_t)functionPointer < (functionOffset + binary->map->function_start->position.size)) {
			functionPointer = SDMSTFindFunctionAddress(&functionPointer, binary);
		}
	}
	if (binary->header->arch.cputype == CPU_TYPE_X86_64 || binary->header->arch.cputype == CPU_TYPE_I386) {
		for (uint32_t i = 0; i < textSections; i++) {
			if (hasLCFunctionStarts && binary->map->subroutine_map->count) {
				for (uint32_t j = 0; j < binary->map->subroutine_map->count; j++) {
					if (binary->map->subroutine_map->subroutine[j].section_offset == k32BitMask) {
						uint64_t subOffset = memOffset+binary->map->subroutine_map->subroutine[j].offset;
						if (subOffset < (address+size)) {
							binary->map->subroutine_map->subroutine[j].section_offset = textSectionOffset;
							//binary->map->subroutine_map->subroutine[j].offset = (uintptr_t)(memOffset+binary->map->subroutine_map->subroutine[j].offset);
						}
					}
				}
			}
			else {
				// SDM: Fall back on manually parsing it if we cannot find the subroutine mappings...
				Dl_info info;
				uint64_t loaded;
				if (binary->memory_ref) {
					loaded = (uint64_t)dladdr((void*)(address), &info);
				}
				else {
					loaded = address;
				}
				if (loaded) {
					if (((flags & S_REGULAR)==0) && ((flags & S_ATTR_PURE_INSTRUCTIONS) || (flags & S_ATTR_SOME_INSTRUCTIONS))) {
						uint64_t offset = 0;
						bool isIntel64bitArch = (SDMBinaryIs64Bit(binary->header) && binary->header->arch.cputype == CPU_TYPE_X86_64);
						if (size) {
							while (offset < (size - (isIntel64bitArch ? Intel_x86_64bit_StackSetupLength : Intel_x86_32bit_StackSetupLength))) {
								uint32_t result = 0;
								if (isIntel64bitArch) {
									result = (uint32_t)memcmp((void*)(address+offset), &(Intel_x86_64bit_StackSetup[0]), Intel_x86_64bit_StackSetupLength);
								}
								else {
									result = (uint32_t)memcmp((void*)(address+offset), &(Intel_x86_32bit_StackSetup[0]), Intel_x86_32bit_StackSetupLength);
								}
								if (!result) {
									char *buffer = calloc(1024, sizeof(char));
									binary->map->subroutine_map->subroutine = realloc(binary->map->subroutine_map->subroutine, (sizeof(struct loader_subroutine)*(binary->map->subroutine_map->count+1)));
									struct loader_subroutine *subroutine = (struct loader_subroutine *)calloc(1, sizeof(struct loader_subroutine));
									subroutine->offset = (uintptr_t)(address+offset);
									
									sprintf(buffer, kSubFormatter, subroutine->offset);
									subroutine->name = calloc(1024, sizeof(char));
									sprintf(subroutine->name, kSubName, subroutine->offset);
									
									subroutine->section_offset = textSectionOffset;
									
									memcpy(&(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count]), subroutine, sizeof(struct loader_subroutine));
									free(subroutine);
									free(buffer);
									binary->map->subroutine_map->count++;
								}
								offset++;
							}
						}
					}
				}
				else {
					printf("Image for address (%08llx) is not loaded\n",address);
				}
			}
			textSectionOffset += (SDMBinaryIs64Bit(binary->header) ? sizeof(struct loader_section_64) : sizeof(struct loader_section_32));
		}
		
		if (binary->map->subroutine_map->count == 0) {
			// SDM: checking eh_frame for symbols
			if (binary->map->frame_map && binary->map->frame_map->count) {
				for (uint32_t index = 0; index < binary->map->frame_map->count; index++) {
					struct loader_eh_frame *frame = &(binary->map->frame_map->frame[index]);
					if (frame->type == loader_eh_frame_fde_type) {
						SDMSTCreateSubtroutineForFrame(binary, frame);
					}
				}
			}
		}
		
		// SDM: we check this again because there might not be an eh_frame was part of the binary, if there is then we just want to map the methods to the known subroutines
		if (binary->map->subroutine_map->count == 0) {
			// SDM: checking objc for methods
			if (binary->objc->clsCount) {
				for (uint32_t index = 0; index < binary->objc->clsCount; index++) {
					struct loader_objc_class *class = &(binary->objc->cls[index]);
					SDMSTCreateSubroutinesForClass(binary, class);
					SDMSTCreateSubroutinesForClass(binary, class->superCls);
				}
			}
		}
		printf("Found %i subroutines\n",binary->map->subroutine_map->count);
	}
}

void SDMSTCreateSubtroutineForFrame(struct loader_binary *binary, struct loader_eh_frame *frame) {
	
	uint64_t subroutine_offset = ((uint64_t)frame->fde.pc_begin - ((uint64_t)binary->header));
	
	char *buffer = calloc(1024, sizeof(char));
	binary->map->subroutine_map->subroutine = realloc(binary->map->subroutine_map->subroutine, ((binary->map->subroutine_map->count+1)*sizeof(struct loader_subroutine)));
	struct loader_subroutine *subroutine = (struct loader_subroutine *)calloc(1, sizeof(struct loader_subroutine));
	subroutine->offset = (uintptr_t)(subroutine_offset);
	
	sprintf(buffer, kSubFormatter, subroutine->offset);
	subroutine->name = calloc(1024, sizeof(char));
	sprintf(subroutine->name, kSubName, subroutine->offset);
	
	subroutine->section_offset = k32BitMask;
	
	memcpy(&(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count]), subroutine, sizeof(struct loader_subroutine));
	free(subroutine);
	free(buffer);
	binary->map->subroutine_map->count++;
}


struct loader_subroutine* SDMFindSubroutineFromName(struct loader_binary *binary, char *name) {
	for (uint32_t index = 0; index < binary->map->subroutine_map->count; index++) {
		if (strcmp(name, binary->map->subroutine_map->subroutine[index].name) == 0) {
			return &(binary->map->subroutine_map->subroutine[index]);
		}
	}
	return NULL;
}

CoreRange SDMSTRangeOfSubroutine(struct loader_subroutine *subroutine, struct loader_binary *binary) {
	CoreRange range = CoreRangeCreate(0, 0);
	if (subroutine) {
		for (uint32_t i = 0; i < binary->map->subroutine_map->count; i++) {
			if (binary->map->subroutine_map->subroutine[i].offset == subroutine->offset) {
				range.offset = (uintptr_t)(subroutine->offset);
				uint32_t next = i + 1;
				if (next < binary->map->subroutine_map->count) {
					range.length = ((binary->map->subroutine_map->subroutine[next].offset) - range.offset);
					
					if (binary->map->frame_map && binary->map->frame_map->count) {
						for (uint32_t index = 0; index < binary->map->frame_map->count; index++) {
							struct loader_eh_frame *frame = &(binary->map->frame_map->frame[index]);
							if (frame->type == loader_eh_frame_fde_type) {
								uint64_t offset = ((uint64_t)frame->fde.pc_begin - ((uint64_t)binary->header));
								if (offset == range.offset) {
									range.length = frame->fde.pc_range;
									break;
								}
							}
						}
					}
				}
				else {
					if (subroutine->section_offset != k32BitMask) {
						uint64_t size, address;
						uint64_t memOffset = SDMCalculateVMSlide(binary);
						if (SDMBinaryIs64Bit(binary->header)) {
							size = ((struct loader_section_64 *)(subroutine->section_offset))->position.size;
							address = ((struct loader_section_64 *)(subroutine->section_offset))->position.addr + memOffset;
						}
						else {
							size = ((struct loader_section_32 *)(subroutine->section_offset))->position.size;
							address = ((struct loader_section_32 *)(subroutine->section_offset))->position.addr + memOffset;
						}
						range.length = ((address+size) - range.offset);
					}
				}
				break;
			}
		}
	}
	return range;
}

#endif
