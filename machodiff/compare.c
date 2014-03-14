//
//  compare.c
//  machodiff
//
//  Created by Sam Marshall on 2/23/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_compare_c
#define machodiff_compare_c

#include "compare.h"
#include <dispatch/dispatch.h>
#include <inttypes.h>
#include "arch.h"
#include "lexer.h"
#include "subroutine.h"
#include "hash.h"
#include <uuid/uuid.h>


bool SDMDiffAddName(struct loader_diff_symbol *diff, struct loader_symbol *symbol);

bool SDMDiffAddName(struct loader_diff_symbol *diff, struct loader_symbol *symbol) {
	bool result = false;
	if (symbol->stub == false) {
		diff->name = calloc(strlen(symbol->symbol_name)+1, sizeof(char));
		memcpy(diff->name, symbol->symbol_name, strlen(symbol->symbol_name));
		result = true;
	}
	return result;
}

// SDM: this will give some variation due to the approximation in unique when parsing dynamically created block_ref symbols in a binary.
void SDMDiffAddSymbols(struct loader_diff *diff, struct loader_binary *input_one, struct loader_binary *input_two) {
	
	for (uint32_t index = 0; index < input_one->map->subroutine_map->count; index++) {
		struct loader_diff_symbol *symbol = calloc(1, sizeof(struct loader_diff_symbol));
		
		struct loader_subroutine *subroutine_b1 = &(input_one->map->subroutine_map->subroutine[index]);
		
		struct loader_symbol *symbol_b1 = SDMSTFindSymbolForSubroutine(input_one->map->symbol_table, subroutine_b1);
		bool named = SDMDiffAddName(symbol, symbol_b1);
		
		symbol->input_one.binary = input_one;
		symbol->input_one.symbol = symbol_b1;
		symbol->input_one.subroutine = subroutine_b1;
		
		struct loader_subroutine *subroutine_b2 = SDMFindSubroutineFromName(input_two, subroutine_b1->name);
		if (subroutine_b2 != NULL) {
			// SDM: found a symbol name match!
			
			struct loader_symbol *symbol_b2 = SDMSTFindSymbolForSubroutine(input_two->map->symbol_table, subroutine_b2);
			
			symbol->input_two.binary = input_two;
			symbol->input_two.symbol = symbol_b2;
			symbol->input_two.subroutine = subroutine_b2;

			if (named == false) {
				named = SDMDiffAddName(symbol, symbol_b2);
			}
			
			CoreRange subroutine_range1 = SDMSTRangeOfSubroutine(subroutine_b1, input_one);
			CoreRange subroutine_range2 = SDMSTRangeOfSubroutine(subroutine_b2, input_two);
			
			bool compare_result = SDMCompareSymbol(symbol, subroutine_range1, input_one, subroutine_range2, input_two);
			if (compare_result) {
				// SDM: they seem to match, they can be stubbed out.
			}
			else {
				// SDM: symbols are different!
			}
		}
		else {
			// SDM: we need to guess
			
		}
		
		char *name = NULL;
		if (!named) {
			uuid_t t;
			uuid_generate(t);
			symbol->name = calloc(37, sizeof(char));
			uuid_unparse(t, symbol->name);
		}
		
		name = symbol->name;

		char *hash = calloc((HASH_LENGTH+1), sizeof(char));
		hash = (char *)StringToSHA1(name, (uint32_t)strlen(name), (unsigned char *)hash);
		
		cmap_str_setObjectForKey(diff->map, hash, symbol);
	}
	
	
//	uint32_t add_counter = 0;
//	for (uint32_t index = 0; index < input->map->subroutine_map->count; index++) {
//		bool has_name = SDMNameListContainsName(diff, &(input->map->subroutine_map->subroutine[index]), input);
//		// SDM: add a check in here for the contents of the subroutine, because names won't work when symbols are stripped.
//		if (has_name == false) {
//			diff->symbol = realloc(diff->symbol, sizeof(struct loader_diff_symbol)*(diff->name_count+1));
//			diff->symbol[diff->name_count].name = input->map->subroutine_map->subroutine[index].name;
//			diff->symbol[diff->name_count].binary = input;
//			diff->symbol[diff->name_count].offset = input->map->subroutine_map->subroutine[index].offset;
//			diff->name_count++;
//			add_counter++;
//		}
//	}
	
	//struct loader_cpp_map *cpp_map1 = SDMSTCPPMapInitialize();
	//struct loader_cpp_map *cpp_map2 = SDMSTCPPMapInitialize();
	
	/*
	
	for (uint32_t index = 0; index < input_one->map->subroutine_map->count; index++) {
		bool unnamed_subroutine1, unnamed_subroutine2 = false;
		uintptr_t offset1, offset2 = 0;
		
		char *subroutine_name1 = input_one->map->subroutine_map->subroutine[index].name;
		if (strncmp(subroutine_name1, kSubPrefix, sizeof(char[4])) == 0) {
			unnamed_subroutine1 = true;
		}
		
		char *subroutine_name2 = input_two->map->subroutine_map->subroutine[index].name;
		if (strncmp(subroutine_name2, kSubPrefix, sizeof(char[4])) == 0) {
			unnamed_subroutine2 = true;
		}

		uintptr_t calculated_offset1 = (uintptr_t)(input_one->map->subroutine_map->subroutine[index].offset );//+ (SDMBinaryIs64Bit(input_one->header) ? (uint64_t)input_one->header : 0));
		
		uintptr_t calculated_offset2 = (uintptr_t)(input_two->map->subroutine_map->subroutine[index].offset );//+ (SDMBinaryIs64Bit(input_two->header) ? (uint64_t)input_two->header : 0));
		
		printf("Found Item:\n");
		
		offset1 = calculated_offset1;
		CoreRange subroutine_range1 = SDMSTRangeOfSubroutine(&(input_one->map->subroutine_map->subroutine[index]), input_one);
		char *name1 = NULL;
		if (SDMSTCPPSymbolName(subroutine_name1)) {
			//struct loader_cpp_lexer_type *demangle1 = SDMSTDecodeNameString(cpp_map1, subroutine_name1);
			//name1 = SDMSTCPPSymbolNameGenerate(demangle1, "::");
			name1 = SDMSTCPPDemangleName(&(subroutine_name1[1]));
		}
		else if (SDMSTCSymbolName(subroutine_name1)) {
			name1 = &(subroutine_name1[1]);
		}
		else {
			name1 = subroutine_name1;
		}
		printf("\tName: %s \n\tOffset: %lx \n\tLength: %lld\n",name1,offset1,subroutine_range1.length);
		
		offset2 = calculated_offset2;
		CoreRange subroutine_range2 = SDMSTRangeOfSubroutine(&(input_two->map->subroutine_map->subroutine[index]), input_two);
		char *name2 = NULL;
		if (SDMSTCPPSymbolName(subroutine_name2)) {
			//struct loader_cpp_lexer_type *demangle2 = SDMSTDecodeNameString(cpp_map2, subroutine_name2);
			//name2 = SDMSTCPPSymbolNameGenerate(demangle2, "::");
			name2 = SDMSTCPPDemangleName(&(subroutine_name1[1]));
		}
		else if (SDMSTCSymbolName(subroutine_name2)) {
			name2 = &(subroutine_name2[1]);
		}
		else {
			name2 = subroutine_name2;
		}
		printf("\tName: %s \n\tOffset: %lx \n\tLength: %lld\n",name2,offset2,subroutine_range2.length);
		
		printf("\n");
		
		CoreRange subroutine_range = SDMSTRangeOfSubroutine(&(input_one->map->subroutine_map->subroutine[index]), input_one);
		
		csh handle;
		cs_insn *insn;
		size_t count;
		
		cs_arch arch_type = SDM_CS_ArchType(&(input_one->header->arch), 0);
		cs_mode mode_type = SDM_CS_ModeType(&(input_one->header->arch), 0);
		
		if (cs_open(arch_type, mode_type, &handle) == CS_ERR_OK) {
			count = cs_disasm_ex(handle, PtrCast(Ptr(subroutine_range.offset), uint8_t *), (uint32_t)subroutine_range.length, offset1, 0, &insn);
			if (count > 0) {
				size_t j;
				for (j = 0; j < count; j++) {
					printf("0x%"PRIx64":\t%s\t\t%s\n", insn[j].address, insn[j].mnemonic,insn[j].op_str);
				}
				cs_free(insn, count);
			}
		}
		else {
			printf("ERROR: Failed to disassemble given code!\n");
		}
		printf("\n");
		cs_close(&handle);
	}
	
	//SDMSTCPPMapRelease(cpp_map1);
	//SDMSTCPPMapRelease(cpp_map2);
	*/
}

bool SDMCompareSymbol(struct loader_diff_symbol *symbol, CoreRange range_one, struct loader_binary *input_one, CoreRange range_two, struct loader_binary *input_two) {
	bool status = false;
	
	Pointer ptr_subroutine_one = PtrCast(Ptr(range_one.offset), Pointer);
	
	Pointer ptr_subroutine_two = PtrCast(Ptr(range_two.offset), Pointer);
	
	if (range_one.length == range_two.length) {
		status = (memcmp(ptr_subroutine_one, ptr_subroutine_two, (unsigned long)range_one.length) == 0);
	}
	else {
		// SDM: do comparison on binary code.
	}
	
	return status;
}

#endif
