//
//  match.h
//  machodiff
//
//  Created by Sam Marshall on 3/19/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_match_h
#define machodiff_match_h

#include "util.h"
#include "diff_type.h"

struct loader_match_tree * SDMBuildMatchTree(CoreRange buffer1, CoreRange buffer2);

uint8_t SDMMatchPercentFromTree(struct loader_match_tree *tree, uint64_t total_length);
uint64_t SDMMatchLengthFromTree(struct loader_match_tree *tree);

void SDMReleaseMatchTree(struct loader_match_tree *tree);

#endif
