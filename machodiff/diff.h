//
//  diff.h
//  machodiff
//
//  Created by Sam Marshall on 2/22/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_diff_h
#define machodiff_diff_h

#include "loader.h"
#include "cmap.h"

struct loader_diff_symbol_imp {
	struct loader_binary *binary;
	struct loader_symbol *symbol;
	struct loader_subroutine *subroutine;
} ATR_PACK;

struct loader_diff_symbol {
	char *name;
	bool match;
	struct loader_diff_symbol_imp input_one;
	struct loader_diff_symbol_imp input_two;
} ATR_PACK;

struct loader_symboL_index {
	char *symbol_name;
} ATR_PACK;

struct loader_diff {
	cmap_str map;
	struct loader_symboL_index *index;
	uint32_t index_count;
} ATR_PACK;

bool SDMMakeNewFolderAtPath(char *path, mode_t mode);

struct loader_binary * SDMLoadTarget(char *path, uint8_t type);

void SDMPerformComparison(struct loader_binary *input_one, struct loader_binary *input_two, char *output_path);

void SDMDiffRelease(struct loader_diff *diff);

#endif
