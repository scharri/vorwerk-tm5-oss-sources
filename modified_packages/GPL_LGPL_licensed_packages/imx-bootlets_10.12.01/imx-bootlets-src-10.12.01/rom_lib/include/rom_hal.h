
/*========================================================================*/
/**
    @file    rom_hal.h

General Description: HAB Hardware Abstraction Layer Function implementation

===========================================================================

              Freescale Semiconductor Confidential Proprietary
        (c) Freescale Semiconductor, Inc. 2009. All rights reserved.

Presence of a copyright notice is not an acknowledgement of
publication. This software file listing contains information of
Freescale Semiconductor, Inc. that is of a confidential and
proprietary nature and any viewing or use of this file is prohibited
without specific written permission from Freescale Semiconductor, Inc.

===========================================================================
Revision History:

                    Modification Date   Tracking
Author              (dd-mmm-yyyy)       Number     Description of Changes
---------------     -------------     ----------   -----------------------
Fareed Mohammed      29-Apr-2010      ENGR123065   Updates for HAB 4.0.4 
==========================================================================
			    INCLUDE FILES
===========================================================================*/
#ifndef _ROM_HAL_H
#define _ROM_HAL_H
#include "chip_io_map.h"
#include "hab_hal.h"		/* HAB library definitions */
#include "hab_rom.h"		/* for preconfigure API */
#include "regs.h"           /* for pheripheral memory map */
#include "efuse.h"
#include "cpu_support.h"

/*==========================================================================
			  LOCAL FUNCTION PROTOTYPES
===========================================================================*/

/*==========================================================================
			       LOCAL CONSTANTS
===========================================================================*/

#define SRK_FUSES_NUM 32

/* External memory base */
#define EM_BASE               0x40000000
#define EM_END                (EM_BASE + 0x1FFFFFFF)

/* External memory base */
#define OCRAM_FREE_AREA_BASE  0x0
#define OCRAM_FREE_AREA_END   (OCRAM_FREE_AREA_BASE + 0xE3FF)

/* pointing to bank0, word0 */
#define REG_OTP_BASE          0x8002C020

/*==========================================================================
				 LOCAL MACROS
===========================================================================*/

/*==========================================================================
		     LOCAL STRUCTURES AND OTHER TYPEDEFS
==========================================================================*/


/*=========================================================================
			       LOCAL VARIABLES
===========================================================================*/


/*=========================================================================
			       GLOBAL VARIABLES
===========================================================================*/

/* Selection of plugins applicable to Huashan (almost) */
const struct hab_plugin* const hab_hal_plugin[] = {
    HAB_PLUGIN(dcp2),           /* DCPv2 base functionality  */
    HAB_PLUGIN(dcp2_sha256),    /* sha-256 */
    HAB_PLUGIN(cmd_base),	    /* Base commands */
    HAB_PLUGIN(cmd_nop),	    /* @rom NOP command */
    HAB_PLUGIN(srk),		    /* SRK table format */
    HAB_PLUGIN(x509_pkcs1),	    /* X.509v3 certificate format */
    HAB_PLUGIN(cms_pkcs1_sha256), /* CMS signature format */
    HAB_PLUGIN(sw_sha256),	    /* SW SHA-256 implementation */
    HAB_PLUGIN(sw_pkcs1),	    /* SW PKCS#1 implementation */
    HAB_PLUGIN(sw_fp),          /* SW prime field (RSA) arithmetic */
    HAB_PLUGIN(sw_keystore),    /* SW keystore */
#ifdef TGT_3700
    HAB_PLUGIN(rtl_test1)       /* Skipping RSA computation for iRAM build */
#endif
};

/* Plugin count */
const uint32_t hab_hal_plugin_count = HAB_ENTRIES_IN(hab_hal_plugin);

/* Peripheral areas accessible to DCD commands */
const hab_hal_region_t hab_hal_peripheral[] = {   
    /* DRAM Control*/
	HAB_PERIPHERAL((uint32_t *)REGS_DRAM_BASE, (uint32_t *)REGS_DRAM_END),
    /* BCH ECC Accelerator*/
	HAB_PERIPHERAL((uint32_t *)REGS_BCH_BASE, (uint32_t *)REGS_BCH_END),
	/* GPMI*/
	HAB_PERIPHERAL((uint32_t *)REGS_GPMI_BASE, (uint32_t *)REGS_GPMI_END),	
    /* Synchronous Serial Port 0~3 */
    HAB_PERIPHERAL((uint32_t *)REGS_SSP0_BASE, (uint32_t *)REGS_SSP0_END),
    HAB_PERIPHERAL((uint32_t *)REGS_SSP1_BASE, (uint32_t *)REGS_SSP1_END),
    HAB_PERIPHERAL((uint32_t *)REGS_SSP2_BASE, (uint32_t *)REGS_SSP2_END),
    HAB_PERIPHERAL((uint32_t *)REGS_SSP3_BASE, (uint32_t *)REGS_SSP3_END),
    /* PIN ctrl block */
    HAB_PERIPHERAL((uint32_t *)REGS_PINCTRL_BASE, (uint32_t *)REGS_PINCTRL_END),
    /* Digital Control*/
	HAB_PERIPHERAL((uint32_t *)REGS_DIGCTL_BASE, (uint32_t *)REGS_DIGCTL_END),
#ifdef TGT_SIM
	/* Simulation*/
	HAB_PERIPHERAL((uint32_t *)REGS_SIMDBG_BASE, (uint32_t *)REGS_SIMDBG_END),
	HAB_PERIPHERAL((uint32_t *)REGS_SIMGPMISEL_BASE, (uint32_t *)REGS_SIMGPMISEL_END),
	HAB_PERIPHERAL((uint32_t *)REGS_SIMSSPSEL_BASE, (uint32_t *)REGS_SIMSSPSEL_END),
	HAB_PERIPHERAL((uint32_t *)REGS_SIMMEMSEL_BASE, (uint32_t *)REGS_SIMMEMSEL_END),
	HAB_PERIPHERAL((uint32_t *)REGS_SIMENET_BASE, (uint32_t *)REGS_SIMENET_END),
#endif
    /* GPIO Ctrl */
    HAB_PERIPHERAL((uint32_t *)REGS_GPIOMON_BASE, (uint32_t *)REGS_GPIOMON_END),
    /* Clock Ctrl */
    HAB_PERIPHERAL((uint32_t *)REGS_CLKCTRL_BASE, (uint32_t *)REGS_CLKCTRL_END),
    /* Power */
	HAB_PERIPHERAL((uint32_t *)REGS_POWER_BASE, (uint32_t *)REGS_POWER_END),
	/* Real Time Clock */
    HAB_PERIPHERAL((uint32_t *)REGS_RTC_BASE, (uint32_t *)REGS_RTC_END),
    /* I2C */
    HAB_PERIPHERAL((uint32_t *)REGS_I2C0_BASE, (uint32_t *)REGS_I2C1_END),
    HAB_PERIPHERAL((uint32_t *)REGS_I2C1_BASE, (uint32_t *)REGS_I2C1_END),
    /* TIMROT */
    HAB_PERIPHERAL((uint32_t *)REGS_TIMROT_BASE, (uint32_t *)REGS_TIMROT_END),
    /* UARTs: 5 AUART+1DUART*/
    HAB_PERIPHERAL((uint32_t *)REGS_UARTAPP0_BASE, (uint32_t *)REGS_UARTAPP0_END), 
    HAB_PERIPHERAL((uint32_t *)REGS_UARTAPP1_BASE, (uint32_t *)REGS_UARTAPP1_END), 
    HAB_PERIPHERAL((uint32_t *)REGS_UARTAPP2_BASE, (uint32_t *)REGS_UARTAPP2_END), 
    HAB_PERIPHERAL((uint32_t *)REGS_UARTAPP3_BASE, (uint32_t *)REGS_UARTAPP3_END), 
    HAB_PERIPHERAL((uint32_t *)REGS_UARTAPP4_BASE, (uint32_t *)REGS_UARTAPP4_END), 
    HAB_PERIPHERAL((uint32_t *)REGS_UARTDBG_BASE, (uint32_t *)REGS_UARTDBG_END), 
};

/* Pheripheral count */
const uint32_t hab_hal_peripheral_count = HAB_ENTRIES_IN(hab_hal_peripheral);

/* Memory areas accessible to DCD commands or image loaders */
const hab_hal_region_t hab_hal_memory[] = {
    /* External memory */
    HAB_MEMORY((uint8_t *)EM_BASE, (uint8_t *)EM_END),
    /* OCRAM */
    HAB_MEMORY((uint8_t *)OCRAM_FREE_AREA_BASE, (uint8_t *)OCRAM_FREE_AREA_END),    
};

/* External memory count */
const uint32_t hab_hal_memory_count = HAB_ENTRIES_IN(hab_hal_memory);

/* IC configuration table */
const hab_hal_ict_t hab_hal_ict = {
    /* preconfigure function */
    hab_eng_sw_preconfigure,
    /* Preconfiguration data interpreted by hab_preconfigure(). */
    NULL,
    /** @rom Region reserved for HAB data guaranteed not to be overwritten
     *  until no further HAB library calls will be made: must be at least
     *  1.5kB, word-aligned and byte-accessible for read & write.
     */
    HAB_MEMORY((uint8_t *)HAB_HAL_PERSISTANT_START, (uint8_t *)HAB_HAL_PERSISTANT_END),
    /** @rom Region reserved for HAB data which may be overwritten between HAB
     *  library calls: must be at least 3.5kB, word-aligned and
     *  byte-accessible for read & write.
     */
    HAB_MEMORY((uint8_t *)HAB_HAL_SCRATCH_START, (uint8_t *)HAB_HAL_SCRATCH_END),
    /** @rom Region accessible to @ref shw DMA engines which may be
     *  overwritten between HAB library calls: must be at least 256 bytes,
     *  word-aligned and word-accessible for read and write (stack is used if
     *  this region is empty).
     */
    HAB_MEMORY((uint8_t *)HAB_HAL_DMA_START, (uint8_t *)HAB_HAL_DMA_END),
    /* IC configuration flags */
    (hab_hal_ic_flg_t) 0,
    /* Address to which result of fabrication test may be written. */
    NULL
};

#endif
