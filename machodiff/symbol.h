//
//  symbol.h
//  machodiff
//
//  Created by Sam Marshall on 3/11/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_symbol_h
#define machodiff_symbol_h

#include "loader_type.h"

#define kStubName "__sdmst_stub_"

#define kSubPrefix "sub_"
#define kSubFormatter "%lx"
#define kSubName "sub_%lx"

void SDMGenerateSymbols(struct loader_binary *binary);

Pointer SDMSTFindFunctionAddress(Pointer *fPointer, struct loader_binary *binary);

bool SMDSTSymbolDemangleAndCompare(char *symFromTable, char *symbolName);

#endif
