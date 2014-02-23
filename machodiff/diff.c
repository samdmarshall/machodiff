//
//  diff.c
//  machodiff
//
//  Created by Sam Marshall on 2/22/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_diff_c
#define machodiff_diff_c

#include "diff.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <unistd.h>

#define kBufferSize 1024

char* SDMCreatePathWithName(char *path, char *name) {
	char *subpath = calloc(1, strlen(path)+strlen(name)+2);
	sprintf(subpath,"%s/%s",path,name);
	return subpath;
}

char* SDMCreateDirectoryAtPath(char *name, char *path) {
	char *subpath = SDMCreatePathWithName(path, name);
	bool status = SDMMakeNewFolderAtPath(subpath, 0700);
	if (status == false) {
		printf("error creating subpath for dump %s\n",subpath);
	}
	return subpath;
}

bool SDMMakeNewFolderAtPath(char *path, mode_t mode) {
	bool status = false;
	struct stat st;
	int result = stat(path, &st);
	if (result == -1) {
		int mkdirResult = mkdir(path, mode);
		status = ((mkdirResult == 0 || (mkdirResult == -1 && errno == EEXIST)) ? true : false);
	}
	else if (result == 0) {
		status = true;
	}
	return status;
}

struct loader_binary * SDMLoadTarget(char *path, uint8_t type) {
	printf("%s:\n",path);
	struct loader_binary *input = SDMLoadBinaryWithPath(path, type);
	printf("\n");
	return input;
}

bool SDMNameListContainsName(struct loader_diff *diff, struct loader_subroutine *subroutine, struct loader_binary *input) {
	bool found = false;
	for (uint32_t index = 0; index < diff->name_count; index++) {
		if (strcmp(subroutine->name, diff->symbol[index].name) == 0) {
			found = (input == diff->symbol[index].binary ? (diff->symbol[index].offset == subroutine->offset) : true);
			if (found) {
				break;
			}
		}
	}
	return found;
}

// SDM: this will give some variation due to the approximation in unique when parsing dynamically created block_ref symbols in a binary.
void SDMDiffAddSymbols(struct loader_diff *diff, struct loader_binary *input) {
	uint32_t add_counter = 0;
	for (uint32_t index = 0; index < input->map->subroutine_map->count; index++) {
		bool has_name = SDMNameListContainsName(diff, &(input->map->subroutine_map->subroutine[index]), input);
		// SDM: add a check in here for the contents of the subroutine, because names won't work when symbols are stripped.
		if (has_name == false) {
			diff->symbol = realloc(diff->symbol, sizeof(struct loader_diff_symbol)*(diff->name_count+1));
			diff->symbol[diff->name_count].name = input->map->subroutine_map->subroutine[index].name;
			diff->symbol[diff->name_count].binary = input;
			diff->symbol[diff->name_count].offset = input->map->subroutine_map->subroutine[index].offset;
			diff->name_count++;
			add_counter++;
		}
	}
}

struct loader_diff * SDMGenerateSymbolList(struct loader_binary *input_one, struct loader_binary *input_two) {
	struct loader_diff *diff = calloc(1, sizeof(struct loader_diff));
	diff->symbol = calloc(1, sizeof(struct loader_diff_symbol));
	diff->name_count = 0;
	
	SDMDiffAddSymbols(diff, input_one);
	
	SDMDiffAddSymbols(diff, input_two);
	
	return diff;
}

void SDMWriteSubroutineData(CoreRange range, char *path, char *name, char *append) {
	if (range.length) {
		char *path_name = calloc(strlen(name)+1, sizeof(char));
		memcpy(path_name, name, strlen(name));
		char file_name[kBufferSize] = {0}, *pch = strtok(path_name,"/");
		while (pch != NULL) {
			memcpy(file_name, pch, strlen(pch));
			pch = strtok(NULL, "/");
			if (pch != NULL) {
				memset(file_name, 0, kBufferSize);
			}
		}
		strlcat(file_name, append, kBufferSize);
		
		char *file_path = SDMCreatePathWithName(path, file_name);
		FILE *fd = fopen(file_path, "wb");
		if (fd) {
			fwrite(Ptr(range.offset), (unsigned long)range.length, sizeof(char), fd);
		}
		fclose(fd);
		free(file_path);
		free(path_name);
	}
}

void SDMAnalyzeSymbol(struct loader_diff_symbol *symbol, struct loader_binary *input_one, struct loader_binary *input_two, char *output) {
	bool should_dump = false;
	
	struct loader_subroutine *sub_one = SDMFindSubroutineFromName(input_one, symbol->name);
	
	struct loader_subroutine *sub_two = SDMFindSubroutineFromName(input_two, symbol->name);
	
	CoreRange range_one = SDMSTRangeOfSubroutine(sub_one, input_one);
	
	CoreRange range_two = SDMSTRangeOfSubroutine(sub_two, input_two);
	
	if (range_one.length == range_two.length) {
		should_dump = memcmp(Ptr(range_one.offset), Ptr(range_two.offset), (unsigned long)range_one.length);
	} else {
		should_dump = true;
	}
	
	if (should_dump) {
		char *out_path = SDMCreateDirectoryAtPath(symbol->name, output);
		SDMWriteSubroutineData(range_one, out_path, input_one->name, "-1");
		SDMWriteSubroutineData(range_two, out_path, input_two->name, "-2");
		free(out_path);
	}
	
}

void SDMPerformComparison(struct loader_binary *input_one, struct loader_binary *input_two, char *output_path) {
	struct loader_diff *diff = SDMGenerateSymbolList(input_one, input_two);
	for (uint32_t index = 0; index < diff->name_count; index++) {
		SDMAnalyzeSymbol(&(diff->symbol[index]), input_one, input_two, output_path);
	}
	free(diff);
}

#endif
