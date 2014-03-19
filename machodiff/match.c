//
//  match.c
//  machodiff
//
//  Created by Sam Marshall on 3/19/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_match_c
#define machodiff_match_c

#include "match.h"

struct loader_match_tree * SDMBuildMatchTree(CoreRange buffer1, CoreRange buffer2) {
	struct loader_match_tree *root = calloc(1, sizeof(struct loader_match_tree));
	root->parent = NULL;
	struct loader_match_tree *current_node = root;
	uint64_t counter = 1;
	bool match_result = false;
	uint64_t lesser_length = (buffer1.length < buffer2.length ? buffer1.length : buffer2.length);
	uint64_t compare_length = 0;
	uint8_t *offset1 = PtrCast(Ptr(buffer1.offset), uint8_t *);
	uint8_t *offset2 = PtrCast(Ptr(buffer2.offset), uint8_t *);
	
	while (compare_length < lesser_length) {
		match_result = (memcmp(offset1, offset2, sizeof(char[counter])) == 0);
		if (match_result) {
			counter++;
			compare_length++;
		}
		else {
			if (counter > 1) {
				// SDM: break in chain
				current_node->matched1 = CoreRangeCreate((uint64_t)offset1, counter);
				current_node->matched2 = CoreRangeCreate((uint64_t)offset2, counter);
				struct loader_match_tree *child = calloc(1, sizeof(struct loader_match_tree));
				child->parent = current_node;
				current_node->child = child;
			
				current_node = child;
			}
			else {
				compare_length++;
			}
			
			offset1 = (uint8_t*)PtrAdd(offset1, counter);
			offset2 = (uint8_t*)PtrAdd(offset2, counter);
			
			counter = 1;
		}
	}
	
	return root;
}

uint8_t SDMMatchPercentFromTree(struct loader_match_tree *tree, uint64_t total_length) {
	uint64_t matched_length = SDMMatchLengthFromTree(tree);
	double matched = (double)matched_length;
	double total = (double)total_length;
	uint8_t result = (uint8_t)round((matched/total)*100);
	
	return result;
}

uint64_t SDMMatchLengthFromTree(struct loader_match_tree *tree) {
	uint64_t matched_length = 0;
	if (tree) {
		struct loader_match_tree *node = tree->child;
		if (node) {
			matched_length += SDMMatchLengthFromTree(node);
		}
		matched_length += tree->matched1.length;
	}
	return matched_length;
}

void SDMReleaseMatchTree(struct loader_match_tree *tree) {
	if (tree) {
		struct loader_match_tree *node = tree->child;
		if (node) {
			SDMReleaseMatchTree(node);
		}
		free(tree);
	}
}

#endif
