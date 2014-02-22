//
//  objc_lexer.c
//  loader
//
//  Created by Sam Marshall on 11/3/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_objc_lexer_c
#define loader_objc_lexer_c

#include "objc_lexer.h"
#include <objc/runtime.h>
#include <string.h>

#define kObjcTypeEncodingCount 21

static char* ObjcTypeEncoding[kObjcTypeEncodingCount] = {
	kObjcCharEncoding,
	kObjcIntEncoding,
	kObjcShortEncoding,
	kObjcLongEncoding,
	kObjcLLongEncoding,
	kObjcUCharEncoding,
	kObjcUIntEncoding,
	kObjcUShortEncoding,
	kObjcULongEncoding,
	kObjcULLongEncoding,
	kObjcFloatEncoding,
	kObjcDoubleEncoding,
	kObjcBoolEncoding,
	kObjcVoidEncoding,
	kObjcStringEncoding,
	kObjcIdEncoding,
	kObjcClassEncoding,
	kObjcSelEncoding,
	kObjcBitEncoding,
	kObjcPointerEncoding,
	kObjcUnknownEncoding
};

static char* ObjcTypeEncodingNames[kObjcTypeEncodingCount] = {
	"char",
	"int",
	"short",
	"long",
	"long long",
	"unsigned char",
	"unsigned int",
	"unsigned short",
	"unsigned long",
	"unsigned long long",
	"float",
	"double",
	"bool",
	"void",
	"char*",
	"id",
	"Class",
	":",
	"bitmask",
	"*",
	"UnknownType"
};

#define kObjcContainerTypeEncodingCount 1

static char *ObjcContainerTypeEncodingNames[kObjcContainerTypeEncodingCount] = {
	"struct"
};

#define kObjcStackSizeCount 10

static char *ObjcStackSize[kObjcStackSizeCount] = {
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9"
};

CoreRange SDMSTObjcStackSize(char *type, uint64_t offset, uint64_t *stackSize) {
	uint64_t counter = 0;
	bool findStackSize = true;
	while (findStackSize) {
		findStackSize = false;
		for (uint32_t i = 0; i < kObjcStackSizeCount; i++) {
			if (strncmp(&(type[offset+counter]), ObjcStackSize[i], sizeof(char)) == 0) {
				counter++;
				findStackSize = true;
				break;
			}
		}
	}
	CoreRange stackRange = CoreRangeCreate((uintptr_t)offset, counter);
	char *stack = calloc((uint32_t)stackRange.length+1, sizeof(char));
	memcpy(stack, &(type[offset]), (uint32_t)stackRange.length);
	*stackSize = atoi(stack);
	free(stack);
	return stackRange;
}

char* SDMSTObjcPointersForToken(struct loader_objc_lexer_token *token) {
	char *pointers = calloc(1, sizeof(char)*(token->pointerCount+1));
	if (token->pointerCount) {
		for (uint32_t i = 0; i < token->pointerCount; i++) {
			pointers = strcat(pointers, "*");
		}
	}
	return pointers;
}

CoreRange SDMSTObjcGetTokenRangeFromOffset(char *type, uint64_t offset, char *token) {
	uint64_t counter = 0;
	while ((strncmp(&(type[offset+counter]), token, strlen(token)) != 0) && offset+counter < strlen(type)) {
		counter++;
	}
	return CoreRangeCreate((uintptr_t)offset, counter);
}

char* SDMSTObjcCreateMethodDescription(struct loader_objc_lexer_type *type, char *name) {
	uint32_t nameLength = 1;
	if (name) {
		nameLength += strlen(name);
	} else {
		name = "";
	}
	char *description = calloc(1, sizeof(char)*(nameLength+3+strlen(type->token[0].type)));
	uint32_t counter = 0;
	uint32_t argCount = 0;
	for (uint32_t i = counter+3; i < type->tokenCount; i++) {
		argCount++;
	}
	if (counter != argCount) {
		sprintf(description,"(%s)",type->token[0].type);
		uint32_t offset = 0;
		while (counter < argCount) {
			if (offset) {
				description = realloc(description, sizeof(char)*(strlen(description)+2));
				sprintf(description,"%s ",description);
			}
			CoreRange methodArgRange = SDMSTObjcGetTokenRangeFromOffset(name, offset, kObjcSelEncoding);
			char *argName = calloc(1, sizeof(char)*((uint32_t)methodArgRange.length+1));
			memcpy(argName, &(name[offset]), (uint32_t)methodArgRange.length);
			uint32_t formatLength = (uint32_t)(8+strlen(argName)+strlen(type->token[counter+3].type)+GetDigitsOfNumber(counter));
			char *formatName = calloc(1, sizeof(char)*formatLength);
			sprintf(formatName,"%s:(%s)_arg%01i",argName,type->token[counter+3].type,counter);
			description = realloc(description, sizeof(char)*(strlen(description)+formatLength));
			memcpy(&(description[strlen(description)]), formatName, formatLength);
			free(formatName);
			free(argName);
			offset = offset + (uint32_t)methodArgRange.length + 1;
			counter++;
		}
	} else {
		sprintf(description,"(%s)%s",type->token[0].type,name);
	}
	description = realloc(description, sizeof(char)*(strlen(description)+2));
	sprintf(description,"%s;",description);
	return description;
}

CoreRange SDMSTObjcGetRangeFromTokens(char *startToken, char *endToken, char *type, uint64_t offset) {
	uint64_t stack = 1;
	uint64_t counter = 0;
	while (stack != 0) {
		if (strncmp(&(type[offset+counter]), startToken, sizeof(char)) == 0) {
			stack++;
		}
		if (strncmp(&(type[offset+counter]), endToken, sizeof(char)) == 0) {
			stack--;
		}
		counter++;
	}
	counter--;
	return CoreRangeCreate((uintptr_t)offset, counter);

}

CoreRange SDMSTObjcGetStructContentsRange(char *type, uint64_t offset) {
	return SDMSTObjcGetRangeFromTokens(kObjcStructTokenStart, kObjcStructTokenEnd, type, offset);
}

CoreRange SDMSTObjcGetArrayContentsRange(char *type, uint64_t offset) {
	return SDMSTObjcGetRangeFromTokens(kObjcArrayTokenStart, kObjcArrayTokenEnd, type, offset);
}

CoreRange SDMSTObjcGetStructNameRange(char *contents, uint64_t offset) {
	return SDMSTObjcGetTokenRangeFromOffset(contents, offset, kObjcStructDefinitionToken);
}

struct loader_objc_lexer_type* SDMSTMemberCountOfStructContents(char *structContents, CoreRange nameRange) {
	return SDMSTObjcDecodeTypeWithLength(structContents, nameRange.length);
}

uint32_t SDMSTParseToken(struct loader_objc_lexer_type *decode, char *type, uint64_t offset) {
	uint32_t parsedLength = 1;
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
					} else {
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
	} else {
		uint64_t stackSize;
		CoreRange stackRange = SDMSTObjcStackSize(type, offset, &stackSize);
		if (stackRange.length) {
			parsedLength = (uint32_t)stackRange.length;
		} else {
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
	return parsedLength;
}

struct loader_objc_lexer_type* SDMSTObjcDecodeTypeWithLength(char *type, uint64_t decodeLength) {
	struct loader_objc_lexer_type *decode = calloc(1, sizeof(struct loader_objc_lexer_type));
	decode->token = (struct loader_objc_lexer_token *)calloc(1, sizeof(struct loader_objc_lexer_token));
	uint64_t length = decodeLength;
	if (length) {
		uint64_t offset = 0;
		while (offset < length) {
			uint32_t parsedLength = SDMSTParseToken(decode, type, offset);
			offset = offset + parsedLength;
		}
		
	}
	return decode;
}

struct loader_objc_lexer_type* SDMSTObjcDecodeType(char *type) {
	return SDMSTObjcDecodeTypeWithLength(type, strlen(type));
}

uint64_t SDMSTObjcDecodeSizeOfType(struct loader_objc_lexer_token *token) {
	uint64_t size = 0;
	if (token) {
		if (token->childrenCount) {
			for (uint32_t i = 0; i < token->childrenCount; i++) {
				size += SDMSTObjcDecodeSizeOfType(&(token->children[i]));
			}
		} else {
			if (token->pointerCount) {
				size += sizeof(Pointer);
			} else {
				switch (token->typeClass) {
					case ObjcCharEncoding: {
						size += sizeof(char);
						break;
					};
					case ObjcIntEncoding: {
						size += sizeof(int);
						break;
					};
					case ObjcShortEncoding: {
						size += sizeof(short);
						break;
					};
					case ObjcLongEncoding: {
						size += sizeof(long);
						break;
					};
					case ObjcLLongEncoding: {
						size += sizeof(long long);
						break;
					};
					case ObjcUCharEncoding: {
						size += sizeof(unsigned char);
						break;
					};
					case ObjcUIntEncoding: {
						size += sizeof(unsigned int);
						break;
					};
					case ObjcUShortEncoding: {
						size += sizeof(unsigned short);
						break;
					};
					case ObjcULongEncoding: {
						size += sizeof(unsigned long);
						break;
					};
					case ObjcULLongEncoding: {
						size += sizeof(unsigned long long);
						break;
					};
					case ObjcFloatEncoding: {
						size += sizeof(float);
						break;
					};
					case ObjcDoubleEncoding: {
						size += sizeof(double);
						break;
					};
					case ObjcBoolEncoding: {
						size += sizeof(signed char);
						break;
					};
					case ObjcStringEncoding: {
						size += sizeof(char *);
						break;
					};
					case ObjcIdEncoding: {
						size += sizeof(id);
						break;
					};
					case ObjcClassEncoding: {
						size += sizeof(Class);
						break;
					};
					case ObjcSelEncoding: {
						size += sizeof(SEL);
						break;
					};
					case ObjcBitEncoding: {
						size += sizeof(char);
						break;
					};
					case ObjcPointerEncoding: {
						size += sizeof(Pointer);
						break;
					};
					default: {
						break;
					};
				}
			}
		}
	}
	return size;
}

#endif