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
#include "compare.h"
#include "subroutine.h"
#include "file_include.h"

#define kBufferSize 1024

char* SDMCreatePathWithName(char *path, char *name);
char* SDMCreateDirectoryAtPath(char *name, char *path);

bool SDMNameListContainsName(struct loader_diff *diff, struct loader_subroutine *subroutine, struct loader_binary *input);
struct loader_diff * SDMGenerateSymbolList(struct loader_binary *input_one, struct loader_binary *input_two);
void SDMWriteSubroutineData(CoreRange range, char *path, char *name, char *append);
void SDMAnalyzeSymbol(struct loader_diff_symbol *symbol, struct loader_binary *input_one, struct loader_binary *input_two, char *output);

char* SDMCreatePathWithName(char *path, char *name) {
	char *subpath = calloc(1, strlen(path)+strlen(name)+2);
	sprintf(subpath,"%s/%s",path,name);
	return subpath;
}

char* SDMCreateDirectoryAtPath(char *name, char *path) {
	char *subpath = SDMCreatePathWithName(path, name);
	bool status = SDMMakeNewFolderAtPath(subpath, 0700);
	if (status == false) {
		printf("error creating subpath for dump %s\n\n",subpath);
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

struct loader_diff * SDMGenerateSymbolList(struct loader_binary *input_one, struct loader_binary *input_two) {
	struct loader_diff *diff = calloc(1, sizeof(struct loader_diff));
	diff->symbol = calloc(1, sizeof(struct loader_diff_symbol));
	diff->name_count = 0;
	
	SDMDiffAddSymbols(diff, input_one, input_two);
	
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
	
	struct loader_subroutine *sub_one = SDMFindSubroutineFromName(input_one, symbol->name);
	
	struct loader_subroutine *sub_two = SDMFindSubroutineFromName(input_two, symbol->name);
	
	CoreRange range_one = SDMSTRangeOfSubroutine(sub_one, input_one);
	
	CoreRange range_two = SDMSTRangeOfSubroutine(sub_two, input_two);
	
	bool should_dump = SDMCompareSymbol(symbol, range_one, input_one, range_two, input_two);
	
	if (should_dump) {
		char *out_path = SDMCreateDirectoryAtPath(symbol->name, output);
		SDMWriteSubroutineData(range_one, out_path, input_one->name, "-1");
		SDMWriteSubroutineData(range_two, out_path, input_two->name, "-2");
		free(out_path);
	}
	
}

void SDMPerformComparison(struct loader_binary *input_one, struct loader_binary *input_two, char *output_path) {
	struct loader_diff *diff = SDMGenerateSymbolList(input_one, input_two);
	//for (uint32_t index = 0; index < diff->name_count; index++) {
	//	SDMAnalyzeSymbol(&(diff->symbol[index]), input_one, input_two, output_path);
	//}
	free(diff);
}

void SDMDiffRelease(struct loader_diff *diff) {
	if (diff) {
		if (diff->symbol) {
			for (uint32_t index = 0; index < diff->name_count; index++) {
				free(diff->symbol[index].name);
			}
			free(diff->symbol);
		}
		free(diff);
	}
}

#endif
