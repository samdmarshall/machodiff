//
//  objc_runtime.h
//  loader
//
//  Created by Sam Marshall on 11/2/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_objc_runtime_h
#define loader_objc_runtime_h

#define HIDE_CXX_DESTRUCT 0

#include "objc_lexer.h"
#include "objc1_runtime.h"
#include "objc2_runtime.h"

struct loader_objc_ivar {
	char *name;
	char *type;
	uint64_t offset;
} ATR_PACK;

enum loader_objc_method_type {
	loader_objc_method_invalid_type = 0,
	loader_objc_method_instance_type,
	loader_objc_method_class_type
};

struct loader_objc_method {
	char *name;
	char *type;
	uint64_t offset;
	uint8_t method_type;
} ATR_PACK;

struct loader_objc_protocol {
	char *name;
	uint64_t offset;
	struct loader_objc_method *method;
	uint32_t methodCount;
} ATR_PACK;

struct loader_objc_class {
	struct loader_objc_class *superCls;
	char *className;
	struct loader_objc_ivar *ivar;
	uint32_t ivarCount;
	struct loader_objc_method *method;
	uint32_t methodCount;
	struct loader_objc_protocol *protocol;
	uint32_t protocolCount;
} ATR_PACK;

struct loader_objc_module {
	char *impName;
	struct loader_objc_class *symbol;
} ATR_PACK;

struct loader_objc_module_raw {
	uint32_t version;
	uint32_t size;
	uint32_t name;
	uint32_t symtab;
} ATR_PACK;

struct loader_objc_module_container {
	struct loader_objc_module *module;
	uint32_t moduleCount;
} ATR_PACK;

struct loader_objc_map {
	struct loader_objc_class *cls;
	uint32_t clsCount;
	CoreRange classRange;
	CoreRange catRange;
	CoreRange protRange;
	CoreRange clsMRange;
	CoreRange instMRange;
} ATR_PACK;

#define CLS_CLASS               	0x1
#define CLS_META                	0x2
#define CLS_INITIALIZED         	0x4
#define CLS_POSING					0x8
#define CLS_MAPPED                 0x10
#define CLS_FLUSH_CACHE            0x20
#define CLS_GROW_CACHE             0x40
#define CLS_NEED_BIND              0x80
#define CLS_METHOD_ARRAY          0x100
#define CLS_JAVA_HYBRID           0x200
#define CLS_JAVA_CLASS            0x400
#define CLS_INITIALIZING          0x800
#define CLS_FROM_BUNDLE          0x1000
#define CLS_HAS_CXX_STRUCTORS    0x2000
#define CLS_NO_METHOD_ARRAY      0x4000
#define CLS_HAS_LOAD_METHOD      0x8000
#define CLS_CONSTRUCTING        0x10000
#define CLS_EXT                 0x20000

#define SDMSTObjc1ValidClassCheck(a) ((a | CLS_CLASS) == 0 || (a | CLS_META) == 0 || (a | CLS_INITIALIZED) == 0 || (a | CLS_POSING) == 0 || (a | CLS_MAPPED) == 0 || (a | CLS_FLUSH_CACHE) == 0 || (a | CLS_GROW_CACHE) == 0 || (a | CLS_NEED_BIND) == 0 || (a | CLS_METHOD_ARRAY) == 0 || (a | CLS_JAVA_HYBRID) == 0 || (a | CLS_JAVA_CLASS) == 0 || (a | CLS_INITIALIZING) == 0 || (a | CLS_FROM_BUNDLE) == 0 || (a | CLS_HAS_CXX_STRUCTORS) == 0 || (a | CLS_NO_METHOD_ARRAY) == 0 || (a | CLS_HAS_LOAD_METHOD) == 0 || (a | CLS_CONSTRUCTING) == 0 || (a | CLS_EXT) == 0)

void SDMObjc1MatchProtocolMethodImp(struct loader_objc_protocol *protocol, struct loader_objc_class *class, uint8_t type);
void SDMObjc1CreateProtocolMethodsForClassOfType(uint64_t offset, struct loader_objc_1_protocol *protocol, struct loader_objc_protocol *class, uint8_t type);

struct loader_objc_class* SDMSTObjc1CreateClassFromProtocol(struct loader_objc_map *objcData, struct loader_objc_1_protocol *prot, uint64_t offset);
struct loader_objc_class* SDMSTObjc1CreateClassFromCategory(struct loader_objc_map *objcData, struct loader_objc_1_category *cat, uint64_t offset);
uint8_t SDMSTGetObjc1MethodType(struct loader_objc_map *objcData, struct loader_objc_1_class_method *method, uint64_t offset);
struct loader_objc_class* SDMSTObjc1CreateClassFromClass(struct loader_objc_map *objcData, struct loader_objc_1_class *cls, uint64_t offset);
void SDMSTObjc1CreateClassFromSymbol(struct loader_objc_map *objcData, struct loader_objc_1_symtab *symtab, uint64_t mem_offset);
struct loader_objc_class* SDMSTObjc2ClassCreateFromClass(struct loader_objc_2_class *cls, struct loader_objc_2_class *parentClass, CoreRange dataRange, uint64_t offset, uint8_t class_type);

#endif
