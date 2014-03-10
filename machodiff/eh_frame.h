//
//  eh_frame.h
//  machodiff
//
//  Created by Sam Marshall on 3/9/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_eh_frame_h
#define machodiff_eh_frame_h

#include "util.h"

struct loader_eh_frame_header {
	uint32_t length;
	uint32_t id;
} ATR_PACK;

struct loader_eh_frame_cie_64 {
	struct loader_eh_frame_header header;
} ATR_PACK;

struct loader_eh_frame_cie_32 {
	struct loader_eh_frame_header header;
} ATR_PACK;

struct loader_eh_frame_fde_64_info {
	uint64_t pc_begin;
	uint64_t pc_range;
} ATR_PACK;

struct loader_eh_frame_fde_32_info {
	uint32_t pc_begin;
	uint32_t pc_range;
} ATR_PACK;

struct loader_eh_frame_fde_64 {
	struct loader_eh_frame_header header;
	struct loader_eh_frame_fde_64_info info;
} ATR_PACK;

struct loader_eh_frame_fde_32 {
	struct loader_eh_frame_header header;
	struct loader_eh_frame_fde_32_info info;
} ATR_PACK;

#endif
