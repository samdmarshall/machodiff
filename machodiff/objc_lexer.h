//
//  objc_lexer.h
//  loader
//
//  Created by Sam Marshall on 11/3/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_objc_lexer_h
#define loader_objc_lexer_h

#include "util.h"

#define kObjcCharEncoding "c"
#define kObjcIntEncoding "i"
#define kObjcShortEncoding "s"
#define kObjcLongEncoding "l"
#define kObjcLLongEncoding "q"
#define kObjcUCharEncoding "C"
#define kObjcUIntEncoding "I"
#define kObjcUShortEncoding "S"
#define kObjcULongEncoding "L"
#define kObjcULLongEncoding "Q"
#define kObjcFloatEncoding "f"
#define kObjcDoubleEncoding "d"
#define kObjcBoolEncoding "B"
#define kObjcVoidEncoding "v"
#define kObjcStringEncoding "*"
#define kObjcIdEncoding "@"
#define kObjcClassEncoding "#"
#define kObjcSelEncoding ":"
#define kObjcBitEncoding "b"
#define kObjcPointerEncoding "^"
#define kObjcUnknownEncoding "?"

#define kObjcConstEncoding "r"
#define kObjcInEncoding "n"
#define kObjcInOutEncoding "N"
#define kObjcOutEncoding "o"
#define kObjcByCopyEncoding "O"
#define kObjcByRefEncoding "R"
#define kObjcOnewayEncoding "V"

enum LoaderObjcLexerTokenType {
	ObjcCharEncoding = 1,
	ObjcIntEncoding,
	ObjcShortEncoding,
	ObjcLongEncoding,
	ObjcLLongEncoding,
	ObjcUCharEncoding,
	ObjcUIntEncoding,
	ObjcUShortEncoding,
	ObjcULongEncoding,
	ObjcULLongEncoding,
	ObjcFloatEncoding,
	ObjcDoubleEncoding,
	ObjcBoolEncoding,
	ObjcVoidEncoding,
	ObjcStringEncoding,
	ObjcIdEncoding,
	ObjcClassEncoding,
	ObjcSelEncoding,
	ObjcBitEncoding,
	ObjcPointerEncoding,
	ObjcUnknownEncoding,
	ObjcStructEncoding,
	ObjcArrayEncoding
};

#define kObjcNameTokenStart "\""
#define kObjcNameTokenEnd "\""

#define kObjcArrayTokenStart "["
#define kObjcArrayTokenEnd "]"

#define kObjcStructTokenStart "{"
#define kObjcStructTokenEnd "}"

#define kObjcStructDefinitionToken "="

#define SDMObjcLexerConvertIndexToToken(a) (enum LoaderObjcLexerTokenType)(a+1)
#define SDMObjcLexerConvertTokenToIndex(a) (uint32_t)(a-1)

struct loader_objc_lexer_token {
	char *type;
	char *typeName;
	enum LoaderObjcLexerTokenType typeClass;
	struct loader_objc_lexer_token *children;
	uint32_t childrenCount;
	uint32_t pointerCount;
	uint32_t arrayCount;
} ATR_PACK loader_objc_lexer_token;

struct loader_objc_lexer_type {
	struct loader_objc_lexer_token *token;
	uint32_t tokenCount;
} ATR_PACK loader_objc_lexer_type;

struct loader_objc_lexer_type* SDMSTObjcDecodeTypeWithLength(char *type, uint64_t decodeLength);
struct loader_objc_lexer_type* SDMSTObjcDecodeType(char *type);
CoreRange SDMSTObjcGetTokenRangeFromOffset(char *type, uint64_t offset, char *token);
char* SDMSTObjcPointersForToken(struct loader_objc_lexer_token *token);
char* SDMSTObjcCreateMethodDescription(struct loader_objc_lexer_type *type, char *name);
uint64_t SDMSTObjcDecodeSizeOfType(struct loader_objc_lexer_token *token);

#endif
