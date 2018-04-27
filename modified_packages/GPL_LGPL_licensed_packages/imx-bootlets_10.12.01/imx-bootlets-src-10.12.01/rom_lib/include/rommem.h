//////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//
//! \file rommem.h
//! \brief ROM globals
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
#ifndef _ROMMEM_H
#define _ROMMEM_H

#include "rom_types.h"

#ifndef  __LANGUAGE_ASM__
extern uint32_t g_wBoot_Mode ;            // Bootmode 
extern const uint32_t rom_version ;       // This var at fixed addr, holds ROM versioning information 
extern const uint32_t checksum ;          // This var at fixed addr, holds CRC-32 of ROM
//extern const uint32_t pSdkIfc ;           // This var at fixed addr, holds pointer to SDK interface table
//extern const uint32_t L2_pg_tbl ;         // This var at fixed addr, hold addr of rom_l2_table (above)
//extern const uint32_t rom_l2_table[256] ; // L2 page table mapping ROM (for SDK use)
#endif

#endif // _ROMMEM_H
//! @}
