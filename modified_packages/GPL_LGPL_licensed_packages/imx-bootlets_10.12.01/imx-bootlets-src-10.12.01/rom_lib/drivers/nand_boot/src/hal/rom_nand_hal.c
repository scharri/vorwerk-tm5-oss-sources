////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_hal
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_hal.c
//! \brief This file provides the top level functions for the ROM NAND HAL.
//!
//!
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include "rom_types.h"
#include "regspinctrl.h"
#include "return_codes.h"
#include "rom_nand_hal_structs.h"
#include "rom_nand_hal_api.h"
#include "rom_nand_hal_dma.h"
#include "rom_nand_internal.h"
#include "rom_nand_hal_gpmi.h"
//#include "rom_nand_status_codes.h"
#include <itf_rom.h>
#include "rom_utils.h"

#include "debug.h"
#include <string.h>

#include "regsbch.h"
#include "rom_ba_nand_hal.h"

#define BCH_PARITY_SIZE (13)

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////
extern rom_nand_Context_t *rom_nand_pCtx;


////////////////////////////////////////////////////////////////////////////////
// Definitions
//
// These values are used to initialize the GPMI timing registers.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////
//! \brief Default NAND descriptor.
//!
//! This structure is the default configuration for most NANDs.  This is generic
//! enough to load the first page of the FCB which will over-ride these values.
const NAND_Descriptor_t zNANDDescriptor = 
{
    // SectorDescriptor
    {
        2112,          // Total Page Size        
        0             // Sectors per page
//        0              // Number of bits to shift for Sectors per page.
#ifdef WRITES_ALLOWED
        ,64            // Number of pages per block
#endif
    },  
    // Device Command Codes
    {
        0x90,       // ReadIDCode
        0xFF,       // ResetCode
        0x00,       // Read1Code             
        0x30        // Read1_2ndCycleCode

#ifdef WRITES_ALLOWED
        ,0x70,      // ReadStatusCode
        0x80,       // PageProgramCode
        0x10,       // PageProgramCode
        0x60,       // BlockEraseCode
        0xD0        // BlockErase2Code
#endif
    },
    // Device Addressing.
    3,		        // Number of Row Address bytes required
    2,		        // Number of Col Address bytes required
    8,              // btNANDDataBusWidth
    256,            // Number of Total Blocks in NAND.
    1,              // Number of LBA addressed per page bytes
    false           // Is it BA_NAND, default false
};

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! \brief Initialize the NAND at the HAL layer.
//!
//! This function will initialize the low level GPMI hardware and the HAL
//! variables.
//!
//! \param[in]  pNandEFuse Nand boot related otp/efuse values.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//! \retval ERROR_ROM_NAND_DRIVER_NAND_INIT_FAILED Unable to initialize HAL.
//!
//! \post The GPMI has been configured and the NAND has been exercised.
//!
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_Init(void* param) 
{
    efNAND_t * pNandEFuse = (efNAND_t *)param;
    RtStatus_t Status;
    NAND_Descriptor_t * pNANDDevice; 

    // Copy the descriptor from ROM into the NAND RAM Context area.
    rom_nand_pCtx->zNANDDescriptor = zNANDDescriptor;

    pNANDDevice = (NAND_Descriptor_t *)&(rom_nand_pCtx->zNANDDescriptor);

    //memcpy(rom_nand_pCtx->zNANDDescriptor, &zNANDDescriptor, sizeof(NAND_Descriptor_t));

    // Enable the GPMI
    //! /TODO: Configure GPMI clock

// MICHAL - do NOT initialize HW twice, as that causes ROM loader to hang!
    ////Status = g_pNand->HalGpmiHWInit(pNandEFuse);
    Status = SUCCESS;

    // Fail if the GPMI is not initialized.
    if (Status != SUCCESS)
    {
        return Status;
    }

    // Update nand device parameters from efuses
    pNANDDevice->btNumRowBytes = pNandEFuse->efNANDRowAddressBytes;
    pNANDDevice->btNumColBytes = pNandEFuse->efNANDColumnAddressBytes;
    pNANDDevice->stc_NANDDeviceCommandCodes.btRead1Code = pNandEFuse->efNANDReadCmdCode1; 
    pNANDDevice->stc_NANDDeviceCommandCodes.btRead1_2ndCycleCode = pNandEFuse->efNANDReadCmdCode2; 

    // Discovering ba_nand 
    {
        // To begin with initialize bBA_NAND to false
        pNANDDevice->bBA_NAND = false;

        // Read parameter page to determine if it is an ONFi BA NAND
        Status = g_pNand->HalReadParameterPage(0, (uint8_t *)rom_nand_pCtx->bufA);
        if (Status == SUCCESS)
        {
            // Check to see if it is ONFi BA NAND
            BA_NAND_Parameter_Page_t *pPP = (BA_NAND_Parameter_Page_t *)rom_nand_pCtx->bufA;

            DBG_PRINTF("ReadParameterPage:: signature: 0x%x, features supported: 0x%x\n", pPP->u32ONFiSignature, pPP->u16FeaturesSupported);

            if (pPP->u32ONFiSignature == ONFI_SIGNATURE &&
#ifdef TGT_SIM
                pPP->u16FeaturesSupported.u16) // The micron's verilog model has issues and this is a workaround for it.
#else
                pPP->u16FeaturesSupported.u16FS.SupportsBlockAbstractedAccessMode)
#endif
            {
                // Yes it is ONFi BA

                // Update pointer in pNANDDevice to communicate from BA NAND.
                pNANDDevice->bBA_NAND = true;
                //pNANDDevice->stc_NANDSectorDescriptor.u16SectorInPageMask = 1; // Default, not used for BA NAND
                //pNANDDevice->stc_NANDSectorDescriptor.u16SectorToPageShift = 0; // Default, not used for BA NAND

                DBG_PRINTF("pPP->u16SectorSize: %d, pPP->u16SectorMultiple: %d\n", pPP->u16SectorSize, pPP->u16SectorMultiple);
    
#if defined(TGT_SIM) || defined(TGT_3700)
#ifdef TGT_3700
                if (pPP->u16SectorSize == 0) // For sims do not even check this condition<-micron's buggy verilog model
#endif
                {
                    // safe value, in case device failed to report sector size
                    // actual sector size is get by 2^pPP->u16SectorSize, here we default to 4096 bytes sector size
                    pPP->u16SectorSize = 12; 
                }
#endif
                pNANDDevice->stc_NANDSectorDescriptor.u16SectorSize = (1 << pPP->u16SectorSize); // Sector Size
                pNANDDevice->stc_NANDSectorDescriptor.u32TotalPageSize = 
                    pNANDDevice->stc_NANDSectorDescriptor.u16SectorSize; // Total bytes to read per read command.

                pNANDDevice->u32NumLBAPerRead = pNANDDevice->stc_NANDSectorDescriptor.u16SectorSize / 512; // Each LBA is 512 bytes apart 
            }
        }
        else
        {
            DBG_PRINTF("ReadParameterPage returned error: 0x%x\n", Status);
        }
    }

    // init DMA timeout
    rom_nand_pCtx->zNandDmaTime.uDMATimeout = MAX_TRANSACTION_TIMEOUT;

    DBG_PRINTF("NAND Init Success!\n");

    return(SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Initialize the NAND low level GPMI hardware
//!
//! This function will initialize the low level GPMI hardware and ECC hardware.
//!
//! \param[out]  pNandEFuse Number of expected NANDs - updated here.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//! \retval ERROR_ROM_NAND_DRIVER_NO_ECC_PRESENT  ECC is not supported.
//! \retval ERROR_ROM_NAND_DRIVER_NO_GPMI_PRESENT  GPMI is not supported.
//!
//! \post The GPMI has been configured and the NAND has been exercised.
//!
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_HWInit(void * param) 
{
    RtStatus_t Status;
    ReadIDCode zReadID;
    efNAND_t * pNandEFuse = (efNAND_t *) param;

    // Determine if the correct ECC size if available.
    Status = rom_nand_hal_CheckECCDecodeCapability(
                    rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);
    // Fail if the GPMI is not initialized.
    if (Status != SUCCESS)
    {
        return Status;
    }

    // If no eFuse has been blown, we'll default to 1 NAND.
    if (pNandEFuse->efNANDsInUse == 0)
    {
        pNandEFuse->efNANDsInUse = 1;
    }


    // Enable the GPMI
    //! /TODO: Configure GPMI clock
    Status = rom_nand_hal_EnableGPMI(false, pNandEFuse->efNANDsInUse, 
                                     //rom_nand_pCtx->zNandEFuse.efAltCEPinConfig,
                                     pNandEFuse->efEnableIntPullups,
                                     rom_nand_pCtx->u32Use1_8V);
    // Fail if the GPMI is not initialized.
    if (Status != SUCCESS)
    {
        return Status;
    }

    // If more than 2 NANDs, truncate.
    if (pNandEFuse->efNANDsInUse > MAX_NUMBER_NANDS)
    {
        pNandEFuse->efNANDsInUse = MAX_NUMBER_NANDS;
    }

    // Now reset the bch block.
    rom_nand_hal_ResetBCH();

    // Take the APBH out of reset.
    // APBH - disable reset, enable clock.
    HW_APBH_CTRL0_CLR( BM_APBH_CTRL0_SFTRST | BM_APBH_CTRL0_CLKGATE );

    // Set GPMI DMA timeout for ~1msec in preparation for Reset. 
    rom_nand_hal_SetGPMITimeout(6);


    // Reset the APBH NAND channels and clr IRQs
    BW_APBH_CHANNEL_CTRL_RESET_CHANNEL(0x1 << (NAND0_APBH_CH));
    HW_APBH_CTRL1_CLR(0x1 << (NAND0_APBH_CH));

    // Reset the NAND so we're in a known state.
    // Don't return a failure because we may still be able to boot from the 
    // other nands.  The reset is not a good indicator of whether
    // a NAND is present.
    Status = g_pNand->HalReset(0);
    DBG_PRINTF("Reset Status %x\n", Status);

    Status = g_pNand->HalReadID(0, (uint8_t *)&zReadID);
    DBG_PRINTF("ReadID Status %x and ReadID Code %x\n", Status, zReadID.usDeviceID);

    // Set GPMI DMA timeout to default.  
    rom_nand_hal_SetGPMITimeout(0);

    // Configure ECC parameters
    rom_nand_hal_UpdateECCParams(&rom_nand_pCtx->zNANDEccParams, pNandEFuse->efNANDsInUse);

    return(SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Read the ID of a NAND.
//!
//! \fntype Function
//!
//! This function reads the ID of a NAND (6 bytes) using the ReadID command.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to reset.
//! \param[out] pReadIDCode Pointer to data location to store ReadID result.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//! \retval ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT     GPMI DMA timed out.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_ReadNandID(uint32_t u32NandDeviceNumber, 
                                   uint8_t * pReadIDCode) 
{
    NAND_dma_read_id_t  dma_read_id;
    RtStatus_t  retCode;

    // Change the default ReadID code to what is being passed in.
    dma_read_id.txCLEByte = NAND_CMD_COMMON_READ_ID;
    dma_read_id.txALEByte = 0x00;

    // I can use the stack for the descriptor because data cache is not
    // enabled and this call is synchronous.
    rom_nand_hal_BuildReadIdDma(&dma_read_id, u32NandDeviceNumber, 
                           pReadIDCode);

    g_pNand->HalStartDma((dma_cmd_t *)&dma_read_id, u32NandDeviceNumber);

    retCode = g_pNand->HalWaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber);

    return(retCode);
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Reset the NAND
//!
//! \fntype Function
//!
//! This function sends a reset command to the NAND.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to reset.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//! \retval ERROR_ROM_NAND_DRIVER_RESET_FAILED   Reset failed.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_ResetNAND(uint32_t u32NandDeviceNumber) 
{
    RtStatus_t  retCode;
    NAND_dma_reset_device_t  dma_reset_device;

    // Load the reset command
    // Most devices have the same reset command (0xFF)
    dma_reset_device.tx_reset_command_buf[0] = NAND_CMD_COMMON_RESET;

    // Build the Reset Device DMA chain.
    rom_nand_hal_BuildResetDma(&dma_reset_device, u32NandDeviceNumber);

    // Kick it off.
    g_pNand->HalStartDma((dma_cmd_t *)&dma_reset_device, u32NandDeviceNumber);

    retCode = g_pNand->HalWaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber);

    if (retCode != SUCCESS)
        return ERROR_ROM_NAND_DRIVER_RESET_FAILED;

    return(retCode);    // Success or Failure?
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Update DMA Descriptor.
//!
//! This function call the initialize DMA function and pass it the newest
//! values that it is aware of.  These values are contained within the 
//! ROM NAND Context variable.
//!
//! \param[in]  None
//!
//! \return void
//!
//! \post The non-variable data in the DMA descriptor is updated.
//!
//!
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_UpdateDmaDescriptor(void)
{
    NAND_Descriptor_t * pNANDDevice = (NAND_Descriptor_t *)&(rom_nand_pCtx->zNANDDescriptor);

    DBG_PRINTF("Total Size %d\n", pNANDDevice->stc_NANDSectorDescriptor.u32TotalPageSize);
    DBG_PRINTF("Num row bytes %d\n", pNANDDevice->btNumRowBytes);
    DBG_PRINTF("Address of pNANDDevice 0x%X\n", pNANDDevice);

    g_pNand->HalInitReadDma ((&(rom_nand_pCtx->DmaReadDma)), 
        pNANDDevice->btNumRowBytes, 
        pNANDDevice->btNumColBytes,
        pNANDDevice->btNANDDataBusWidth,
        rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel,
        pNANDDevice->stc_NANDDeviceCommandCodes.btRead1Code,
        pNANDDevice->stc_NANDDeviceCommandCodes.btRead1_2ndCycleCode);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Initialize the Read DMA Descriptor.
//!
//! This function will initialize the Read DMA descriptor using the values
//! that remain the same throughout the different transactions.  Many of
//! these default values are dummy values that get overwritten when the
//! actual read /e rom_nand_hal_ReadNand is sent.
//!
//! \param[in]  pReadDmaDescriptor Pointer to Read DMA Descriptor struct.
//! \param[in]  u32NumRowBytes Number of row bytes sent for this NAND.
//! \param[in]  u32BusWidth Data Bus Width (8 or 16).
//! \param[in]  u32ECCSize Size of the ECC (4 or 8).
//! \param[in]  u32ReadCode1 Read Command code - 1st command.
//! \param[in]  u32ReadCode2 Read Command code - 2nd command.
//!
//! \return void
//!
//! \post The non-variable data in the DMA descriptor is fixed.
//!
//!
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_InitReadDma(  void * pReadDmaDescriptor, 
                                uint32_t u32NumRowBytes, 
                                uint32_t u32NumColBytes, 
                                uint32_t u32BusWidth,
                                uint32_t u32ECCSize,
                                uint32_t u32ReadCode1,
                                uint32_t u32ReadCode2)
{ 
    uint32_t u32NumberOfECCbytes;
    //int32_t iSectorOffset=0;
    NAND_dma_read_t  *pDmaReadDescriptor = (NAND_dma_read_t *)pReadDmaDescriptor;
    NAND_read_seed_t *pDmaReadSeed = (NAND_read_seed_t *) &(pDmaReadDescriptor->NAND_DMA_Read_Seed);

    // Initialize ReadSeed
    // Initialize mask to read from the beginning of a page including metadata
    pDmaReadSeed->uiECCMask = BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_PAGE; 

    // Calculate ecc bytes for 1 page from zNANDEccParams,
    // Block0 might have separate ecc level than other blocks.
    // Formula for calculating number of parity bits for each block is (ecc_level * 13)
    // todo: make a const for 13
    u32NumberOfECCbytes = (rom_nand_pCtx->zNANDEccParams.m_u32EccBlock0EccLevel * BCH_PARITY_SIZE) + // block0
        (rom_nand_pCtx->zNANDEccParams.m_u32NumEccBlocksPerPage *                       // blockN
         rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel * BCH_PARITY_SIZE);              
    // Convert the result to bytes
    // todo: make a const for 8
    u32NumberOfECCbytes = (u32NumberOfECCbytes + (8-1)) / 8;

    // Alway 2 column bytes.
    pDmaReadSeed->uiAddressSize = u32NumColBytes + u32NumRowBytes;
    
    // Start off assuming we'll only read the data size.  
    pDmaReadSeed->uiReadSize = (rom_nand_pCtx->zSerializerState.m_u32PageDataSize + u32NumberOfECCbytes);

    // set the Word size
    // default to 8 bit data width
    if (u32BusWidth == 16)
    {
        pDmaReadSeed->uiWordSize =  BV_GPMI_CTRL0_WORD_LENGTH__16_BIT;
    } else
    {
        pDmaReadSeed->uiWordSize =  BV_GPMI_CTRL0_WORD_LENGTH__8_BIT;
    }      

    // Setup the Read Data command words.
    pDmaReadSeed->tx_cle1 = (uint8_t)u32ReadCode1; 
    pDmaReadSeed->tx_cle2 = (uint8_t)u32ReadCode2; 
        
    // Fill in the Column Address (Always 2 bytes)
    pDmaReadSeed->bType2Columns[0] = (reg8_t)(0);
    pDmaReadSeed->bType2Columns[1] = (reg8_t)(0);

    // Fill in the Row Address. (Can be 2 or 3 bytes)
    // Fill in the Column Address (Always 2 bytes)
    pDmaReadSeed->bType2Rows[0] = (reg8_t)(0);
    pDmaReadSeed->bType2Rows[1] = (reg8_t)(0);
    pDmaReadSeed->bType2Rows[2] = (reg8_t)(0);

    // Buffer pointers used for DMA chain.
    pDmaReadSeed->pDataBuffer = NULL;
    pDmaReadSeed->pAuxBuffer = NULL;

    rom_nand_hal_BuildReadDma(pDmaReadDescriptor, 0, 
                              pDmaReadSeed, false); 
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Read a given number of bytes from the NAND.
//!
//! This function will read a # of bytes from the NAND.  The structure holding
//! the address, the # of bytes to read, the commands to send, the buffer
//! pointer, the auxillary pointer, the ECCSize, etc are part of the Read 
//! DMA descriptor.  This descriptor is passed in and overwritten with 
//! the data that needs to change for this transaction.  The Read DMA
//! descriptor will have been filled in with the values that are constant
//! for all transactions using the /e rom_nand_hal_InitReadDma function.
//!
//! \param[in]  pReadDmaDescriptor Pointer to the Read Dma Descriptor.
//! \param[in]  u32NandDeviceNumber Physical NAND number to initialize.
//! \param[in]  u32ColumnOffset Offset in page to start at.
//! \param[in]  u32PageNum Physical page to read.
//! \param[in]  u32ReadSize Number of bytes to read from NAND.
//! \param[in]  p8PageBuf Buffer pointer where the data will be placed.
//! \param[in]  p8AuxillaryBuf Buffer pointer for auxillary buffer.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//! \post If successful, the data is in pPageBuf.  The auxillary data (ECC)
//!       is appended at the end of the valid data.
//!
//!
//! \internal
//! \note p8PageBuf must be larger than 2112 because the ECC working area
//!       is appended after the valid data.
//! \note u32ReadSize is the total size which includes the ECC.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_ReadNand(void * pReadDmaDescriptor, 
                                 uint32_t u32NandDeviceNumber, 
                                 uint32_t u32ColumnOffset, 
                                 uint32_t u32PageNum, 
                                 uint32_t u32ReadSize,
                                 uint8_t *p8PageBuf,
                                 uint8_t *p8AuxillaryBuf)
{  
    NAND_dma_read_t  *pDmaReadDescriptor = (NAND_dma_read_t *) pReadDmaDescriptor;
    NAND_read_seed_t *pDmaReadSeed = (NAND_read_seed_t *) &(pDmaReadDescriptor->NAND_DMA_Read_Seed);

    DBG_PRINTF("Read NAND - NAND Read size %d\n", u32ReadSize);

    // Fill in the Column Address (Always 2 bytes)
    pDmaReadSeed->bType2Columns[0] = (reg8_t)(u32ColumnOffset & 0xFF);
    pDmaReadSeed->bType2Columns[1] = (reg8_t)((u32ColumnOffset>>8) & 0xFF);

    // Fill in the Row Address. (Can be 2 or 3 bytes)
    pDmaReadSeed->bType2Rows[0] = (reg8_t)(u32PageNum & 0xFF);
    pDmaReadSeed->bType2Rows[1] = (reg8_t)((u32PageNum>>8) & 0xFF);
    // This is always created, but the Address size determines whether
    // this data is actually sent.
    pDmaReadSeed->bType2Rows[2] = (reg8_t)((u32PageNum>>16) & 0xFF);

    // Set how many bytes need to be read.
    pDmaReadSeed->uiReadSize = u32ReadSize;
    // Set the location where data will be read into.
    pDmaReadSeed->pDataBuffer = p8PageBuf;
    // Set the location where auxillary buffers will reside..
    pDmaReadSeed->pAuxBuffer = p8AuxillaryBuf;

    // Set the ECC Enable/Disable for this transaction.  
    pDmaReadSeed->bEnableHWECC = rom_nand_pCtx->bEnableHWECC;

    // initialize mask to read from the beginning of a page including metadata
    pDmaReadSeed->uiECCMask = BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_PAGE;

    rom_nand_hal_BuildQuickReadDma(pDmaReadDescriptor, u32NandDeviceNumber, 
                              pDmaReadSeed);   
#ifdef VERBOSE_PRINTF
    DBG_PRINTF("tx_cle1_addr_dma.next  0x%X\n", pDmaReadDescriptor->tx_cle1_addr_dma.nxt);
    DBG_PRINTF("tx_cle1_addr_dma.cmd  0x%X\n", pDmaReadDescriptor->tx_cle1_addr_dma.cmd);
    DBG_PRINTF("tx_cle1_addr_dma.bar  0x%X\n", pDmaReadDescriptor->tx_cle1_addr_dma.bar);
    DBG_PRINTF("tx_cle1_addr_dma.gpmi_ctrl0  0x%X\n", pDmaReadDescriptor->tx_cle1_addr_dma.gpmi_ctrl0);
    DBG_PRINTF("tx_cle1_addr_dma.gpmi_cmp  0x%X\n", pDmaReadDescriptor->tx_cle1_addr_dma.gpmi_compare);
    DBG_PRINTF("tx_cle1_addr_dma.gpmi_eccctrl  0x%X\n", pDmaReadDescriptor->tx_cle1_addr_dma.gpmi_eccctrl);

    DBG_PRINTF("tx_cle2_dma.next  0x%X\n", pDmaReadDescriptor->tx_cle2_dma.nxt);
    DBG_PRINTF("tx_cle2_dma.cmd  0x%X\n", pDmaReadDescriptor->tx_cle2_dma.cmd);
    DBG_PRINTF("tx_cle2_dma.bar  0x%X\n", pDmaReadDescriptor->tx_cle2_dma.bar);
    DBG_PRINTF("tx_cle2_dma.gpmi_ctrl0  0x%X\n", pDmaReadDescriptor->tx_cle2_dma.gpmi_ctrl0);

    DBG_PRINTF("wait_dma.next  0x%X\n", pDmaReadDescriptor->wait_dma.nxt);
    DBG_PRINTF("wait_dma.cmd  0x%X\n", pDmaReadDescriptor->wait_dma.cmd);
    DBG_PRINTF("wait_dma.bar  0x%X\n", pDmaReadDescriptor->wait_dma.bar);
    DBG_PRINTF("wait_dma.gpmi_ctrl0  0x%X\n", pDmaReadDescriptor->wait_dma.gpmi_ctrl0);

    DBG_PRINTF("sense_dma.next  0x%X\n", pDmaReadDescriptor->sense_dma.nxt);
    DBG_PRINTF("sense_dma.cmd  0x%X\n", pDmaReadDescriptor->sense_dma.cmd);
    DBG_PRINTF("sense_dma.bar  0x%X\n", pDmaReadDescriptor->sense_dma.bar);
    DBG_PRINTF("sense_dma.gpmi_ctrl0  0x%X\n", pDmaReadDescriptor->sense_dma.gpmi_ctrl0);

    DBG_PRINTF("rx_data_dma.next  0x%X\n", pDmaReadDescriptor->rx_data_dma.nxt);
    DBG_PRINTF("rx_data_dma.cmd  0x%X\n", pDmaReadDescriptor->rx_data_dma.cmd);
    DBG_PRINTF("rx_data_dma.bar  0x%X\n", pDmaReadDescriptor->rx_data_dma.bar);
    DBG_PRINTF("rx_data_dma.gpmi_ctrl0  0x%X\n", pDmaReadDescriptor->rx_data_dma.gpmi_ctrl0);
    DBG_PRINTF("rx_data_dma.gpmi_cmp  0x%X\n", pDmaReadDescriptor->rx_data_dma.gpmi_compare);
    DBG_PRINTF("rx_data_dma.gpmi_eccctrl  0x%X\n", pDmaReadDescriptor->rx_data_dma.gpmi_eccctrl);
    DBG_PRINTF("rx_data_dma.gpmi_ecccount  0x%X\n", pDmaReadDescriptor->rx_data_dma.gpmi_ecccount);
    DBG_PRINTF("rx_data_dma.gpmi_payload  0x%X\n", pDmaReadDescriptor->rx_data_dma.gpmi_payload);
    DBG_PRINTF("rx_data_dma.gpmi_auxiliary  0x%X\n", pDmaReadDescriptor->rx_data_dma.gpmi_auxiliary);
    DBG_PRINTF("rx_wait4done_dma.next  0x%X\n", pDmaReadDescriptor->rx_wait4done_dma.nxt);
    DBG_PRINTF("rx_wait4done_dma.cmd  0x%X\n", pDmaReadDescriptor->rx_wait4done_dma.cmd);
    DBG_PRINTF("rx_wait4done_dma.bar  0x%X\n", pDmaReadDescriptor->rx_wait4done_dma.bar);
    DBG_PRINTF("rx_wait4done_dma.gpmi_ctrl0  0x%X\n", pDmaReadDescriptor->rx_wait4done_dma.gpmi_ctrl0);
    DBG_PRINTF("rx_wait4done_dma.gpmi_cmp  0x%X\n", pDmaReadDescriptor->rx_wait4done_dma.gpmi_compare);
    DBG_PRINTF("rx_wait4done_dma.gpmi_eccctrl  0x%X\n", pDmaReadDescriptor->rx_wait4done_dma.gpmi_eccctrl);

    DBG_PRINTF("DmaReadSeed.uiAddressSize  0x%X\n", pDmaReadSeed->uiAddressSize);
    DBG_PRINTF("DmaReadSeed.uiReadSize  0x%X\n", pDmaReadSeed->uiReadSize);
    DBG_PRINTF("DmaReadSeed.uiWordSize  0x%X\n", pDmaReadSeed->uiWordSize);
    DBG_PRINTF("DmaReadSeed.zNANDEccParams.u32EccType  0x%X\n", pDmaReadSeed->zNANDEccParams.u32EccType);
    DBG_PRINTF("DmaReadSeed.pDataBuffer  0x%X\n", pDmaReadSeed->pDataBuffer);
    DBG_PRINTF("DmaReadSeed.pAuxBuffer  0x%X\n", pDmaReadSeed->pAuxBuffer);
    DBG_PRINTF("DmaReadSeed.cleBuff  0x%X\n", (uint32_t)pDmaReadSeed->tx_cle1_addr_buf[0]);
#endif
    
    // Clear the ECC Complete flag.
    rom_nand_hal_ClearEccCompleteFlag();

    // Kick off the DMA.
    g_pNand->HalStartDma((dma_cmd_t *)pDmaReadDescriptor, u32NandDeviceNumber);

    return(SUCCESS); 
}

uint32_t GetEccType(uint32_t u32EccLevel)
{
    switch((nand_ecc_levels_t)u32EccLevel)
    {
    case BCH_Ecc_0bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__NONE;
    case BCH_Ecc_2bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC2;
    case BCH_Ecc_4bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC4;
    case BCH_Ecc_6bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC6;
    case BCH_Ecc_8bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC8;
    case BCH_Ecc_10bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC10;
    case BCH_Ecc_12bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC12;
    case BCH_Ecc_14bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC14;
    case BCH_Ecc_16bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC16;
    case BCH_Ecc_18bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC18;
    case BCH_Ecc_20bit:
        return BV_BCH_FLASH0LAYOUT0_ECC0__ECC20;
    }

    return BV_BCH_FLASH0LAYOUT0_ECC0__NONE;
}
////////////////////////////////////////////////////////////////////////////////
//! \brief sets BCH flash layout registers with values from readseed params
//!
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to initialize.
//! \param[in]  pReadSeed readseed data 
//!
void  rom_nand_hal_SetFlashLayout(uint32_t u32NandDeviceNumber, 
                                NAND_ECC_Params_t * pEccParams)
{
    // for nand0
    // set flash0layout0 bch ecc register
    BW_BCH_FLASH0LAYOUT0_NBLOCKS(pEccParams->m_u32NumEccBlocksPerPage);
    BW_BCH_FLASH0LAYOUT0_META_SIZE(pEccParams->m_u32MetadataBytes);
    BW_BCH_FLASH0LAYOUT0_ECC0(GetEccType(pEccParams->m_u32EccBlock0EccLevel));
    BW_BCH_FLASH0LAYOUT0_DATA0_SIZE(pEccParams->m_u32EccBlock0Size);

    // set flash0layout1 bch ecc register
    BW_BCH_FLASH0LAYOUT1_PAGE_SIZE(pEccParams->m_u32PageSize);
    BW_BCH_FLASH0LAYOUT1_ECCN(GetEccType(pEccParams->m_u32EccBlockNEccLevel));
    BW_BCH_FLASH0LAYOUT1_DATAN_SIZE(pEccParams->m_u32EccBlockNSize);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Read page from the NAND.
//!
//! This function will read a page from the NAND.  
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to initialize.
//! \param[in]  u32PageNum Physical page to read..
//! \param[out] p8PageBuf Buffer pointer where the data will be placed.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//! \post If successful, the data is in pPageBuf.  The auxillary data (ECC)
//!       is appended at the end of the valid data.
//!
//!
//! \internal
//! \note p8PageBuf must be larger than 2112 because the ECC working area
//!       is appended after the valid data.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_ReadPage(uint32_t u32NandDeviceNumber, 
                               uint32_t u32PageNum, uint8_t *p8PageBuf)
{
    uint32_t    i;    
    uint32_t iCol=0;   
    uint32_t u32NumberOfEccBytes;
    //int32_t iSectorOffset=0;
    NAND_dma_read_t  *pDmaReadDescriptor = (NAND_dma_read_t *) &(rom_nand_pCtx->DmaReadDma);
    NAND_read_seed_t *pDmaReadSeed = (NAND_read_seed_t *) &(pDmaReadDescriptor->NAND_DMA_Read_Seed);
    NAND_Descriptor_t * pNANDDevice = (NAND_Descriptor_t *)&(rom_nand_pCtx->zNANDDescriptor);

    DBG_PRINTF("NAND Read Sector %d on NAND %d\n", u32PageNum, u32NandDeviceNumber);

    pDmaReadSeed->bEnableHWECC = rom_nand_pCtx->bEnableHWECC;
    
    // Initialize readseed
    // initialize mask to read from the beginning of a page including metadata
    pDmaReadSeed->uiECCMask = BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_PAGE;

    // Calculate ecc bytes for 1 page from zNANDEccParams,
    // Block0 might have separate ecc level than other blocks.
    // Formula for calculating number of parity bits for each block is (ecc_level * 13)
    u32NumberOfEccBytes = (rom_nand_pCtx->zNANDEccParams.m_u32EccBlock0EccLevel * BCH_PARITY_SIZE) + // block0
        (rom_nand_pCtx->zNANDEccParams.m_u32NumEccBlocksPerPage *                       // blockN
         rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel * BCH_PARITY_SIZE);              
    // Convert the result to bytes
    u32NumberOfEccBytes = (u32NumberOfEccBytes + (8-1)) / 8;

    rom_nand_pCtx->zNANDEccParams.m_u32PageSize = rom_nand_pCtx->zNANDEccParams.m_u32MetadataBytes +
        rom_nand_pCtx->zNANDEccParams.m_u32EccBlock0Size +
       (rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNSize * rom_nand_pCtx->zNANDEccParams.m_u32NumEccBlocksPerPage) +
        u32NumberOfEccBytes;

    
    // Alway 2 column bytes.
    pDmaReadSeed->uiAddressSize = pNANDDevice->btNumColBytes + pNANDDevice->btNumRowBytes;
    
    // Start off assuming we'll only read the data size.  
    if( pDmaReadSeed->bEnableHWECC )
    {
        pDmaReadSeed->uiReadSize = rom_nand_pCtx->zNANDEccParams.m_u32PageSize;
    }
    else
    {
        // if no ecc, then assume full 2112 bytes
        pDmaReadSeed->uiReadSize = pNANDDevice->stc_NANDSectorDescriptor.u32TotalPageSize;
    }

    // set the Word size
    // default to 8 bit data width
    if (pNANDDevice->btNANDDataBusWidth == 16)
    {
        pDmaReadSeed->uiWordSize =  BV_GPMI_CTRL0_WORD_LENGTH__16_BIT;
        // also shift iCol.
        iCol >>= 1;
    } else
    {
        pDmaReadSeed->uiWordSize =  BV_GPMI_CTRL0_WORD_LENGTH__8_BIT;
    }      

    // Setup the Read Data command words.
    pDmaReadSeed->tx_cle1 = 
        pNANDDevice->stc_NANDDeviceCommandCodes.btRead1Code; 
    pDmaReadSeed->tx_cle2 = 
        pNANDDevice->stc_NANDDeviceCommandCodes.btRead1_2ndCycleCode; 
        
    // Fill in the Column Address (Always 2 bytes)
    pDmaReadSeed->bType2Columns[0] = (reg8_t)(iCol & 0xFF);
    pDmaReadSeed->bType2Columns[1] = (reg8_t)((iCol>>8) & 0xFF);

    // Fill in the Row Address. (Can be 2 or 3 bytes)
    for (i=0;i<pNANDDevice->btNumRowBytes;i++)
    {
        pDmaReadSeed->bType2Rows[i] = (reg8_t)((u32PageNum>>(8*i)) & 0xFF);
    }   


    DBG_PRINTF("Column %d :  Row %d\n", iCol, u32PageNum);

    // Buffer pointers used for DMA chain.
    pDmaReadSeed->pDataBuffer = p8PageBuf;
    pDmaReadSeed->pAuxBuffer = ((uint8_t *)(p8PageBuf + NAND_AUX_BUFFER_INDEX));

    DBG_PRINTF("DataBuffer @ 0x%X :  Aux Buffer @ 0x%X\n", 
               (uint32_t)(pDmaReadSeed->pDataBuffer), 
               (uint32_t)(pDmaReadSeed->pAuxBuffer));

    // For bch, initialize aux to 0, metadata and ecc status info for each block is returned in this area
    if( pDmaReadSeed->bEnableHWECC )
    {
        // Set bch flash layout registers
        rom_nand_hal_SetFlashLayout(u32NandDeviceNumber, &(rom_nand_pCtx->zNANDEccParams));
    }


    rom_nand_hal_BuildReadDma(pDmaReadDescriptor, u32NandDeviceNumber, 
                              pDmaReadSeed, false);

    // Clear the ECC Complete flag.
    rom_nand_hal_ClearEccCompleteFlag();

    // Kick off the DMA.
    g_pNand->HalStartDma((dma_cmd_t *)pDmaReadDescriptor, u32NandDeviceNumber);

    rom_nand_pCtx->zNandDmaTime.uStartDMATime = ROM_GET_TIME_USEC();

    return(SUCCESS); 
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Wait for NAND Read Transaction Completion.
//!
//!
//! This function will wait for the DMA to complete then check the ECC
//! status to determine if this is a valid read.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to initialize.
//! \param[in]  u32EccThreshold Threshold of correctable ECC errors.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred. 
//! \retval ERROR_ROM_NAND_ECC_FAILED  ECC correction failed.
//! \retval ERROR_ROM_NAND_ECC_THRESHOLD ECC threshold reached.
//! \retval ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT DMA timed out.
//!
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_WaitForReadComplete(uint32_t u32NandDeviceNumber,
                                            uint32_t u32EccThreshold)
{
    RtStatus_t retCode = SUCCESS;

    DBG_PRINTF("Wait For Dma Completion\n");

    retCode = g_pNand->HalWaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber);

    if (retCode != SUCCESS)
    {
        return retCode;
    }

    if( rom_nand_pCtx->bEnableHWECC )
    {
        // Check the ECC Status to determine if this is a valid read.
        retCode = g_pNand->HalECCStatus(u32NandDeviceNumber, 
                                        u32EccThreshold);    
    }
    return retCode;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Check ECC Status.
//!
//! This function will check the status of the ECC after a Read to determine
//! if the ECC Passed, Failed or the Threshold was exceeded.  Note that
//! even though the DMA is complete, the ECC8_COMPLETE bit must go high
//! before the ECC is valid.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to initialize.
//! \param[in]  u32Threshold Threshold to test ECC corrections against.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//! \retval ERROR_ROM_NAND_ECC_FAILED           If ECC failed for Read
//! \retval ERROR_ROM_NAND_ECC_THRESHOLD  If ECC threshold was exceeded.
//!
//!
//! \internal
//! \todo Fill in the Dillo emulation section.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_CheckECCStatus(uint32_t u32NandDeviceNumber, uint32_t u32Threshold)
{
    RtStatus_t retStatus = SUCCESS;
    bool bThresholdReached = false;
    uint32_t u32MaxCorrections = rom_nand_hal_FindEccErrors(u32Threshold);
    NAND_dma_read_t  *pDmaReadDescriptor = (NAND_dma_read_t *) &(rom_nand_pCtx->DmaReadDma);
    NAND_read_seed_t *pDmaReadSeed = (NAND_read_seed_t *) &(pDmaReadDescriptor->NAND_DMA_Read_Seed);

    // bch
    if(u32MaxCorrections) 
    {
        // u32MaxCorrections will be Status0 for BCH. 
        // get the errors from auxillary pointer at offset after metadata bytes
        uint8_t * p8AuxPointer = (uint8_t *)pDmaReadSeed->pAuxBuffer;
        int i=0; 
        int indexToAuxBuffer = 0;
        uint32_t u32Temp;
        uint32_t uNumBlks = rom_nand_pCtx->zNANDEccParams.m_u32NumEccBlocksPerPage+1;

        // get the status of Blocks. Each block's status is in a byte, starts at the beginning of a new word where metadata ends
        indexToAuxBuffer = rom_nand_pCtx->zNANDEccParams.m_u32MetadataBytes + (rom_nand_pCtx->zNANDEccParams.m_u32MetadataBytes % 4);
        // now get the max ecc corrections of data blocks including metadata ecc
        for(i=0; i<uNumBlks; i++) 
        {
            uint32_t u32EccLevel;
            
            if(i == 0)
                u32EccLevel = rom_nand_pCtx->zNANDEccParams.m_u32EccBlock0EccLevel;
            else
                u32EccLevel = rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel;

            u32Temp = p8AuxPointer[indexToAuxBuffer + i];
            if (u32Temp == BV_BCH_STATUS0_STATUS_BLK0__UNCORRECTABLE)
            {
                DBG_PRINTF("BCH Status ECC Sub-Block Not Recoverable %d = 0x%X\n", i, u32Temp);
            }
            else if(u32Temp == BV_BCH_STATUS0_STATUS_BLK0__ERASED)
            {
                DBG_PRINTF("BCH Status ECC Sub-Block Erased %d = 0x%X\n", i, u32Temp);
            }
            else
            {
                if( u32EccLevel == u32Temp )
                {
                    bThresholdReached = true;
                }
                DBG_PRINTF("BCH Status Sub-Block %d = 0x%X\n", i, u32Temp);
            }
        }
    }

    if (u32MaxCorrections & BM_BCH_STATUS0_ALLONES)
    {
        DBG_PRINTF("ECC returned ALL ONES Error\n");
        retStatus = ERROR_ROM_NAND_ECC_FAILED;
    }
    else if (u32MaxCorrections & BM_BCH_STATUS0_UNCORRECTABLE)
        retStatus = ERROR_ROM_NAND_ECC_FAILED;
    else if (u32MaxCorrections & BM_BCH_STATUS0_CORRECTED)
    {
        if(bThresholdReached)
        {
            retStatus = ERROR_ROM_NAND_ECC_THRESHOLD;
        }
    }

    // Clear the ECC Complete IRQ.
    BW_BCH_CTRL_COMPLETE_IRQ(0);
    return retStatus;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Get the current status of the DMA & GPMI block
//!
//! This function will return the current value of the GPMI and DMA status.
//!
//! \param[in]  void
//!
//! \return Status of call or error.
//! \retval 0            If DMA is complete.
//! \retval ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT   If DMA has timed out.
//! \retval ERROR_ROM_NAND_DMA_BUSY  If DMA is busy.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_CurrentGpmiDmaStatus(uint32_t u32NandDeviceNumber)
{
    RtStatus_t iResult=ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT;

    DBG_PRINTF("NAND Status\n");

    iResult = rom_nand_hal_GetDmaStatus(u32NandDeviceNumber, 
                                  rom_nand_pCtx->zNandDmaTime.uStartDMATime, 
                                  rom_nand_pCtx->zNandDmaTime.uDMATimeout);

    // Don't run an ECC here because we're only returning the DMA completion status.

    return iResult;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Shutdown the NAND at the HAL layer.
//!
//! This function will cleanup the NAND HAL and leave it in a known state.
//!
//! \param[in]  void
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//! \post The GPMI has been gated.
//!
//!
//! \internal
//! To view function details, see rom_nand_hal_api.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_Shutdown(void) 
{

    rom_nand_hal_DisableGPMI();

    rom_nand_hal_DisableBCH();

    return(SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Figure out using bad block marker if given block is good or bad.
//!
//! This function will read first 3 and last 3 pages of given block and if any 
//! one page shows bad block marker offset set then it will return true.
//!
//! \param[in]  u32Block, given block
//! \param[in]  pu8Buf, address of buffer to read page data
//!
//! \return true, If block is determined to be bad.
//! \retval false, If block is good.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
bool rom_nand_hal_IsBlockBad(uint32_t u32Block, uint8_t* pu8Buf)
{
    // Read page 0, check metadata[0], if bad return false else continue same 
    // for pages 1, 2, last-2, last-1 and last.

    uint32_t u32PageNum=0; // start page number

    int nCount=6; // number of pages to verify

    RtStatus_t ReadStatus=SUCCESS; // status

    bool bBadBlockFound = false; // initialize return value to false

    bool bHwEccSavedState = rom_nand_pCtx->bEnableHWECC; // Save the state of bEnableHWECC 

    uint32_t u32PagesPerBlock = rom_nand_pCtx->zSerializerState.m_uSectorsPerBlock;
    u32PageNum = u32PagesPerBlock * u32Block;
    // read raw just bad block marker byte for 6 pages and return true if one of
    // them is non 0xff otherwise return false.

    // disable ecc to read raw
    rom_nand_pCtx->bEnableHWECC = false;

    // the bad block marker will be @ rom_nand_pCtx->zSerializerState.m_u32PageDataSize offset

    while(nCount){ 
        // read bad block marker byte
        rom_nand_hal_ReadPage(0, u32PageNum, pu8Buf);

        // wait for dma to complete
        ReadStatus = g_pNand->HalWaitForReadComplete(0, rom_nand_pCtx->zNANDEccParams.m_u32EccBlockNEccLevel);             

        // check the status to be success and byte good block
        if((ReadStatus == SUCCESS) && (pu8Buf[rom_nand_pCtx->zSerializerState.m_u32BBMarkerPhysicalOffset] == 0xFF))
        {
            // go to the next page
            u32PageNum ++;
            
            // increment count
            nCount --;

            // if we are half way through
            if(nCount == 3)
            {
                // just finished verifying 0, 1 and 2 pages of the block,
                // now go for last-2, last-1 and last.
                u32PageNum = (u32PagesPerBlock * u32Block) + (u32PagesPerBlock - 3);
            }
        }
        else 
        {
            // either failed to read or byte value is non-0xff both can be due to bad block
            bBadBlockFound = true;

            // no need to go any further
            break;
        }
    }

    // restore rom_nand_pCtx->bEnableHWECC 
    rom_nand_pCtx->bEnableHWECC = bHwEccSavedState;

    // return result
    return bBadBlockFound;
}

//! @}
