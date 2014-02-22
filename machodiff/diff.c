//
//  diff.c
//  machodiff
//
//  Created by Sam Marshall on 2/22/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_diff_c
#define machodiff_diff_c

#include "diff.h"
#include <string.h>

struct loader_binary * SDMLoadTarget(char *path, uint8_t type) {
	printf("%s:\n",path);
	struct loader_binary *input = SDMLoadBinaryWithPath(path, type);
	printf("\n");
	return input;
}

bool SDMNameListContainsName(struct loader_diff *diff, struct loader_subroutine *subroutine, struct loader_binary *input) {
	bool found = false;
	for (uint32_t index = 0; index < diff->name_count; index++) {
		if (strcmp(subroutine->name, diff->symbol[index].name) == 0) {
			found = (input == diff->symbol[index].binary ? (diff->symbol[index].offset == subroutine->offset) : true);
			if (found) {
				break;
			}
		}
	}
	return found;
}

void SDMDiffAddSymbols(struct loader_diff *diff, struct loader_binary *input) {
	uint32_t add_counter = 0;
	for (uint32_t index = 0; index < input->map->subroutine_map->count; index++) {
		bool has_name = SDMNameListContainsName(diff, &(input->map->subroutine_map->subroutine[index]), input);
		if (has_name == false) {
			diff->symbol = realloc(diff->symbol, sizeof(struct loader_diff_symbol)*(diff->name_count+1));
			diff->symbol[diff->name_count].name = input->map->subroutine_map->subroutine[index].name;
			diff->symbol[diff->name_count].binary = input;
			diff->symbol[diff->name_count].offset = input->map->subroutine_map->subroutine[index].offset;
			diff->name_count++;
			add_counter++;
		}
	}
}

struct loader_diff * SDMGenerateSymbolList(struct loader_binary *input_one, struct loader_binary *input_two) {
	struct loader_diff *diff = calloc(1, sizeof(struct loader_diff));
	diff->symbol = calloc(1, sizeof(struct loader_diff_symbol));
	diff->name_count = 0;
	
	SDMDiffAddSymbols(diff, input_one);
	
	SDMDiffAddSymbols(diff, input_two);
	
	printf("found %i unique symbols\n",diff->name_count);
	
	return diff;
}

void SDMPerformComparison(struct loader_binary *input_one, struct loader_binary *input_two, char *output_path) {
	struct loader_diff *diff = SDMGenerateSymbolList(input_one, input_two);
	for (uint32_t index = 0; index < diff->name_count; index++) {
		
	}
}

#endif
