//
//  file.h
//  machodiff
//
//  Created by Sam Marshall on 4/1/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_file_h
#define machodiff_file_h

#include "file_include.h"
#include <stdbool.h>

#define FWRITE_STRING_TO_FILE(a,b) fwrite(a, sizeof(char), strlen(a), b)

bool make_dir(char *path, mode_t mode);

#endif
