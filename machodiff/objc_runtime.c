//
//  objc_runtime.c
//  loader
//
//  Created by Sam Marshall on 11/10/13.
//  Copyright (c) 2013 Sam Marshall. All rights reserved.
//

#ifndef loader_objc_runtime_c
#define loader_objc_runtime_c

#include "objc_runtime.h"
#include <string.h>

void SDMSTObjc2ClassPopulate(struct loader_objc_class *newClass, struct loader_objc_2_class *cls, CoreRange dataRange, uint64_t offset, uint8_t class_type);

struct loader_objc_class* SDMSTObjc1CreateClassFromProtocol(struct loader_objc_map *objcData, struct loader_objc_1_procotol *prot, uint64_t offset) {
	struct loader_objc_class *newClass = calloc(1, sizeof(struct loader_objc_class));
	if (prot) {
		newClass->className = Ptr(PtrAdd(offset, prot->name));
	}
	return newClass;
}

struct loader_objc_class* SDMSTObjc1CreateClassFromCategory(struct loader_objc_map *objcData, struct loader_objc_1_category *cat, uint64_t offset) {
	struct loader_objc_class *newClass = calloc(1, sizeof(struct loader_objc_class));
	if (cat) {
		newClass->className = Ptr(PtrAdd(offset, cat->name));
	}
	return newClass;
}

uint8_t SDMSTGetObjc1MethodType(struct loader_objc_map *objcData, struct loader_objc_1_class_method *method, uint64_t offset) {
	uint8_t type = loader_objc_method_invalid_type;
	
	if ((uint64_t)method >= (offset+objcData->instMRange.offset) && (uint64_t)method < (offset+(objcData->instMRange.offset+objcData->instMRange.length))) {
		type = loader_objc_method_instance_type;
	}
	else if ((uint64_t)method >= (offset+objcData->clsMRange.offset) && (uint64_t)method < (offset+(objcData->clsMRange.offset+objcData->clsMRange.length))) {
		type = loader_objc_method_class_type;
	}
	
	return type;
}

struct loader_objc_class* SDMSTObjc1CreateClassFromClass(struct loader_objc_map *objcData, struct loader_objc_1_class *cls, uint64_t offset) {
	struct loader_objc_class *newClass = calloc(1, sizeof(struct loader_objc_class));
	if (cls) {
		if (cls->superClass != 0) {
			bool isValidClass = SDMSTObjc1ValidClassCheck(((uint32_t)(cls->info)));
			if (cls->superClass != cls->isa && isValidClass) {
				struct loader_objc_1_class *objc1class = (struct loader_objc_1_class *)PtrAdd(offset, cls->superClass);
				newClass->superCls = SDMSTObjc1CreateClassFromClass(objcData, objc1class, offset);
			}
			else if (isValidClass) {
				newClass->superCls = (struct loader_objc_class *)PtrAdd(offset, cls->superClass);
			}
			else {
				newClass->superCls = 0;
			}
			newClass->className = Ptr(PtrAdd(offset, cls->name));
			
			struct loader_objc_1_class_ivar_info *ivarInfo = (struct loader_objc_1_class_ivar_info *)PtrAdd(offset, cls->ivars);
			if (ivarInfo) {
				newClass->ivarCount = ivarInfo->count;
				newClass->ivar = calloc(newClass->ivarCount, sizeof(struct loader_objc_ivar));
				struct loader_objc_1_class_ivar *ivarOffset = (struct loader_objc_1_class_ivar *)PtrAdd(ivarInfo, sizeof(struct loader_objc_1_class_ivar_info));
				for (uint32_t i = 0; i < newClass->ivarCount; i++) {
					newClass->ivar[i].name = Ptr(PtrAdd(offset, ivarOffset[i].name));
					newClass->ivar[i].type = Ptr(PtrAdd(offset, ivarOffset[i].type));
					newClass->ivar[i].offset = (uintptr_t)(ivarOffset[i].offset);
				}
				
			}
						
			struct loader_objc_1_class_method_info *methodInfo = (struct loader_objc_1_class_method_info *)PtrAdd(offset, cls->methods);
			if (methodInfo && (((uint64_t)methodInfo >= (uint64_t)PtrAdd(PtrHighPointer(offset), objcData->classRange.offset) && (uint64_t)methodInfo < (uint64_t)PtrAdd(PtrHighPointer(offset), (objcData->clsMRange.offset + (uint64_t)objcData->clsMRange.length))) || ((uint64_t)methodInfo >= (uint64_t)PtrAdd(PtrHighPointer(offset), objcData->instMRange.offset) && (uint64_t)methodInfo < (uint64_t)PtrAdd(PtrHighPointer(offset), (objcData->instMRange.offset + objcData->instMRange.length))))) {
				newClass->methodCount = methodInfo->count;
				newClass->method = calloc(newClass->methodCount, sizeof(struct loader_objc_method));
				struct loader_objc_1_class_method *methodOffset = (struct loader_objc_1_class_method *)PtrAdd(methodInfo, sizeof(struct loader_objc_1_class_method_info));
				for (uint32_t i = 0; i < newClass->methodCount; i++) {
					newClass->method[i].name = Ptr(PtrAdd(offset, methodOffset[i].name));
					newClass->method[i].type = Ptr(PtrAdd(offset, methodOffset[i].type));
					newClass->method[i].offset = (uintptr_t)(methodOffset[i].imp);
					newClass->method[i].method_type = SDMSTGetObjc1MethodType(objcData, methodOffset, PtrHighPointer(offset));
				}
			}
			
			struct loader_objc_1_procotol *protocolInfo = (struct loader_objc_1_procotol *)PtrAdd(offset, cls->protocols);
			if (protocolInfo) {
				
			}
		}
	}
	return newClass;
}

void SDMSTObjc1CreateClassFromSymbol(struct loader_objc_map *objcData, struct loader_objc_1_symtab *symtab, uint64_t mem_offset) {
	if (symtab) {
		uint64_t memOffset = mem_offset;
		uint32_t counter = symtab->catCount + symtab->classCount;
		struct loader_objc_1_symtab_definition *symbol = (struct loader_objc_1_symtab_definition *)PtrAdd(symtab, sizeof(struct loader_objc_1_symtab));
		for (uint32_t i = 0; i < counter; i++) {
			Pointer definition_pointer = PtrCast(PtrAdd(mem_offset, symbol[i].defintion), Pointer);
			
			if (((Ptr(definition_pointer) >= (PtrAdd(PtrHighPointer(mem_offset), objcData->classRange.offset))) && (Ptr(definition_pointer) <= (PtrAdd(PtrHighPointer(mem_offset), ((uint64_t)(objcData->classRange.offset) + (uint64_t)objcData->classRange.length)))))) {
				struct loader_objc_1_class *objc1class = (struct loader_objc_1_class *)PtrAdd(memOffset, symbol[i].defintion);
				struct loader_objc_class *newClass = SDMSTObjc1CreateClassFromClass(objcData, objc1class, memOffset);
				memcpy(&(objcData->cls[objcData->clsCount]), newClass, sizeof(struct loader_objc_class));
				free(newClass);
				objcData->clsCount++;
				objcData->cls = realloc(objcData->cls, sizeof(struct loader_objc_class)*(objcData->clsCount+1));
			}
			
			if ((Ptr(definition_pointer) >= PtrAdd(PtrHighPointer(mem_offset), objcData->catRange.offset)) && (Ptr(definition_pointer) <= (PtrAdd(PtrHighPointer(mem_offset), (objcData->catRange.offset + objcData->catRange.length))))) {
				struct loader_objc_1_category *objc1cat = (struct loader_objc_1_category *)PtrAdd(memOffset,symbol[i].defintion);
				struct loader_objc_class *newClass = SDMSTObjc1CreateClassFromCategory(objcData, objc1cat, memOffset);
				memcpy(&(objcData->cls[objcData->clsCount]), newClass, sizeof(struct loader_objc_class));
				free(newClass);
				objcData->clsCount++;
				objcData->cls = realloc(objcData->cls, sizeof(struct loader_objc_class)*(objcData->clsCount+1));
			}
			
			symbol = (struct loader_objc_1_symtab_definition *)PtrAdd(symbol, sizeof(struct loader_objc_1_symtab_definition));
		}
	}
}

void SDMSTObjc2ClassPopulate(struct loader_objc_class *newClass, struct loader_objc_2_class *cls, CoreRange dataRange, uint64_t offset, uint8_t class_type) {
	struct loader_objc_2_class_data *data = (struct loader_objc_2_class_data *)PtrAdd(cls->data, offset);
	cls->data = data;
	newClass->className = Ptr(PtrAdd(data->name, offset));
	
	if (data->ivar) {
		struct loader_objc_2_class_ivar_info *ivarInfo = ((struct loader_objc_2_class_ivar_info *)PtrAdd(data->ivar, offset));
		if (ivarInfo && (uint64_t)ivarInfo != offset) {
			newClass->ivarCount = ivarInfo->count;
			newClass->ivar = calloc(newClass->ivarCount, sizeof(struct loader_objc_ivar));
			struct loader_objc_2_class_ivar *ivarOffset = (struct loader_objc_2_class_ivar *)PtrAdd(ivarInfo, sizeof(struct loader_objc_2_class_ivar_info));
			for (uint32_t i = 0; i < newClass->ivarCount; i++) {
				newClass->ivar[i].name = Ptr(PtrAdd(offset, ivarOffset[i].name));
				newClass->ivar[i].type = Ptr(PtrAdd(offset, ivarOffset[i].type));
				newClass->ivar[i].offset = (uintptr_t)(ivarOffset[i].offset);
			}
		}
	}
	
	if (data->method) {
		struct loader_objc_2_class_method_info *methodInfo = ((struct loader_objc_2_class_method_info *)PtrAdd(data->method, offset));
		if (methodInfo && (uint64_t)methodInfo != offset) {
			newClass->methodCount = methodInfo->count;
			newClass->method = calloc(newClass->methodCount, sizeof(struct loader_objc_method));
			struct loader_objc_2_class_method *methodOffset = (struct loader_objc_2_class_method *)PtrAdd(methodInfo, sizeof(struct loader_objc_2_class_method_info));
			for (uint32_t i = 0; i < newClass->methodCount; i++) {
				newClass->method[i].name = Ptr(PtrAdd(offset, methodOffset[i].name));
				newClass->method[i].type = Ptr(PtrAdd(offset, methodOffset[i].type));
				newClass->method[i].offset = (uintptr_t)(methodOffset[i].imp);
				newClass->method[i].method_type = (class_type == loader_objc_2_class_metaclass_type ? loader_objc_method_class_type : loader_objc_method_instance_type);
			}
		}
	}
	
	if (data->protocol) {
		struct loader_objc_2_class_protocol_info *protocolInfo = ((struct loader_objc_2_class_protocol_info *)PtrAdd(data->protocol, offset));
		if (protocolInfo && (uint64_t)protocolInfo != offset) {
			newClass->protocolCount = (uint32_t)(protocolInfo->count);
			newClass->protocol = calloc(newClass->protocolCount, sizeof(struct loader_objc_protocol));
			struct  loader_objc_2_class_protocol *protocolOffset = (struct  loader_objc_2_class_protocol *)PtrAdd(protocolInfo, sizeof(struct loader_objc_2_class_protocol_info));
			for (uint32_t i = 0; i < newClass->protocolCount; i++) {
				newClass->protocol[i].offset = (uintptr_t)(protocolOffset[i].offset);
			}
		}
	}
}

struct loader_objc_class* SDMSTObjc2ClassCreateFromClass(struct loader_objc_2_class *cls, struct loader_objc_2_class *parentClass, CoreRange dataRange, uint64_t offset, uint8_t class_type) {
	struct loader_objc_class *newClass = calloc(1, sizeof(struct loader_objc_class));
	if (cls != parentClass) {
		SDMSTObjc2ClassPopulate(newClass, cls, dataRange, offset, class_type);
		
		if ((PtrAdd(offset, cls->isa >= Ptr(dataRange.offset)) && (PtrAdd(offset, cls->isa) < (PtrAdd(offset, (dataRange.offset + dataRange.length)))))) {
			struct loader_objc_2_class *isa = (struct loader_objc_2_class *)PtrAdd((cls->isa), offset);
			newClass->superCls = SDMSTObjc2ClassCreateFromClass(isa, cls, dataRange, offset, loader_objc_2_class_metaclass_type);
		}
	}
	return newClass;
}

#endif