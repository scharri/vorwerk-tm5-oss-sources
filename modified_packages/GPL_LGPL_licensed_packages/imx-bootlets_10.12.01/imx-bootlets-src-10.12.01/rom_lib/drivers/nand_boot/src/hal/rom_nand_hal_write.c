////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_hal
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file nand_hal_write.c
//! \brief Write Functions in the NAND HAL layer.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include "rom_types.h"
#include "regs.h"
#include "regspinctrl.h"
#include "return_codes.h"
#include "itf_rom.h"
#include "rom_nand_api.h"
#include "rom_nand_hal_api.h"
#include "rom_nand_hal_structs.h"
#include "rom_nand_internal.h"
#include "rom_nand_hal_gpmi.h"
#include <string.h>
#include "efuse.h"
#if BRAZO_ROM_FLASH
#include "regsbrazoio.h"
#endif
#include "regsbch.h"
////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Definitions
//
// These values are used to initialize the GPMI timing registers.
////////////////////////////////////////////////////////////////////////////////

// Timeout uses the microsecond register so this should be 12msec.
#define MAX_TRANSACTION_TIMEOUT             12000

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

RtStatus_t rom_nand_hal_Read_Status(uint32_t iDeviceNum, uint32_t * pStatusResult);
////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////
uint8_t g_u8NANDAuxBufferType2[600];

uint32_t g_uiLastSector;

extern rom_nand_Context_t *rom_nand_pCtx;

NAND_dma_program_t  NAND_dma_program_page;
NAND_dma_block_erase_t  NAND_dma_erase_block;
NAND_dma_read_status_t  dma_read_status;
////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

#ifdef WRITES_ALLOWED
 

/////////////////////////////////////////////////////////////////////////////////
//! Name: rom_nand_hal_WriteSectorData
//!
//!  Type: Function
//!
//!  Description: 
//!
//!  Inputs: 
//!
//!  Outputs: 
//!
//!  Notes:  Need to verify that four 512 sectors are always sent
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_WriteSectorData(uint32_t u32NandDeviceNumber, 
                                        uint32_t u32PageNum, 
                                        uint8_t *p8PageBuf)
{

    uint32_t i, offsetbyte, offsetbit, eFuseValue, BIPreserved;    
    RtStatus_t rtCode;
    uint32_t u32EccSize = rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel;


    // Create a pointer to this device.
    NAND_Descriptor_t * pNANDDevice = &(rom_nand_pCtx->zNANDDescriptor);

    // Set all the redundant area to 0xFF.
    memset(g_u8NANDAuxBufferType2, 0xFF, sizeof(g_u8NANDAuxBufferType2));

    // TT_Fixme - I need to add the ECC calculation in here.
    // Set address size to # row bytes + 2 (for column bytes).
    NAND_dma_program_page.NandProgSeed.NandSizeVars.uiAddressSize = pNANDDevice->btNumRowBytes+pNANDDevice->btNumColBytes;
    
    //NAND_dma_program_page.uiWriteSize = 
    //    pNANDDevice->stc_NANDSectorDescriptor.u32PageDataSize;
    NAND_dma_program_page.NandProgSeed.NandSizeVars.uiWriteSize = 
        pNANDDevice->stc_NANDSectorDescriptor.u32TotalPageSize;
    
    // set the Word size
    // normally use 8 bit data width
    NAND_dma_program_page.NandProgSeed.NandSizeVars.uiWordSize = (pNANDDevice->btNANDDataBusWidth == 16) ?
        BV_GPMI_CTRL0_WORD_LENGTH__16_BIT : BV_GPMI_CTRL0_WORD_LENGTH__8_BIT;    
    
    // Always 2 bytes and we're always going to be column 0.
    NAND_dma_program_page.NandProgSeed.bType2Columns[0] = (reg8_t)0;
    NAND_dma_program_page.NandProgSeed.bType2Columns[1] = (reg8_t)0;

    // Fill in the Row Address. (Can be 2 or 3 bytes)
    for (i=0;i<pNANDDevice->btNumRowBytes;i++)
    {
        NAND_dma_program_page.NandProgSeed.bType2Rows[i] = 
            (reg8_t)((u32PageNum>>(8*i)) & 0xFF);
    }

    // Load Command Code for Serial Data Input (0x80)
    NAND_dma_program_page.NandProgSeed.tx_cle1 = 
        pNANDDevice->stc_NANDDeviceCommandCodes.btSerialDataInputCode;

    // Load Command Code for Page Program (0x10)
    NAND_dma_program_page.NandProgSeed.tx_cle2 = 
        pNANDDevice->stc_NANDDeviceCommandCodes.btPageProgramCode;

    // Load Command Code for Read Status (0x70)
    NAND_dma_program_page.NandProgSeed.u8StatusCmd = 
        pNANDDevice->stc_NANDDeviceCommandCodes.btReadStatusCode;

    // set bEnableHWECC of progseed
    NAND_dma_program_page.NandProgSeed.bEnableHWECC = rom_nand_pCtx->bEnableHWECC;

    offsetbyte = rom_nand_pCtx->zSerializerState.m_u32BBMarkByteOffsetInPageData;
    offsetbit = 	  rom_nand_pCtx->zSerializerState.m_u32BBMarkBitOffset;


    eFuseValue = HW_OCOTP_ROMn_RD(4);
    BIPreserved = (eFuseValue&BM_OCOTP_ROM4_NAND_BADBLOCK_MARKER_PRESERVE_DISABLE) >> BP_OCOTP_ROM4_NAND_BADBLOCK_MARKER_PRESERVE_DISABLE; 
    BIPreserved ^= 0x1;
	
    if(BIPreserved)
    {
        if(rom_nand_pCtx->bEnableHWECC)
        {
           // pState->m_uCurrentSector is the last sector of page
           /* bad block reserve for swaping 2 bytes */
            *((uint8_t* )g_u8NANDAuxBufferType2)=(*((uint8_t* )p8PageBuf+ offsetbyte)>>offsetbit)| (*((uint8_t* )p8PageBuf+ offsetbyte+1)<<(8-offsetbit));
       
            *((uint8_t* )p8PageBuf+ offsetbyte)=*((uint8_t* )p8PageBuf+ offsetbyte) |(0xff<<offsetbit);
            *((uint8_t* )p8PageBuf+ offsetbyte +1 )=*((uint8_t* )p8PageBuf+ offsetbyte +1) |(0xff>>(8-offsetbit));
        }
        else
        {
            // the whole page is expected to be in main buffer and aux buffer is not used.
            p8PageBuf[rom_nand_pCtx->zSerializerState.m_u32BBMarkerPhysicalOffset]=0xFF;
        }
    }

    // initialize zNANDEccParams data structure of NandProgSeed.
    rom_nand_hal_SetFlashLayout(u32NandDeviceNumber, &(rom_nand_pCtx->zNANDEccParams));
    // Build the DMA that will program this sector.
    rom_nand_hal_BuildProgramDma(&NAND_dma_program_page, u32NandDeviceNumber, 
                                NAND_dma_program_page.NandProgSeed.NandSizeVars.uiAddressSize, 
                                pNANDDevice->stc_NANDSectorDescriptor.u32TotalPageSize,
                                &rom_nand_pCtx->zNANDEccParams, p8PageBuf, g_u8NANDAuxBufferType2);

    rom_nand_hal_StartDma((dma_cmd_t *)&NAND_dma_program_page, u32NandDeviceNumber);

    rtCode = rom_nand_hal_WaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber);

    // Check the status of the write.
    // Check the Pass Fail bit (bit 0) and the inverted Write Protect bit (bit 7)
    if ((NAND_dma_program_page.NandProgSeed.u16Status & 0x81) ^ 0x80)
    {
        rtCode = ERROR_ROM_NAND_DRIVER_PROGRAM_FAILED;
        // Writes are not normally included in 
    }

    return(rtCode);    // Success or failure?

}

/////////////////////////////////////////////////////////////////////////////////
//! Name: rom_nand_hal_Erase_Block
//!
//!  Type: Function
//!
//!  Description: Erase the given block in the given Device.
//!
//!  Inputs: 
//!
//!  Outputs: 
//!
//!  Notes: 
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_Erase_Block(uint32_t u32NandDeviceNumber, 
                                    uint32_t u32BlockNum)
{

    uint32_t i;
    uint32_t u32RowAddress;    
    RtStatus_t rtCode;
    // Create a pointer to this device.
    //NAND_Descriptor_t * pNANDDevice = &zNANDDescriptor;
    NAND_Descriptor_t * pNANDDevice = &(rom_nand_pCtx->zNANDDescriptor);

    // Calculate the Row.
    u32RowAddress = 
        (u32BlockNum * 
         pNANDDevice->stc_NANDSectorDescriptor.u32PagesPerBlock);

    NAND_dma_erase_block.NandEraseSeed.uiBlockAddressBytes = 
        pNANDDevice->btNumRowBytes;

    // Fill in the Row Address. (Can be 2 or 3 bytes)
    for (i=0;i<pNANDDevice->btNumRowBytes;i++)
    {
        NAND_dma_erase_block.NandEraseSeed.tx_block[i] = 
            (reg8_t)((u32RowAddress>>(8*i)) & 0xFF);
    }

    // Load Command Code for Serial Data Input (0x80)
    NAND_dma_erase_block.NandEraseSeed.tx_cle1 = 
        pNANDDevice->stc_NANDDeviceCommandCodes.btBlockEraseCode;

    // Load Command Code for Page Program (0x10)
    NAND_dma_erase_block.NandEraseSeed.tx_cle2 = 
        pNANDDevice->stc_NANDDeviceCommandCodes.btBlockErase2Code;

    // Load Command Code for Read Status (0x70)
    NAND_dma_erase_block.NandEraseSeed.u8StatusCmd = 
        pNANDDevice->stc_NANDDeviceCommandCodes.btReadStatusCode;

    // Build the DMA that will program this sector.
    rom_nand_hal_BuildEraseDma(&NAND_dma_erase_block, pNANDDevice->btNumRowBytes, 
                               u32NandDeviceNumber);

    rom_nand_hal_StartDma((dma_cmd_t *)&NAND_dma_erase_block, u32NandDeviceNumber);

    if ((rtCode = rom_nand_hal_WaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber)) != SUCCESS)
        return(rtCode);    // Success or failure?

    // Check Status to ensure we successfully erased the block.
    // Check the Pass Fail bit (bit 0) and the inverted Write Protect bit (bit 7)
    if ((NAND_dma_erase_block.NandEraseSeed.u16Status & 0x81) ^ 0x80)
    {
        return ERROR_ROM_NAND_DRIVER_ERASE_FAILED;
    }

    return(rtCode);    // Success or failure?

}

/////////////////////////////////////////////////////////////////////////////////
//! Name: rom_nand_hal_Read_Status
//!
//!  Type: Function
//!
//!  Description: 
//!
//!  Inputs: 
//!
//!  Outputs: 
//!
//!  Notes: 
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_Read_Status(uint32_t u32NandDeviceNumber, uint32_t * pStatusResult)
{
    
    RtStatus_t  retCode;

    // Send one byte
    dma_read_status.uiReadStatusSize = 1;
    
    dma_read_status.tx_cle1 = 
        rom_nand_pCtx->zNANDDescriptor.stc_NANDDeviceCommandCodes.btReadStatusCode; 

    // Receive one byte back.
    dma_read_status.uiReadStatusResultSize = 1;

    rom_nand_hal_BuildReadStatusDma(&dma_read_status, u32NandDeviceNumber,
                                    &dma_read_status.rx_Result);

    rom_nand_hal_StartDma((dma_cmd_t *)&dma_read_status, u32NandDeviceNumber);

    retCode = rom_nand_hal_WaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber);

    *pStatusResult = (uint32_t)dma_read_status.rx_Result;
    
    return retCode;
}

#endif      //#ifdef WRITES_ALLOWED

//! @}
