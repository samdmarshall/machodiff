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

struct loader_diff_symbol {
	char *name;
	uintptr_t offset;
	struct loader_binary *binary;
} ATR_PACK;

struct loader_diff {
	struct loader_diff_symbol *symbol;
	uint32_t name_count;
} ATR_PACK;

struct loader_binary * SDMLoadTarget(char *path, uint8_t type);

void SDMPerformComparison(struct loader_binary *input_one, struct loader_binary *input_two, char *output_path);

#endif
