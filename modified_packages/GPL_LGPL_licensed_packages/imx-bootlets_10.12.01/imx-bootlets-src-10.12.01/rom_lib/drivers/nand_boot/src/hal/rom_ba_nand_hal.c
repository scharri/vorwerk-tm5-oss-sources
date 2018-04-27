#include "return_codes.h"
#include "rom_types.h"
#include "rom_nand_hal_structs.h"
#include "rom_nand_hal_dma.h"
#include "rom_nand_internal.h"
#include <itf_rom.h>
#include "rom_ba_nand_hal.h"
#include "debug.h"
#include "rom_utils.h"
////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////
extern rom_nand_Context_t *rom_nand_pCtx;

////////////////////////////////////////////////////////////////////////////////
//! \brief Sends ba nand abort command to device
//!
//! This function prepares reset dma chain to send abort command
//!
//! \param[in]  u32NandDeviceNumber
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_ba_nand_hal_Abort(uint32_t u32NandDeviceNumber) 
{
    RtStatus_t  retCode;
    NAND_dma_reset_device_t  dma;

    DBG_PRINTF("BA NAND ABORT Cmd\n");
    // Load the reset command
    // Most devices have the same reset command (0xFF)
    dma.tx_reset_command_buf[0] = eBANandLBAAbort;

    // Build the Reset Device DMA chain.
    rom_nand_hal_BuildResetDma(&dma, u32NandDeviceNumber);

    // Kick it off.
    g_pNand->HalStartDma((dma_cmd_t *)&dma.tx_dma, u32NandDeviceNumber);

    retCode = g_pNand->HalWaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber);

    if (retCode != SUCCESS)
        return ERROR_ROM_NAND_DRIVER_RESET_FAILED;

    return(retCode);    // Success or Failure?
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Sends read command to ba nand device
//!
//! This function call rom_nand_hal_BuildReadDma to build dma descriptor to read
//! a sectors of data. 
//!
//! \param[in]  u32NandDeviceNumber
//! \param[in]  u32SectorCount, number of sectors to read
//! \param[in]  u32SectorAddress, address of sector 
//! \param[in]  pu8Buf, address of buffer to return data read
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_ba_nand_hal_SendReadCmd(uint32_t u32NandDeviceNumber, uint32_t u32SectorCount, uint32_t u32SectorAddress, uint8_t *pu8Buf)
{
    int i;
    NAND_dma_read_t  *pDmaReadDescriptor = (NAND_dma_read_t *) &(rom_nand_pCtx->DmaReadDma);
    NAND_read_seed_t *pDmaReadSeed = (NAND_read_seed_t *) &(pDmaReadDescriptor->NAND_DMA_Read_Seed);
    NAND_Descriptor_t * pNANDDevice = (NAND_Descriptor_t *)&(rom_nand_pCtx->zNANDDescriptor);

    DBG_PRINTF("NAND Read Sector %d on NAND %d\n", u32SectorAddress, u32NandDeviceNumber);

    pDmaReadSeed->bEnableHWECC = rom_nand_pCtx->bEnableHWECC;

    // Set address size, includes sector count and sector address bytes
    pDmaReadSeed->uiAddressSize = pNANDDevice->btNumColBytes + pNANDDevice->btNumRowBytes;

    // Setup the Read Data command words.
    pDmaReadSeed->tx_cle1 = pNANDDevice->stc_NANDDeviceCommandCodes.btRead1Code; 
    pDmaReadSeed->tx_cle2 = pNANDDevice->stc_NANDDeviceCommandCodes.btRead1_2ndCycleCode; 
        
    // Fill in the Column Address (Always 2 bytes)
    pDmaReadSeed->bType2Columns[0] = (reg8_t)(u32SectorCount & 0xFF);
    pDmaReadSeed->bType2Columns[1] = (reg8_t)((u32SectorCount>>8) & 0xFF);

    // Fill in the Row Address. (5 bytes)
    for (i=0;i<pNANDDevice->btNumRowBytes;i++)
    {
        pDmaReadSeed->bType2Rows[i] = (reg8_t)((u32SectorAddress>>(8*i)) & 0xFF);
    }   

    // Set size of data to read
    pDmaReadSeed->uiReadSize = rom_nand_pCtx->zSerializerState.m_u32PageDataSize;

    // set the Word size, default to 8 bit data width
    pDmaReadSeed->uiWordSize =  BV_GPMI_CTRL0_WORD_LENGTH__8_BIT;

    // Buffer pointers used for DMA chain.
    pDmaReadSeed->pDataBuffer = pu8Buf;
    pDmaReadSeed->pAuxBuffer = NULL;

    // build the dma descriptor
    rom_nand_hal_BuildReadDma(pDmaReadDescriptor, u32NandDeviceNumber, 
                              pDmaReadSeed, false);

    // Kick off the DMA.
    g_pNand->HalStartDma((dma_cmd_t *)pDmaReadDescriptor, u32NandDeviceNumber);

    rom_nand_pCtx->zNandDmaTime.uStartDMATime = ROM_GET_TIME_USEC();

    return (g_pNand->HalWaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber));
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Reads parameters page from ONFI Nand.
//!
//! This function will call API to setup dma descriptors to read parameters page 
//! for onfi NAND, it starts dma and returns result from HalWaitDma API. The 
//! buffer pu8Buf should be at least 1024 bytes in size
//!
//! \param[in]  u32NandDeviceNumber
//! \param[in]  pu8Buf address of buffer to return data read
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_ReadParameterPage(uint32_t u32NandDeviceNumber, uint8_t *pu8Buf)
{
    // build the dma descriptor
    NAND_dma_read_t  *pDmaReadDescriptor = (NAND_dma_read_t *) &(rom_nand_pCtx->DmaReadDma);
    NAND_read_seed_t *pDmaReadSeed = (NAND_read_seed_t *) &(pDmaReadDescriptor->NAND_DMA_Read_Seed);

    pDmaReadSeed->tx_cle1 = eBANandReadParameterPage; 
    pDmaReadSeed->tx_addr[0]=0;

    rom_nand_hal_BuildParamsPageDmaDesc(pDmaReadDescriptor, u32NandDeviceNumber, 
                              pu8Buf, 1024);

    // Kick off the DMA.
    g_pNand->HalStartDma((dma_cmd_t *)pDmaReadDescriptor, u32NandDeviceNumber);

    rom_nand_pCtx->zNandDmaTime.uStartDMATime = ROM_GET_TIME_USEC();

    return (g_pNand->HalWaitDma(MAX_TRANSACTION_TIMEOUT, u32NandDeviceNumber));
}
