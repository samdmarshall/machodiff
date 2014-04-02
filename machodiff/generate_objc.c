//
//  generate_objc.c
//  machodiff
//
//  Created by Sam Marshall on 4/1/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_generate_objc_c
#define machodiff_generate_objc_c

#include "generate_objc.h"
#include "file.h"

void GenerateClassHeader(FILE *fd, struct loader_objc_class *class);

char *string;

#define WriteString(fd, ...) \
string = calloc(4096, sizeof(char)); \
sprintf(string, __VA_ARGS__); \
FWRITE_STRING_TO_FILE(string,fd); \
free(string);

void GenerateClassHeader(FILE *fd, struct loader_objc_class *class) {
	WriteString(fd, "@interface %s : %s ",class->className, (class->superCls ? class->superCls->className : ""));
	
	if (class->protocolCount) {
		WriteString(fd, "<");
		for (uint32_t prot_index = 0; prot_index < class->protocolCount; prot_index++) {
			WriteString(fd, "%s",class->protocol[prot_index].name);
			if (prot_index+1 < class->protocolCount) {
				WriteString(fd, ",");
			}
		}
		WriteString(fd, ">");
	}
	WriteString(fd, " {\n");
	
	for (uint32_t ivar_index = 0; ivar_index < class->ivarCount; ivar_index++) {
		struct loader_objc_ivar *ivar = &(class->ivar[ivar_index]);
		if (ivar) {
			struct loader_objc_lexer_type *type = SDMSTObjcDecodeType(ivar->type);
			if (type->token[0].typeName && strcmp(type->token[0].type, "struct") == 0) {
				WriteString(fd,"\t%s %s ",type->token[0].type,type->token[0].typeName);
			}
			else {
				char *ivar_type = (type->token[0].typeName != NULL ? type->token[0].typeName : type->token[0].type);
				WriteString(fd, "\t%s ",ivar_type);
			}
			WriteString(fd, "%s; // 0x%016llx\n", ivar->name, ivar->offset);
		}
	}
	WriteString(fd, "}\n");
	
	if (class->superCls) {
		for (uint32_t method_index = 0; method_index < class->superCls->methodCount; method_index++) {
			struct loader_objc_method *method = &(class->superCls->method[method_index]);
			if (method) {
				WriteString(fd, "%s %s // IMP=0x%016llx\n",(method->method_type == loader_objc_method_instance_type ? "-" : "+"),SDMSTObjcCreateMethodDescription(SDMSTObjcDecodeType(method->type),method->name),(uint64_t)method->offset );
			}
		}
	}
	
	for (uint32_t method_index = 0; method_index < class->methodCount; method_index++) {
		struct loader_objc_method *method = &(class->method[method_index]);
		if (method) {
			if (strcmp(method->name, ".cxx_destruct") == 0 || strcmp(method->name, ".cxx_construct") == 0 ) {
				WriteString(fd, "// ");
			}
			WriteString(fd, "%s %s // IMP=0x%016llx\n",(method->method_type == loader_objc_method_instance_type ? "-" : "+"),SDMSTObjcCreateMethodDescription(SDMSTObjcDecodeType(method->type),method->name),(uint64_t)method->offset );
		}
	}
	
	if (class->protocolCount) {
		for (uint32_t prot_index = 0; prot_index < class->protocolCount; prot_index++) {
			WriteString(fd, "\n#pragma mark %s Interface\n",class->protocol[prot_index].name);
			for (uint32_t method_index = 0; method_index < class->protocol[prot_index].methodCount; method_index++) {
				struct loader_objc_method *method = &(class->protocol[prot_index].method[method_index]);
				if (method) {
					WriteString(fd, "%s %s",(method->method_type == loader_objc_method_instance_type ? "-" : "+"),SDMSTObjcCreateMethodDescription(SDMSTObjcDecodeType(method->type),method->name));
					if (method->offset) {
						WriteString(fd, " // IMP=0x%016llx",method->offset);
					}
					WriteString(fd, "\n");
				}
			}
		}
	}
	
	WriteString(fd, "\n@end\n\n");
}

void GenerateClassImplementation(FILE *fd, struct loader_objc_class *class) {
	
}

void GenerateObjcHeaders(struct loader_objc_map *objc_map, char *path) {
	if (objc_map) {
		char *header_dir = calloc(strlen(path)+9, sizeof(char));
		sprintf(header_dir, "%s/Headers",path);
		make_dir(header_dir, 0700);
		for (uint32_t class_index = 0; class_index < objc_map->clsCount; class_index++) {
			struct loader_objc_class *class = &(objc_map->cls[class_index]);
			if (class) {
				char *header_path = calloc(strlen(header_dir)+strlen(class->className)+4, sizeof(char));
				sprintf(header_path, "%s/%s.h",header_dir,class->className);
				
				FILE *file = fopen(header_path, "w");
				if (file) {
					GenerateClassHeader(file, class);
					fclose(file);
				}

				free(header_path);
			}
		}
		free(header_dir);
	}
}

#endif
