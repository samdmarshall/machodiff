//
//  symbol.c
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_symbol_c
#define machodiff_symbol_c

#include "symbol.h"
#include <mach-o/nlist.h>
#include "loader.h"
#include "hash.h"

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
				struct loader_symbol *symbol = (struct loader_symbol *)calloc(1, sizeof(struct loader_symbol));
				if (symbol) {
					binary->map->symbol_table->symbol = realloc(binary->map->symbol_table->symbol, sizeof(struct loader_symbol)*(unsigned long)(binary->map->symbol_table->count+1));
					symbol->symbol_number = symbol_index;
					symbol->offset = (uintptr_t)PtrAdd(symbol_address, SDMCalculateVMSlide(binary));
					if (entry->n_un.n_strx && (entry->n_un.n_strx < symtab_cmd->strsize)) {
						symbol->symbol_name = PtrAdd(strTable, entry->n_un.n_strx);
						symbol->stub = false;
					}
					else {
						symbol->symbol_name = calloc(1024, sizeof(char));
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

Pointer SDMSTFindFunctionAddress(Pointer *fPointer, struct loader_binary *binary) {
	Pointer pointer = NULL;
	uint64_t offset = 0;
	pointer = read_uleb128(PtrCast(*fPointer, uint8_t*), &offset);
	
	if (offset) {
		char *buffer = calloc(1024, sizeof(char));
		binary->map->subroutine_map->subroutine = realloc(binary->map->subroutine_map->subroutine, sizeof(struct loader_subroutine)*(binary->map->subroutine_map->count+1));
		struct loader_subroutine *subroutine = &(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count]);
		subroutine->offset = (uintptr_t)PtrAdd(offset, (binary->map->subroutine_map->count ? PtrCast(binary->map->subroutine_map->subroutine[binary->map->subroutine_map->count-1].offset, uintptr_t) : 0));
		sprintf(buffer, kSubFormatter, (uintptr_t)subroutine->offset);
		subroutine->name = calloc(1024, sizeof(char));
		sprintf(subroutine->name, kSubName, (uintptr_t)subroutine->offset);
		subroutine->section_offset = k32BitMask;
		free(buffer);
		binary->map->subroutine_map->count++;
	}
	return pointer;
}

struct loader_symbol* SDMSTFindSymbolForSubroutine(struct loader_symtab *symbol_table, struct loader_subroutine *subroutine) {
	struct loader_symbol *symbol = NULL;
	for (uint32_t index = 0; index < symbol_table->count; index++) {
		struct loader_symbol *item = &(symbol_table->symbol[index]);
		if (item->offset == subroutine->offset) {
			return item;
		}
	}
	return symbol;
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

char* SDMSTCreateSymbolHash(char *name) {
	char *hash = calloc((HASH_LENGTH+1), sizeof(char));
	hash = (char *)StringToSHA1(name, (uint32_t)strlen(name), (unsigned char *)hash);
	return hash;
}

#endif
