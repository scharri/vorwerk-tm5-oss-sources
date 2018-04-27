////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_boot
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_ba_nand_api.c
//! \brief This file provides the API for the ROM BA NAND.
//!
//!
////////////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////////////
//   Includes and external references
////////////////////////////////////////////////////////////////////////////////

#include <itf_rom.h>
#include "rom_types.h"
#include "rom_nand_api.h"
#include "rom_nand_internal.h"
#include "return_codes.h"

//#include "rom_nand_status_codes.h"
#include "rom_nand_hal_structs.h"
#include "rommem.h"         // Contains eFuse shadow register extern vars
#include "debug.h"
#include <string.h>
#include "rom_nand_hamming_code_ecc.h"

#include "efuse.h"
#include "persistent_bit.h"
#include "bootmode.h"
#include "regsclkctrl.h"    // for chip reset.
#include "regsdigctl.h"     // for microseconds time.
#include "rom_utils.h"     // for chip reset macro.
#if TGT_SIM
#include "regssimgpmisel.h"
#include <rom_utils.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////
extern rom_nand_Context_t *rom_nand_pCtx;


////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Find Boot Control Blocks to initialize NAND Serializer structure.
//!
//! This function reads MBR and using location of Firmware Config Block it 
//! updates members of NAND serialization structure
//!
//! \param[in]	 pBuffer Use this buffer for reading sectors during discovery.
//!
//! \retval    0 (SUCCESS)    If no error has occurred.
//! \retval    ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT  GPMI DMA timed out during read.
//! \retval    ERROR_ROM_NAND_DRIVER_NO_BCB  Couldn't find Boot Control Block.
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_ba_nand_FindBootControlBlocks(uint8_t * pBuffer)
{
    RtStatus_t Status;
    uint32_t u32Temp;
    uint32_t u32CurrentNAND = 0;    // Assume we'll boot from primary. 
    uint32_t u32NumSectorsToRead; // Size of firmware in sectors, read from config block
    uint32_t u32ReadSector = 0;

    // Check for secondary boot  
    u32Temp = 0; // Default to primary
    if (!rom_nand_pCtx->zNandEFuse.efDisableSecondaryBoot)
    {
        // Grab the persistent Data
        Status = load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32Temp);

        // This bit will be 1 if an error occurred during first boot and restarted.
        u32Temp = ((u32Temp & BM_PERSIST1_ROM_REDUNDANT_BOOT) >> BP_PERSIST1_ROM_REDUNDANT_BOOT);

        // I don't think this test is needed, but this is too critical to ignore.
        // If an error occurs, assume primary boot blocks.
        if (Status != SUCCESS)
        {
            u32Temp = 0;
        }
    }

    DBG_PRINTF("Boot Search on %s\n", u32Temp ? "Secondary" : "Primary");

    rom_nand_pCtx->zSerializerState.m_u32UseSecondaryBoot = u32Temp;

    // Read MBR
    // Send read command first
    Status = g_pNand->BANandHalSendReadCmd(u32CurrentNAND, 1, u32ReadSector, pBuffer);
    if (Status != SUCCESS)
        return Status;

    // Find Firmware Config Block location
    Status = g_pNand->ReadMBR(pBuffer, &u32ReadSector);
    if (Status != SUCCESS)
        return ERROR_ROM_NAND_DRIVER_NO_BCB;

    DBG_PRINTF("BA NAND MBR: Firmware Block Start Sector %d\n", u32ReadSector);


    // Read Firmware Config Block
    // Send read command first
    Status = g_pNand->BANandHalSendReadCmd(u32CurrentNAND, 1, u32ReadSector, pBuffer);
    if (Status != SUCCESS)
        return Status;

    // Search firmware config block to read start sector of firmware
    Status = g_pNand->ReadConfigBlock(u32CurrentNAND, pBuffer, &u32ReadSector, &u32NumSectorsToRead, rom_nand_pCtx->zSerializerState.m_u32UseSecondaryBoot);
    if (Status != SUCCESS)
        return Status;

    DBG_PRINTF("BA NAND: Boot Firmware Start Sector %d\n", u32ReadSector);

    // Set the current sector to start of firmware
    rom_nand_pCtx->zSerializerState.m_uCurrentSector = u32ReadSector;
    rom_nand_pCtx->zSerializerState.m_uCurrentNand = u32CurrentNAND;

    // not used for BA-NANDs, initializing to safe values
    rom_nand_pCtx->zSerializerState.m_uSectorsPerBlock = 1;
    //rom_nand_pCtx->zSerializerState.m_uCurrentSectorInBlock = 0;
    rom_nand_pCtx->zSerializerState.m_uSectorsRead = 0;
    rom_nand_pCtx->zSerializerState.m_uCurrentBlock = 0; 
    rom_nand_pCtx->zSerializerState.m_u32BBAreaStartPage = 0; 
    rom_nand_pCtx->zSerializerState.m_u32NumberOfBadBlocks = 0;
    rom_nand_pCtx->zSerializerState.m_u32BBMarkByteOffsetInPageData = 0;
    rom_nand_pCtx->zSerializerState.m_u32BBMarkBitOffset = 0;
    rom_nand_pCtx->zSerializerState.m_uSectorsToRead = u32NumSectorsToRead;

    return SUCCESS;
}
