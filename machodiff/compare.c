//
//  compare.c
//  machodiff
//
//  Created by Sam Marshall on 2/23/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_compare_c
#define machodiff_compare_c

#include "compare.h"
#include <unistd.h>
#include <string.h>
#include <dispatch/dispatch.h>

// SDM: this will give some variation due to the approximation in unique when parsing dynamically created block_ref symbols in a binary.
void SDMDiffAddSymbols(struct loader_diff *diff, struct loader_binary *input_one, struct loader_binary *input_two) {
//	uint32_t add_counter = 0;
//	for (uint32_t index = 0; index < input->map->subroutine_map->count; index++) {
//		bool has_name = SDMNameListContainsName(diff, &(input->map->subroutine_map->subroutine[index]), input);
//		// SDM: add a check in here for the contents of the subroutine, because names won't work when symbols are stripped.
//		if (has_name == false) {
//			diff->symbol = realloc(diff->symbol, sizeof(struct loader_diff_symbol)*(diff->name_count+1));
//			diff->symbol[diff->name_count].name = input->map->subroutine_map->subroutine[index].name;
//			diff->symbol[diff->name_count].binary = input;
//			diff->symbol[diff->name_count].offset = input->map->subroutine_map->subroutine[index].offset;
//			diff->name_count++;
//			add_counter++;
//		}
//	}
	
	for (uint32_t index = 0; index < input_one->map->subroutine_map->count; index++) {
		bool unnamed_subroutine = false;
		uintptr_t offset = 0;
		
		char *subroutine_name = input_one->map->subroutine_map->subroutine[index].name;
		if (strncmp(subroutine_name, "sub_", sizeof(char[4])) == 0) {
			unnamed_subroutine = true;
		}
		
		if (unnamed_subroutine) {
			int has_offset = sscanf(subroutine_name, "sub_%lx",&offset);
			if (has_offset == 1) {
				//printf("offset: %lx %lx\n",offset,(input_one->map->subroutine_map->subroutine[index].offset - (SDMBinaryIs64Bit(input_one->header) ? (uint64_t)input_one->header : 0)));
			}
		}
		
	}
}

bool SDMCompareSymbol(struct loader_diff_symbol *symbol, CoreRange range_one, struct loader_binary *input_one, CoreRange range_two, struct loader_binary *input_two) {
	bool status = false;
	
	Pointer ptr_subroutine_one = PtrCast(Ptr(range_one.offset), Pointer);
	
	Pointer ptr_subroutine_two = PtrCast(Ptr(range_two.offset), Pointer);
	
	if (range_one.length == range_two.length) {
		status = memcmp(ptr_subroutine_one, ptr_subroutine_two, (unsigned long)range_one.length);
	} else {
		status = true;
	}
	
	if (status) {
		
	}
	
	return status;
}

#endif
