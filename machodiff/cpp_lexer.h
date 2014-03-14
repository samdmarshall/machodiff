//
//  cpp_lexer.h
//  machodiff
//
//  Created by Sam Marshall on 2/24/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_cpp_lexer_h
#define machodiff_cpp_lexer_h

#if __cplusplus
extern "C" {
#endif
	
	char* SDMSTCPPDemangleName(char *name);
	bool SDMSTCPPSymbolName(char *name);
	bool SDMSTCSymbolName(char *name);
	
#if __cplusplus
};
#endif

/*
#define kCPPType_prefix "_Z"

#define kCPPType_void "v"
#define kCPPType_bool "b"
#define kCPPType_char "c"
#define kCPPType_schar "a"
#define kCPPType_uchar "h"
#define kCPPType_short "s"
#define kCPPType_ushort "t"
#define kCPPType_int "i"
#define kCPPType_uint "j"
#define kCPPType_long "l"
#define kCPPType_ulong "m"
#define kCPPType_llong "x"
#define kCPPType_ullong "y"
#define kCPPType_float "f"
#define kCPPType_double "d"
#define kCPPType_ldouble "e"

#define kCPPType_variadic "z"

#define kCPPType_namespace "N"
#define kCPPType_pointer "P"
#define kCPPType_const "K"
#define kCPPType_function "F"
#define kCPPType_reference "R"
#define kCPPType_array "A"
#define kCPPType_literal "L"

#define kCPPType_operator_new "nw"
#define kCPPType_operator_new_array "na"
#define kCPPType_operator_delete "dl"
#define kCPPType_operator_delete_array "da"

#define kCPPType_arguments "E"

#define kCPPType_constructor "C"
#define kCPPType_destructor "D"
#define kCPPType_template "I"

#include "util.h"

struct loader_cpp_token_name {
	char *name;
	uint32_t name_length;
} ATR_PACK;

struct loader_cpp_namespace {
	char *space_name;
} ATR_PACK;

struct loader_cpp_map {
	struct loader_cpp_namespace *cpp_namespace;
	uint32_t namespace_count;
} ATR_PACK;

struct loader_cpp_lexer_token {
	bool has_separator;
	char *type;
	char *type_name;
	struct loader_cpp_lexer_type *child;
} ATR_PACK;

struct loader_cpp_lexer_type {
	struct loader_cpp_map *map;
	struct loader_cpp_lexer_token *token;
	uint32_t token_count;
} ATR_PACK;

struct loader_cpp_map* SDMSTCPPMapInitialize(void);

bool SDMSTCPPSymbolName(char *name);

struct loader_cpp_lexer_type* SDMSTDecodeNameString(struct loader_cpp_map *map, char *name);

char* SDMSTCPPSymbolNameGenerate(struct loader_cpp_lexer_type *token_name, char *separator);

void SDMSTCPPNamespaceRelease(struct loader_cpp_namespace *cpp_namespace);
void SDMSTCPPMapRelease(struct loader_cpp_map *cpp_map);

bool SDMSTCSymbolName(char *name);

char* SDMSTDemangleSymbolName(char *name);
*/

#endif
