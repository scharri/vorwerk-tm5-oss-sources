//////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//
//! \file   test_utils.h
//! \brief  ROM utility functions
//
// Copyright (c) SigmaTel, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary  Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may comprise trade secrets of SigmaTel, Inc.
// or its associates, and any unauthorized use thereof is prohibited.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __TEST_UTILS_H
#define __TEST_UTILS_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "rom_types.h"
#include "return_codes.h"

#if (defined(TGT_CHIP))
    // test_utils.h should NOT be used with TGT_CHIP
#endif

#if (defined(TGT_DILLO) || defined(TGT_SIM) || defined(TGT_3700) || defined(TGT_MX28))

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

#define VECTOR_RESET                    0x00
#define VECTOR_UNDEFINED_INSTRUCTION    0x04
#define VECTOR_SWI                      0x08
#define VECTOR_PREFETCH_ABORT           0x0c
#define VECTOR_DATA_ABORT               0x10
#define VECTOR_NOT_ASSIGNED             0x14
#define VECTOR_IRQ                      0x18
#define VECTOR_FIQ                      0x1c
#define VECTOR_OFFSET                   (0x00000000)

#define ROM_TEST_FILLPASSFLAG(addr,val) (*(uint32_t *)addr = val)
////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////

// These values are defined in the linker file
extern unsigned char __ghsbegin_irq_stack[];  // starting address
extern unsigned int __ghssize_irq_stack[];    // length in bytes

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

typedef void (*voidvoid)(void);
RtStatus_t rom_test_InterruptSetup(void);
void rom_test_SetupDefaultClocks( void );
voidvoid *rom_test_InstallVector(unsigned int vector, voidvoid Handler);
void rom_test_DefaultVectorReset(void);
void rom_test_DefaultVectorUndefined(void);
void rom_test_DefaultVectorSwi(void);
void rom_test_DefaultVectorPrefetchAbort(void);
void rom_test_DefaultVectorDataAbort(void);
void rom_test_DefaultVectorReserved(void);
void rom_test_DefaultVectorFiq(void);
void rom_test_SetupDefaultVectors(void);
extern void rom_test_StackInit(void);
void rom_test_PrintOcotp(void);
RtStatus_t rom_test_PowerInit(void);
void rom_test_delayUs(unsigned int uSeconds);
void rom_test_JtagSwitch(void);
void rom_test_FillPassFlag(uint32_t addr, uint32_t value);
#endif

#endif //__TEST_UTILS_H
//! @}
