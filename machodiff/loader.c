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
#include <mach-o/fat.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <mach-o/loader.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include "arch.h"
#include "reader.h"

#pragma mark -
#pragma mark Private Types

#define Intel_x86_32bit_StackSetupLength 0x3
static uint8_t Intel_x86_32bit_StackSetup[Intel_x86_32bit_StackSetupLength] = {0x55, 0x89, 0xe5};

#define Intel_x86_64bit_StackSetupLength 0x4
static uint8_t Intel_x86_64bit_StackSetup[Intel_x86_64bit_StackSetupLength] = {0x55, 0x48, 0x89, 0xe5};

#pragma mark -
#pragma mark Private Function Declaration


struct loader_map *SDMCreateBinaryMap(struct loader_generic_header *header);

bool SDMIsBinaryLoaded(char *path, struct loader_binary *binary);
bool SDMLoadBinaryFromFile(struct loader_binary *binary, char *path, uint8_t target_arch);
bool SDMMatchArchToCPU(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type);
uint8_t SDMGetFatBinaryEndianness(uint32_t magic);

uint64_t SDMCalculateVMSlide(struct loader_binary *binary);
uint64_t SDMComputeFslide(struct loader_segment_map *segment_map, bool is64Bit);

void SDMGenerateSymbols(struct loader_binary *binary);

bool SDMMapObjcClasses32(struct loader_binary *binary);
bool SDMMapObjcClasses64(struct loader_binary *binary);

Pointer SDMSTFindFunctionAddress(Pointer *fPointer, struct loader_binary *binary);
void SDMSTFindSubroutines(struct loader_binary *binary);

void SDMSTCreateSubroutinesForClass(struct loader_binary *binary, struct loader_objc_class *class);

uint32_t SDMSTMapMethodsOfClassToSubroutines(struct loader_objc_class *class, struct loader_binary *binary);
void SDMSTMapMethodsToSubroutines(struct loader_binary *binary);
void SDMSTMapSymbolsToSubroutines(struct loader_binary *binary);

bool SMDSTSymbolDemangleAndCompare(char *symFromTable, char *symbolName);

void SDMReleaseMap(struct loader_map *map);

CoreRange SDMSTEH_FramePointer(struct loader_segment *text, bool is64Bit, uint64_t header_offset);
bool SDMSTTEXTHasEH_Frame(struct loader_segment *text, bool is64Bit, uint64_t header_offset, CoreRange *eh_frame);

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

struct loader_map * SDMCreateBinaryMap(struct loader_generic_header *header) {
	struct loader_map *map = calloc(1, sizeof(struct loader_map));
	map->segment_map = calloc(1, sizeof(struct loader_segment_map));
	
	map->symbol_table = calloc(1, sizeof(struct loader_symtab));
	
	map->dependency_map = calloc(1, sizeof(struct loader_dependency_map));
	map->dependency_map->dependency = calloc(1, sizeof(uintptr_t));
	map->dependency_map->count = 0;
	
	map->subroutine_map = calloc(1, sizeof(struct loader_subroutine_map));
	
	bool is64Bit = SDMBinaryIs64Bit(header);
	struct loader_loadcmd *loadCmd = (struct loader_loadcmd *)PtrAdd(header, (is64Bit ? sizeof(struct loader_64_header) : sizeof(struct loader_32_header)));
	for (uint32_t command_index = 0; command_index < header->ncmds; command_index++) {
		switch (loadCmd->cmd) {
			case LC_SYMTAB: {
				map->symbol_table->symtab = PtrCast(loadCmd, uintptr_t *);
				break;
			};
			case LC_SEGMENT:
			case LC_SEGMENT_64: {
				struct loader_segment *segment = PtrCast(loadCmd, struct loader_segment *);
				if ((map->segment_map->text == NULL) && !strncmp(SEG_TEXT,segment->segname,sizeof(segment->segname))) {
					map->segment_map->text = segment;
					CoreRange offset = {0,0};
					bool is64bit = SDMBinaryIs64Bit(header);
					bool result = SDMSTTEXTHasEH_Frame(segment, is64bit, (uint64_t)header, &offset);
					if (result) {
						map->frame_map = SDMSTParseCallFrame(offset, is64bit);
					}
				}
				if ((map->segment_map->link == NULL) && !strncmp(SEG_LINKEDIT,segment->segname,sizeof(segment->segname))) {
					map->segment_map->link = segment;
				}
				if ((map->segment_map->objc == NULL) && !strncmp((is64Bit ? SEG_DATA : SEG_OBJC), segment->segname, sizeof(segment->segname))) {
					map->segment_map->objc = segment;
				}
				break;
			};
			case LC_LOAD_DYLIB: {
				map->dependency_map->dependency = realloc(map->dependency_map->dependency, sizeof(uintptr_t)*(map->dependency_map->count+1));
				map->dependency_map->dependency[map->dependency_map->count] = *PtrCast(loadCmd, uintptr_t *);
				map->dependency_map->count++;
				break;
			};
			case LC_FUNCTION_STARTS: {
				map->function_start = PtrCast(loadCmd, struct loader_function_start *);
				break;
			};
			default: {
				break;
			};
		}
		loadCmd = (struct loader_loadcmd *)PtrAdd(loadCmd, loadCmd->cmdsize);
	}
	return map;
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

void SDMGenerateSymbols(struct loader_binary * binary) {
	uintptr_t symbol_address = 0;
	binary->map->symbol_table->symbol = calloc(1, sizeof(struct loader_symbol));
	binary->map->symbol_table->count = 0;
	struct loader_symtab_cmd *symtab_cmd = PtrCast(binary->map->symbol_table->symtab, struct loader_symtab_cmd *);
	bool is64Bit = SDMBinaryIs64Bit(binary->header);
	uint64_t fslide = SDMComputeFslide(binary->map->segment_map, is64Bit);//(binary->memory_ref == true ? SDMComputeFslide(binary->map->segment_map, is64Bit) : 0);
	if (symtab_cmd != NULL) {
		struct loader_generic_nlist *entry = (struct loader_generic_nlist *)PtrAdd(binary->header, (symtab_cmd->symoff + fslide));
		for (uint32_t symbol_index = 0; symbol_index < symtab_cmd->nsyms; symbol_index++) {
			if (!(entry->n_type & N_STAB) && ((entry->n_type & N_TYPE) == N_SECT)) {
				char *strTable = PtrAdd(binary->header, (symtab_cmd->stroff + fslide));
				if (is64Bit) {
					uint64_t *n_value = (uint64_t *)PtrAdd(entry, sizeof(struct loader_generic_nlist));
					symbol_address = (uintptr_t)*n_value;
				}
				else {
					uint32_t *n_value = (uint32_t *)PtrAdd(entry, sizeof(struct loader_generic_nlist));
					symbol_address = (uintptr_t)*n_value;
				}
				binary->map->symbol_table->symbol = realloc(binary->map->symbol_table->symbol, sizeof(struct loader_symbol)*(unsigned long)(binary->map->symbol_table->count+0x1));
				struct loader_symbol *symbol = (struct loader_symbol *)calloc(1, sizeof(struct loader_symbol));
				if (symbol) {
					symbol->symbol_number = symbol_index;
					symbol->offset = (uintptr_t)PtrAdd(symbol_address, SDMCalculateVMSlide(binary));
					if (entry->n_un.n_strx && (entry->n_un.n_strx < symtab_cmd->strsize)) {
						symbol->symbol_name = PtrAdd(strTable, entry->n_un.n_strx);
						symbol->stub = false;
					}
					else {
						symbol->symbol_name = calloc(1 + strlen(kStubName) + GetDigitsOfNumber(binary->map->symbol_table->count), sizeof(char));
						sprintf(symbol->symbol_name, "%s%llu", kStubName, binary->map->symbol_table->count);
						symbol->stub = true;
					}
					memcpy(&(binary->map->symbol_table->symbol[binary->map->symbol_table->count]), symbol, sizeof(struct loader_symbol));
					free(symbol);
					binary->map->symbol_table->count++;
				}
			}
			entry = (struct loader_generic_nlist *)PtrAdd(entry, (sizeof(struct loader_generic_nlist) + (is64Bit ? sizeof(uint64_t) : sizeof(uint32_t))));
		}
	}
}

bool SDMMapObjcClasses32(struct loader_binary * binary) {
	bool result = (binary->map->segment_map->objc ? true : false);
	if (result) {
		struct loader_objc_map *objc_data = calloc(1, sizeof(struct loader_objc_map));
		struct loader_segment_32 *objc_segment = ((struct loader_segment_32 *)(binary->map->segment_map->objc));
		uint32_t module_count = 0;
		struct loader_section_32 *section = (struct loader_section_32 *)PtrAdd(objc_segment, sizeof(struct loader_segment_32));
		uint32_t section_count = objc_segment->info.nsects;
		struct loader_objc_module_raw *module = NULL;
		uint64_t mem_offset = SDMCalculateVMSlide(binary);
		if (binary->memory_ref == false) {
			mem_offset = (uint64_t)binary->header - 0x1000;
		}
		
		for (uint32_t index = 0; index < section_count; index++) {
			char *sectionName = Ptr(section->name.sectname);
			if (strncmp(sectionName, kObjc1ModuleInfo, sizeof(char[16])) == 0) {
				uint64_t offset = section->info.offset;
				struct loader_segment_32 *text_segment = PtrCast(binary->map->segment_map->text, struct loader_segment_32 *);
				uint64_t addr_offset = section->position.addr - text_segment->data.vm_position.addr;
				if (section->info.offset != addr_offset) {
					offset = addr_offset;
				}
				module = (struct loader_objc_module_raw *)PtrAdd(binary->header, offset);
				module_count = (section->position.size)/sizeof(struct loader_objc_module_raw);
			}
			if (strncmp(sectionName, kObjc1Class, sizeof(char[16])) == 0) {
				objc_data->classRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			if (strncmp(sectionName, kObjc1Category, sizeof(char[16])) == 0) {
				objc_data->catRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			if (strncmp(sectionName, kObjc1Protocol, sizeof(char[16])) == 0) {
				objc_data->protRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			if (strncmp(sectionName, kObjc1ClsMeth, sizeof(char[16])) == 0) {
				objc_data->clsMRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			if (strncmp(sectionName, kObjc1InstMeth, sizeof(char[16])) == 0) {
				objc_data->instMRange = CoreRangeCreate((uint32_t)((uint64_t)(section->position.addr)+(uint64_t)mem_offset), section->position.size);
			}
			section = (struct loader_section_32 *)PtrAdd(section, sizeof(struct loader_section_32));
		}
		if (module_count) {
			objc_data->cls = calloc(1, sizeof(struct loader_objc_class));
			objc_data->clsCount = 0;
			for (uint32_t index = 0; index < module_count; index++) {
				if (module[index].symtab) {
					struct loader_objc_1_symtab *symtab = (struct loader_objc_1_symtab *)PtrAdd(mem_offset, module[index].symtab);
					SDMSTObjc1CreateClassFromSymbol(objc_data, symtab, mem_offset);
				}
			}
		}
		binary->objc = objc_data;
	}
	return result;
}

bool SDMMapObjcClasses64(struct loader_binary * binary) {
	bool result = (binary->map->segment_map->objc ? true : false);
	if (result) {
		binary->objc = calloc(1, sizeof(struct loader_objc_map));
		struct loader_segment_64 *objc_segment = ((struct loader_segment_64 *)(binary->map->segment_map->objc));
		uint64_t mem_offset = SDMCalculateVMSlide(binary);
		if (binary->memory_ref == false) {
			mem_offset = (uint64_t)binary->header;
		}
		CoreRange data_range = CoreRangeCreate((uintptr_t)((uint64_t)(objc_segment->data.vm_position.addr)+((uint64_t)PtrLowPointer(mem_offset))),objc_segment->data.vm_position.size);
		struct loader_section_64 *section = (struct loader_section_64 *)PtrAdd(objc_segment, sizeof(struct loader_segment_64));
		uint32_t section_count = objc_segment->info.nsects;
		for (uint32_t index = 0; index < section_count; index++) {
			char *section_name = Ptr(section->name.sectname);
			if (strncmp(section_name, kObjc2ClassList, sizeof(char[16])) == 0) {
				binary->objc->clsCount = (uint32_t)((section->position.size)/sizeof(uint64_t));
				break;
			}
			section = (struct loader_section_64 *)PtrAdd(section, sizeof(struct loader_section_64));
		}
		if (binary->objc->clsCount) {
			binary->objc->cls = calloc(binary->objc->clsCount, sizeof(struct loader_objc_class));
			for (uint32_t index = 0; index < binary->objc->clsCount; index++) {
				uint64_t offset = section->info.offset;
				struct loader_segment_64 *text_segment = PtrCast(binary->map->segment_map->text, struct loader_segment_64 *);
				uint64_t addr_offset = section->position.addr - text_segment->data.vm_position.addr;
				if (section->info.offset != addr_offset) {
					offset = addr_offset;
				}
				uint64_t *classes = (uint64_t*)PtrAdd(mem_offset, offset);
				struct loader_objc_2_class *cls = (struct loader_objc_2_class *)PtrAdd(PtrLowPointer(mem_offset), classes[index]);
				struct loader_objc_class *objclass = SDMSTObjc2ClassCreateFromClass(cls, 0, data_range,PtrLowPointer(mem_offset), loader_objc_2_class_class_type);
				memcpy(&(binary->objc->cls[index]), objclass, sizeof(struct loader_objc_class));
				free(objclass);
			}
		}
	}
	result = (binary->objc->cls ? true : false);
	return result;
}

Pointer SDMSTFindFunctionAddress(Pointer *fPointer, struct loader_binary *binary) {
	Pointer pointer = NULL;
	uint64_t offset = 0;
	pointer = read_uleb128(PtrCast(*fPointer, uint8_t*), &offset);
	
	if (offset) {
		char *buffer = calloc(1024, sizeof(char));
		binary->map->subroutine_map->subroutine = realloc(binary->map->subroutine_map->subroutine, sizeof(struct loader_subroutine)*(binary->map->subroutine_map->count+0x1));
		struct loader_subroutine *subroutine = &(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count]);
		subroutine->offset = (uintptr_t)PtrAdd(offset, (binary->map->subroutine_map->count ? PtrCast(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count-0x1].offset, uintptr_t) : PtrCast(binary->header, uintptr_t)));
		sprintf(buffer, kSubFormatter, (uintptr_t)PtrSub(subroutine->offset, binary->header));
		subroutine->name = calloc((5 + strlen(buffer)), sizeof(char));
		sprintf(subroutine->name, kSubName, (uintptr_t)PtrSub(subroutine->offset, binary->header));
		subroutine->section_offset = k32BitMask;
		free(buffer);
		binary->map->subroutine_map->count++;
	}
	return pointer;
}

void SDMSTFindSubroutines(struct loader_binary *binary) {
	bool hasLCFunctionStarts = false;
	binary->map->subroutine_map->subroutine = (struct loader_subroutine *)calloc(1, sizeof(struct loader_subroutine));
	binary->map->subroutine_map->count = 0;
	uint32_t textSections = 0, flags = 0;
	uint64_t pageZero = 0, size = 0, address = 0;
	uintptr_t textSectionOffset = (uintptr_t)(binary->map->segment_map->text);

	if (SDMBinaryIs64Bit(binary->header)) {
		textSections = ((struct loader_segment_64 *)(binary->map->segment_map->text))->info.nsects;
		pageZero = ((struct loader_segment_64 *)(binary->map->segment_map->text))->data.vm_position.addr;
	}
	else {
		textSections = ((struct loader_segment_32 *)(binary->map->segment_map->text))->info.nsects;
		pageZero = ((struct loader_segment_32 *)(binary->map->segment_map->text))->data.vm_position.addr;
	}
	
	pageZero += binary->file_offset;
	uint64_t binaryOffset = pageZero;

	uint64_t memOffset = 0;
	if (binary->memory_ref) {
		memOffset = (uint64_t)_dyld_get_image_vmaddr_slide(binary->image_index);
	}
	else {
		memOffset = (uint64_t)(binary->header) - binaryOffset;
	}
	
	if (SDMBinaryIs64Bit(binary->header)) {
		flags = ((struct loader_section_64 *)(textSectionOffset))->info.flags;
		size = ((struct loader_section_64 *)(textSectionOffset))->position.size;
		
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_64 *)textSectionOffset)->position.addr);
	}
	else {
		flags = ((struct loader_section_32 *)(textSectionOffset))->info.flags;
		size = ((struct loader_section_32 *)(textSectionOffset))->position.size;
		
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_32 *)textSectionOffset)->position.addr);
	}
	
	if (SDMBinaryIs64Bit(binary->header)) {
		textSectionOffset += sizeof(struct loader_segment_64);
	}
	else {
		textSectionOffset += sizeof(struct loader_segment_32);
	}
	
	if (binary->map->function_start) {
		hasLCFunctionStarts = true;
		uintptr_t functionOffset = (uintptr_t)(binary->map->function_start->position.addr+memOffset+pageZero);
		Pointer functionPointer = (Pointer)functionOffset;
		while ((uintptr_t)functionPointer < (functionOffset + binary->map->function_start->position.size)) {
			functionPointer = SDMSTFindFunctionAddress(&functionPointer, binary);
		}
	}
	if (binary->header->arch.cputype == CPU_TYPE_X86_64 || binary->header->arch.cputype == CPU_TYPE_I386) {
		for (uint32_t i = 0x0; i < textSections; i++) {
			if (hasLCFunctionStarts && binary->map->subroutine_map->count) {
				for (uint32_t j = 0; j < binary->map->subroutine_map->count; j++) {
					if (binary->map->subroutine_map->subroutine[j].section_offset == k32BitMask) {
						uint64_t subOffset = pageZero+memOffset+binary->map->subroutine_map->subroutine[j].offset;
						if (subOffset < (address+size)) {
							binary->map->subroutine_map->subroutine[j].section_offset = textSectionOffset;
							binary->map->subroutine_map->subroutine[j].offset = (uintptr_t)(pageZero+memOffset+binary->map->subroutine_map->subroutine[j].offset);
						}
					}
				}
			}
			else {
				// SDM: Fall back on manually parsing it if we cannot find the subroutine mappings...
				Dl_info info;
				uint64_t loaded;
				if (binary->memory_ref) {
					loaded = (uint64_t)dladdr((void*)(address), &info);
				}
				else {
					loaded = address;
				}
				if (loaded) {
					if (((flags & S_REGULAR)==0) && ((flags & S_ATTR_PURE_INSTRUCTIONS) || (flags & S_ATTR_SOME_INSTRUCTIONS))) {
						uint64_t offset = 0;
						bool isIntel64bitArch = (SDMBinaryIs64Bit(binary->header) && binary->header->arch.cputype == CPU_TYPE_X86_64);
						if (size) {
							while (offset < (size - (isIntel64bitArch ? Intel_x86_64bit_StackSetupLength : Intel_x86_32bit_StackSetupLength))) {
								uint32_t result = 0;
								if (isIntel64bitArch) {
									result = (uint32_t)memcmp((void*)(address+offset), &(Intel_x86_64bit_StackSetup[0]), Intel_x86_64bit_StackSetupLength);
								}
								else {
									result = (uint32_t)memcmp((void*)(address+offset), &(Intel_x86_32bit_StackSetup[0]), Intel_x86_32bit_StackSetupLength);
								}
								if (!result) {
									char *buffer = calloc(1024, sizeof(char));
									binary->map->subroutine_map->subroutine = realloc(binary->map->subroutine_map->subroutine, ((binary->map->subroutine_map->count+1)*sizeof(struct loader_subroutine)));
									struct loader_subroutine *subroutine = (struct loader_subroutine *)calloc(1, sizeof(struct loader_subroutine));
									subroutine->offset = (uintptr_t)(address+offset);
									
									sprintf(buffer, kSubFormatter, subroutine->offset);
									subroutine->name = calloc(5 + (strlen(buffer)), sizeof(char));
									sprintf(subroutine->name, kSubName, subroutine->offset);
									
									subroutine->section_offset = textSectionOffset;
									
									memcpy(&(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count]), subroutine, sizeof(struct loader_subroutine));
									free(subroutine);
									free(buffer);
									binary->map->subroutine_map->count++;
								}
								offset++;
							}
						}
					}
				}
				else {
					printf("Image for address (%08llx) is not loaded\n",address);
				}
			}
			textSectionOffset += (SDMBinaryIs64Bit(binary->header) ? sizeof(struct loader_section_64) : sizeof(struct loader_section_32));
		}
		if (binary->map->subroutine_map->count == 0) {
			if (binary->objc->clsCount) {
				for (uint32_t index = 0; index < binary->objc->clsCount; index++) {
					struct loader_objc_class *class = &(binary->objc->cls[index]);
					SDMSTCreateSubroutinesForClass(binary, class);
					SDMSTCreateSubroutinesForClass(binary, class->superCls);
				}
			}
		}
		printf("Found %i subroutines\n",binary->map->subroutine_map->count);
	}
}

void SDMSTCreateSubroutinesForClass(struct loader_binary *binary, struct loader_objc_class *class) {
	uint64_t pageZero = 0, address = 0;
	if (SDMBinaryIs64Bit(binary->header)) {
		pageZero = ((struct loader_segment_64 *)(binary->map->segment_map->text))->data.vm_position.addr;
	}
	else {
		pageZero = ((struct loader_segment_32 *)(binary->map->segment_map->text))->data.vm_position.addr;
	}
	pageZero += binary->file_offset;
	uint64_t binaryOffset = pageZero;
	uintptr_t textSectionOffset = (uintptr_t)(binary->map->segment_map->text);
	if (SDMBinaryIs64Bit(binary->header)) {
		textSectionOffset += sizeof(struct loader_segment_64);
	}
	else {
		textSectionOffset += sizeof(struct loader_segment_32);
	}

	uint64_t memOffset;
	if (binary->memory_ref) {
		memOffset = (uint64_t)_dyld_get_image_vmaddr_slide(binary->image_index);
	}
	else {
		memOffset = (uint64_t)(binary->header) - binaryOffset;
	}
	if (SDMBinaryIs64Bit(binary->header)) {
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_64 *)textSectionOffset)->position.addr);
	}
	else {
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_32 *)textSectionOffset)->position.addr);
	}
	
	for (uint32_t method_index = 0; method_index < class->methodCount; method_index++) {
		struct loader_objc_method *method = &(class->method[method_index]);
						
		char *buffer = calloc(1024, sizeof(char));
		binary->map->subroutine_map->subroutine = realloc(binary->map->subroutine_map->subroutine, ((binary->map->subroutine_map->count+1)*sizeof(struct loader_subroutine)));
		struct loader_subroutine *subroutine = (struct loader_subroutine *)calloc(1, sizeof(struct loader_subroutine));
		subroutine->offset = (uintptr_t)(address+(method->offset) - (address-memOffset));
			
		sprintf(buffer, kSubFormatter, subroutine->offset);
		subroutine->name = calloc(5 + (strlen(buffer)), sizeof(char));
		sprintf(subroutine->name, kSubName, subroutine->offset);
		
		subroutine->section_offset = textSectionOffset;
			
		memcpy(&(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count]), subroutine, sizeof(struct loader_subroutine));
		free(subroutine);
		free(buffer);
		binary->map->subroutine_map->count++;
		
	}
	
}

uint32_t SDMSTMapMethodsOfClassToSubroutines(struct loader_objc_class *class, struct loader_binary *binary) {
	uint32_t counter = 0;
	if (class != NULL) {
		for (uint32_t method_index = 0; method_index < class->methodCount; method_index++) {
			struct loader_objc_method *method = &(class->method[method_index]);
			
			//printf("%s %s %lx\n",(method->method_type == loader_objc_method_instance_type ? "-" : "+"),SDMSTObjcCreateMethodDescription(SDMSTObjcDecodeType(method->type),method->name),method->offset);
			
			for (uint32_t subroutine_index = 0; subroutine_index < binary->map->subroutine_map->count; subroutine_index++) {
				uint32_t method_offset = PtrLowPointer(method->offset);
				uint32_t subroutine_offset = (((uint32_t)binary->map->subroutine_map->subroutine[subroutine_index].offset - ((uint32_t)((uintptr_t)binary->header)) + (uint32_t)(SDMBinaryIs64Bit(binary->header) ? 0 : 0x1000)));
				if (method_offset == subroutine_offset) {
					
					char *method_name = method->name;
					
					uint32_t name_length = (uint32_t)strlen(class->className)+5+(uint32_t)strlen(method_name);
					char *new_name = calloc(1+name_length, sizeof(char));
					char method_type = (method->method_type == loader_objc_method_instance_type ? '-' : (method->method_type == loader_objc_method_class_type ? '+' : '?'));
					sprintf(new_name, "%c[%s %s]",method_type,class->className,method_name);
					binary->map->subroutine_map->subroutine[subroutine_index].name = realloc(binary->map->subroutine_map->subroutine[subroutine_index].name, name_length+1);
					memcpy(binary->map->subroutine_map->subroutine[subroutine_index].name, new_name, name_length);
					
					counter++;
				}
			}
		}
	}
	return counter;
}

void SDMSTMapMethodsToSubroutines(struct loader_binary *binary) {
	if (binary->objc) {
		uint32_t counter = 0;
		for (uint32_t class_index = 0; class_index < binary->objc->clsCount; class_index++) {
			struct loader_objc_class *class = &(binary->objc->cls[class_index]);
			//printf("Class: %s\n",class->className);
			counter += SDMSTMapMethodsOfClassToSubroutines(class, binary);
			counter += SDMSTMapMethodsOfClassToSubroutines(class->superCls, binary);
		}
		printf("Mapped %i Methods to Subroutines\n",counter);
	}
}

void SDMSTMapSymbolsToSubroutines(struct loader_binary *binary) {
	uint32_t counter = 0;
	for (uint32_t i = 0; i < binary->map->symbol_table->count; i++) {
		for (uint32_t j = 0; j < binary->map->subroutine_map->count; j++) {
			if (binary->map->symbol_table->symbol[i].offset == (binary->map->subroutine_map->subroutine[j].offset - (uint64_t)binary->header)) {
				uint32_t name_length = sizeof(char)*(uint32_t)strlen(binary->map->symbol_table->symbol[i].symbol_name);
				binary->map->subroutine_map->subroutine[j].name = realloc(binary->map->subroutine_map->subroutine[j].name, name_length+0x1);
				memcpy(binary->map->subroutine_map->subroutine[j].name, binary->map->symbol_table->symbol[i].symbol_name, name_length);
				counter++;
			}
		}
	}
	printf("Mapped %i Symbols to Subroutines\n",counter);
}

bool SDMMatchArchToCPU(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	if (SDMArchCPU_X86(arch, endian_type)) {
		if (SDMArchCPUSUB_X86_64(arch, target_arch, endian_type)) {
			result = true;
		}
		else if (SDMArchCPUSUB_I386(arch, target_arch, endian_type)) {
			result = true;
		}
	}
	else if (SDMArchCPU_ARM(arch, endian_type)) {
		if (SDMArchCPUSUB_ARMV6(arch, target_arch, endian_type)) {
			result = true;
		}
		else if (SDMArchCPUSUB_ARMV7(arch, target_arch, endian_type)) {
			result = true;
		}
		else if (SDMArchCPUSUB_ARMV7S(arch, target_arch, endian_type)) {
			result = true;
		}
		else if (SDMArchCPUSUB_ARM64(arch, target_arch, endian_type)) {
			result = true;
		}
	}
	else if (SDMArchCPU_PPC(arch, endian_type)) {
		if (target_arch == loader_arch_ppc_type) {
			result = true;
		}
	}
	else if (SDMArchCPU_PPC64(arch, endian_type)) {
		if (target_arch == loader_arch_ppc64_type) {
			result = true;
		}
	}
	return result;
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

bool SMDSTSymbolDemangleAndCompare(char *symFromTable, char *symbolName) {
	bool matchesName = false;
	if (symFromTable && symbolName) {
		uint32_t tabSymLength = (uint32_t)strlen(symFromTable);
		uint32_t symLength = (uint32_t)strlen(symbolName);
		if (symLength <= tabSymLength) {
			char *offset = strstr(symFromTable, symbolName);
			if (offset) {
				uint32_t originOffset = (uint32_t)(offset - symFromTable);
				if (tabSymLength-originOffset == symLength) {
					matchesName = (strcmp(&symFromTable[originOffset], symbolName) == 0);
				}
			}
		}
	}
	return matchesName;
}

void SDMReleaseMap(struct loader_map *map) {
	if (map) {
		if (map->segment_map) {
			free(map->segment_map);
		}
		
		if (map->symbol_table) {
			for (uint32_t index = 0; index < map->symbol_table->count; index++) {
				if (map->symbol_table->symbol[index].stub) {
					free(map->symbol_table->symbol[index].symbol_name);
				}
			}
			free(map->symbol_table->symbol);
			free(map->symbol_table);
		}
		
		if (map->dependency_map) {
			free(map->dependency_map->dependency);
			free(map->dependency_map);
		}
		
		if (map->subroutine_map) {
			for (uint32_t index = 0; index < map->subroutine_map->count; index++) {
				free(map->subroutine_map->subroutine[index].name);
			}
			free(map->subroutine_map->subroutine);
			
			free(map->subroutine_map);
		}
		
		free(map);
	}
}

CoreRange SDMSTEH_FramePointer(struct loader_segment *text, bool is64Bit, uint64_t header_offset) {
	CoreRange result = {0,0};
	Pointer offset = (Pointer)text;
	uint32_t sections_count = 0;
	if (is64Bit) {
		struct loader_segment_64 *text_segment = PtrCast(text, struct loader_segment_64 *);
		sections_count = text_segment->info.nsects;
		offset = (Pointer)PtrAdd(offset, sizeof(struct loader_segment_64));
		for (uint32_t index = 0; index < sections_count; index++) {
			struct loader_section_64 *text_section = (struct loader_section_64 *)offset;
			if (strncmp(text_section->name.sectname, EH_FRAME, sizeof(char[16])) == 0) {
				Pointer result_offset = PtrCast(PtrCastSmallPointer(text_section->info.offset), Pointer);
				uint64_t addr_offset = text_section->position.addr - text_segment->data.vm_position.addr;
				if (text_section->info.offset != addr_offset) {
					result_offset = PtrCast(addr_offset, Pointer);
				}
				result.offset = (uint64_t)PtrAdd(result_offset, header_offset);
				result.length = (uint64_t)text_section->position.size;
				break;
			}
			offset = (Pointer)PtrAdd(offset, sizeof(struct loader_section_64));
		}
	}
	else {
		struct loader_segment_32 *text_segment = PtrCast(text, struct loader_segment_32 *);
		sections_count = text_segment->info.nsects;
		offset = (Pointer)PtrAdd(offset, sizeof(struct loader_segment_32));
		for (uint32_t index = 0; index < sections_count; index++) {
			struct loader_section_32 *text_section = (struct loader_section_32 *)offset;
			if (strncmp(text_section->name.sectname, EH_FRAME, sizeof(char[16])) == 0) {
				Pointer result_offset = PtrCast(PtrCastSmallPointer(text_section->info.offset), Pointer);
				uint64_t addr_offset = text_section->position.addr - text_segment->data.vm_position.addr;
				if (text_section->info.offset != addr_offset) {
					result_offset = PtrCast(addr_offset, Pointer);
				}
				result.offset = (uint64_t)PtrAdd(result_offset, header_offset);
				break;
			}
			offset = (Pointer)PtrAdd(offset, sizeof(struct loader_section_32));
		}
	}
	if (sections_count == 0) {
		result = (CoreRange){0,0};
	}
	return result;
}

bool SDMSTTEXTHasEH_Frame(struct loader_segment *text, bool is64Bit, uint64_t header_offset, CoreRange *eh_frame) {
	*eh_frame = SDMSTEH_FramePointer(text, is64Bit, header_offset);
	return ((eh_frame->offset != 0 && eh_frame->length != 0) ? true : false);
}

#pragma mark -
#pragma mark Public Function Definitions

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

struct loader_subroutine* SDMFindSubroutineFromName(struct loader_binary *binary, char *name) {
	for (uint32_t index = 0; index < binary->map->subroutine_map->count; index++) {
		if (strcmp(name, binary->map->subroutine_map->subroutine[index].name) == 0) {
			return &(binary->map->subroutine_map->subroutine[index]);
		}
	}
	return NULL;
}

CoreRange SDMSTRangeOfSubroutine(struct loader_subroutine *subroutine, struct loader_binary *binary) {
	CoreRange range = {0, 0};
	if (subroutine) {
		for (uint32_t i = 0; i < binary->map->subroutine_map->count; i++) {
			if (binary->map->subroutine_map->subroutine[i].offset == subroutine->offset) {
				range.offset = (uintptr_t)(subroutine->offset);
				uint32_t next = i + 1;
				if (next < binary->map->subroutine_map->count) {
					range.length = ((binary->map->subroutine_map->subroutine[next].offset) - range.offset);
					// SDM: validate against __eh_frame
				}
				else {
					uint64_t size, address;
					uint64_t memOffset = SDMCalculateVMSlide(binary);
					if (SDMBinaryIs64Bit(binary->header)) {
						size = ((struct loader_section_64 *)(subroutine->section_offset))->position.size;
						address = ((struct loader_section_64 *)(subroutine->section_offset))->position.addr + memOffset;
					}
					else {
						size = ((struct loader_section_32 *)(subroutine->section_offset))->position.size;
						address = ((struct loader_section_32 *)(subroutine->section_offset))->position.addr + memOffset;
					}
					range.length = ((address+size) - range.offset);
				}
				break;
			}
		}
	}
	return range;
}

#endif