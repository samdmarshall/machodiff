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

struct loader_binary * SDMLoadTarget(char *path, uint8_t type);

void SDMPerformComparison(struct loader_binary *input_one, struct loader_binary *input_two);

#endif
