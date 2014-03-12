//
//  loader.c
//  loader
//
//  Created by Sam Marshall on 2/17/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef loader_loader_c
#define loader_loader_c

#pragma mark -
#pragma mark #include
#include "loader.h"
#include <mach-o/dyld.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "arch.h"
#include "reader.h"
#include "subroutine.h"
#include "map.h"

#pragma mark -
#pragma mark Private Function Definitions

uint64_t SDMCalculateVMSlide(struct loader_binary * binary) {
	uint64_t slide = k64BitMask;
	if (binary->image_index < _dyld_image_count()) {
		slide = (uint64_t)_dyld_get_image_vmaddr_slide(binary->image_index);
	}
	else {
		slide = 0;
	}
	return slide;
}

bool SDMIsBinaryLoaded(char *path, struct loader_binary * binary) {
	bool isLoaded = false;
	uint32_t count = _dyld_image_count();
	for (uint32_t index = 0; index < count; index++) {
		const char *image_name = _dyld_get_image_name(index);
		if (strncmp(image_name, path, strlen(image_name)) == 0) {
			isLoaded = true;
			binary->image_index = index;
			binary->header = PtrCast(_dyld_get_image_header(index), struct loader_generic_header *);
			break;
		}
	}
	return isLoaded;
}

uint64_t SDMComputeFslide(struct loader_segment_map * segment_map, bool is64Bit) {
	uint64_t fslide = 0;
	if (is64Bit) {
		struct loader_segment_64 *text_segment = PtrCast(segment_map->text, struct loader_segment_64 *);
		struct loader_segment_64 *link_segment = PtrCast(segment_map->link, struct loader_segment_64 *);
		if (link_segment != NULL && text_segment != NULL) {
			fslide = (uint64_t)(link_segment->data.vm_position.addr - text_segment->data.vm_position.addr) - link_segment->data.file_position.addr;
		}
	}
	else {
		struct loader_segment_32 *text_segment = PtrCast(segment_map->text, struct loader_segment_32 *);
		struct loader_segment_32 *link_segment = PtrCast(segment_map->link, struct loader_segment_32 *);
		if (link_segment != NULL && text_segment != NULL) {
			fslide = (uint64_t)(link_segment->data.vm_position.addr - text_segment->data.vm_position.addr) - link_segment->data.file_position.addr;
		}
	}
	return fslide;
}

uint8_t SDMGetFatBinaryEndianness(uint32_t magic) {
	uint8_t type = loader_endian_invalid_type;
	if (magic == FAT_MAGIC) {
		type = loader_endian_big_type;
	}
	if (SDMSwapEndian32(magic) == FAT_MAGIC) {
		type = loader_endian_little_type;
	}
	return type;
}

bool SDMLoadBinaryFromFile(struct loader_binary *binary, char *path, uint8_t target_arch) {
	bool result = false;
	if (path && target_arch != loader_arch_invalid_type) {
		struct stat fs;
		int status = stat(path, &fs);
		if (status == 0) {
			int fd = open(path, O_RDONLY);
			if (fd != 0xff) {
				off_t size = fs.st_size;
				off_t offset = 0;
				struct loader_magic *binary_magic = mmap(NULL, (unsigned long)size, PROT_READ, MAP_PRIVATE, fd, offset);
				
				if (binary_magic->magic == FAT_MAGIC || SDMSwapEndian32(binary_magic->magic) == FAT_MAGIC) {
					struct loader_fat_header *fat_binary = (struct loader_fat_header *)binary_magic;
					binary->endian_type = SDMGetFatBinaryEndianness(fat_binary->magic.magic);
					offset += sizeof(struct loader_fat_header);
					uint32_t arch_count = EndianFix(binary->endian_type, fat_binary->n_arch);
					for (uint32_t arch_index = 0; arch_index < arch_count; arch_index++) {
						struct loader_arch_header *arch_header = (struct loader_arch_header *)PtrAdd(fat_binary, offset);
						bool found_arch = SDMMatchArchToCPU(&(arch_header->arch), target_arch, binary->endian_type);
						if (found_arch) {
							uint32_t fat_size = EndianFix(binary->endian_type, arch_header->size)
							uint32_t fat_offset = EndianFix(binary->endian_type, arch_header->offset);
							binary->header = (struct loader_generic_header *)calloc(fat_size, sizeof(char));
							binary->file_offset = fat_offset;
							lseek(fd, fat_offset, SEEK_SET);
							read(fd, binary->header, fat_size);
							result = true;
							break;
						}
						offset += sizeof(struct loader_arch_header);
					}
				}
				else {
					struct loader_generic_header *slim_binary = (struct loader_generic_header *)binary_magic;
					
					bool found_arch = SDMMatchArchToCPU(&(slim_binary->arch), target_arch, binary->endian_type);
					if (found_arch) {
						binary->header = (struct loader_generic_header *)calloc((uint32_t)size, sizeof(char));
						binary->file_offset = 0;
						lseek(fd, offset, SEEK_SET);
						read(fd, binary->header, (uint32_t)size);
						result = true;
					}
				}
				
				munmap(binary_magic, (size_t)size);
			}
			close(fd);
		}
	}
	return result;
}

uint8_t SDMIsBinaryFat(char *path) {
	uint8_t fat = loader_binary_arch_invalid_type;
	struct loader_binary *binary = calloc(1, sizeof(struct loader_binary));
	bool in_memory = SDMIsBinaryLoaded(path, binary);
	if (in_memory) {
		fat = loader_binary_arch_slim_type;
	}
	else {
		struct stat fs;
		int status = stat(path, &fs);
		if (status == 0) {
			int fd = open(path, O_RDONLY);
			if (fd) {
				uint32_t header = 0xdeadbeef;
				read(fd, &header, sizeof(uint32_t));
				if (header == FAT_CIGAM || header == FAT_MAGIC) {
					fat = loader_binary_arch_fat_type;
				}
				else {
					fat = loader_binary_arch_slim_type;
				}
			}
			close(fd);
		}
	}
	SDMReleaseBinary(binary);
	return fat;
}

bool SDMBinaryIs64Bit(struct loader_generic_header *header) {
	bool isCPU64Bit = ((header->arch.cputype & CPU_ARCH_ABI64) == CPU_ARCH_ABI64);
	bool isMagic64Bit = (header->magic.magic == MH_MAGIC_64 || header->magic.magic == MH_CIGAM_64);
	return (isCPU64Bit && isMagic64Bit);
}

struct loader_binary * SDMLoadBinaryWithPath(char *path, uint8_t target_arch) {
	struct loader_binary *binary = calloc(1, sizeof(struct loader_binary));
	bool is_loaded = false;
	binary->memory_ref = SDMIsBinaryLoaded(path, binary);
	binary->name = path;
	if (binary->memory_ref) {
		is_loaded = true;
	}
	else {
		binary->image_index = k32BitMask;
		is_loaded = SDMLoadBinaryFromFile(binary, path, target_arch);
	}
	if (is_loaded) {
		binary->map = SDMCreateBinaryMap(binary->header);
		
		bool loadedObjc = false;
		bool is64Bit = SDMBinaryIs64Bit(binary->header);
		if (is64Bit) {
			loadedObjc = SDMMapObjcClasses64(binary);
		}
		else {
			loadedObjc = SDMMapObjcClasses32(binary);
		}
		SDMSTFindSubroutines(binary);
		SDMGenerateSymbols(binary);
		SDMSTMapSymbolsToSubroutines(binary);
		if (loadedObjc) {
			SDMSTMapMethodsToSubroutines(binary);
		}
	}
	else {
		SDMReleaseBinary(binary);
		binary = NULL;
	}
	return binary;
}

void SDMReleaseBinary(struct loader_binary *binary) {
	if (binary) {
		if (binary->memory_ref == false) {
			free(binary->header);
		}
		
		SDMReleaseMap(binary->map);
		
		free(binary);
	}
}

#endif