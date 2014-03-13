//
//  map.c
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_map_c
#define machodiff_map_c

#include "map.h"
#include "loader.h"
#include "link_include.h"

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
					CoreRange offset = CoreRangeCreate(0, 0);
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
				map->dependency_map->dependency[map->dependency_map->count] = *PtrCast(loadCmd, Pointer);
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

uint32_t SDMSTMapMethodsOfClassToSubroutines(struct loader_objc_class *class, struct loader_binary *binary) {
	uint32_t counter = 0;
	if (class != NULL) {
		for (uint32_t method_index = 0; method_index < class->methodCount; method_index++) {
			struct loader_objc_method *method = &(class->method[method_index]);
			
			//printf("%s %s %lx\n",(method->method_type == loader_objc_method_instance_type ? "-" : "+"),SDMSTObjcCreateMethodDescription(SDMSTObjcDecodeType(method->type),method->name),method->offset);
			
			for (uint32_t subroutine_index = 0; subroutine_index < binary->map->subroutine_map->count; subroutine_index++) {
				uint64_t method_offset = PtrLowPointer(method->offset);
				uint64_t subroutine_offset = ((uint64_t)binary->map->subroutine_map->subroutine[subroutine_index].offset);
				if (method_offset == subroutine_offset) {
					
					char *method_name = SDMSTCreateNameForMethod(method, class->className);
					uint32_t method_name_length = (uint32_t)strlen(method_name);
					binary->map->subroutine_map->subroutine[subroutine_index].name = realloc(binary->map->subroutine_map->subroutine[subroutine_index].name, method_name_length+1);
					memcpy(binary->map->subroutine_map->subroutine[subroutine_index].name, method_name, method_name_length);
					
					free(method_name);
					
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
				binary->map->subroutine_map->subroutine[j].name = realloc(binary->map->subroutine_map->subroutine[j].name, name_length+1);
				memcpy(binary->map->subroutine_map->subroutine[j].name, binary->map->symbol_table->symbol[i].symbol_name, name_length);
				counter++;
			}
		}
	}
	printf("Mapped %i Symbols to Subroutines\n",counter);
}

void SDMSTMapSubroutineSectionOffset(struct loader_binary *binary) {
	uint32_t textSections = 0;
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
		size = ((struct loader_section_64 *)(textSectionOffset))->position.size;
		
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_64 *)textSectionOffset)->position.addr);
	}
	else {
		size = ((struct loader_section_32 *)(textSectionOffset))->position.size;
		
		address = (uint64_t)PtrAdd(memOffset, ((struct loader_section_32 *)textSectionOffset)->position.addr);
	}
	
	if (SDMBinaryIs64Bit(binary->header)) {
		textSectionOffset += sizeof(struct loader_segment_64);
	}
	else {
		textSectionOffset += sizeof(struct loader_segment_32);
	}
	
	
	for (uint32_t i = 0; i < textSections; i++) {
		if (binary->map->subroutine_map->count) {
			for (uint32_t j = 0; j < binary->map->subroutine_map->count; j++) {
				if (binary->map->subroutine_map->subroutine[j].section_offset == k32BitMask) {
					uint64_t subOffset = memOffset+binary->map->subroutine_map->subroutine[j].offset;
					if (subOffset < (address+size)) {
						binary->map->subroutine_map->subroutine[j].section_offset = textSectionOffset;
					}
				}
			}
		}
		textSectionOffset += (SDMBinaryIs64Bit(binary->header) ? sizeof(struct loader_section_64) : sizeof(struct loader_section_32));
	}
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

#endif
