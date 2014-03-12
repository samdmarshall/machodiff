//
//  objc.c
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_objc_c
#define machodiff_objc_c

#include "objc.h"
#include "loader.h"
#include <dlfcn.h>
#include <string.h>
#include <mach-o/dyld.h>

void SDMSTCreateSubroutinesForClass(struct loader_binary *binary, struct loader_objc_class *class) {
	uint64_t pageZero = 0, address = 0;
	if (SDMBinaryIs64Bit(binary->header)) {
		pageZero = ((struct loader_segment_64 *)(binary->map->segment_map->text))->data.vm_position.addr;
	}
	else {
		pageZero = ((struct loader_segment_32 *)(binary->map->segment_map->text))->data.vm_position.addr;
	}
	pageZero += binary->file_offset;
	uint64_t binaryOffset = pageZero;
	uintptr_t textSectionOffset = (uintptr_t)(binary->map->segment_map->text);
	if (SDMBinaryIs64Bit(binary->header)) {
		textSectionOffset += sizeof(struct loader_segment_64);
	}
	else {
		textSectionOffset += sizeof(struct loader_segment_32);
	}
	
	uint64_t memOffset;
	if (binary->memory_ref) {
		memOffset = (uint64_t)_dyld_get_image_vmaddr_slide(binary->image_index);
	}
	else {
		memOffset = (uint64_t)(binary->header) - binaryOffset;
	}
	if (SDMBinaryIs64Bit(binary->header)) {
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_64 *)textSectionOffset)->position.addr);
	}
	else {
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_32 *)textSectionOffset)->position.addr);
	}
	
	for (uint32_t method_index = 0; method_index < class->methodCount; method_index++) {
		struct loader_objc_method *method = &(class->method[method_index]);
		
		char *buffer = calloc(1024, sizeof(char));
		binary->map->subroutine_map->subroutine = realloc(binary->map->subroutine_map->subroutine, ((binary->map->subroutine_map->count+1)*sizeof(struct loader_subroutine)));
		struct loader_subroutine *subroutine = (struct loader_subroutine *)calloc(1, sizeof(struct loader_subroutine));
		subroutine->offset = (uintptr_t)(address+(method->offset) - (address-memOffset));
		
		sprintf(buffer, kSubFormatter, subroutine->offset);
		subroutine->name = calloc(5 + (strlen(buffer)), sizeof(char));
		sprintf(subroutine->name, kSubName, subroutine->offset);
		
		subroutine->section_offset = textSectionOffset;
		
		memcpy(&(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count]), subroutine, sizeof(struct loader_subroutine));
		free(subroutine);
		free(buffer);
		binary->map->subroutine_map->count++;
		
	}
}

#endif
