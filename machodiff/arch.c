//
//  arch.c
//  machodiff
//
//  Created by Sam Marshall on 2/24/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_arch_c
#define machodiff_arch_c

#include "arch.h"

cs_arch SDM_CS_ArchType(struct loader_arch *arch, uint8_t endian_type) {
	cs_arch type = CS_ARCH_ALL;
	
	if (SDMArchCPU_X86(arch, endian_type)) {
		type = CS_ARCH_X86;
	}
	
	if (SDMArchCPU_PPC(arch, endian_type)) {
		type = CS_ARCH_PPC;
	}
	
	if (SDMArchCPU_ARM(arch, endian_type)) {
		if (SDMArchCPUSUB_ARM64(arch, loader_arch_arm64_type, endian_type)) {
			type = CS_ARCH_ARM64;
		} else {
			type = CS_ARCH_ARM;
		}
	}
	return type;
}

cs_mode SDM_CS_ModeType(struct loader_arch *arch, uint8_t endian_type) {
	cs_mode mode = CS_MODE_LITTLE_ENDIAN;
	
	if (SDMArchCPUSUB_I386(arch, loader_arch_i386_type, endian_type)) {
		mode = CS_MODE_32;
	}
	
	if (SDMArchCPUSUB_X86_64(arch, loader_arch_x86_64_type, endian_type)) {
		mode = CS_MODE_64;
	}
	
	if (SDMArchCPUSUB_ARMV6(arch, loader_arch_armv6_type, endian_type) || SDMArchCPUSUB_ARMV7(arch, loader_arch_armv7_type, endian_type) || SDMArchCPUSUB_ARMV7S(arch, loader_arch_armv7s_type, endian_type)) {
		mode = CS_MODE_ARM;
	}
	
	return mode;
}

#pragma mark -
#pragma mark INTEL

bool SDMArchCPU_X86(struct loader_arch *arch, uint8_t endian_type) {
	bool result = false;
	cpu_type_t type = (cpu_type_t)EndianFix(endian_type, (uint32_t)arch->cputype);
	if ((type & CPU_TYPE_X86) == CPU_TYPE_X86) {
		result = true;
	}
	return result;
}


bool SDMArchCPUSUB_I386(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	uint32_t subtype = (uint32_t)EndianFix(endian_type, (uint32_t)arch->subtype);
	if (((subtype & CPU_SUBTYPE_LIB64) != CPU_SUBTYPE_LIB64) && ((subtype & CPU_SUBTYPE_I386_ALL) == CPU_SUBTYPE_I386_ALL) && target_arch == loader_arch_i386_type) {
		result = true;
	}
	return result;
}

bool SDMArchCPUSUB_X86_64(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	uint32_t subtype = (uint32_t)EndianFix(endian_type, (uint32_t)arch->subtype);
	if (((subtype & CPU_SUBTYPE_LIB64) == CPU_SUBTYPE_LIB64) && ((subtype & CPU_SUBTYPE_X86_ALL) == CPU_SUBTYPE_X86_ALL) && target_arch == loader_arch_x86_64_type) {
		result = true;
	}
	return result;
}

#pragma mark -
#pragma mark PPC

bool SDMArchCPU_PPC(struct loader_arch *arch, uint8_t endian_type) {
	bool result = false;
	cpu_type_t type = (cpu_type_t)EndianFix(endian_type, (uint32_t)arch->cputype);
	if ((type & CPU_TYPE_POWERPC) == CPU_TYPE_POWERPC) {
		result = true;
	}
	return result;
}

#pragma mark -
#pragma mark PPC64

bool SDMArchCPU_PPC64(struct loader_arch *arch, uint8_t endian_type) {
	bool result = false;
	cpu_type_t type = (cpu_type_t)EndianFix(endian_type, (uint32_t)arch->cputype);
	if ((type & CPU_TYPE_POWERPC64) == CPU_TYPE_POWERPC64) {
		result = true;
	}
	return result;
}

#pragma mark -
#pragma mark ARM

bool SDMArchCPU_ARM(struct loader_arch *arch, uint8_t endian_type) {
	bool result = false;
	cpu_type_t type = (cpu_type_t)EndianFix(endian_type, (uint32_t)arch->cputype);
	if ((type & CPU_TYPE_ARM) == CPU_TYPE_ARM) {
		result = true;
	}
	return result;
}

bool SDMArchCPUSUB_ARMV6(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	uint32_t subtype = (uint32_t)EndianFix(endian_type, (uint32_t)arch->subtype);
	if ((subtype & CPU_SUBTYPE_ARM_V6) == CPU_SUBTYPE_ARM_V6 && target_arch == loader_arch_armv6_type) {
		result = true;
	}
	return result;
}

bool SDMArchCPUSUB_ARMV7(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	uint32_t subtype = (uint32_t)EndianFix(endian_type, (uint32_t)arch->subtype);
	if ((subtype & CPU_SUBTYPE_ARM_V7) == CPU_SUBTYPE_ARM_V7 && target_arch == loader_arch_armv7_type) {
		result = true;
	}
	return result;
}

bool SDMArchCPUSUB_ARMV7S(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	uint32_t subtype = (uint32_t)EndianFix(endian_type, (uint32_t)arch->subtype);
	if ((subtype & CPU_SUBTYPE_ARM_V7S) == CPU_SUBTYPE_ARM_V7S && target_arch == loader_arch_armv7s_type) {
		result = true;
	}
	return result;
}

bool SDMArchCPUSUB_ARM64(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	uint32_t subtype = (uint32_t)EndianFix(endian_type, (uint32_t)arch->subtype);
	if ((subtype & CPU_SUBTYPE_ARM_V8) == CPU_SUBTYPE_ARM_V8 && target_arch == loader_arch_arm64_type) {
		result = true;
	}
	return result;
}

bool SDMMatchArchToCPU(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type) {
	bool result = false;
	if (SDMArchCPU_X86(arch, endian_type)) {
		if (SDMArchCPUSUB_I386(arch, target_arch, endian_type)) {
			result = true;
		}
		else if (SDMArchCPUSUB_X86_64(arch, target_arch, endian_type)) {
			result = true;
		}
	}
	else if (SDMArchCPU_ARM(arch, endian_type)) {
		if (SDMArchCPUSUB_ARMV6(arch, target_arch, endian_type)) {
			result = true;
		}
		else if (SDMArchCPUSUB_ARMV7(arch, target_arch, endian_type)) {
			result = true;
		}
		else if (SDMArchCPUSUB_ARMV7S(arch, target_arch, endian_type)) {
			result = true;
		}
		else if (SDMArchCPUSUB_ARM64(arch, target_arch, endian_type)) {
			result = true;
		}
	}
	else if (SDMArchCPU_PPC(arch, endian_type)) {
		if (target_arch == loader_arch_ppc_type) {
			result = true;
		}
	}
	else if (SDMArchCPU_PPC64(arch, endian_type)) {
		if (target_arch == loader_arch_ppc64_type) {
			result = true;
		}
	}
	return result;
}

#endif

