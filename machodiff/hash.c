//
//  hash.c
//  machodiff
//
//  Created by Sam Marshall on 3/14/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_hash_c
#define machodiff_hash_c

#include "hash.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

unsigned char* StringToSHA1(char *str, uint32_t length, unsigned char output[HASH_LENGTH]) {
	unsigned char hash[HASH_LENGTH];
	SHA1(PtrCast(str, unsigned char*), length, hash);
	memcpy(output, hash, sizeof(char[HASH_LENGTH]));
	return output;
}

#endif
