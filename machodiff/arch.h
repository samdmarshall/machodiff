//
//  arch.h
//  machodiff
//
//  Created by Sam Marshall on 2/24/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#ifndef machodiff_arch_h
#define machodiff_arch_h

#include "loader.h"
#include "capstone.h"

#ifndef CPU_SUBTYPE_ARM_V8
#define CPU_SUBTYPE_ARM_V8		((cpu_subtype_t) 13)
#endif

cs_arch SDM_CS_ArchType(struct loader_arch *arch, uint8_t endian_type);
cs_mode SDM_CS_ModeType(struct loader_arch *arch, uint8_t endian_type);

bool SDMArchCPU_X86(struct loader_arch *arch, uint8_t endian_type);
bool SDMArchCPUSUB_I386(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type);
bool SDMArchCPUSUB_X86_64(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type);

bool SDMArchCPU_PPC(struct loader_arch *arch, uint8_t endian_type);

bool SDMArchCPU_PPC64(struct loader_arch *arch, uint8_t endian_type);

bool SDMArchCPU_ARM(struct loader_arch *arch, uint8_t endian_type);
bool SDMArchCPUSUB_ARMV6(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type);
bool SDMArchCPUSUB_ARMV7(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type);
bool SDMArchCPUSUB_ARMV7S(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type);
bool SDMArchCPUSUB_ARM64(struct loader_arch *arch, uint8_t target_arch, uint8_t endian_type);

#endif
