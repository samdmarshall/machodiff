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

struct loader_binary * SDMLoadTarget(char *path, uint8_t type) {
	printf("%s:\n",path);
	struct loader_binary *input = SDMLoadBinaryWithPath(path, type);
	printf("\n");
	return input;
}

void SDMPerformComparison(struct loader_binary *input_one, struct loader_binary *input_two, char *output_path) {
	
}

#endif
