////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_boot
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_api.c
//! \brief This file provides the API for the ROM NAND.
//!
//!
////////////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////////////
//   Includes and external references
////////////////////////////////////////////////////////////////////////////////

//#include "itf_rom.h"
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
#include "configblock.h"
#include "rom_ba_nand_hal.h"
#if TGT_SIM
#include "regssimgpmisel.h"
#include <rom_utils.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////
extern void rom_nand_hal_UpdateECCParams(NAND_ECC_Params_t *pNANDEccParams, uint32_t u32NumNandsInUse);
////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
//#define __PRINT_PREP_PROGRESS
//#define VERBOSE_PRINTF

//! Define setting the NAND Chip Enable to GPMI_CE0.
#define NAND0   0

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

//! \brief Pointer to the NAND boot driver context information.
rom_nand_Context_t *rom_nand_pCtx = NULL;

// MICHAL - global variable to store FCB's committed image
uint32_t m_u32UpdateStatus = 0;

////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////

//! \brief FCB Fingerprint values.
//!
//! These are the fingerprints that are spaced at defined values in the
//! first page of the block to indicate this is a FCB.  
const FingerPrintValues zFCBFingerPrints = 
{
    FCB_FINGERPRINT,
    FCB_VERSION
};

//! \brief DBBT Fingerprint values.
//!
//! These are the fingerprints that are spaced at defined values in the
//! first page of the block to indicate this is a DBBT.  
const FingerPrintValues zDBBTFingerPrints = 
{
    DBBT_FINGERPRINT,
    DBBT_VERSION
};

//! \brief Nand boot driver interface implementation.
//!
//! This structure is placed in ROM to conserve data space. To patch the
//! driver, a plugin would need to first copy this table to RAM, modify
//! the copy, then update the interface table to point to the copy.
const nand_BootItf_t rom_NandBootItf =
{
    // Standard boot driver API
    {
        rom_nand_BootInit,
        rom_nand_BootNext,
        rom_nand_BootSkip,
        rom_nand_BootStop,
        rom_nand_BootCtrl,
        1 // Nand driver supports redundant boot
    },
    
    &rom_nand_pCtx,
    
    (int (*)(uint32_t, const void *, uint32_t *, uint8_t *))rom_nand_BootBlockSearch,       //!< Search for a matching boot block.
    rom_nand_BlockIsInBadBlockTable,    //!< Looks for a block in the bad block table.
    rom_nand_FindFCB,               //!< Searches for and reads the FCB.
    rom_nand_ReadBadBlockTable,     //!< Loads the bad block table.
    rom_nand_FindBootControlBlocks, //!< Initialize the state machine.
    rom_nand_StartSectorRead,       //!< Start the Sector Read.
    rom_nand_WaitOnRead,            //!< Wait on Read Completion.
    rom_nand_SkipSectors,           //!< Skip a number of sectors.
    rom_nand_FindNextGoodBlock,     //!< Look for a good block.
    rom_nand_hal_Init,              //!< Initialize the HAL layer.
    rom_nand_hal_HWInit,            //!< Initialize the HAL NAND Hardware.
    rom_nand_hal_ConfigurePinmux,   //!< Configure the GPMI Pinmux. (GPMI)
    rom_nand_hal_ReadPage,            //!< Read a sector from the HAL layer.
    rom_nand_hal_InitReadDma,       //!< Initialize Read DMA with values that don't change.
    rom_nand_hal_ReadNand,          //!< Generic NAND Read function.
    rom_nand_hal_WaitForReadComplete, //!< Initialize the HAL layer.
    rom_nand_hal_CurrentGpmiDmaStatus, //!< Current DMA status.
    rom_nand_hal_GpmiSetNandTiming, //!< Update NAND Timing.
    rom_nand_hal_CheckECCStatus,    //!< Check the status of the ECC
    rom_nand_hal_ResetNAND,         //!< Reset the NAND.
    rom_nand_hal_ReadNandID,        //!< Read the ID of the NAND.
    rom_nand_hal_StartDma,          //!< Kick off a NAND DMA.
    rom_nand_hal_WaitDma,           //!< Wait for NANDs DMA to finish.
    rom_nand_hal_UpdateDmaDescriptor,//!< Update the DMA descriptor with new values.
    rom_nand_hal_Shutdown,          //!< Shutdown the NAND HAL
    rom_ba_nand_hal_SendReadCmd,    //!< Send read command
    rom_ba_nand_FindBootControlBlocks, //!< Find config blocks for ba nand
    rom_ReadMBR,                    //!< Reads MBR, first block and validates it.
    rom_ReadConfigBlock,            //!< Reads Firmware Config block validates it.
    rom_nand_hal_ReadParameterPage, //!< Reads parameter page from ONFi NAND
    rom_nand_hal_IsBlockBad,        //!< Hal layer API to determine if block is bad using mfr marked bad block scheme.
    rom_nand_VerifyFCB,             //!< Verifies FCB by running software ECC
    rom_nand_GetBCBChecksum         //!< Api to run BCB checksum algorithm
};

#define g_pNand     (&rom_NandBootItf)



// Map for ecc levels
const uint32_t mapToEccLevel[BCH_Ecc_Max_Levels]=
{
    BCH_Ecc_0bit,
    BCH_Ecc_2bit,
    BCH_Ecc_4bit,
    BCH_Ecc_6bit,
    BCH_Ecc_8bit,
    BCH_Ecc_10bit,
    BCH_Ecc_12bit,
    BCH_Ecc_14bit,
    BCH_Ecc_16bit,
    BCH_Ecc_18bit,
    BCH_Ecc_20bit
};

////////////////////////////////////////////////////////////////////////////////
//! \brief  Initialize the boot driver.
//!
//! Driver initialization function. The loader will call this function before
//! using any of the other driver functions. The driver is to locate all buffers
//! and static data in the memory region pMem[size] passed through the parameter
//! structure.
//!
//! \param[in]  pInit   Pointer to initialization structure.
////////////////////////////////////////////////////////////////////////////////
int rom_nand_BootInit(rom_BootInit_t *pInit)
{
    RtStatus_t retStatus;

    // make sure the memory region is large enough for the driver context
    if (pInit->size < sizeof(rom_nand_Context_t))
    {
        return ERROR_ROM;
    }

    DBG_PRINTF("\nrom_nand_BootInit:%t\n");
    rom_nand_pCtx = pInit->pMem;
    rom_nand_pCtx->pDmaBuf = NULL;
    rom_nand_pCtx->pLoaderBuf = NULL;
    rom_nand_pCtx->skipCount = 0; 
    rom_nand_pCtx->bEnableHWECC = true; //default to true;
    // Reset the number of bad blocks.
    rom_nand_pCtx->zSerializerState.m_u32NumberOfBadBlocks = 0;   

    //Read the eFuse that tells me how many NANDs are in the system.
    // Since 1 bit per NAND, we can tell how many NANDs are in system but we
    // only need to worry about the first 2 NANDs for the ROM.

    {
        uint32_t eFuseValue, u32Temp;
    
        // I've reserved some bits in the 2nd ROM register.
        eFuseValue = HW_OCOTP_ROMn_RD(1);
        // This eFuse represents the presence or absence of a NAND.
        // Currently, only the least significant nibble is used. 
        // If no eFuses are programmed, the HAL init will assume there
        // are 2 NANDs and then confirm the 2nd NANDs presence with a
        // reset.
        rom_nand_pCtx->zNandEFuse.efNANDsInUse = 
            ((eFuseValue & BM_OCOTP_ROM1_NUMBER_OF_NANDS) >> 
                                 BP_OCOTP_ROM1_NUMBER_OF_NANDS);        

        // Add 1.8 V device support boot media
        rom_nand_pCtx->u32Use1_8V = (pInit->mode & VOLTAGE_SEL_BIT) ? 1 : 0;  	

        // The search stride value is calculated from OCOTP_ROM1_BOOT_SEARCH_STRIDE.
        // it is otp value times NAND_SEARCH_STRIDE.
        u32Temp = ((eFuseValue & BM_OCOTP_ROM1_BOOT_SEARCH_STRIDE) >> 
                                  BP_OCOTP_ROM1_BOOT_SEARCH_STRIDE);
        
        if (u32Temp == 0) 
        {
            // Search stride cannot be 0. 
            u32Temp = 1; 
        }
        rom_nand_pCtx->zNandEFuse.efNANDBootSearchStride = (u32Temp * NAND_SEARCH_STRIDE);

        // The number of search stride pages to read is set with this eFuse value.
        // This is a power of 2 value so 0 = 1, 1 = 2, 2 = 4, etc.
        u32Temp = ((eFuseValue & BM_OCOTP_ROM1_BOOT_SEARCH_COUNT) >> 
                                  BP_OCOTP_ROM1_BOOT_SEARCH_COUNT);
        // The default number of 64 pages to read is 4.
        if (u32Temp == 0) u32Temp = 2; //1, default search stride is modified to 4 instead of 2.

        rom_nand_pCtx->zNandEFuse.efNANDBootSearchLimit = (1 << u32Temp);

        // The Altnernate Chip Enables must be known before taking this high.
        // Internal pullups are implemented in the chip and if these OTP bits
        // are set then we enable them.
      //  rom_nand_pCtx->zNandEFuse.efAltCEPinConfig = (eFuseValue & 
      //                                        BM_OCOTP_ROM1_ENABLE_NAND_CE_RDY_ALT_PINS);
        rom_nand_pCtx->zNandEFuse.efEnableIntPullups = (eFuseValue & 
                                              BM_OCOTP_ROM1_ENABLE_NAND_CE_RDY_PULLUPS);

        rom_nand_pCtx->zNandEFuse.efDisableSecondaryBoot = (eFuseValue &
                                              BM_OCOTP_ROM1_DISABLE_SECONDARY_BOOT);
        // The 3.3V/1.8V bootmode is important because we need to set the voltage of the pins
        // before power up.  We get this from the boot mode that is passed in.
        //rom_nand_pCtx->u32Use1_8V = (pInit->mode & VOLTAGE_SEL_BIT) ? 1 : 0;  

        // Bit 3 of the bootmode selects either ECC4 (0) or ECC8 (1).
        // Ecc is no longer in boot mode. Initializing it to 0 and should be overwritten in FindFCB().
        
        rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel = 0;//(pInit->mode & NAND_ECC_TYPE) ? 8 : 4;  

        // There are some more bits reserved in ROM3 too
        eFuseValue = HW_OCOTP_ROMn_RD(4);

        // Initialize zNandEFuse.efNANDRowAddressBytes 
        u32Temp = ((eFuseValue & BM_OCOTP_ROM4_NAND_ROW_ADDRESS_BYTES) >> 
                              BP_OCOTP_ROM4_NAND_ROW_ADDRESS_BYTES);
        if(u32Temp == 0)
        {
            // Default row address bytes are 3
            u32Temp = 3;
        }
        rom_nand_pCtx->zNandEFuse.efNANDRowAddressBytes = u32Temp;

        // Initialize zNandEFuse.efNANDColumnAddressBytes 
        u32Temp = ((eFuseValue & BM_OCOTP_ROM4_NAND_COLUMN_ADDRESS_BYTES) >> 
                              BP_OCOTP_ROM4_NAND_COLUMN_ADDRESS_BYTES);
        if(u32Temp == 0)
        {
            // Default col address bytes are 2
            u32Temp = 2;
        }
        rom_nand_pCtx->zNandEFuse.efNANDColumnAddressBytes = u32Temp;

        // Initialize zNandEFuse.efNANDReadCmdCode1 
        u32Temp = ((eFuseValue & BM_OCOTP_ROM4_NAND_READ_CMD_CODE1) >> 
                              BP_OCOTP_ROM4_NAND_READ_CMD_CODE1);
        rom_nand_pCtx->zNandEFuse.efNANDReadCmdCode1 = u32Temp;

        // Initialize zNandEFuse.efNANDReadCmdCode2 
        u32Temp = ((eFuseValue & BM_OCOTP_ROM4_NAND_READ_CMD_CODE2) >> 
                              BP_OCOTP_ROM4_NAND_READ_CMD_CODE2);
        if(u32Temp == 0)
        {
            // Default read cmd code 2 is 0x30
            u32Temp = 0x30;
        }
        rom_nand_pCtx->zNandEFuse.efNANDReadCmdCode2 = u32Temp;

        // Initialize zNandEFuse.efBIPreserve
        
        u32Temp = ((eFuseValue & BM_OCOTP_ROM4_NAND_BADBLOCK_MARKER_PRESERVE_DISABLE) >> 
                              BP_OCOTP_ROM4_NAND_BADBLOCK_MARKER_PRESERVE_DISABLE);
		
        
        rom_nand_pCtx->zNandEFuse.efBIPreserve = u32Temp ^ 0x1;

  
        DBG_PRINTF("LaserFuseValue2 0x%X\n", eFuseValue);
        DBG_PRINTF("NANDs In Use  %d\n", rom_nand_pCtx->zNandEFuse.efNANDsInUse);
        DBG_PRINTF("NAND row bytes %d\n", rom_nand_pCtx->zNandEFuse.efNANDRowAddressBytes);
        DBG_PRINTF("NAND col bytes %d\n", rom_nand_pCtx->zNandEFuse.efNANDColumnAddressBytes);
        DBG_PRINTF("NAND read cmd code1 %d\n", rom_nand_pCtx->zNandEFuse.efNANDReadCmdCode1);
        DBG_PRINTF("NAND read cmd code2 %d\n", rom_nand_pCtx->zNandEFuse.efNANDReadCmdCode2);
        DBG_PRINTF("ECC Type 0x%X\n", rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);
        DBG_PRINTF("NANDs Search Stride %d\n", rom_nand_pCtx->zNandEFuse.efNANDBootSearchStride);
        DBG_PRINTF("NANDs Search Limit %d\n", rom_nand_pCtx->zNandEFuse.efNANDBootSearchLimit);
        DBG_PRINTF("NANDs Pin Voltage %s\n", "3.3V");
        DBG_PRINTF("NAND Alternate Chip Enables %d\n", rom_nand_pCtx->zNandEFuse.efAltCEPinConfig);        
        DBG_PRINTF("Internal pullups implemented for %d\n", rom_nand_pCtx->zNandEFuse.efEnableIntPullups);        
    }

    DBG_PRINTF("Init HAL at %t\n");
    retStatus = g_pNand->HalInit(&rom_nand_pCtx->zNandEFuse);
    DBG_PRINTF("NANDs In Use %d\n", rom_nand_pCtx->zNandEFuse.efNANDsInUse);
    DBG_PRINTF("Completed Init HAL at %t\n");
    if (retStatus != SUCCESS )
    {
        return retStatus;
    }

    // Check to see if NAND is block abstracted (BA-NAND)
    if (rom_nand_pCtx->zNANDDescriptor.bBA_NAND)
    {
        uint32_t eFuseValue, u32Temp;
        rom_nand_pCtx->bEnableHWECC = false; // No ECC used for BA NANDs.
        rom_nand_pCtx->zSerializerState.m_u32PageDataSize = rom_nand_pCtx->zNANDDescriptor.stc_NANDSectorDescriptor.u16SectorSize;
        rom_nand_pCtx->zSerializerState.m_u32SectorDataRead = 0;
        rom_nand_pCtx->chunksPerRead = rom_nand_pCtx->zSerializerState.m_u32PageDataSize / BYTES_PER_CHUNK;
    
        // If otp does not provide then set defaults to row address bytes, readcmd1 and readcmd2

        eFuseValue = HW_OCOTP_ROMn_RD(4);

        u32Temp = ((eFuseValue & BM_OCOTP_ROM4_NAND_ROW_ADDRESS_BYTES) >> 
                              BP_OCOTP_ROM4_NAND_ROW_ADDRESS_BYTES);
        if(u32Temp == 0)
        {
            rom_nand_pCtx->zNANDDescriptor.btNumRowBytes = BANAND_ROW_ADDRESS_BYTES;
        }

        u32Temp = ((eFuseValue & BM_OCOTP_ROM4_NAND_READ_CMD_CODE1) >> 
                              BP_OCOTP_ROM4_NAND_READ_CMD_CODE1);
        if(u32Temp == 0)
        {
            rom_nand_pCtx->zNANDDescriptor.stc_NANDDeviceCommandCodes.btRead1Code = eBANandLBARead; 
        }

        u32Temp = ((eFuseValue & BM_OCOTP_ROM4_NAND_READ_CMD_CODE2) >> 
                              BP_OCOTP_ROM4_NAND_READ_CMD_CODE2);
        if(u32Temp == 0)
        {
            rom_nand_pCtx->zNANDDescriptor.stc_NANDDeviceCommandCodes.btRead1_2ndCycleCode = eBANandLBAReadCycle2; 
        }

#ifdef TGT_SIM
        DBG_PRINTF("Found BA Nand\n");
        DBG_PRINTF("BA Nand Sector Size = %d\n", rom_nand_pCtx->zNANDDescriptor.stc_NANDSectorDescriptor.u16SectorSize);
#endif

    }
    DBG_PRINTF("Discover NAND BCB:%t\n");

    // Initialize the state machine only use one buffer.
    if(rom_nand_pCtx->zNANDDescriptor.bBA_NAND)
    {
        retStatus = g_pNand->BANandReadMediaInit(rom_nand_pCtx->bufA);
    }
    else
    {
        retStatus = g_pNand->ReadMediaInit(rom_nand_pCtx->bufA);
    }

    rom_nand_pCtx->nextChunk = rom_nand_pCtx->chunksPerRead;
    return retStatus;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief  Get the next cipher block(s) from the boot image.
//!
//! Driver boot image read function. The loader calls this function to request
//! the next 1 or more cipher blocks from the boot image. The driver should
//! return all blocks it has available, up to the number requested. If the
//! driver has no blocks ready, it should start a media read operation and
//! return 0 blocks. The loader will continue to request blocks until the
//! entire boot image is read or some other action terminates reading.
//!
//! When this function returns 1 or more blocks, the driver cannot reuse the
//! memory of the returned blocks until the next call. The loader implicitly
//! frees the memory of any previous block(s) returned on each new call of
//! this function. Thus, on entry, the driver can use all block memory that
//! was assigned at initialization.
//!
//! \param[in]  pCount  The number of blocks requested. Will be 1 or more.
//!                     0 and negative values are not allowed.
//! \param[out] pCount  The number of blocks returned. Can be 1 or more, up to
//!                     the number requested. 0 means no blocks were available,
//!                     so try again later. A negative value indicates an error.
//!
//! \return A pointer the next set of 1 or more cipher blocks read from the
//!         boot image.
//! \retval NULL    If there are no blocks ready or an error occurred.
////////////////////////////////////////////////////////////////////////////////
chunk_t *rom_nand_BootNext(int *pCount)
{
    int ready;
    chunk_t * pNext;
    int err;
    
    DBG_PRINTF(">Next(%d)", *pCount);

    // make sure the driver has been initialized
    if (rom_nand_pCtx == NULL)
    {
        *pCount = ERROR_ROM;
        return NULL;
    }

    // check for dma in flight
    if (rom_nand_pCtx->pDmaBuf != NULL)
    {
        int status = g_pNand->ReadComplete();

        if (status == SUCCESS)
        {
            // DMA completed successfully
            rom_nand_pCtx->pLoaderBuf = rom_nand_pCtx->pDmaBuf;
            rom_nand_pCtx->pDmaBuf = NULL;
            rom_nand_pCtx->nextChunk = 0;
        }
        else if (status == ERROR_ROM_NAND_DMA_BUSY)
        {
            // DMA is still in flight
            *pCount = 0;
            return NULL;
        }
        else
        {
            // Return the error code
            *pCount = status;
            return NULL;
        }
    }

    // determine the number chunks ready in the driver buffer
    ready = rom_nand_pCtx->chunksPerRead - rom_nand_pCtx->nextChunk;
    if (ready < 0)
    {
        ready = 0;
    }
    // adjust the ready count if we need to skip
    if (rom_nand_pCtx->skipCount)
    {
        if (rom_nand_pCtx->skipCount > ready)
        {
            unsigned sectorsToSkip;
            
            rom_nand_pCtx->nextChunk = rom_nand_pCtx->chunksPerRead;
            rom_nand_pCtx->skipCount -= ready;
            
            // skip whole sectors
            sectorsToSkip = rom_nand_pCtx->skipCount / rom_nand_pCtx->chunksPerRead;
            g_pNand->SkipSectors(sectorsToSkip, (
                (rom_nand_pCtx->pLoaderBuf == rom_nand_pCtx->bufA) ?
                rom_nand_pCtx->bufB : rom_nand_pCtx->bufA
                ));
            
            // The resulting skip count is the number of chunks left over that
            // doesn't fit evenly into a sector. ready will remain 0 and the
            // skip count will be processed again the next time through.
            rom_nand_pCtx->skipCount %= rom_nand_pCtx->chunksPerRead;
        }
        else
        {
            rom_nand_pCtx->nextChunk += rom_nand_pCtx->skipCount;
            rom_nand_pCtx->skipCount  = 0;
        }

        ready = rom_nand_pCtx->chunksPerRead - rom_nand_pCtx->nextChunk;
        
        if (ready < 0)
        {
            ready = 0;
        }
    }

    // if caller is asking for more chunks than we have, start the next dma
    if (*pCount > ready)
    {
        *pCount = ready;

        // select the next buffer
        if (rom_nand_pCtx->pLoaderBuf == rom_nand_pCtx->bufA)
        {
            rom_nand_pCtx->pDmaBuf = rom_nand_pCtx->bufB;
        }
        else
        {
            rom_nand_pCtx->pDmaBuf = rom_nand_pCtx->bufA;
        }
        
        // start DMA transfer
        err = g_pNand->ReadMedia(rom_nand_pCtx->pDmaBuf);
        if (err != SUCCESS)
        {
            *pCount = err;
            return NULL;
        }
    }

    // calculate the return pointer, then adjust the next index
    pNext = (chunk_t *)&rom_nand_pCtx->pLoaderBuf[rom_nand_pCtx->nextChunk * BYTES_PER_CHUNK];
    rom_nand_pCtx->nextChunk += *pCount;

    return (ready ? pNext : NULL);
}

////////////////////////////////////////////////////////////////////////////////
//! \brief  Skip over the next cipher block(s) from the boot image.
//!
//! Driver boot image seek function. The loader uses this function to tell the
//! driver to skip the next \e count cipher blocks in the boot image. On the
//! next call to rom_BootNext(), the driver should return the blocks that
//! immediately follow the last skipped block.
//!
//! \param[in]  count   Number of blocks to skip.
////////////////////////////////////////////////////////////////////////////////
int rom_nand_BootSkip(int count)
{
    rom_nand_pCtx->skipCount += count;
    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief  Shut down the boot driver.
//!
//! Driver shut down function. The loader will call this function when it is
//! finished using the driver services. The driver should terminate any active
//! DMA, and place it's hardware in an inactive, low power state. Also, the
//! driver must release use of the memory region that was passed to it during
//! initialization.
//!
//! \todo Determine why the DMA wait loop stalls with stub driver.
////////////////////////////////////////////////////////////////////////////////
int rom_nand_BootStop(void)
{
    int status = SUCCESS;
    
    // check for valid context
    if (rom_nand_pCtx == NULL)
    {
        return ERROR_ROM;
    }
    
    // wait for DMA to complete. if there is an error, it will fall
    // through and be returned while still shutting down the hal.
 //   if (rom_nand_pCtx->pDmaBuf)
 //   {
 //       do {
 //           status = g_pNand->ReadComplete();
 //       } while (status == ERROR_ROM_NAND_DMA_BUSY);
 //   }
    
    // shut down hal
    g_pNand->HalShutdown();
    return status;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief  Control boot driver operations and settings.
//!
//! Driver control function. For now this function supports two actions which
//! are used to coordinate changes in system power conditions (ie. clocks and
//! voltage) with the driver.
//!
//! - BOOT_PAUSE. This action is invoked prior to changing system conditions.
//! The driver must ensure that any pending operations that would be affected
//! by a power change are completed or cancelled before returning. This is a
//! synchronous call, so the driver may wait as long as needed, but should time
//! out if there is a problem.
//!
//! - BOOT_RESUME. This action is invoked after changing system conditions.
//! The parameter \e pArg will provide information describing the new settings.
//! The driver should update as needed (ie. change dividers, ungate clocks,
//! etc.), then resume operations.
//!
//! \param[in]  action  Requested driver control action.
//! \param[in]  pArg    Pointer to control action parameters. This argument
//!                     will be different for each action.
//! \todo Fill in this functionality if needed.
////////////////////////////////////////////////////////////////////////////////
int rom_nand_BootCtrl(rom_BootAction_t action, void *pArg)
{
    switch (action)
    {
    case BOOT_PAUSE:
        DBG_PRINTF("..demo_Ctrl(BOOT_PAUSE, %x)\n", pArg);
        // Wait for any pending DMA transactions to complete.
        g_pNand->HalWaitForReadComplete(0, rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);
        g_pNand->HalWaitForReadComplete(1, rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);
        break;
    case BOOT_RESUME:
        DBG_PRINTF("..demo_Ctrl(BOOT_RESUME, %x)\n", pArg);
        // Initialize the chip.
        //g_pNand->HalChipInit(&rom_nand_pCtx->zNANDEccParams.efNANDsInUse);
        // Update the NAND Timings.
        //g_pNand->HalUpdateGPMITiming();
        break;
    default:
        DBG_PRINTF("..demo_Ctrl(%d, %x)\n", action, pArg);
        break;
    }
    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief  Setup the boot driver
//!
//! This function is used to install the drivers required by the boot
//! driver harness.  
//!
//!
//! \param[in]  pDriver  Boot interface driver.
//!
//! \todo Move this to the rom_nand_hal_api_write.c file.
////////////////////////////////////////////////////////////////////////////////
int rom_nand_BootDriverSetupDriver(rom_BootItf_t *pDriver)
{
    int iRtn = ERROR_ROM;

    if(pDriver)
    {
        nand_BootItf_t * nandDriver = (nand_BootItf_t *)pDriver;
        
        pDriver->Init = rom_nand_BootInit;
        pDriver->Next = rom_nand_BootNext;
        pDriver->Skip = rom_nand_BootSkip;
        pDriver->Control = rom_nand_BootCtrl;
        pDriver->Stop = rom_nand_BootStop;
        pDriver->nRedundantBootSupported = 1;

        nandDriver->BootBlockSearch = (int (*)(uint32_t, const void *, uint32_t *, uint8_t *))rom_nand_BootBlockSearch;
        nandDriver->BlockIsInBadBlockTable = rom_nand_BlockIsInBadBlockTable;
        nandDriver->FindFCB = rom_nand_FindFCB;
        nandDriver->ReadBadBlockTable = rom_nand_ReadBadBlockTable;
        nandDriver->ReadMediaInit = rom_nand_FindBootControlBlocks;
        nandDriver->ReadMedia = rom_nand_StartSectorRead;
        nandDriver->ReadComplete = rom_nand_WaitOnRead;
        nandDriver->SkipSectors = rom_nand_SkipSectors;
        nandDriver->FindNextGoodBlock = rom_nand_FindNextGoodBlock;
        nandDriver->HalInit = rom_nand_hal_Init;
        nandDriver->HalGpmiHWInit = rom_nand_hal_HWInit;
        nandDriver->HalGpmiConfigurePinmux = rom_nand_hal_ConfigurePinmux;
        nandDriver->HalReadPage = rom_nand_hal_ReadPage;
        nandDriver->HalInitReadDma = rom_nand_hal_InitReadDma;
        nandDriver->HalReadNand = rom_nand_hal_ReadNand;
        nandDriver->HalWaitForReadComplete = rom_nand_hal_WaitForReadComplete;
        nandDriver->HalDmaStatus = rom_nand_hal_CurrentGpmiDmaStatus;
        nandDriver->HalUpdateGPMITiming = rom_nand_hal_GpmiSetNandTiming;
        nandDriver->HalECCStatus = rom_nand_hal_CheckECCStatus;
        nandDriver->HalReadID = rom_nand_hal_ReadNandID;
        nandDriver->HalReset = rom_nand_hal_ResetNAND;
        nandDriver->HalStartDma = rom_nand_hal_StartDma;
        nandDriver->HalWaitDma = rom_nand_hal_WaitDma;
        nandDriver->HalUpdateDmaDescriptor = rom_nand_hal_UpdateDmaDescriptor;
        nandDriver->HalShutdown = rom_nand_hal_Shutdown;
        nandDriver->BANandHalSendReadCmd = rom_ba_nand_hal_SendReadCmd;
        nandDriver->BANandReadMediaInit = rom_ba_nand_FindBootControlBlocks;
        nandDriver->ReadMBR = rom_ReadMBR;
        nandDriver->ReadConfigBlock = rom_ReadConfigBlock;
        nandDriver->HalReadParameterPage = rom_nand_hal_ReadParameterPage;
        nandDriver->HalIsBlockBad = rom_nand_hal_IsBlockBad;
        nandDriver->VerifyFCB = rom_nand_VerifyFCB;
        nandDriver->GetBCBChecksum = rom_nand_GetBCBChecksum;    

        iRtn = SUCCESS;
    }
    return iRtn;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Search the NAND for a boot block (FCB or DBBT).
//!
//! This function searches through the NAND looking for a matching boot block.
//! A block is considered matching if all three finger print values match
//! those in the \a pFingerPrintValues table.
//!
//! \param[in] u32NandDeviceNumber Physical NAND number to search.
//! \param[in] pFingerPrintValues Pointer to a table of finger prints. 
//! \param[in,out] p32SearchSector On entering this function, this points to
//!     the number of the sector to start searching from. On exit, it's set
//!     to the sector at which the search stopped. If an error is returned,
//!     this value is not modified.
//! \param[in] pBuffer Pointer to memory to store the page reads. The buffer must
//!     be large enough to hold an entire sector (2K).
//!
//! \retval    SUCCESS    No error has occurred.
//! \retval    ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT  GPMI DMA timed out during read.
//! \retval    ERROR_ROM_NAND_DRIVER_NO_BCB  Couldn't find Boot Control Block.
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_BootBlockSearch(uint32_t u32NandDeviceNumber, 
                                    const FingerPrintValues *pFingerPrintValues,
                                    uint32_t * p32SearchSector,
                                    uint8_t * pBuffer)
{
    bool bFoundBootBlock;
    uint32_t i;
    RtStatus_t retStatus = SUCCESS;
    BootBlockStruct_t *pBootBlock;
    uint32_t iReadSector = 0;

    // find the boot block
    bFoundBootBlock = false;
    for(i=0;((i<rom_nand_pCtx->zNandEFuse.efNANDBootSearchLimit) && 
             ((retStatus == SUCCESS) || (retStatus == ERROR_ROM_NAND_ECC_FAILED)) &&  // If ECC error, let's try again.
             !bFoundBootBlock);i++)
    {
        // We skip NAND_SEARCH_STRIDE pages between read attempts.
        iReadSector = (i * rom_nand_pCtx->zNandEFuse.efNANDBootSearchStride) + *p32SearchSector;

        DBG_PRINTF("%t @ Read Start\n");
        // look through these blocks, looking for the NAND Control Block. And wait for the DMA to complete.
        g_pNand->HalReadPage(u32NandDeviceNumber, iReadSector, pBuffer);
        retStatus = g_pNand->HalWaitForReadComplete(u32NandDeviceNumber, 
                                                    rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);
        DBG_PRINTF("%t @ Read Complete\n");
        
        // If ECC was successful, we need to check the signature.
        if((retStatus == SUCCESS) || (retStatus == ERROR_ROM_NAND_ECC_THRESHOLD))
        {
	    uint32_t crc;
            pBootBlock = (BootBlockStruct_t*)pBuffer;

            // Match signatures to determine if this was successful.
            crc = g_pNand->GetBCBChecksum(pBuffer+4, 512-4);
            // Match signatures to determine if this was successful.
            if (crc == pBootBlock->m_u32Checksum &&
                pBootBlock->m_u32FingerPrint == pFingerPrintValues->m_u32FingerPrint &&  
                pBootBlock->m_u32Version == pFingerPrintValues->m_u32Version)
            {
                bFoundBootBlock = true;
            }
            if (retStatus == ERROR_ROM_NAND_ECC_THRESHOLD)
            {  
                //#warning Set SDK_Rewrite persistent bit here - primary boot block needs fixin'
                write_persistent_word(HW_RTC_ROM_PERSISTENT_BITS1_SET, BM_PERSIST1_NAND_SDK_BLOCK_REWRITE);
#ifdef TGT_SIM
                {
                uint32_t u32PersistentData;
                load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32PersistentData);
                DBG_PRINTF("SDK Rewrite\n");

                }
#endif
                // Stub to set the SDK_Rewrite persistent bit.
                retStatus = SUCCESS;
            }
        }
    }
    
    if(!bFoundBootBlock && retStatus==SUCCESS)
    {
        //didn't find a Boot block within the desired search area.
        retStatus = ERROR_ROM_NAND_DRIVER_NO_BCB;
    }
    
    // Remember where we stopped.
    *p32SearchSector = iReadSector;

    return retStatus;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Determine if current sector is in the Bad Block Table.
//!
//! This function searches through the NAND bad block table to determine if
//! the block is bad.
//!
//! \param[in]  u32BlockToMatch Physical Block number to match..
//!
//! \retval    TRUE   Block is in table
//! \retval    FALSE  Block is not in table
//!
////////////////////////////////////////////////////////////////////////////////
bool rom_nand_BlockIsInBadBlockTable(uint32_t u32BlockToMatch)
{
    int i;
    uint16_t * pu16BadBlock = &(rom_nand_pCtx->u16BadBlockTable[0]);

    for (i=0; i<rom_nand_pCtx->zSerializerState.m_u32NumberOfBadBlocks; i++)
    {
        if (*pu16BadBlock++ == u32BlockToMatch)
        {
            return true;
        }
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Software ECC on a page of FCB data.
//!
//! \param[in] pBuffer Pointer to buffer to use for sector reads.
//! \param[out]pFCBGoodCopy returns a copy of good FCB structure
//!
//! \retval SUCCESS The FCB was found. 
//! \retval ERROR_ROM_NAND_DRIVER_FCB_TRIPLE_RED_CHK_FAILED  .
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_VerifyFCB(uint8_t * pBuffer, void **pFCBGoodCopy)
{
    RtStatus_t retStatus=SUCCESS;
    BootBlockStruct_t * pFCB;
    uint8_t * pParity;

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // ECC check for FCB block
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // first copy of FCB data is stored at offset NAND_HC_ECC_OFFSET_DATA_COPY
    // The parity bytes are stored at offset NAND_HC_ECC_OFFSET_PARITY_COPY
    // for every 8 bits of data, we have 5 bits of parity, for 512 bytes (4096 bits) we have 512 8-bits data packets,
    // we will have each 5-bit parity in a byte so we need 512 bytes for parity data.
    pFCB = (BootBlockStruct_t *)(pBuffer+NAND_HC_ECC_OFFSET_DATA_COPY);
    pParity   = (pBuffer+NAND_HC_ECC_OFFSET_PARITY_COPY); 

    DBG_PRINTF("--->Running Hamming check on FCB.\n");
    retStatus = HammingCheck((uint8_t*)pFCB, pParity);
    if( retStatus == SUCCESS )
    {
        DBG_PRINTF("--->Hamming check on FCB is successful.\n");
        *pFCBGoodCopy = pFCB;
    }

    return retStatus;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Searches for the FCB and saves its contents if found.
//!
//! Two conditions are required to claim that the FCB has been found.  The ECC
//! must not exceed the limit and the fingerprints embedded in the page must
//! match the expected fingerprints.
//!
//! \param[in] u32CurrentNAND Number of the NAND to read from.
//! \param[in,out] On entering this function, this points to
//!     the number of the sector to start searching from. On exit, it's set
//!     to the sector at which the search stopped. If an error is returned,
//!     this value is not modified.
//! \param[in] pBuffer Pointer to buffer to use for sector reads.
//!
//! \retval SUCCESS The FCB was found and read.
//! \retval ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT  GPMI DMA timed out during read.
//! \retval ERROR_ROM_NAND_DRIVER_NO_BCB The FCB was not found.
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_FindFCB(uint32_t u32CurrentNAND, uint32_t * pReadSector, uint8_t * pBuffer)
{
    RtStatus_t retStatus=SUCCESS;
    NAND_Timing_t zNANDTiming;
    uint32_t iReadSector = 0;
    BootBlockStruct_t * pFCB=NULL;
    NAND_Descriptor_t * pNANDDescriptor = &(rom_nand_pCtx->zNANDDescriptor);
    SerializerState_t * pState = &rom_nand_pCtx->zSerializerState;
    bool bFoundBootBlock;
    int i;

    // search for FCB
    
    // disable ECC check for FCB block
    rom_nand_pCtx->bEnableHWECC = false;

    // find the boot block
    bFoundBootBlock = false;
    for(i=0;((i<rom_nand_pCtx->zNandEFuse.efNANDBootSearchLimit) && 
             (retStatus == SUCCESS || retStatus == ERROR_ROM_NAND_ECC_FAILED) &&  // If ECC error, let's try again.
             !bFoundBootBlock);i++)
    {
        // We skip NAND_SEARCH_STRIDE pages between read attempts.
        iReadSector = (i * rom_nand_pCtx->zNandEFuse.efNANDBootSearchStride) + *pReadSector;

        DBG_PRINTF("%t @ Read Start\n");
        // look through these blocks, looking for the NAND Control Block. And wait for the DMA to complete.
        g_pNand->HalReadPage(u32CurrentNAND, iReadSector, pBuffer);
        retStatus = g_pNand->HalWaitForReadComplete(u32CurrentNAND, 
                                                    rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);
        DBG_PRINTF("%t @ Read Complete\n");
        
        if(retStatus == SUCCESS)
        {
            // check software ECC, VerifyFCB will return correct copy of FCB in pFCB pointer
            retStatus = g_pNand->VerifyFCB(pBuffer, (void**)&pFCB);

            if( retStatus == SUCCESS )
            {
                uint32_t crc = g_pNand->GetBCBChecksum(((uint8_t *)pFCB)+4, 512-4);
                // Match signatures to determine if this was successful.
                if (crc == pFCB->m_u32Checksum &&
                    pFCB->m_u32FingerPrint == zFCBFingerPrints.m_u32FingerPrint &&  
                    pFCB->m_u32Version == zFCBFingerPrints.m_u32Version)
                {
                    bFoundBootBlock = true;

                    // MICHAL - save this data in global variable
                    m_u32UpdateStatus = pFCB->TM41_Block.m_u32UpdateStatus;
    		    DBG_PRINTF("MICHAL: update status = %d\n", m_u32UpdateStatus);
    		    return SUCCESS;
                }
            }
        }
    }
    
    // Remember where we stopped.
    *pReadSector = iReadSector;
    rom_nand_pCtx->bEnableHWECC = true;

    DBG_PRINTF("FCB Search Result = 0x%X\n", retStatus);

    if(!bFoundBootBlock)
    {
        if(retStatus == SUCCESS)
        {
            //didn't find a Boot block within the desired search area.
            retStatus = ERROR_ROM_NAND_DRIVER_NO_BCB;
        }
        return retStatus;
    }

    // check for valid ecctype in FCB
    if( pFCB->FCB_Block.m_u32EccBlockNEccType >= BCH_Ecc_Max_Levels )
    {
        // Invalid ECC specified in FCB, can't load the image.
        retStatus = ERROR_ROM_NAND_DRIVER_FCB_INVALID_ECC;
        DBG_PRINTF("--->FCB has invalid/unsupported ECC: %d, Error: 0x%X.\n", 
            pFCB->FCB_Block.m_u32EccBlockNEccType, retStatus);
        return (retStatus);
    }

    DBG_PRINTF("--->FCB found on NAND %d at sector %d.\n", u32CurrentNAND, *pReadSector);

    // Set ECC first from the lookup
    rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel = mapToEccLevel[pFCB->FCB_Block.m_u32EccBlockNEccType];

    DBG_PRINTF("ECC Size %d\n", rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);

    // initialize zNANDEccParams with values from FCB
    rom_nand_pCtx->zNANDEccParams.m_u32EccBlock0Size = pFCB->FCB_Block.m_u32EccBlock0Size;
    rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNSize = pFCB->FCB_Block.m_u32EccBlockNSize;
    rom_nand_pCtx->zNANDEccParams.m_u32EccBlock0EccLevel = mapToEccLevel[pFCB->FCB_Block.m_u32EccBlock0EccType];
    rom_nand_pCtx->zNANDEccParams.m_u32NumEccBlocksPerPage = pFCB->FCB_Block.m_u32NumEccBlocksPerPage;
    rom_nand_pCtx->zNANDEccParams.m_u32MetadataBytes = pFCB->FCB_Block.m_u32MetadataBytes;
    rom_nand_pCtx->zNANDEccParams.m_u32PageSize = pFCB->FCB_Block.m_u32TotalPageSize;
    rom_nand_pCtx->zNANDEccParams.m_u32EraseThreshold = pFCB->FCB_Block.m_u32EraseThreshold;

    rom_nand_hal_UpdateECCParams(&rom_nand_pCtx->zNANDEccParams, rom_nand_pCtx->zNandEFuse.efNANDsInUse);

    // load timing parameters
    zNANDTiming = pFCB->FCB_Block.m_NANDTiming;
    g_pNand->HalUpdateGPMITiming(&zNANDTiming, 0);

    // Load the parameters from the NAND page.
    rom_nand_pCtx->zSerializerState.m_uSectorsPerBlock = 
        pFCB->FCB_Block.m_u32SectorsPerBlock;
    rom_nand_pCtx->zSerializerState.m_u32PageDataSize = 
        pFCB->FCB_Block.m_u32DataPageSize;

    pNANDDescriptor->stc_NANDSectorDescriptor.u32TotalPageSize = 
        pFCB->FCB_Block.m_u32TotalPageSize;        
    // chunksPerRead should be set to the bytes we will load, exactly ((b0+(bn*NumBlks))/16).
    rom_nand_pCtx->chunksPerRead = (rom_nand_pCtx->zNANDEccParams.m_u32EccBlock0Size + 
                                (rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNSize * 
                                 rom_nand_pCtx->zNANDEccParams.m_u32NumEccBlocksPerPage)) / BYTES_PER_CHUNK;
        
    //! /note Keep the boot blocks on page boundaries even for 4k pages.
    // Instead of saving the Page Mask and Page Shift here, continue to 
    // load the ROM assuming a page shift of 0 because the Boot Blocks
    // assume they are placed on a page boundary.  We'll store these 
    // parameters in an unused variable and reinstate them after the
    // boot blocks are found and before the firmware gets loaded.
    //rom_nand_pCtx->zSerializerState.m_uCurrentSectorInBlock = 0;
        //((pFCB->FCB_Block.m_u32SectorToPageShift << 16) | 
        //(pFCB->FCB_Block.m_u32SectorInPageMask));   
    // Force to always read a page at a time by setting shift and
    // mask to zero.

    DBG_PRINTF("Total Page Size = %d\n", 
           pNANDDescriptor->stc_NANDSectorDescriptor.u32TotalPageSize);

    rom_nand_pCtx->bBootPatch = (pFCB->FCB_Block.m_u32BootPatch == 1) ? true : false;

    pState->m_uSectorsRead = 0;
    
    // Depending upon whether we're booting from the primary boot blocks or the secondary
    // boot blocks, we may need to start at different places.  The Primary boot block
    // will always be used unless an error occurs.
    pState->m_uCurrentNand = 0;
    if (!rom_nand_pCtx->zSerializerState.m_u32UseSecondaryBoot)
    {
        pState->m_uCurrentSector = pFCB->FCB_Block.m_u32Firmware1_startingSector;
        pState->m_uSectorsToRead = pFCB->FCB_Block.m_u32SectorsInFirmware1;
    }
    else
    {
        pState->m_uCurrentSector = pFCB->FCB_Block.m_u32Firmware2_startingSector;
        pState->m_uSectorsToRead = pFCB->FCB_Block.m_u32SectorsInFirmware2;
    }
    
    pState->m_uCurrentBlock = (pState->m_uCurrentSector / pState->m_uSectorsPerBlock);
    pState->m_u32BBAreaStartPage = pFCB->FCB_Block.m_u32DBBTSearchAreaStartAddress;
    pState->m_u32BBMarkByteOffsetInPageData = pFCB->FCB_Block.m_u32BadBlockMarkerByte;
    pState->m_u32BBMarkBitOffset = pFCB->FCB_Block.m_u32BadBlockMarkerStartBit;
    pState->m_u32BBMarkerPhysicalOffset = pFCB->FCB_Block.m_u32BBMarkerPhysicalOffset;

#if 0
    if( pFCB->FCB_Block.m_u32TotalPageSize > TYPICAL_NAND_TOTAL_PAGE_SIZE ) // > 2112
        BW_SIMGPMISEL_CHANn_CFG_DEV_CODE(u32CurrentNAND, BV_SIMGPMISEL_CHANn_CFG_DEV_CODE__NAND_8_4M_4k);
    else
        BW_SIMGPMISEL_CHANn_CFG_DEV_CODE(u32CurrentNAND, BV_SIMGPMISEL_CHANn_CFG_DEV_CODE__NAND_8_4M_2k);
    
    //DBG_PRINTF(" GPMI--> Delay for SIMGPMISEL...\n");
    i = ROM_GET_TIME_USEC();
    while(!ROM_TIME_USEC_EXPIRED(i, 500));
#endif
     return retStatus;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Loads the bad block table from the NAND.
//!
//! Initiates a read from the sector listed in the FCB as the discovered bad
//! block table search area sector. Once the DMA finishes, the contents of the 
//! table are saved into the NAND boot driver context.
//!
//! \param[in] u32CurrentNAND Number of the NAND to read from.
//! \param[in] pBuffer Pointer to buffer to use for sector reads.
//!
//! \retval SUCCESS The bad block table was read successfully.
//! \retval ERROR_ROM_NAND_DRIVER_NO_DBBT No bad block table was found.
//!
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_ReadBadBlockTable(uint32_t u32CurrentNAND, 
                                      uint32_t * pReadSector, 
                                      uint8_t * pBuffer)
{
    RtStatus_t retStatus = SUCCESS;  

    // Now search for the DBBT.
    // MICHAL added +10 bytes offset?
    retStatus = g_pNand->BootBlockSearch(u32CurrentNAND, &zDBBTFingerPrints, pReadSector, pBuffer);

    // if successful, we'll want to load the starting sector of boot image.
    if(retStatus != SUCCESS)//lets kick off the first one
    {
        retStatus = ERROR_ROM_NAND_DRIVER_NO_DBBT;
    } 
    else
    {
        int i,j,NumSectors;
        uint32_t u32BBSector;
        BootBlockStruct_t * pBBHeader;
        BadBlockTableNand_t * pTable = (BadBlockTableNand_t *)pBuffer;

        // Now use the FCB to load important variables for finding the Bad Block tables for this NAND.
        pBBHeader = (BootBlockStruct_t *)pBuffer;        

        DBG_PRINTF("--->DBBT found on NAND %d at sector %d.\n", u32CurrentNAND, *pReadSector);

        // Only need to read the table for the NAND we're trying to boot from.
        // The BB header will always be 8K.  8K/2K = 4 sectors to jump.
        // NAND0 starts right after header.
        u32BBSector = *pReadSector + BB_HEADER_SIZE_IN_2K;   
        NumSectors = pBBHeader->DBBT_Block.m_u32Number2KPagesBB;

        // Reset the number of bad blocks.
        rom_nand_pCtx->zSerializerState.m_u32NumberOfBadBlocks = 0;   

        // Cycle through all the pages of bad blocks that this NAND has.
        // Hopefully just 1.
        for (j=0; j<NumSectors;j++)
        {

            // Now load the Bad Block Table and Wait for the DMA to complete.
            //look through these blocks, looking for the NAND Control Block.
            // Increment BBSector in anticipation of next read.
            g_pNand->HalReadPage(u32CurrentNAND, u32BBSector++, pBuffer);
            retStatus = g_pNand->HalWaitForReadComplete(u32CurrentNAND, 
                                                        rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);             
    
            // Fill in the Bad Block Table.
            DBG_PRINTF("NAND%d  BB -> ", u32CurrentNAND);
            // Now fill in the bad block table.  I'll only fill in blocks up to Max BB Table Size.
            for (i = 0; i < pTable->uNumberBB; i++)
            {
                // Fixed http://jira/browse/ROM-30

                rom_nand_pCtx->u16BadBlockTable[rom_nand_pCtx->zSerializerState.m_u32NumberOfBadBlocks++] = 
                        (uint16_t)pTable->u32BadBlock[i];
                DBG_PRINTF("%d ", pTable->u32BadBlock[i]);

                // This should never happen, but just in case, there are multiple Bad Blocks
                // with the same number, I can't risk overwriting the array.
                if (rom_nand_pCtx->zSerializerState.m_u32NumberOfBadBlocks >= MAX_BB_TABLE_SIZE)
                {
                    break;
                }
            }        
            DBG_PRINTF("\n Total BB = %d", rom_nand_pCtx->zSerializerState.m_u32NumberOfBadBlocks);            
        }
        
    }    
    
    return retStatus;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Find Boot Control Blocks to initialize NAND Serializer structure.
//!
//! This function searches for the Firmware Control Block (FCB) which contains 
//! NAND Timing and Control values, Discovered Bad Block Table Search Area 
//! address and Primary & Secondary Firmware sector location.  
//!
//! \param[in]	 pBuffer Use this buffer for reading sectors during discovery.
//!
//! \retval    0 (SUCCESS)    If no error has occurred.
//! \retval    ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT  GPMI DMA timed out during read.
//! \retval    ERROR_ROM_NAND_DRIVER_NO_BCB  Couldn't find Boot Control Block.
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_FindBootControlBlocks(uint8_t * pBuffer)
{
    RtStatus_t retStatus;
    uint32_t u32Temp = 0, iReadSector = 0;
    uint32_t u32CurrentNAND = NAND0;    // Assume we'll boot from primary. 

    // Initialize rom_nand_pCtx->zSerializerState.m_u32UseSecondaryBoot
    u32Temp = 0; // Default to primary
    if (!rom_nand_pCtx->zNandEFuse.efDisableSecondaryBoot)
    {
        // Grab the persistent Data
        retStatus = load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32Temp);

        // This bit will be 1 if an error occurred during first boot and restarted.
        u32Temp = ((u32Temp & BM_PERSIST1_ROM_REDUNDANT_BOOT) >> BP_PERSIST1_ROM_REDUNDANT_BOOT);

        // I don't think this test is needed, but this is too critical to ignore.
        // If an error occurs, assume primary boot blocks.
        if (retStatus) u32Temp = 0;
    }
    DBG_PRINTF("%s boot\n", u32Temp ? "Secondary" : "Primary");

    rom_nand_pCtx->zSerializerState.m_u32UseSecondaryBoot = u32Temp;

    // Now we want to start the search for the FCB.  
    // search for FCB
    retStatus = g_pNand->FindFCB(u32CurrentNAND, &iReadSector, pBuffer);
    if (retStatus != SUCCESS)
    {
        DBG_PRINTF("Failed to find FCB\n");
        return ERROR_ROM_NAND_DRIVER_NO_FCB;
    }

    // Now search for the Discovered Bad Block Table (DBBT).
    // Start the search where the FCB tells us it should be.
    iReadSector = rom_nand_pCtx->zSerializerState.m_u32BBAreaStartPage;            

    // Most probably Bad block table will be missing for patch boot.
    if (iReadSector > 0)
    {
        // search for first DBBT.
        retStatus = g_pNand->ReadBadBlockTable(u32CurrentNAND, &iReadSector, pBuffer);

        // If we failed while searching, we'll assume there are no bad blocks.
        if (retStatus != SUCCESS)
        {
            DBG_PRINTF("Failed to find DBBT\n");
            rom_nand_pCtx->zSerializerState.m_u32NumberOfBadBlocks = 0;
            // Force return code to success.
            retStatus = SUCCESS;
        } 
    }

    return retStatus;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Asynchronously checks the DMA status.
//!
//! This function asks the NAND HAL layer for the current DMA status. If a
//! DMA is in flight, the return code #ERROR_ROM_NAND_DMA_BUSY is
//! returned immediately. When the DMA is finished, the ECC is checked on
//! the sector that was just read. If the ECC threshold is met, then
//! a persistent bit is set to inform the SDK that the boot image is
//! on the verge of being corrupted and the return code is then changed
//! to a SUCCESS.  If the ECC threshold is exceeded, we set a persistent
//! bit to indicate that we need to boot from our secondary blocks and we
//! reset the digital portion of the chip.  And finally, the current sector
//! statistics are updated.
//!
//! \retval    SUCCESS    If no error has occurred.
//! \retval    ERROR_ROM_NAND_DMA_BUSY DMA is still in flight.
//! \retval    ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT  GPMI DMA timed out during read.
//! \retval    ERROR_ROM_NAND_ECC_FAILED  ECC Failed to correct data.
//! \retval    ERROR_ROM_NAND_ECC_THRESHOLD  ECC Threshold was reached.
//!
//! \note This function should not be called by other plugins because the
//!       tracking for the current sector  (serializer) is handled here.  
//! \todo In the future this tracking should be moved into rom_nand_BootNext.
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_WaitOnRead(void)
{
    RtStatus_t retStatus;
        
    retStatus = g_pNand->HalDmaStatus(rom_nand_pCtx->zSerializerState.m_uCurrentNand);
    if (retStatus == ERROR_ROM_NAND_DMA_BUSY)
    {
        return retStatus;
    }

    // Check the ECC.
    if (retStatus == SUCCESS)
    {
        // Check for ECC here then set persistent bit and reset
        // if the ECC is uncorrectable.  Reset the device and we should start using the
        // secondary blocks.
        // Check the ECC Status to determine if this is a valid read.
        if( rom_nand_pCtx->bEnableHWECC )
        {
            retStatus = g_pNand->HalECCStatus(rom_nand_pCtx->zSerializerState.m_uCurrentNand, rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);

            if (retStatus == ERROR_ROM_NAND_ECC_THRESHOLD)
            {            
                //#warning Set SDK_Rewrite persistent bit here - primary boot block needs fixin'
                // Set persistent bit to indicate to the SDK that a rewrite is needed.
                // Still readable, so return SUCCESS.
                write_persistent_word(HW_RTC_ROM_PERSISTENT_BITS1_SET, BM_PERSIST1_NAND_SDK_BLOCK_REWRITE);
    #ifdef TGT_SIM
                {  
                uint32_t u32PersistentData;
                load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32PersistentData);
                DBG_PRINTF("SDK Rewrite\n");
                }
    #endif
                retStatus = SUCCESS;
            } 
            else if (retStatus == ERROR_ROM_NAND_ECC_FAILED)
            {
                uint32_t u32PersistentData;

                DBG_PRINTF("Unrecoverable Error during read.  Resetting/Powerdown Chip\n");

                // Fixed http://jira/browse/ROM-18
                // read the persistent bit, if it is already set for nand secondary boot then shut it down
                load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32PersistentData);
                if(u32PersistentData & BM_PERSIST1_ROM_REDUNDANT_BOOT)
                {
                    ROM_CHIP_POWER_DOWN();
                }
                else
                {
                    //#warning Set BootFromSecondary persistent bit here then reset the chip.
                    write_persistent_word(HW_RTC_ROM_PERSISTENT_BITS1_SET, BM_PERSIST1_ROM_REDUNDANT_BOOT);
        #ifdef TGT_SIM
                    load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32PersistentData);
        #endif
                    ROM_CHIP_RESET();
                }
            }
        }
    }

    DBG_PRINTF("%t @ Read Complete\n");

    // Figure out what happened.
    if (retStatus == ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT)
    {
        // Backup the current sector pointers for a retry.
        if(rom_nand_pCtx->zNANDDescriptor.bBA_NAND)
            rom_nand_pCtx->zSerializerState.m_uCurrentSector -= rom_nand_pCtx->zNANDDescriptor.u32NumLBAPerRead;
        else
            rom_nand_pCtx->zSerializerState.m_uCurrentSector --;
    }
    else if (retStatus == SUCCESS)
    {        
        // If success, keep going.
        if(rom_nand_pCtx->zNANDDescriptor.bBA_NAND)
        {
            rom_nand_pCtx->zSerializerState.m_uCurrentSector += rom_nand_pCtx->zNANDDescriptor.u32NumLBAPerRead;
            rom_nand_pCtx->zSerializerState.m_uSectorsToRead -= rom_nand_pCtx->zNANDDescriptor.u32NumLBAPerRead;
            rom_nand_pCtx->zSerializerState.m_uSectorsRead += rom_nand_pCtx->zNANDDescriptor.u32NumLBAPerRead;
        }
        else
        {
            uint32_t  byte, bit;
            NAND_dma_read_t  *pDmaReadDescriptor = (NAND_dma_read_t *) &(rom_nand_pCtx->DmaReadDma); 
            NAND_read_seed_t *pDmaReadSeed = (NAND_read_seed_t *) &(pDmaReadDescriptor->NAND_DMA_Read_Seed);
            uint8_t *pData = (uint8_t *)pDmaReadSeed->pDataBuffer;
            uint8_t *pMetadata = (uint8_t *)pDmaReadSeed->pAuxBuffer;

            rom_nand_pCtx->zSerializerState.m_uCurrentSector ++;
            rom_nand_pCtx->zSerializerState.m_uSectorsToRead --;
            rom_nand_pCtx->zSerializerState.m_uSectorsRead ++;

            // do the BI swapping here
            
            byte = rom_nand_pCtx->zSerializerState.m_u32BBMarkByteOffsetInPageData;
            bit = rom_nand_pCtx->zSerializerState.m_u32BBMarkBitOffset; 	
            if(rom_nand_pCtx->zNandEFuse.efBIPreserve)
            {
          
               // bad block reserve for swaping 1 bytes  

                *(pData+byte) = (*pMetadata<<bit)|(*(pData+byte)&(0xFF>>(8-bit))) ;
                *(pData+1+byte) = (*pMetadata>>(8-bit))|(*(pData+byte+1)&(0xFF<<bit));	   
            }
        }
    }   

    return retStatus;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Kick off next sector read.
//!
//! The next sector to read is updated by rom_nand_WaitOnRead() when it
//! completes successfully. If the next sector should come from the next block,
//! this function finds the next good block by using rom_nand_FindNextGoodBlock().
//! Then the NAND HAL layer is asked start a DMA to read the sector into
//! \a pNewBuffer.
//!
//! \param[out]	 pNewBuffer New buffer available from read. Upon a successful
//!     return the buffer is owned by DMA. It should not be touched until
//!     rom_nand_WaitOnRead() is called and returns.
//!
//! \retval    SUCCESS    If no error has occurred.
//! \retval    ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT  GPMI DMA timed out during read.
//! \retval ERROR_ROM_NAND_LOAD_COMPLETED The end of the firmware was reached
//!     but there were still sectors remaining to skip.
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_StartSectorRead(uint8_t * pNewBuffer)
{    
    RtStatus_t retCode;

    SerializerState_t * pState = &rom_nand_pCtx->zSerializerState;
    
    // For non Block Abstracted NANDs only
    // Have we finished reading an entire block?   Need to determine which next block to 
    // read - is it on another NAND (same block) or is it on NAND0?
    // We don't need to clear the current sector because that is done in FindNextGoodBlock.
    if(!(rom_nand_pCtx->zNANDDescriptor.bBA_NAND) &&
        ((rom_nand_pCtx->zSerializerState.m_uCurrentSector & 
        (pState->m_uSectorsPerBlock - 1)) == 0))
    {
        g_pNand->FindNextGoodBlock(pNewBuffer);
    }

    {            
        DBG_PRINTF("Reading Sector %d on NAND %d\n",
               rom_nand_pCtx->zSerializerState.m_uCurrentSector, 
               rom_nand_pCtx->zSerializerState.m_uCurrentNand);            
    }      

    DBG_PRINTF("%t @ Read Start\n");

    // Initiate read data. 
    if (rom_nand_pCtx->zNANDDescriptor.bBA_NAND)
    {
        retCode = g_pNand->BANandHalSendReadCmd(pState->m_uCurrentNand, 1, pState->m_uCurrentSector, pNewBuffer);
    }
    else
    {
        retCode = g_pNand->HalReadPage(pState->m_uCurrentNand, pState->m_uCurrentSector, pNewBuffer);
    }
    if (retCode == SUCCESS && rom_nand_pCtx->zNANDDescriptor.bBA_NAND)
    {
        // m_u32SectorDataRead is used to keep track of bytes read from a sector, sector size may be greater than 2k and ,
        // If read is success for BA NAND then increment m_u32SectorDataRead with 2048
        rom_nand_pCtx->zSerializerState.m_u32SectorDataRead += rom_nand_pCtx->zSerializerState.m_u32PageDataSize;
    }

    return retCode;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Skip a number of sectors.
//!
//! \param[in] uSectorCount Number of sectors to skip.
//! \param[in] pu8Buf, buffer to dma buffer
//!
//! \retval SUCCESS No error has occurred.
//! \retval ERROR_ROM_NAND_LOAD_COMPLETED The end of the firmware was reached
//!     but there were still sectors remaining to skip.
//!
//! \todo Do we need to make sure no DMA is in flight before skipping?
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_SkipSectors(unsigned uSectorCount, uint8_t* pu8Buf)
{
    SerializerState_t * pState = &rom_nand_pCtx->zSerializerState;
    DBG_PRINTF("SkipSectors %d\n", uSectorCount);
    while (uSectorCount--)
    {
        if(rom_nand_pCtx->zNANDDescriptor.bBA_NAND)
        {
            pState->m_uCurrentSector += rom_nand_pCtx->zNANDDescriptor.u32NumLBAPerRead;
            pState->m_uSectorsToRead -= rom_nand_pCtx->zNANDDescriptor.u32NumLBAPerRead;
            pState->m_uSectorsRead += rom_nand_pCtx->zNANDDescriptor.u32NumLBAPerRead;
        }
        else
        {
            pState->m_uCurrentSector ++;
            pState->m_uSectorsToRead --;
            pState->m_uSectorsRead ++;
        }

        // Check to see if we're on a block boundary for non Block Abstracted
        // NANDs only.
        if(!(rom_nand_pCtx->zNANDDescriptor.bBA_NAND) &&
            ((rom_nand_pCtx->zSerializerState.m_uCurrentSector & 
             (pState->m_uSectorsPerBlock - 1)) == 0))
        {
            g_pNand->FindNextGoodBlock(pu8Buf);
        }
    }
    
    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Looks for the next good block.
//!
//! Repeatedly increments the current block number and scans the bad block
//! table for that block. If the new block number is not found in the table,
//! that means it is a good block (or at least not yet known to be bad) and
//! the search loop is exited. Once a good block is found, the current sector
//! in the NAND is recalculated and the current sector in the block is
//! set to 0.
//!
//! \param[in] pu8Buf, pointer to dma buffer
//!
//! \retval SUCCESS No error has occurred.
//!
//! \todo Break out of the bad block search loop when we reach the end of
//!     the NAND!
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_FindNextGoodBlock(uint8_t* pu8Buf)
{
    SerializerState_t * pState = &rom_nand_pCtx->zSerializerState;

    // Check to see if the next block is in the Discovered Bad Block Table.
    bool bCurrentBlockBad = true;

    // Calculate the current Block.
    pState->m_uCurrentBlock = pState->m_uCurrentSector / pState->m_uSectorsPerBlock;

    DBG_PRINTF("FindNextGoodBlock\n");

    do
    {
        if (rom_nand_pCtx->zSerializerState.m_u32BBAreaStartPage == 0)
        {
            // No bad block table present, read mfgr bad block marker byte to figure out
            bCurrentBlockBad = g_pNand->HalIsBlockBad(pState->m_uCurrentBlock, pu8Buf); 
        }
        else
        {
            bCurrentBlockBad = g_pNand->BlockIsInBadBlockTable(pState->m_uCurrentBlock); 
        }

        if (bCurrentBlockBad)
        {
            DBG_PRINTF("Skipping Bad Block %d\n", pState->m_uCurrentBlock);
            pState->m_uCurrentBlock++;
        } 
    }
    while (bCurrentBlockBad);

    // Calculate the sector we'll want to read.
    pState->m_uCurrentSector = pState->m_uCurrentBlock * pState->m_uSectorsPerBlock;

    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Implements BCB checksum algorithm,
//!
//! \param[in] pointer to BCB buffer.
//! \param[in] size of buffer
//!
//! \retval returns calculated checksum on pBuffer of u32Size.
//!
////////////////////////////////////////////////////////////////////////////////
uint32_t rom_nand_GetBCBChecksum(void * pBuffer, int u32Size)
{
    uint8_t *pu8Buf = (uint8_t *)pBuffer;
    uint32_t crc=0;
    int i;
    
    for(i=0;i<u32Size;i++)
    {
        crc += pu8Buf[i];
    }
    return (crc ^ 0xFFFFFFFF);
}
// eof rom_nand_api.c
//! @}
