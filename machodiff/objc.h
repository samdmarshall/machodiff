//
//  objc.h
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_objc_h
#define machodiff_objc_h

#include "loader_type.h"
#include "objc_runtime.h"

char* SDMSTCreateNameForMethod(struct loader_objc_method *method, char *class_name);

void SDMSTCreateSubroutinesForClass(struct loader_binary *binary, struct loader_objc_class *class);

#endif
