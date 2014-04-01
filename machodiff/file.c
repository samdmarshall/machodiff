//
//  file.c
//  machodiff
//
//  Created by Sam Marshall on 4/1/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_file_c
#define machodiff_file_c

#include "file.h"

bool make_dir(char *path, mode_t mode) {
	bool result = false;
	struct stat st;
	if (stat(path, &st) == -1) {
		int mkdirResult = mkdir(path, mode);
		result = (mkdirResult == 0 ? true : false);
	}
	return result;
}

#endif