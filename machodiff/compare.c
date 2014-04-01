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
#include <uuid/uuid.h>
#include "match.h"

bool SDMDiffAddName(struct loader_diff_symbol *diff, struct loader_symbol *symbol) {
	bool result = false;
	if (symbol->stub == false) {
		diff->name = calloc(strlen(symbol->symbol_name)+1, sizeof(char));
		memcpy(diff->name, symbol->symbol_name, strlen(symbol->symbol_name));
		result = true;
	}
	return result;
}

void SDMDiffAddSymbol(struct loader_diff *diff, struct loader_diff_symbol *symbol) {
	if (diff->index == NULL) {
		diff->index = calloc(1, sizeof(struct loader_symboL_index));
		diff->index_count = 0;
	}
	
	if (symbol->name == NULL) {
		// SDM: this need to be calculated
		uuid_t t;
		uuid_generate(t);
		symbol->name = calloc(37, sizeof(char));
		uuid_unparse(t, symbol->name);
	}
	
	char *hash = SDMSTCreateSymbolHash(symbol->name);

	diff->index = realloc(diff->index, sizeof(struct loader_symboL_index)*(diff->index_count+1));
	diff->index[diff->index_count].symbol_name = hash;
	
	cmap_str_setObjectForKey(diff->map, hash, symbol);
	
	diff->index_count++;
}

struct loader_subroutine* SDMSTFindSubroutineFromInfo(struct loader_binary *binary __attribute__((unused)), struct loader_diff_symbol_imp symbol __attribute__((unused))) {
	return NULL;
}

// SDM: this will give some variation due to the approximation in unique when parsing dynamically created block_ref symbols in a binary.
void SDMDiffParseSymbols(struct loader_diff *diff, struct loader_binary *input_one, struct loader_binary *input_two) {
	
	for (uint32_t index = 0; index < input_one->map->subroutine_map->count; index++) {
		struct loader_diff_symbol *symbol = calloc(1, sizeof(struct loader_diff_symbol));
		
		struct loader_subroutine *subroutine_b1 = &(input_one->map->subroutine_map->subroutine[index]);
		
		struct loader_symbol *symbol_b1 = SDMSTFindSymbolForSubroutine(input_one->map->symbol_table, subroutine_b1);
		
		bool named = false;
		if (symbol_b1 != NULL) {
			named = SDMDiffAddName(symbol, symbol_b1);
		}
		
		symbol->input_one.binary = input_one;
		symbol->input_one.symbol = symbol_b1;
		symbol->input_one.subroutine = subroutine_b1;
		
		struct loader_subroutine *subroutine_b2 = SDMFindSubroutineFromName(input_two, subroutine_b1->name);
		
		if (subroutine_b2 == NULL) {
			// SDM: we need to guess or add a old symbol from binary 1
			subroutine_b2 = SDMSTFindSubroutineFromInfo(input_two, symbol->input_one);
		}
		
		if (subroutine_b2 != NULL) {
			// SDM: found a symbol name match!
			
			struct loader_symbol *symbol_b2 = SDMSTFindSymbolForSubroutine(input_two->map->symbol_table, subroutine_b2);
			
			symbol->input_two.binary = input_two;
			symbol->input_two.symbol = symbol_b2;
			symbol->input_two.subroutine = subroutine_b2;
			
			if (named == false) {
				if (symbol_b2 != NULL) {
					named = SDMDiffAddName(symbol, symbol_b2);
				}
			}
			
			CoreRange subroutine_range1 = SDMSTRangeOfSubroutine(subroutine_b1, input_one);
			CoreRange subroutine_range2 = SDMSTRangeOfSubroutine(subroutine_b2, input_two);
			
			bool compare_result = SDMCompareSymbol(symbol, subroutine_range1, input_one, subroutine_range2, input_two);
			if (compare_result == false) {
				// SDM: symbols are different, find another
				
			}
		}
		
		SDMDiffAddSymbol(diff, symbol);
		
	}
	
	// SDM: now diff the second binary for more symbols
	for (uint32_t index = 0; index < input_two->map->subroutine_map->count; index++) {
		struct loader_subroutine *subroutine_b2 = &(input_two->map->subroutine_map->subroutine[index]);
		
		struct loader_symbol *symbol_b2 = SDMSTFindSymbolForSubroutine(input_two->map->symbol_table, subroutine_b2);
		
		if (symbol_b2) {
			char *hash = NULL;
			if (symbol_b2->stub == false) {
				hash = SDMSTCreateSymbolHash(symbol_b2->symbol_name);
			}
			else {
				// SDM: how do we calculate the uuid for this?
			}
			
			struct loader_diff_symbol *found_symbol = cmap_str_objectForKey(diff->map, hash);
			free(hash);
			if (found_symbol == NULL) {
				// SDM: this needs to be added;
				struct loader_diff_symbol *new_symbol = calloc(1, sizeof(struct loader_diff_symbol));
				SDMDiffAddName(new_symbol, symbol_b2);
				new_symbol->input_two.symbol = symbol_b2;
				new_symbol->input_two.subroutine = subroutine_b2;
				new_symbol->input_two.binary = input_two;
				
				SDMDiffAddSymbol(diff, new_symbol);
			}
		}

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
	}
	*/
}

bool SDMAnalyzeSubroutines(struct loader_binary *input_one, CoreRange range_one, struct loader_binary *input_two, CoreRange range_two) {
	bool result = false;
	
	uint64_t offset_one = ((uint64_t)range_one.offset - (uint64_t)input_one->header);
	uint64_t offset_two = ((uint64_t)range_two.offset - (uint64_t)input_two->header);
	
	csh handle_one, handle_two;
	cs_insn *insn_one, *insn_two;
	size_t count_one, count_two;
	
	cs_arch arch_type_one = SDM_CS_ArchType(&(input_one->header->arch), 0);
	cs_mode mode_type_one = SDM_CS_ModeType(&(input_one->header->arch), 0);
	
	cs_err result_one = cs_open(arch_type_one, mode_type_one, &handle_one);
	
	cs_arch arch_type_two = SDM_CS_ArchType(&(input_two->header->arch), 0);
	cs_mode mode_type_two = SDM_CS_ModeType(&(input_two->header->arch), 0);
	
	cs_err result_two = cs_open(arch_type_two, mode_type_two, &handle_two);
	
	if (result_one == CS_ERR_OK && result_two == CS_ERR_OK) {
		count_one = cs_disasm_ex(handle_one, PtrCast(Ptr(range_one.offset), uint8_t *), (uint32_t)range_one.length, offset_one, 0, &insn_one);
		count_two = cs_disasm_ex(handle_two, PtrCast(Ptr(range_two.offset), uint8_t *), (uint32_t)range_two.length, offset_two, 0, &insn_two);
		
		if (count_one != 0 && count_two != 0) {
			
			//printf("0x%"PRIx64":\t%s\t\t%s\n", insn[j].address, insn[j].mnemonic,insn[j].op_str);
			
			struct loader_match_tree *tree = SDMBuildMatchTree(range_one, range_two);
			
			// SDM: this is hopelessly inaccurate for almost everything
			uint8_t percent = SDMMatchPercentFromTree(tree, range_one.length);
			
			if (percent >= 90) {
				result = true;
			}
			
			SDMReleaseMatchTree(tree);
						
		}
		
		if (count_one) {
			cs_free(insn_one, count_one);
		}
		
		if (count_two) {
			cs_free(insn_two, count_two);
		}
	}
	else {
		printf("ERROR: Failed to disassemble given code!\n");
	}
	
	cs_close(&handle_one);
	cs_close(&handle_two);
	
	return result;
}

bool SDMCompareSymbol(struct loader_diff_symbol *symbol, CoreRange range_one, struct loader_binary *input_one, CoreRange range_two, struct loader_binary *input_two) {
	bool status = false;
	
	Pointer ptr_subroutine_one = PtrCast(Ptr(range_one.offset), Pointer);
	
	Pointer ptr_subroutine_two = PtrCast(Ptr(range_two.offset), Pointer);
	
	bool srs_cmp = false;
	
	if (range_one.length == range_two.length) {
		uint64_t length = (range_one.length < range_two.length ? range_one.length : range_two.length);
		
		int cmp_result = memcmp(ptr_subroutine_one, ptr_subroutine_two, (unsigned long)length);
		if (cmp_result == 0) {
			status = true;
		}
		else {
			srs_cmp = true;
		}
	}
	else {
		srs_cmp = true;
	}
	
	if (srs_cmp == true) {
		// SDM: do comparison on binary code.
		status = SDMAnalyzeSubroutines(input_one, range_one, input_two, range_two);
	}
	
	if (status == true) {
		// SDM: they seem to match, they can be zero'd out.
		symbol->match = status;
	}
	
	return status;
}

#endif
