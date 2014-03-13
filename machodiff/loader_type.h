//
//  loader_type.h
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_loader_type_h
#define machodiff_loader_type_h

#include <mach/machine.h>
#include <mach/vm_prot.h>
#include "util.h"
#include "eh_frame_type.h"

#define EndianFix(type, value) ((type == loader_endian_little_type) ? SDMSwapEndian32(value) : value);

enum loader_endian_type {
	loader_endian_invalid_type = 0,
	loader_endian_little_type,
	loader_endian_big_type,
};

enum loader_arch_type {
	loader_arch_invalid_type = 0,
	loader_arch_i386_type = 1,
	loader_arch_x86_64_type = 2,
	loader_arch_armv6_type = 3,
	loader_arch_armv7_type = 4,
	loader_arch_armv7s_type = 5,
	loader_arch_arm64_type = 6,
	loader_arch_ppc_type = 7,
	loader_arch_ppc64_type = 8,
};

enum loader_binary_arch_type {
	loader_binary_arch_invalid_type = 0,
	loader_binary_arch_slim_type = 1,
	loader_binary_arch_fat_type = 2,
};

struct loader_magic {
	uint32_t magic;
} ATR_PACK;

struct loader_arch {
	cpu_type_t cputype;
	cpu_subtype_t subtype;
} ATR_PACK;

struct loader_fat_header {
	struct loader_magic magic;
	uint32_t n_arch;
} ATR_PACK;

struct loader_arch_header {
	struct loader_arch arch;
	uint32_t offset;
	uint32_t size;
	uint32_t align;
} ATR_PACK;

struct loader_generic_header {
	struct loader_magic magic;
	struct loader_arch arch;
	uint32_t filetype;
	uint32_t ncmds;
	uint32_t sizeofcmds;
	uint32_t flags;
} ATR_PACK;

struct loader_32_header {
	struct loader_generic_header info;
} ATR_PACK;

struct loader_64_header {
	struct loader_generic_header info;
	uint32_t reserved;
} ATR_PACK;

struct loader_loadcmd {
	uint32_t cmd;
	uint32_t cmdsize;
} ATR_PACK;

struct loader_64_position {
	uint64_t addr;
	uint64_t size;
} ATR_PACK;

struct loader_32_position {
	uint32_t addr;
	uint32_t size;
} ATR_PACK;

struct loader_segment {
	struct loader_loadcmd command;
	char segname[16];
} ATR_PACK;

struct loader_segment_data_64 {
	struct loader_64_position vm_position;
	struct loader_64_position file_position;
} ATR_PACK;

struct loader_segment_data_32 {
	struct loader_32_position vm_position;
	struct loader_32_position file_position;
} ATR_PACK;

struct loader_segment_info {
	vm_prot_t maxprot;
	vm_prot_t initprot;
	uint32_t nsects;
	uint32_t flags;
} ATR_PACK;

struct loader_segment_64 {
	struct loader_segment segment;
	struct loader_segment_data_64 data;
	struct loader_segment_info info;
} ATR_PACK;

struct loader_segment_32 {
	struct loader_segment segment;
	struct loader_segment_data_32 data;
	struct loader_segment_info info;
} ATR_PACK;

struct loader_section_name {
	char sectname[16];
	char segname[16];
} ATR_PACK;

struct loader_section_info {
	uint32_t offset;
	uint32_t align;
	uint32_t reloff;
	uint32_t nreloc;
	uint32_t flags;
	uint32_t reserved1;
	uint32_t reserved2;
} ATR_PACK;

struct loader_section_64 {
	struct loader_section_name name;
	struct loader_64_position position;
	struct loader_section_info info;
	uint32_t reserved3;
} ATR_PACK;

struct loader_section_32 {
	struct loader_section_name name;
	struct loader_32_position position;
	struct loader_section_info info;
} ATR_PACK;

struct loader_generic_nlist {
	union {
		uint32_t n_strx;
	} n_un;
	uint8_t n_type;
	uint8_t n_sect;
	uint16_t n_desc;
} ATR_PACK;


struct loader_segment_map {
	struct loader_segment *text;
	struct loader_segment *link;
	struct loader_segment *objc;
} ATR_PACK;

struct loader_symbol {
	uint32_t symbol_number;
	uintptr_t offset;
	char *symbol_name;
	bool stub;
} ATR_PACK;

struct loader_symtab {
	uintptr_t *symtab;
	struct loader_symbol *symbol;
	uint64_t count;
} ATR_PACK;

struct loader_symtab_cmd {
	struct loader_loadcmd cmd;
	uint32_t symoff;
	uint32_t nsyms;
	uint32_t stroff;
	uint32_t strsize;
} ATR_PACK;

struct loader_dependency_map {
	Pointer dependency;
	uint32_t count;
} ATR_PACK;

struct loader_function_start {
	struct loader_loadcmd loadcmd;
	struct loader_32_position position;
} ATR_PACK;

struct loader_subroutine {
	uintptr_t offset;
	char *name;
	uintptr_t section_offset;
} ATR_PACK;

struct loader_subroutine_map {
	struct loader_subroutine *subroutine;
	uint32_t count;
} ATR_PACK;

struct loader_map {
	struct loader_segment_map *segment_map;
	struct loader_symtab *symbol_table;
	struct loader_dependency_map *dependency_map;
	struct loader_function_start *function_start;
	struct loader_subroutine_map *subroutine_map;
	struct loader_eh_frame_map *frame_map;
} ATR_PACK;

struct loader_binary {
	char *name;
	uint32_t image_index;
	bool memory_ref;
	uint64_t file_offset;
	uint8_t endian_type;
	struct loader_generic_header *header;
	struct loader_map *map;
	struct loader_objc_map *objc;
} ATR_PACK;


#endif
