//
//  diff_type.h
//  machodiff
//
//  Created by Sam Marshall on 3/15/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_diff_type_h
#define machodiff_diff_type_h

#include "util.h"
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

#pragma mark -

struct loader_match_tree {
	CoreRange matched1;
	CoreRange matched2;
	struct loader_match_tree *parent; // in the root node this will be NULL
	struct loader_match_tree *child;
} ATR_PACK;


#endif
