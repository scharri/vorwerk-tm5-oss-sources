////////////////////////////////////////////////////////////////////////////////
//! \addtogroup startup
//! @{
//!
//  Copyright (c) 2009 Freescale Semiconductor, Inc.
//!
//! \file    mmu.c
//! \brief   mmu setup function defintion
//! \version 0.1
//! \date    Jul 01,2009
//!
////////////////////////////////////////////////////////////////////////////////
/*===================================================================================
                                        INCLUDE FILES
==================================================================================*/
#include <rom_types.h>
#include "regsdigctl.h"
#include "cpu_support.h"

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*================================================================================
                          Hash Defines And Macros
=================================================================================*/
#define ttbr_base_address 0x1C000
/*=========================================================================
                               LOCAL VARIABLES
==========================================================================*/

/*==========================================================================
FUNCTION: void  cpu_setup_mmu(void)
DESCRIPTION:
   cpu_setup_mmu () function will setup the page table ,enable MMU and cache.

ARGUMENTS PASSED:
   void

RETURN VALUE:
  void

PRE-CONDITIONS:
   None

POST-CONDITIONS:
   MMU and cache are enabled

Detailed Description:

==============================================================================*/
extern void cpu_mmu_setup(void)
{
    uint32_t i;

    for (i = 0x400; i < 0x800; i++)
    {
        /* if section is part of DDR,make it cacheable and non-bufferable */
        *(volatile unsigned int *)(ttbr_base_address + (i << 2)) = (i << 20) | TT_ATTRIB_CACHEABLE_NON_BUFFERABLE;
    }
	/* section corrosponding to OCRAM,make it cacheable and non-bufferable, Write through */
    *(volatile unsigned int *)(ttbr_base_address + (0x000 << 2)) = (0x000 << 20) | TT_ATTRIB_CACHEABLE_NON_BUFFERABLE;
    /* section corrosponding to OCROM,make it cacheable and non-bufferable */
    *(volatile unsigned int *)(ttbr_base_address + (0xFFF << 2)) = (0xFFF << 20) | TT_ATTRIB_CACHEABLE_NON_BUFFERABLE;
	/* section corrosponding to PIO Register region, make it uncacheable and unbufferable */
    *(volatile unsigned int *)(ttbr_base_address + (0x800 << 2)) = (0x800 << 20) | TT_ATTRIB_NON_CACHEABLE_NON_BUFFERABLE;

	/*L1PT configuation done*/
	/* call Assembly routine to enable enable I and D cache ,and then enable MMU*/
	cpu_mmu_enable();
}
