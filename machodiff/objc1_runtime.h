//
//  objc1_runtime.h
//  loader
//
//  Created by Sam Marshall on 11/2/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_objc1_runtime_h
#define loader_objc1_runtime_h

#define kObjc1CatClsMeth "__cat_cls_meth"
#define kObjc1CatInstMeth "__cat_inst_meth"
#define kObjc1StringObject "__string_object"
#define kObjc1CStringObject "__cstring_object"
#define kObjc1MessageRefs "__message_refs"
#define kObjc1SelFixup "__sel_fixup"
#define kObjc1ClsRefs "__cls_refs"
#define kObjc1Class "__class"
#define kObjc1MetaClass "__meta_class"
#define kObjc1ClsMeth "__cls_meth"
#define kObjc1InstMeth "__inst_meth"
#define kObjc1Protocol "__protocol"
#define kObjc1Category "__category"
#define kObjc1ClassVars "__class_vars"
#define kObjc1InstanceVars "__instance_vars"
#define kObjc1ModuleInfo "__module_info"
#define kObjc1Symbols "__symbols"

struct loader_objc_1_class_method_desc_info {
	uint32_t count;
} ATR_PACK;

struct loader_objc_1_class_method_info {
	uint32_t entrySize;
	uint32_t count;
} ATR_PACK;

struct loader_objc_1_class_ivar_info {
	uint32_t count;
} ATR_PACK;

struct loader_objc_1_class_ivar {
	uint32_t name;
	uint32_t type;
	uint32_t offset;
} ATR_PACK;

struct loader_objc_1_class_method {
	uint32_t name;
	uint32_t type;
	uint32_t imp;
} ATR_PACK;

struct loader_objc_1_procotol {
	uint32_t isa;
	uint32_t name;
	uint32_t protocolList;
	uint32_t instanceMethodDesc;
	uint32_t classMethodDesc;
} ATR_PACK;

struct loader_objc_1_category {
	uint32_t name;
	uint32_t className;
	uint32_t instanceMethods;
	uint32_t classMethods;
	uint32_t protocols;
} ATR_PACK;

struct loader_objc_1_class {
	uint32_t isa;
	uint32_t superClass;
	uint32_t name;
	uint32_t version;
	uint32_t info;
	uint32_t instanceSize;
	uint32_t ivars;
	uint32_t methods;
	uint32_t cache;
	uint32_t protocols;
} ATR_PACK;

struct loader_objc_1_symtab_definition {
	uint32_t defintion;
} ATR_PACK;

struct loader_objc_1_symtab {
	uint32_t selectorRefCount;
	uint32_t refs;
	uint16_t classCount;
	uint16_t catCount;
} ATR_PACK;

#endif
