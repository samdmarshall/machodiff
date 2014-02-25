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
#include <string.h>

bool SDMSTDemangleSymbolCPPNamePeekComponent(char *name);
char* SDMSTDemangleCPPArgument(char *clean_name, char *name);
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
	int found_length = sscanf(name, "%ld", &component_length);
	if (found_length == 1) {
		status = true;
	}
	return status;
}

#define CPP_ResolveName(name, type, size) ((memcmp(name, type, sizeof(char)*size) == 0) ? true : false)

char* SDMSTDemangleCPPArgument(char *clean_name, char *name) {
	
	// SDM: primitives
	if (CPP_ResolveName(name, "v", 1)) {
		// void
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "void", 4);
	}
	else if (CPP_ResolveName(name, "m", 1)) {
		// unsigned long
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "unsigned long", 13);
	}
	else if (CPP_ResolveName(name, "b", 1)) {
		// bool
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "bool", 4);
	}
	else if (CPP_ResolveName(name, "l", 1)) {
		// long
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "long", 4);
	}
	else if (CPP_ResolveName(name, "j", 1)) {
		// unsigned int
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "unsigned int", 13);
	}
	else if (CPP_ResolveName(name, "t", 1)) {
		// unsigned short
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "unsigned short", 14);
	}
	else if (CPP_ResolveName(name, "c", 1)) {
		// char
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "char", 4);
	}
	else if (CPP_ResolveName(name, "x", 1)) {
		// char
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "long long", 9);
	}
	
	// SDM: other types
	else if (CPP_ResolveName(name, "P", 1)) {
		// pointer
		clean_name = SDMSTDemangleCPPArgument(clean_name, PtrAdd(name, sizeof(char)));
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, " * ", 3);
	}
	else if (CPP_ResolveName(name, "K", 1)) {
		// const
		clean_name = SDMSTDemangleCPPArgument(clean_name, PtrAdd(name, sizeof(char)));
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, " const ", 7);
	}
	else if (CPP_ResolveName(name, "F", 1)) {
		// function pointer
		clean_name = SDMSTDemangleCPPArgument(clean_name, PtrAdd(name, sizeof(char)));
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "(*)()", 5);
	}
	else if (CPP_ResolveName(name, "R", 1)) {
		// Reference
		clean_name = SDMSTDemangleCPPArgument(clean_name, PtrAdd(name, sizeof(char)));
		clean_name = SDMSTDemangleSymbolNameRealloc(clean_name, "&", 1);
	}
	
	else {
		uint32_t component_length = 0;
		int found_length = sscanf(name, "%ld", &component_length);
		if (found_length == 1) {
			// defined type
		} else {
			
		}
	}
	
	return clean_name;
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
							
							clean_name = SDMSTDemangleCPPArgument(clean_name, &(name[counter]));
							
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

#endif