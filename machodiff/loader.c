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

#pragma mark -
#pragma mark Private Types

#define Intel_x86_32bit_StackSetupLength 0x3
static uint8_t Intel_x86_32bit_StackSetup[Intel_x86_32bit_StackSetupLength] = {0x55, 0x89, 0xe5};

#define Intel_x86_64bit_StackSetupLength 0x4
static uint8_t Intel_x86_64bit_StackSetup[Intel_x86_64bit_StackSetupLength] = {0x55, 0x48, 0x89, 0xe5};

#pragma mark -
#pragma mark Private Function Declaration

#ifndef CPU_SUBTYPE_ARM_V8
#define CPU_SUBTYPE_ARM_V8		((cpu_subtype_t) 13)
#endif

#define EndianFix(type, value) ((type == loader_endian_little_type) ? SDMSwapEndian32(value) : value);

struct loader_map *SDMCreateBinaryMap(struct loader_generic_header *header);

bool SDMIsBinaryLoaded(char *path, struct loader_binary *binary);
bool SDMLoadBinaryFromFile(struct loader_binary *binary, char *path, uint8_t target_arch);
bool SDMMatchArchToCPU(struct loader_arch_header *arch_header, uint8_t target_arch, uint8_t endian_type);
uint8_t SDMGetBinaryEndianness(uint32_t magic);

uint64_t SDMCalculateVMSlide(struct loader_binary *binary);
uint64_t SDMComputeFslide(struct loader_segment_map *segment_map, bool is64Bit);

void SDMGenerateSymbols(struct loader_binary *binary);

bool SDMMapObjcClasses32(struct loader_binary *binary);
bool SDMMapObjcClasses64(struct loader_binary *binary);

void SDMSTFindFunctionAddress(uint8_t **fPointer, struct loader_binary *binary);
void SDMSTFindSubroutines(struct loader_binary *binary);

void SDMSTMapMethodsToSubroutines(struct loader_binary *binary);
void SDMSTMapSymbolsToSubroutines(struct loader_binary *binary);

bool SMDSTSymbolDemangleAndCompare(char *symFromTable, char *symbolName);

void SDMReleaseMap(struct loader_map *map);

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
	struct symtab_command *symtab_cmd = PtrCast(binary->map->symbol_table->symtab, struct symtab_command *);
	bool is64Bit = SDMBinaryIs64Bit(binary->header);
	uint64_t fslide = (binary->memory_ref == true ? SDMComputeFslide(binary->map->segment_map, is64Bit) : 0);
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
						symbol->symbol_name = calloc(1 + strlen(kStubName) + ((binary->map->symbol_table->count==0) ? 1 : (uint32_t)log10(binary->map->symbol_table->count) + 0x1), sizeof(char));
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
				module = (struct loader_objc_module_raw *)PtrAdd(binary->header, section->info.offset);
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
		CoreRange data_range = CoreRangeCreate((uintptr_t)((uint64_t)(objc_segment->data.vm_position.addr)+((uint64_t)(mem_offset & k32BitMask))),objc_segment->data.vm_position.size);
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
				uint64_t *classes = (uint64_t*)PtrAdd(mem_offset, section->info.offset);
				struct loader_objc_2_class *cls = (struct loader_objc_2_class *)PtrAdd((mem_offset & k32BitMask), classes[index]);
				struct loader_objc_class *objclass = SDMSTObjc2ClassCreateFromClass(cls, 0, data_range, (mem_offset & k32BitMask));
				memcpy(&(binary->objc->cls[index]), objclass, sizeof(struct loader_objc_class));
				free(objclass);
			}
		}
	}
	result = (binary->objc->cls ? true : false);
	return result;
}

void SDMSTFindFunctionAddress(uint8_t **fPointer, struct loader_binary *binary) {
	uint8_t *pointer = *fPointer;
	uint32_t bitCount = 0x0;
	uintptr_t offset = 0x0;
	do {
		uint32_t slice = (*pointer & 0x7f);
		if (bitCount < 0x40) {
			offset |= (slice << bitCount);
			bitCount += 0x7;
		}
		else {
			break;
		}
	} while ((*pointer++ & 0x80) != 0);
	
	if (offset) {
		char *buffer = calloc(1024, sizeof(char));
		binary->map->subroutine_map->subroutine = realloc(binary->map->subroutine_map->subroutine, sizeof(struct loader_subroutine)*(binary->map->subroutine_map->count+0x1));
		struct loader_subroutine *subroutine = &(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count]);
		subroutine->offset = offset + (binary->map->subroutine_map->count ? binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count-0x1].offset : 0x0);
		sprintf(buffer, kSubFormatter, (subroutine->offset));
		subroutine->name = calloc((5 + strlen(buffer)), sizeof(char));
		sprintf(subroutine->name, kSubName, (subroutine->offset));
		subroutine->section_offset = k32BitMask;
		free(buffer);
		binary->map->subroutine_map->count++;
	}
	*fPointer = pointer;
}

void SDMSTFindSubroutines(struct loader_binary *binary) {
	bool hasLCFunctionStarts = false;
	binary->map->subroutine_map->subroutine = (struct loader_subroutine *)calloc(1, sizeof(struct loader_subroutine));
	binary->map->subroutine_map->count = 0;
	uint32_t textSections = 0, flags = 0;
	uint64_t pageZero = 0, size = 0, address = 0;
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
	uintptr_t textSectionOffset = (uintptr_t)(binary->map->segment_map->text);
	if (SDMBinaryIs64Bit(binary->header)) {
		textSectionOffset += sizeof(struct loader_segment_64);
	}
	else {
		textSectionOffset += sizeof(struct loader_segment_32);
	}
	if (binary->map->function_start) {
		hasLCFunctionStarts = true;
		uint64_t memOffset;
		if (binary->memory_ref) {
			memOffset = SDMCalculateVMSlide(binary);
		}
		else {
			memOffset = (uint64_t)(binary->header) - binaryOffset;
		}
		uintptr_t functionOffset = (uintptr_t)(binary->map->function_start->position.addr+memOffset+pageZero);
		uint8_t *functionPointer = (uint8_t*)functionOffset;
		while ((uintptr_t)functionPointer < (functionOffset + binary->map->function_start->position.size)) {
			SDMSTFindFunctionAddress(&functionPointer, binary);
		}
	}
	if (binary->header->arch.cputype == CPU_TYPE_X86_64 || binary->header->arch.cputype == CPU_TYPE_I386) {
		for (uint32_t i = 0x0; i < textSections; i++) {
			uint64_t memOffset;
			if (binary->memory_ref) {
				memOffset = (uint64_t)_dyld_get_image_vmaddr_slide(binary->image_index);
			}
			else {
				memOffset = (uint64_t)(binary->header) - binaryOffset;
			}
			if (SDMBinaryIs64Bit(binary->header)) {
				flags = ((struct loader_section_64 *)(textSectionOffset))->info.flags;
				size = ((struct loader_section_64 *)(textSectionOffset))->position.size;
				address = (uint64_t)PtrAdd(memOffset, ((struct section_64 *)textSectionOffset)->addr);
			}
			else {
				flags = ((struct loader_section_32 *)(textSectionOffset))->info.flags;
				size = ((struct loader_section_32 *)(textSectionOffset))->position.size;
				address = (uint64_t)PtrAdd(memOffset, ((struct section *)textSectionOffset)->addr);
			}
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
		printf("Found %i subroutines\n",binary->map->subroutine_map->count);
	}
}

void SDMSTMapMethodsToSubroutines(struct loader_binary *binary) {
	if (binary->objc) {
		uint32_t counter = 0;
		for (uint32_t class_index = 0; class_index < binary->objc->clsCount; class_index++) {
			struct loader_objc_class *class = &(binary->objc->cls[class_index]);
			
			for (uint32_t method_index = 0; method_index < class->methodCount; method_index++) {
				struct loader_objc_method *method = &(class->method[method_index]);
				
				for (uint32_t subroutine_index = 0; subroutine_index < binary->map->subroutine_map->count; subroutine_index++) {
					if ((method->offset & k32BitMask) == ((binary->map->subroutine_map->subroutine[subroutine_index].offset + (SDMBinaryIs64Bit(binary->header) ? -(uint64_t)binary->header : 0x1000)))) {
						
						uint32_t name_length = (uint32_t)strlen(class->className)+1+(uint32_t)strlen(method->name);
						char *new_name = calloc(1+name_length, sizeof(char));
						char method_type = (method->method_type == loader_objc_method_instance_type ? '-' : (method->method_type == loader_objc_method_class_type ? '+' : '?'));
						sprintf(new_name, "%s%c%s",class->className,method_type,method->name);
						binary->map->subroutine_map->subroutine[subroutine_index].name = realloc(binary->map->subroutine_map->subroutine[subroutine_index].name, name_length+1);
						memcpy(binary->map->subroutine_map->subroutine[subroutine_index].name, new_name, name_length);
						
						counter++;
					}
				}
			}
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

bool SDMMatchArchToCPU(struct loader_arch_header *arch_header, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	cpu_type_t type = (cpu_type_t)EndianFix(endian_type, (uint32_t)arch_header->arch.cputype);
	if ((type & CPU_TYPE_X86) == CPU_TYPE_X86) {
		uint32_t subtype = (uint32_t)EndianFix(endian_type, (uint32_t)arch_header->arch.subtype);
		if (((subtype & CPU_SUBTYPE_LIB64) == CPU_SUBTYPE_LIB64) && ((subtype & CPU_SUBTYPE_X86_ALL) == CPU_SUBTYPE_X86_ALL) && target_arch == loader_arch_x86_64_type) {
			result = true;
		}
		else if (((subtype & CPU_SUBTYPE_LIB64) != CPU_SUBTYPE_LIB64) && ((subtype & CPU_SUBTYPE_I386_ALL) == CPU_SUBTYPE_I386_ALL) && target_arch == loader_arch_i386_type) {
			result = true;
		}
	}
	else if ((type & CPU_TYPE_ARM) == CPU_TYPE_ARM) {
		cpu_subtype_t subtype = (cpu_subtype_t)EndianFix(endian_type, (uint32_t)arch_header->arch.subtype);
		if ((subtype & CPU_SUBTYPE_ARM_V6) == CPU_SUBTYPE_ARM_V6 && target_arch == loader_arch_armv6_type) {
			result = true;
		}
		else if ((subtype & CPU_SUBTYPE_ARM_V7) == CPU_SUBTYPE_ARM_V7 && target_arch == loader_arch_armv7_type) {
			result = true;
		}
		else if ((subtype & CPU_SUBTYPE_ARM_V7S) == CPU_SUBTYPE_ARM_V7S && target_arch == loader_arch_armv7s_type) {
			result = true;
		}
		else if ((subtype & CPU_SUBTYPE_ARM_V8) == CPU_SUBTYPE_ARM_V8 && target_arch == loader_arch_arm64_type) {
			result = true;
		}
	}
	else if ((type & CPU_TYPE_POWERPC) == CPU_TYPE_POWERPC) {
		if (target_arch == loader_arch_ppc_type) {
			result = true;
		}
	}
	else if ((type & CPU_TYPE_POWERPC64) == CPU_TYPE_POWERPC64) {
		if (target_arch == loader_arch_ppc64_type) {
			result = true;
		}
	}
	return result;
}

uint8_t SDMGetBinaryEndianness(uint32_t magic) {
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
				struct loader_fat_header *whole_binary = mmap(NULL, (unsigned long)size, PROT_READ, MAP_PRIVATE, fd, offset);
				binary->endian_type = SDMGetBinaryEndianness(whole_binary->magic.magic);
				offset += sizeof(struct loader_fat_header);
				uint32_t arch_count = EndianFix(binary->endian_type, whole_binary->n_arch);
				for (uint32_t arch_index = 0; arch_index < arch_count; arch_index++) {
					struct loader_arch_header *arch_header = (struct loader_arch_header *)PtrAdd(whole_binary, offset);
					bool found_arch = SDMMatchArchToCPU(arch_header, target_arch, binary->endian_type);
					if (found_arch) {
						uint32_t size = EndianFix(binary->endian_type, arch_header->size)
						uint32_t offset = EndianFix(binary->endian_type, arch_header->offset);
						binary->header = calloc(1, size);
						binary->file_offset = offset;
						lseek(fd, offset, SEEK_SET);
						read(fd, binary->header, size);
						result = true;
						break;
					}
					offset += sizeof(struct loader_arch_header);
				}
				munmap(whole_binary, (size_t)size);
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
		SDMGenerateSymbols(binary);
		SDMSTFindSubroutines(binary);
		SDMSTMapSymbolsToSubroutines(binary);

		bool loadedObjc = false;
		bool is64Bit = SDMBinaryIs64Bit(binary->header);
		if (is64Bit) {
			loadedObjc = SDMMapObjcClasses64(binary);
		}
		else {
			loadedObjc = SDMMapObjcClasses32(binary);
		}
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