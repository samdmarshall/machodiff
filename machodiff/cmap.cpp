//
//  cmap.cpp
//  loader
//
//  Created by Sam Marshall on 2/19/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#include "cmap.h"
#include <unordered_map>
#include <string>

#pragma mark -
#pragma mark cmap

cmap cmap_new(void) {
    return reinterpret_cast<cmap>(new std::unordered_map<void *, void *>);
}

void cmap_free(cmap *amap) {
    delete reinterpret_cast<std::unordered_map<void *, void *> *>(amap);
}

void* cmap_objectForKey(cmap *amap, void *key) {
    std::unordered_map<void *, void *> *realmap = reinterpret_cast<std::unordered_map<void *, void *> *>(amap);
    auto i = realmap->find(key);
    
    if (i != realmap->end()) {
        return (*i).second;
    }
    return NULL;
}

void cmap_setObjectForKey(cmap *amap, void *key, void *value) {
    std::unordered_map<void *, void *> *realmap = reinterpret_cast<std::unordered_map<void *, void *> *>(amap);
    auto i = realmap->find(key);
    
    if (i != realmap->end()) {
        if (value) {
            (*i).second = value;
        } else {
            realmap->erase(i);
        }
    }
	else {
        if (value) {
            realmap->insert(std::pair<void *, void *>(key, value));
        }
    }
}

size_t cmap_count(cmap *amap) {
    return reinterpret_cast<std::unordered_map<void *, void *> *>(amap)->size();
}

#pragma mark -
#pragma mark cmap_str

cmap_str cmap_str_new(void) {
    return reinterpret_cast<cmap_str>(new std::unordered_map<std::string, void *>);
}

void cmap_str_free(cmap_str *amap) {
    delete reinterpret_cast<std::unordered_map<std::string, void *> *>(amap);
}

void* cmap_str_objectForKey(cmap_str *amap, const char *key) {
    std::unordered_map<std::string, void *> *realmap = reinterpret_cast<std::unordered_map<std::string, void *> *>(amap);
    auto i = realmap->find(std::string(key));
    
    if (i != realmap->end()) {
        return (*i).second;
    }
    return NULL;
}

void cmap_str_setObjectForKey(cmap_str *amap, const char *key, void *value) {
    std::unordered_map<std::string, void *> *realmap = reinterpret_cast<std::unordered_map<std::string, void *> *>(amap);
    auto i = realmap->find(std::string(key));
    
    if (i != realmap->end()) {
        if (value) {
            (*i).second = value;
        }
		else {
            realmap->erase(i);
        }
    }
	else {
        if (value) {
            realmap->insert(std::pair<std::string, void *>(std::string(key), value));
        }
    }
}

size_t cmap_str_count(cmap_str *amap) {
    return reinterpret_cast<std::unordered_map<std::string, void *> *>(amap)->size();
}