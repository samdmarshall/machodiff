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

uint32_t SDMSTParseCPPToken(struct loader_cpp_lexer_type *decode, char *type, uint64_t offset);

uint32_t SDMSTParseCPPToken(struct loader_cpp_lexer_type *decode, char *type, uint64_t offset) {
	uint32_t parsedLength = 1;
	/*
	uint32_t index = k32BitMask;
	for (uint32_t i = 0; i < kObjcTypeEncodingCount; i++) {
		if (strncmp(&(type[offset]), ObjcTypeEncoding[i], sizeof(char)) == 0) {
			index = i;
			break;
		}
	}
	if (index != k32BitMask && index < kObjcTypeEncodingCount) {
		decode->token[decode->tokenCount].typeClass = SDMObjcLexerConvertIndexToToken(index);
		decode->token[decode->tokenCount].type = ObjcTypeEncodingNames[index];
		if (decode->token[decode->tokenCount].typeName == 0) {
			decode->token[decode->tokenCount].typeName = "";
		}
		switch (decode->token[decode->tokenCount].typeClass) {
			case ObjcCharEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcIntEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcShortEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcLongEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcLLongEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcUCharEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcUIntEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcUShortEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcULongEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcULLongEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcFloatEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcDoubleEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcBoolEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcVoidEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcStringEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcIdEncoding: {
				uint64_t next = offset+1;
				if (strncmp(&(type[next]), kObjcNameTokenStart, sizeof(char)) == 0) {
					CoreRange nameRange = SDMSTObjcGetTokenRangeFromOffset(type, next+1, kObjcNameTokenEnd);
					char *name = calloc(1, sizeof(char)*(3+(uint32_t)nameRange.length));
					char *objectProtocolTest = &(type[nameRange.offset]);
					if (strncmp(objectProtocolTest, "<", 1) == 0 && strncmp(objectProtocolTest+(uint32_t)(nameRange.length-1), ">", 1) == 0) {
						sprintf(&(name[0]),"id");
						memcpy(&(name[2]), &(type[nameRange.offset]), sizeof(char)*nameRange.length);
					}
					else {
						memcpy(name, &(type[nameRange.offset]), sizeof(char)*nameRange.length);
						sprintf(name,"%s*",name);
					}
					decode->token[decode->tokenCount].typeName = name;
					parsedLength += nameRange.length + 2;
				}
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcClassEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcSelEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcBitEncoding: {
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			case ObjcPointerEncoding: {
				decode->token[decode->tokenCount].pointerCount++;
				break;
			};
			case ObjcUnknownEncoding: {
				decode->token[decode->tokenCount].typeName = "";
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				break;
			};
			default: {
				break;
			};
		}
	}
	else {
		uint64_t stackSize;
		CoreRange stackRange = SDMSTObjcStackSize(type, offset, &stackSize);
		if (stackRange.length) {
			parsedLength = (uint32_t)stackRange.length;
		}
		else {
			if (strncmp(&(type[offset]), kObjcNameTokenStart, sizeof(char)) == 0) {
				CoreRange nameRange = SDMSTObjcGetTokenRangeFromOffset(type, offset+1, kObjcNameTokenEnd);
				char *name = calloc(1, sizeof(char)*((uint32_t)nameRange.length+256));
				memcpy(name, &(type[nameRange.offset]), sizeof(char)*nameRange.length);
				decode->token[decode->tokenCount].typeName = name;
				parsedLength += (uint32_t)nameRange.length + 1;
			}
			if (strncmp(&(type[offset]), kObjcPointerEncoding, sizeof(char)) == 0) {
				decode->token[decode->tokenCount].pointerCount++;
			}
			if (strncmp(&(type[offset]), kObjcUnknownEncoding, sizeof(char)) == 0) {
				decode->token[decode->tokenCount].typeName = "";
			}
			if (strncmp(&(type[offset]), kObjcStructTokenStart, sizeof(char)) == 0) {
				uint64_t next = offset+1;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				decode->token[decode->tokenCount].typeClass = ObjcStructEncoding;
				decode->token[decode->tokenCount].type = ObjcContainerTypeEncodingNames[0];
				CoreRange contentsRange = SDMSTObjcGetStructContentsRange(type, next);
				char *contents = calloc(1, sizeof(char)*((uint32_t)contentsRange.length+256));
				memcpy(contents, &(type[next]), contentsRange.length);
				CoreRange nameRange = SDMSTObjcGetStructNameRange(contents, 0);
				char *name = calloc(1, sizeof(char)*((uint32_t)nameRange.length+256));
				memcpy(name, &(contents[nameRange.offset]), sizeof(char)*nameRange.length);
				decode->token[decode->tokenCount].typeName = name;
				
				char *structContentString = &(contents[nameRange.offset+nameRange.length])+sizeof(char);
				CoreRange contentRange = CoreRangeCreate(0, strlen(structContentString));
				struct loader_objc_lexer_type *structContents = SDMSTMemberCountOfStructContents(structContentString, contentRange);
				decode->token[decode->tokenCount].childrenCount = structContents->tokenCount;
				decode->token[decode->tokenCount].children = calloc(structContents->tokenCount, sizeof(struct loader_objc_lexer_token));
				for (uint32_t i = 0; i < structContents->tokenCount; i++) {
					struct loader_objc_lexer_token *child = &(decode->token[decode->tokenCount].children[i]);
					struct loader_objc_lexer_token *structMember = &(structContents->token[i]);
					memcpy(child, structMember, sizeof(struct loader_objc_lexer_token));
				}
				parsedLength = (uint32_t)contentsRange.length + 1;
				free(structContents);
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
			}
			if (strncmp(&(type[offset]), kObjcArrayTokenStart, sizeof(char)) == 0) {
				uint64_t next = offset+1;
				uint64_t stackSize;
				CoreRange stackRange = SDMSTObjcStackSize(type, next, &stackSize);
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
				decode->token[decode->tokenCount].typeClass = ObjcArrayEncoding;
				decode->token[decode->tokenCount].arrayCount = (uint32_t)stackSize;
				next += stackRange.length;
				CoreRange arrayTypeRange = SDMSTObjcGetArrayContentsRange(type, next);
				char *arrayTypeString = calloc(1, sizeof(char)*(uint32_t)(arrayTypeRange.length+1));
				memcpy(arrayTypeString, &(type[arrayTypeRange.offset]), arrayTypeRange.length);
				struct loader_objc_lexer_type *arrayType = SDMSTObjcDecodeType(arrayTypeString);
				char *typeAssignment = ObjcTypeEncodingNames[SDMObjcLexerConvertTokenToIndex(ObjcUnknownEncoding)];
				if (arrayType->token[arrayType->tokenCount].type) {
					typeAssignment = arrayType->token[arrayType->tokenCount].type;
				}
				uint32_t typeLength = (uint32_t)strlen(typeAssignment);
				
				decode->token[decode->tokenCount].type = calloc(1, sizeof(char)*(typeLength+1));
				memcpy(decode->token[decode->tokenCount].type, typeAssignment, typeLength);
				
				decode->token[decode->tokenCount].childrenCount = 1;
				decode->token[decode->tokenCount].children = calloc(1, sizeof(struct loader_objc_lexer_token));
				memcpy(decode->token[decode->tokenCount].children, &(arrayType->token[0]), sizeof(struct loader_objc_lexer_token));
				uint64_t arrayTypeNameLength = strlen(arrayType->token[0].typeName);
				char *name = calloc(1, sizeof(char)*((uint32_t)arrayTypeNameLength+1));
				memcpy(name, arrayType->token[0].typeName, arrayTypeNameLength);
				decode->token[decode->tokenCount].typeName = name;
				parsedLength += arrayTypeRange.length + stackRange.length;
				free(arrayType);
				free(arrayTypeString);
				decode->tokenCount++;
				decode->token = realloc(decode->token, sizeof(struct loader_objc_lexer_token)*(decode->tokenCount+1));
			}
		}
	}
	*/
	return parsedLength;
}

struct loader_cpp_lexer_type* SDMSTDecodeNameString(char *name) {
	struct loader_cpp_lexer_type *decode = calloc(1, sizeof(struct loader_cpp_lexer_type));
	decode->token = (struct loader_cpp_lexer_token *)calloc(1, sizeof(struct loader_cpp_lexer_token));
	//uint64_t length = strlen(name);
	return decode;
}

#pragma mark -

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
	int found_length = sscanf(name, "%d", &component_length);
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
		int found_length = sscanf(name, "%d", &component_length);
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