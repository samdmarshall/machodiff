//
//  hash.h
//  machodiff
//
//  Created by Sam Marshall on 3/14/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_hash_h
#define machodiff_hash_h

#include "util.h"

#if defined(__APPLE__)
	#define COMMON_DIGEST_FOR_OPENSSL
	#include <CommonCrypto/CommonDigest.h>
	#define SHA1 CC_SHA1
	#define SHA_DIGEST_LENGTH CC_SHA1_DIGEST_LENGTH
#else
	#include <openssl/sha.h>
#endif

#define HASH_LENGTH SHA_DIGEST_LENGTH

#define printHash(hash) printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15],hash[16],hash[17],hash[18],hash[19]);

unsigned char* StringToSHA1(char *str, uint32_t length, unsigned char output[HASH_LENGTH]);

#endif
