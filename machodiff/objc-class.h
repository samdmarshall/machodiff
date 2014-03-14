//
//  objc-class.h
//  machodiff
//
//  Created by Sam Marshall on 3/14/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_objc_class_h
#define machodiff_objc_class_h

#include "util.h"

struct loader_objc_class_ivar {
	uint8_t type;
	char *name;
} ATR_PACK;

struct loader_objc_class_file {
	char *name;
	struct loader_objc_class_ivar *ivar;
	uint32_t ivar_count;
} ATR_PACK;

struct loader_objc_class_map {
	struct loader_objc_class_file *item;
	uint32_t class_count;
} ATR_PACK;

#endif
