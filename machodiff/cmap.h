//
//  cmap.h
//  loader
//
//  Created by Sam Marshall on 2/19/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef loader_cmap_h
#define loader_cmap_h

#include <sys/types.h>

typedef void* cmap;
typedef void* cmap_str;

#if __cplusplus
extern "C" {
#endif
	
	extern cmap cmap_new(void);
	extern void cmap_free(cmap *amap);
	extern void* cmap_objectForKey(cmap *amap, void *key);
	extern void cmap_setObjectForKey(cmap *amap, void *key, void *value);
	extern size_t cmap_count(cmap *amap);
	
	
	extern cmap_str cmap_str_new(void);
	extern void cmap_str_free(cmap_str *amap);
	extern void* cmap_str_objectForKey(cmap_str *amap, const char *key);
	extern void cmap_str_setObjectForKey(cmap_str *amap, const char *key, void *value);
	extern size_t cmap_str_count(cmap_str *amap);
	
#if __cplusplus
};
#endif

#endif
