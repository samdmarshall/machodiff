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
#include "diff_type.h"

bool SDMMakeNewFolderAtPath(char *path, mode_t mode);

struct loader_binary * SDMLoadTarget(char *path, uint8_t type);

void SDMPerformComparison(struct loader_binary *input_one, struct loader_binary *input_two, char *output_path);

void SDMDiffSymbolRelease(struct loader_diff_symbol *symbol);
void SDMDiffRelease(struct loader_diff *diff);

#endif
