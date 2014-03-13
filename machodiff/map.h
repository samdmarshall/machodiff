//
//  map.h
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_map_h
#define machodiff_map_h

#include "loader_type.h"
#include "objc.h"

struct loader_map *SDMCreateBinaryMap(struct loader_generic_header *header);

bool SDMMapObjcClasses32(struct loader_binary *binary);
bool SDMMapObjcClasses64(struct loader_binary *binary);

uint32_t SDMSTMapMethodsOfClassToSubroutines(struct loader_objc_class *class, struct loader_binary *binary);
void SDMSTMapMethodsToSubroutines(struct loader_binary *binary);
void SDMSTMapSymbolsToSubroutines(struct loader_binary *binary);

void SDMSTMapSubroutineSectionOffset(struct loader_binary *binary);

void SDMReleaseMap(struct loader_map *map);

#endif
