//
//  cpp_lexer.c
//  machodiff
//
//  Created by Sam Marshall on 2/24/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_cpp_lexer_c
#define machodiff_cpp_lexer_c

#include "cpp_lexer.h"
#include <cxxabi.h>
#include <string.h>

char* SDMSTCPPDemangleName(char *name) {
	int status = 0;
	char *clean_name = __cxxabiv1::__cxa_demangle(name, 0, 0, &status);
	if (status == 0) {
		return clean_name;
	}
	return name;
}

#define CPP_ResolveName(name, type, size) ((memcmp(name, type, sizeof(char)*size) == 0) ? true : false)


bool SDMSTCPPSymbolName(char *name) {
	return CPP_ResolveName(name, "__Z", 3);
}

bool SDMSTCSymbolName(char *name) {
	if (!SDMSTCPPSymbolName(name)) {
		return CPP_ResolveName(name, "_", 1);
	}
	return false;
}

/*
#define kNew "new"
#define kNewA "new[]"
#define kDelete "delete"
#define kDeleteA "delete[]"

struct loader_cpp_type_resolve {
	char *type_code;
	char *type_name;
} ATR_PACK;

enum loader_cpp_known_primitive_resolve {
	loader_cpp_known_primitive_resolve_void = 0,
	loader_cpp_known_primitive_resolve_bool,
	loader_cpp_known_primitive_resolve_char,
	loader_cpp_known_primitive_resolve_schar,
	loader_cpp_known_primitive_resolve_uchar,
	loader_cpp_known_primitive_resolve_short,
	loader_cpp_known_primitive_resolve_ushort,
	loader_cpp_known_primitive_resolve_int,
	loader_cpp_known_primitive_resolve_uint,
	loader_cpp_known_primitive_resolve_long,
	loader_cpp_known_primitive_resolve_ulong,
	loader_cpp_known_primitive_resolve_llong,
	loader_cpp_known_primitive_resolve_ullong,
	loader_cpp_known_primitive_resolve_float,
	loader_cpp_known_primitive_resolve_double,
	loader_cpp_known_primitive_resolve_ldouble,
	loader_cpp_known_primitive_resolve_variadic,
	loader_cpp_known_primitive_resolve_count
};

static struct loader_cpp_type_resolve CPPKnownPrimitiveTypes[loader_cpp_known_primitive_resolve_count] = {
	{kCPPType_void, "void"},
	{kCPPType_bool, "bool"},
	{kCPPType_char, "char"},
	{kCPPType_schar, "signed char"},
	{kCPPType_uchar, "unsigned char"},
	{kCPPType_short, "short"},
	{kCPPType_ushort, "unsigned short"},
	{kCPPType_int, "int"},
	{kCPPType_uint, "unsigned int"},
	{kCPPType_long, "long"},
	{kCPPType_ulong, "unsigned long"},
	{kCPPType_llong, "long long"},
	{kCPPType_ullong, "unsigned long long"},
	{kCPPType_float, "float"},
	{kCPPType_double, "double"},
	{kCPPType_ldouble, "long double"},
	{kCPPType_variadic, "..."},
};

enum loader_cpp_known_container_resolve {
	loader_cpp_known_container_resolve_namespace = 0,
	loader_cpp_known_container_resolve_pointer,
	loader_cpp_known_container_resolve_const,
	loader_cpp_known_container_resolve_function,
	loader_cpp_known_container_resolve_reference,
	loader_cpp_known_container_resolve_array,
	loader_cpp_known_container_resolve_literal,
	loader_cpp_known_container_resolve_count
};

static struct loader_cpp_type_resolve CPPKnownContainerTypes[loader_cpp_known_container_resolve_count] = {
	{kCPPType_namespace, ""},
	{kCPPType_pointer, "*"},
	{kCPPType_const, "const"},
	{kCPPType_function, "(*)()"},
	{kCPPType_reference, "&"},
	{kCPPType_array, ""},
	{kCPPType_literal, ""}
};

bool SDMSTCPPNamespaceHasName(struct loader_cpp_map *cpp_map, char *name);
void SDMSTCPPNamespaceAddname(struct loader_cpp_map *cpp_map, char *name);

struct loader_cpp_token_name* SDMSTParseCPPTokenName(char *type);

bool SDMSTCanParseCPPTokenName(char *type);

uint32_t SDMSTDemangleCPPArgument(char *name, struct loader_cpp_lexer_token *child);

uint32_t SDMSTParseCPPToken(struct loader_cpp_lexer_type *decode, char *type, uint64_t offset);

struct loader_cpp_map* SDMSTCPPMapInitialize(void) {
	struct loader_cpp_map *cpp_map = calloc(1, sizeof(struct loader_cpp_map));
	cpp_map->cpp_namespace = calloc(1, sizeof(struct loader_cpp_namespace));
	cpp_map->namespace_count = 0;
	return cpp_map;
}
 
bool SDMSTCPPNamespaceHasName(struct loader_cpp_map *cpp_map, char *name) {
	bool has_namespace = false;
	for (uint32_t index = 0; index < cpp_map->namespace_count; index++) {
		char *namespace_name = cpp_map->cpp_namespace[index].space_name;
		if (strcmp(namespace_name, name) == 0) {
			return true;
		}
	}
	return has_namespace;
}

void SDMSTCPPNamespaceAddname(struct loader_cpp_map *cpp_map, char *name) {
	cpp_map->cpp_namespace = realloc(cpp_map->cpp_namespace, sizeof(struct loader_cpp_namespace)*(cpp_map->namespace_count+1));
	cpp_map->cpp_namespace[cpp_map->namespace_count].space_name = name;
	cpp_map->namespace_count++;
}

struct loader_cpp_token_name* SDMSTParseCPPTokenName(char *type) {
	struct loader_cpp_token_name *token_name = calloc(1, sizeof(struct loader_cpp_token_name));
	token_name->name_length = 0;
	if (type) {
		uint32_t component_length = 0;
		int found_length = sscanf(type, "%d", &(token_name->name_length));
		if (found_length == 1) {
			component_length += GetDigitsOfNumber(token_name->name_length);
		}
		token_name->name = calloc(component_length+1, sizeof(char));
		memcpy(token_name->name, &(type[component_length]), sizeof(char)*token_name->name_length);
	}
	return token_name;
}

bool SDMSTCanParseCPPTokenName(char *type) {
	bool can_parse = false;
	uint32_t name_length = 0;
	int found_length = sscanf(type, "%d", &(name_length));
	if (found_length == 1) {
		can_parse = true;
	}
	return can_parse;
}

uint32_t SDMSTDemangleCPPArgument(char *name, struct loader_cpp_lexer_token *child) {
	uint32_t parse_length = 0;
	
	bool found_type = false;
	for (uint32_t index = 0; index < loader_cpp_known_primitive_resolve_count; index++) {
		char *type_code = CPPKnownPrimitiveTypes[index].type_code;
		uint32_t code_length = (uint32_t)strlen(type_code);
		
		char *type_name = CPPKnownPrimitiveTypes[index].type_name;
		uint32_t name_length = (uint32_t)strlen(type_name);
		
		if (CPP_ResolveName(name, type_code, code_length)) {
			child->type = calloc(code_length+1, sizeof(char));
			memcpy(child->type, type_code, code_length);
			
			child->type_name = calloc(name_length+1, sizeof(char));
			memcpy(child->type_name, type_name, name_length);
			
			found_type = true;
			parse_length += code_length;
			
			break;
		}
	}
	
	if (!found_type) {
		// SDM: other types
		if (CPP_ResolveName(name, "P", 1)) {
			// pointer
			//clean_name = SDMSTDemangleCPPArgument(clean_name, PtrAdd(name, sizeof(char)));
			//clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, " * ", 3);
		}
		else if (CPP_ResolveName(name, "K", 1)) {
			// const
			//clean_name = SDMSTDemangleCPPArgument(clean_name, PtrAdd(name, sizeof(char)));
			//clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, " const ", 7);
		}
		else if (CPP_ResolveName(name, "F", 1)) {
			// function pointer
			//clean_name = SDMSTDemangleCPPArgument(clean_name, PtrAdd(name, sizeof(char)));
			//clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "(*)()", 5);
		}
		else if (CPP_ResolveName(name, "R", 1)) {
			// Reference
			//clean_name = SDMSTDemangleCPPArgument(clean_name, PtrAdd(name, sizeof(char)));
			//clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "&", 1);
		}
		else {
			bool defined_type = SDMSTCanParseCPPTokenName(name);
			if (defined_type) {
				// defined type
			} else {
				
			}
		}
	}
	
	return parse_length;
}

uint32_t SDMSTParseCPPToken(struct loader_cpp_lexer_type *decode, char *type, uint64_t offset) {
	uint32_t parsedLength = 0;
	if (offset == 1) {
		if (CPP_ResolveName(type, kCPPType_prefix, strlen(kCPPType_prefix))) {
			parsedLength = 2;
		}
		else {
			printf("This isn't a cpp mangled name!\n");
		}
	}
	else {
		decode->token = realloc(decode->token, sizeof(struct loader_cpp_lexer_token)*(decode->token_count+1));

		if (CPP_ResolveName(type, kCPPType_namespace, strlen(kCPPType_namespace))) {
			parsedLength = 1;
			
			uint32_t token_name_length = 0;
			
			bool can_parse = SDMSTCanParseCPPTokenName(&(type[parsedLength]));
			if (can_parse) {
				struct loader_cpp_token_name *token = SDMSTParseCPPTokenName(&(type[parsedLength]));
				if (!SDMSTCPPNamespaceHasName(decode->map, token->name)) {
					SDMSTCPPNamespaceAddname(decode->map, token->name);
				}
				
				decode->token[decode->token_count].type_name = calloc(token->name_length+1, sizeof(char));
				memcpy(decode->token[decode->token_count].type_name, token->name, sizeof(char)*token->name_length);
				
				token_name_length = token->name_length;
				
				parsedLength += GetDigitsOfNumber(token_name_length);

			}
			else {
				char *short_name = &(type[parsedLength]);
				
				char *end_name = strchr(short_name, '_');
				uint32_t name_length = (uint32_t)((uint64_t)end_name - (uint64_t)short_name) + 1;
				for (uint32_t index = 0; index < decode->map->namespace_count; index++) {
					char *check_namespace = decode->map->cpp_namespace[index].space_name;
					if (strncasecmp(check_namespace, short_name, name_length) == 0) {
						token_name_length = (uint32_t)strlen(check_namespace);
						decode->token[decode->token_count].type_name = calloc(token_name_length+1, sizeof(char));
						memcpy(decode->token[decode->token_count].type_name, check_namespace, sizeof(char)*token_name_length);
					}
				}
				
			}
			
			decode->token[decode->token_count].type = kCPPType_namespace;
			
			parsedLength += token_name_length;
			
			decode->token_count++;
		}
		
		bool can_parse = SDMSTCanParseCPPTokenName(&(type[parsedLength]));
		if (can_parse) {
			struct loader_cpp_token_name *token = SDMSTParseCPPTokenName(&(type[parsedLength]));
			
			decode->token[decode->token_count].type_name = calloc(token->name_length+1, sizeof(char));
			memcpy(decode->token[decode->token_count].type_name, token->name, sizeof(char)*token->name_length);
			decode->token[decode->token_count].type = "";
			
			parsedLength += token->name_length + GetDigitsOfNumber(token->name_length);
		}
		else {
			
			if (CPP_ResolveName(type, "L", 1)) {
				// ::
				decode->token[decode->token_count].has_separator = true;
								
				parsedLength += 1;
			}
			else if (CPP_ResolveName(type, "C", 1)) {
				// constructor
			}
			else if (CPP_ResolveName(type, "D", 1)) {
				// destructor
			}
			else if (CPP_ResolveName(type, "I", 1)) {
				// <>
			}
			else if (CPP_ResolveName(type, "E", 1)) {
				// SDM: end of function name
				
				decode->token[decode->token_count].has_separator = true;
				
				decode->token[decode->token_count].type_name = NULL;
				decode->token[decode->token_count].type = kCPPType_arguments;
				
				parsedLength += 1;
				
				decode->token[decode->token_count].child = calloc(1, sizeof(struct loader_cpp_lexer_type));
				decode->token[decode->token_count].child->token = calloc(1, sizeof(struct loader_cpp_lexer_token));
				decode->token[decode->token_count].child->token_count = 0;
				
				char *all_arguments = &(type[1]);
				char *arguments = all_arguments;
				
				uint32_t argument_length = 0;
				while (argument_length < strlen(all_arguments)) {
					decode->token[decode->token_count].child->token = realloc(decode->token[decode->token_count].child->token, sizeof(struct loader_cpp_lexer_token)*(decode->token[decode->token_count].child->token_count+1));
					uint32_t length = SDMSTDemangleCPPArgument(arguments, &(decode->token[decode->token_count].child->token[decode->token[decode->token_count].child->token_count]));
					decode->token[decode->token_count].child->token_count++;
					arguments = &(arguments[length]);
					argument_length += length;
				}
				
				parsedLength += argument_length;
				
			}
			else if (CPP_ResolveName(type, "nw", 2)) {
				// SDM: operator new
				// "::new"
				decode->token[decode->token_count].has_separator = true;
				
				decode->token[decode->token_count].type_name = calloc(strlen(kNew)+1, sizeof(char));
				memcpy(decode->token[decode->token_count].type_name, kNew, sizeof(char)*strlen(kNew));
				decode->token[decode->token_count].type = "";
				
				parsedLength += 2;
			}
			else if (CPP_ResolveName(type, "na", 2)) {
				// SDM: operator new
				// "::new[]"
				decode->token[decode->token_count].has_separator = true;
				
				decode->token[decode->token_count].type_name = calloc(strlen(kNewA)+1, sizeof(char));
				memcpy(decode->token[decode->token_count].type_name, kNewA, sizeof(char)*strlen(kNewA));
				decode->token[decode->token_count].type = "";
				
				parsedLength += 2;
			}
			else if (CPP_ResolveName(type, "da", 2)) {
				// SDM: operator new
				// "::delete[]"
				decode->token[decode->token_count].has_separator = true;
				
				decode->token[decode->token_count].type_name = calloc(strlen(kDeleteA)+1, sizeof(char));
				memcpy(decode->token[decode->token_count].type_name, kDeleteA, sizeof(char)*strlen(kDeleteA));
				decode->token[decode->token_count].type = "";
				
				parsedLength += 2;
			}
			else if (CPP_ResolveName(type, "na", 2)) {
				// SDM: operator new
				// "::delete"
				decode->token[decode->token_count].has_separator = true;
				
				decode->token[decode->token_count].type_name = calloc(strlen(kDelete)+1, sizeof(char));
				memcpy(decode->token[decode->token_count].type_name, kDelete, sizeof(char)*strlen(kDelete));
				decode->token[decode->token_count].type = "";
				
				parsedLength += 2;
			}
			
		}
		
		decode->token_count++;

	}
	
	return parsedLength;
}

struct loader_cpp_lexer_type* SDMSTDecodeNameString(struct loader_cpp_map *map, char *name) {
	struct loader_cpp_lexer_type *decode = calloc(1, sizeof(struct loader_cpp_lexer_type));
	decode->token = (struct loader_cpp_lexer_token *)calloc(1, sizeof(struct loader_cpp_lexer_token));
	decode->map = map;
	uint64_t length = strlen(name);
	if (length) {
		if (CPP_ResolveName(name, "_", 1)) {
			uint64_t offset = 1;
			while (offset < length) {
				uint32_t parsedLength = SDMSTParseCPPToken(decode, &(name[offset]), offset);
				offset = offset + parsedLength;
			}
		}
	}
	return decode;
}

#pragma mark -

bool SDMSTDemangleSymbolCPPNamePeekComponent(char *name);
//char* SDMSTDemangleCPPArgument(char *clean_name, char *name);
char* SDMSTDemangleSymbolNameRealloc(char *name, char *component, uint32_t component_length);

char* SDMSTDemangleSymbolNameRealloc(char *name, char *component, uint32_t component_length) {
	uint32_t length = (uint32_t)strlen(name);
	name = realloc(name, sizeof(char)*(length+1+component_length));
	memset(&(name[length]), 0, sizeof(char)*(component_length+1));
	strncpy(&(name[length]), component, component_length);
	return name;
}

bool SDMSTDemangleSymbolCPPNamePeekComponent(char *name) {
	bool status = false;
	uint32_t component_length = 0;
	int found_length = sscanf(name, "%d", &component_length);
	if (found_length == 1) {
		status = true;
	}
	return status;
}

char* SDMSTDemangleSymbolName(char *name) {
	uint32_t counter = 1;
	char *clean_name = calloc(1, sizeof(char));
	if (CPP_ResolveName(name,"_", 1)) {
		if (CPP_ResolveName(&(name[1]), "_Z", 2)) {
			// SDM: C++ name
			
			counter += 2;
			
			bool nested = CPP_ResolveName(&(name[counter]), "N", 1);
			if (nested) {
				counter++;
			}
			
			while (counter < strlen(name)) {
				uint32_t component_length = 0;
				int found_length = sscanf(&(name[counter]), "%d", &component_length);
				if (found_length == 1) {
					counter += GetDigitsOfNumber(component_length);
					clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, &(name[counter]), component_length);
					bool another_component = SDMSTDemangleSymbolCPPNamePeekComponent(&(name[counter+component_length]));
					if (nested && another_component) {
						clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "::", 2);
					}
					counter += component_length;
				}
				else {
					
					if (CPP_ResolveName(&(name[counter]), "L", 1)) {
						if (strlen(clean_name)) {
							clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "::", 2);
						}
						counter++;
					}
					else if (CPP_ResolveName(&(name[counter]), "C", 1)) {
						// constructor
						counter += 2;
					}
					else if (CPP_ResolveName(&(name[counter]), "D", 1)) {
						// destructor
						counter += 2;
					}
					else if (CPP_ResolveName(&(name[counter]), "I", 1)) {
						// <>
						clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "<", 1);
						counter++;
						uint32_t type_length = 0;
						bool named = SDMSTDemangleSymbolCPPNamePeekComponent(&(name[counter]));
						while (named == false) {
							counter++;
							named = SDMSTDemangleSymbolCPPNamePeekComponent(&(name[counter]));
						}
						if (named) {
							int found_length = sscanf(&(name[counter]), "%d", &type_length);
							if (found_length == 1) {
								counter += GetDigitsOfNumber(type_length);
								clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, &(name[counter]), type_length);
							}
						}
						clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, ">", 1);
						counter += type_length;
					}
					else if (CPP_ResolveName(&(name[counter]), "E", 1)) {
						// SDM: end of function name
						clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "(", 1);
						counter++;
						// SDM: loop contents
						while (counter < strlen(name)) {
							uint32_t original_length = (uint32_t)strlen(clean_name);
							
							//clean_name = SDMSTDemangleCPPArgument(clean_name, &(name[counter]));
							
							uint32_t new_lenth = (uint32_t)strlen(clean_name);
							
							counter += (uint32_t)((new_lenth - original_length) != 0 ? : 1);
							
							if (counter < strlen(name)) {
								clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, ", ", 2);
							}
						}
					}
					else if (CPP_ResolveName(&(name[counter]), "nw", 2)) {
						// SDM: operator new
						clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "::new", 5);
						counter += 2;
					}
					else if (CPP_ResolveName(&(name[counter]), "na", 2)) {
						// SDM: operator new
						clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "::new[]", 7);
						counter += 2;
					}
					else if (CPP_ResolveName(&(name[counter]), "da", 2)) {
						// SDM: operator new
						clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "::delete[]", 10);
						counter += 2;
					}
					else if (CPP_ResolveName(&(name[counter]), "na", 2)) {
						// SDM: operator new
						clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "::delete", 8);
						counter += 2;
					}
					else  {
						break;
					}
				}
			}
			
			clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, ")", 1);
			
		}
		else {
			// SDM: C name
			clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, &(name[counter]), (uint32_t)strlen(name));
		}
	}
	else {
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, name, (uint32_t)strlen(name));
	}
	return clean_name;
}

char* SDMSTCPPSymbolNameGenerate(struct loader_cpp_lexer_type *token_name, char *separator) {
	uint32_t name_length = 0;
	char *name = calloc(name_length+1, sizeof(char));
	
	for (uint32_t index = 0; index < token_name->token_count; index++) {
		struct loader_cpp_lexer_token *item = &(token_name->token[index]);
		if (item->type_name) {
			uint32_t append_length = (uint32_t)strlen(item->type_name);
			name = realloc(name, sizeof(char)*(name_length+append_length+1));
			memcpy(&(name[name_length]), item->type_name, append_length);
			name_length += append_length;
		}
		
		if (CPP_ResolveName(item->type, kCPPType_arguments, 1)) {
			name = realloc(name, sizeof(char)*(name_length+1+1));
			memcpy(&(name[name_length]), "(", 1);
			name_length++;

			char *arguments = SDMSTCPPSymbolNameGenerate(token_name->token[index].child, ",");
			uint32_t arguments_length = (uint32_t)strlen(arguments);
			name = realloc(name, sizeof(char)*(name_length+arguments_length+1));
			memcpy(&(name[name_length]), arguments, arguments_length);
			name_length += arguments_length;
			free(arguments);
			
			name = realloc(name, sizeof(char)*(name_length+1+1));
			memcpy(&(name[name_length]), ")", 1);
			name_length++;
			
		}
		
		if (!item->has_separator && index+1 < token_name->token_count && !CPP_ResolveName(token_name->token[index+1].type, kCPPType_arguments, 1)) {
			uint32_t separator_length = (uint32_t)strlen(separator);
			name = realloc(name, sizeof(char)*(name_length+separator_length+1));
			memcpy(&(name[name_length]), separator, separator_length);
			name_length += separator_length;
		}
	}
	
	name[name_length] = '\0';
	
	return name;
}

void SDMSTCPPNamespaceRelease(struct loader_cpp_namespace *cpp_namespace) {
	free(cpp_namespace->space_name);
}

void SDMSTCPPMapRelease(struct loader_cpp_map *cpp_map) {
	if (cpp_map) {
		if (cpp_map->cpp_namespace) {
			for (uint32_t index = 0; index < cpp_map->namespace_count; index++) {
				SDMSTCPPNamespaceRelease(&(cpp_map->cpp_namespace[index]));
			}
			free(cpp_map->cpp_namespace);
		}
		free(cpp_map);
	}
}

*/

#endif