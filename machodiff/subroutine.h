//
//  subroutine.h
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_subroutine_h
#define machodiff_subroutine_h

#include "loader_type.h"
#include "objc.h"
#include "eh_frame.h"
#include "diff_type.h"

void SDMSTFindSubroutines(struct loader_binary *binary);

void SDMSTCreateSubtroutineForFrame(struct loader_binary *binary, struct loader_eh_frame *frame);
void SDMSTCreateSubroutinesForClass(struct loader_binary *binary, struct loader_objc_class *class);

struct loader_subroutine* SDMFindSubroutineFromName(struct loader_binary *binary, char *name);

CoreRange SDMSTRangeOfSubroutine(struct loader_subroutine *subroutine, struct loader_binary *binary);

#endif
