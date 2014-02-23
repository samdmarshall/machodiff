//
//  compare.h
//  machodiff
//
//  Created by Sam Marshall on 2/23/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_compare_h
#define machodiff_compare_h

#include "diff.h"

void SDMDiffAddSymbols(struct loader_diff *diff, struct loader_binary *input_one, struct loader_binary *input_two);
bool SDMCompareSymbol(struct loader_diff_symbol *symbol, CoreRange range_one, struct loader_binary *input_one, CoreRange range_two, struct loader_binary *input_two);

#endif
