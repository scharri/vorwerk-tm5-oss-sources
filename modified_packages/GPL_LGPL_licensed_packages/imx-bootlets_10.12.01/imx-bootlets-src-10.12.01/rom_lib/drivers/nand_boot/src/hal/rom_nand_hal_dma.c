////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_hal
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_hal_dma.c
//! \brief This file provides the DMA functions for the ROM NAND HAL.
//!
//!
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "return_codes.h"
#include "rom_types.h"
#include "rom_nand_hal_structs.h"
#include "rom_nand_hal_dma.h"
#include "rom_nand_hal_dma_descriptor.h"
#include "rom_nand_hal_api.h"
#include "rom_nand_internal.h"
//#include "rom_nand_status_codes.h"
#include "regs.h"
#include "rom_utils.h"

#if BRAZO_ROM_FLASH
#include "regsbrazoio.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! \brief Build the DMA to send a NAND Reset Command to the device.
//!
//! This function sends a NAND Reset command to the device.  This is common
//! for all NANDs currently supported..
//!
//! \param[in]  pChain Pointer to the DMA Chain.
//! \param[in]  u32NandDeviceNumber Physical NAND number to reset.
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void  rom_nand_hal_BuildResetDma(NAND_dma_reset_device_t* pChain, 
      uint32_t u32NandDeviceNumber)
{    
    // First we want to wait for Ready.  The chip may be busy on power-up.
    // Wait for Ready.
    pChain->wait4rdy_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->sense_rdy_dma);
    pChain->wait4rdy_dma.cmd.U = NAND_DMA_WAIT4RDY_CMD;
    // BAR points to alternate branch if timeout occurs.
    pChain->wait4rdy_dma.bar = (apbh_dma_gpmi1_t*)&(pChain->timeout_dma);
    // Set GPMI wait for ready.
    pChain->wait4rdy_dma.gpmi_ctrl0.U = NAND_DMA_WAIT4RDY_PIO(u32NandDeviceNumber);

    // Now check for successful Ready.
    pChain->sense_rdy_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->tx_dma);
    pChain->sense_rdy_dma.cmd.U = NAND_DMA_SENSE_CMD(0);
    // BAR points to alternate branch if timeout occurs.
    pChain->sense_rdy_dma.bar = (apbh_dma_gpmi1_t*)&(pChain->timeout_dma); 
    // Even though PIO is unused, set it to zero for comparison purposes.
    pChain->sense_rdy_dma.gpmi_ctrl0.U = 0;

    // Next command will be a wait.
    pChain->tx_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->wait_dma);
    // Configure APBH DMA for NAND Reset command.
    pChain->tx_dma.cmd.U = NAND_DMA_COMMAND_CMD(NAND_RESET_DEVICE_SIZE,0,NAND_LOCK,3);

    // Buffer Address Register being used to hold command.
    pChain->tx_dma.bar = pChain->tx_reset_command_buf;
    // Setup GPMI bus for Reset Command.  Need to set CLE high, then
    // low, then ALE toggles high and low.  (Actually, in this case
    // ALE toggling probably isn't necessary)
    pChain->tx_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber,
                                           NAND_RESET_DEVICE_SIZE,0,ASSERT_CS);
    // Nothing needs to happen to the compare.
    pChain->tx_dma.gpmi_compare.U = (reg32_t) NULL;
    // Disable the ECC.    
    pChain->tx_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

    // Setup 2nd complete DMA sequence.
    // Wait for Ready.
    pChain->wait_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->sense_dma);
    pChain->wait_dma.cmd.U = NAND_DMA_WAIT4RDY_CMD;
    // BAR points to alternate branch if timeout occurs.
    pChain->wait_dma.bar = (apbh_dma_gpmi1_t*)0x00;
    // Set GPMI wait for ready.
    pChain->wait_dma.gpmi_ctrl0.U = NAND_DMA_WAIT4RDY_PIO(u32NandDeviceNumber);

    // Now check for success.
    pChain->sense_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->success_dma);
    // Decrement semaphore.
    pChain->sense_dma.cmd.U = NAND_DMA_SENSE_CMD(0);
    // BAR points to alternate branch if timeout occurs.
    pChain->sense_dma.bar = (apbh_dma_gpmi1_t*)&(pChain->timeout_dma);
    // Even though PIO is unused, set it to zero for comparison purposes.
    pChain->sense_dma.gpmi_ctrl0.U = 0;   

    // Initialize the Terminator functions
    // Next function is null.
    pChain->success_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->success_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to success termination code.
    pChain->success_dma.bar = (void *) SUCCESS;

    // Next function is null.
    pChain->timeout_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->timeout_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to timeout termination code.
    pChain->timeout_dma.bar = (void *) ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT;
    
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Build the DMA to send a NAND Read ID Command to the device.
//!
//! This function sends a NAND Read ID command to the device.  This is common
//! for all NANDs currently supported.
//!
//! \param[in]  pChain Pointer to the DMA Chain.
//! \param[in]  u32NandDeviceNumber Physical NAND number to read ID.
//! \param[in]  pReadIDBuffer Pointer to where the Read ID results will go.
//! \param[out] pReadIDBuffer The results of the Read ID command.
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void  rom_nand_hal_BuildReadIdDma(NAND_dma_read_id_t* pChain, 
      uint32_t u32NandDeviceNumber, void*  pReadIDBuffer)
{
    // First we want to wait for Ready.  The chip may be busy on power-up.
    // Wait for Ready.
    pChain->wait4rdy_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->sense_rdy_dma);
    pChain->wait4rdy_dma.cmd.U = NAND_DMA_WAIT4RDY_CMD;
    // BAR points to alternate branch if timeout occurs.
    pChain->wait4rdy_dma.bar = (apbh_dma_gpmi1_t*)0x00;
    // Set GPMI wait for ready.
    pChain->wait4rdy_dma.gpmi_ctrl0.U = NAND_DMA_WAIT4RDY_PIO(u32NandDeviceNumber);

    // Now check for successful Ready.
    pChain->sense_rdy_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->tx_dma);
    pChain->sense_rdy_dma.cmd.U = NAND_DMA_SENSE_CMD(0);
    // BAR points to alternate branch if timeout occurs.
    pChain->sense_rdy_dma.bar = (apbh_dma_gpmi1_t*)&(pChain->timeout_dma);
    // Even though PIO is unused, set it to zero for comparison purposes.
    pChain->sense_rdy_dma.gpmi_ctrl0.U = 0;

    // Next command in chain will be a read.
    pChain->tx_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->rx_dma);
    // Configure APBH DMA to push Read ID command (toggling CLE & ALE)
    // into GPMI_CTRL.
    // Transfer NAND_READ_ID_SIZE to GPMI when GPMI ready.
    // Transfer 1 word to GPMI_CTRL0 (see command below)
    // Wait for end command from GPMI before rx part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    pChain->tx_dma.cmd.U = NAND_DMA_COMMAND_CMD(NAND_READ_ID_SIZE, 0, NAND_LOCK,3);

    // Buffer Address Register being used to hold Read ID command.
    pChain->tx_dma.bar = pChain->tx_readid_command_buf;  //FIXME - ReadID is packed inside chain.
    // Setup GPMI bus for Read ID Command.  Need to set CLE high, then
    // low, then ALE toggles high and low.  Read ID Code sent during
    // CLE, 2nd byte (0x00) sent during ALE.
    pChain->tx_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber,
         NAND_READ_ID_SIZE, BV_GPMI_CTRL0_ADDRESS_INCREMENT__ENABLED, ASSERT_CS);
    // Nothing needs to happen to the compare.
    pChain->tx_dma.gpmi_compare.U = (reg32_t) NULL;
    // Disable the ECC.    
    pChain->tx_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

    // Setup 2nd complete DMA sequence.
    // Setup to use SUCESS DMA sequence if successful.
    pChain->rx_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->success_dma);
    // Configure APBH DMA to push Read ID command
    // into GPMI_CTRL.
    // Transfer NAND_READ_ID_SIZE to GPMI when GPMI ready.
    // Transfer 1 word to GPMI_CTRL0 (see command below)
    // Wait for end command from GPMI before rx part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_WRITE - Perform PIO word transfers then transfer to
    //             memory from peripheral for specified # of bytes.
    pChain->rx_dma.cmd.U = NAND_DMA_RX_NO_ECC_CMD(NAND_READ_ID_RESULT_SIZE, 0);
    // Buffer Address Register being used to hold Read ID result.
    pChain->rx_dma.bar = pReadIDBuffer;
    // Setup GPMI bus for Read ID Result.  Read data back in.
    // Read RESULT_SIZE bytes (8 bit) data
    pChain->rx_dma.gpmi_ctrl0.U = NAND_DMA_RX_PIO(u32NandDeviceNumber,
           BV_GPMI_CTRL0_WORD_LENGTH__8_BIT, NAND_READ_ID_RESULT_SIZE);

    // Initialize the Terminator functions
    // Next function is null.
    pChain->success_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->success_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to success termination code.
    pChain->success_dma.bar = (void *) SUCCESS;

    // Next function is null.
    pChain->timeout_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->timeout_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to timeout termination code.
    pChain->timeout_dma.bar = (void *) ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Build the DMA to send a NAND Read Command to the device.
//!
//! This function sends a NAND Read command to the device.
//!
//! \param[in]  pChain Pointer to the DMA Chain.
//! \param[in]  u32NandDeviceNumber Physical NAND number to read.
//! \param[in]  pReadSeed Pointer to structure containing the details of the
//!             NAND Read Command (size, address, etc).
//! \param[in]  bCmdOnly If true only command will be send without reading any 
//!             bytes.
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void  rom_nand_hal_BuildReadDma(NAND_dma_read_t* pChain, 
                                uint32_t u32NandDeviceNumber, 
                                NAND_read_seed_t * pReadSeed,
                                bool bCmdOnly)
{
#ifdef SUPPORT_16BIT_NANDS
    uint32_t    uiGPMITransferSize;
#endif
    // CLE1 chain size is # columns + # Rows + CLE command.
    uint32_t iCLE1_Size = pReadSeed->uiAddressSize + 1;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Send the 2nd CLE.
    pChain->tx_cle1_addr_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->tx_cle2_dma);
    
    // Configure APBH DMA to push Read command (toggling CLE)
    // into GPMI_CTRL.
    // Transfer CLE1_SIZE (5) bytes to GPMI when GPMI ready.
    // Transfer CLE1 and 4 ADDRESS bytes to GPMI_CTRL0 (see command below)
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    pChain->tx_cle1_addr_dma.cmd.U = NAND_DMA_COMMAND_CMD(iCLE1_Size, 0, NAND_LOCK, 3);
    // Buffer Address Register holds Read Address command.
    //pChain->tx_cle1_addr_dma.bar = pReadSeed->tx_cle1_addr_buf;
    pChain->tx_cle1_addr_dma.bar = pReadSeed->tx_cle1_addr_buf;
    // Setup GPMI bus for first part of Read Command.  Need to set CLE
    // high, then send Read command (0x00), then clear CLE, set ALE high
    // send # address bytes (Column then row) [Type1=2; Type2 = 4].
    pChain->tx_cle1_addr_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber,
                           iCLE1_Size, BV_GPMI_CTRL0_ADDRESS_INCREMENT__ENABLED, ASSERT_CS);

    pChain->tx_cle1_addr_dma.gpmi_compare.U = (reg32_t)NULL;

    pChain->tx_cle1_addr_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Setup next command - wait.
    pChain->tx_cle2_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->wait_dma);
    // Configure APBH DMA to push 2nd Read command (toggling CLE)
    // into GPMI_CTRL.
    // Transfer CLE2_SIZE (1) bytes to GPMI when GPMI ready.
    // Transfer CLE2 byte to GPMI_CTRL0 (see command below)
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    
    pChain->tx_cle2_dma.cmd.U = NAND_DMA_COMMAND_CMD(1, 0, NAND_LOCK, 1);

    // Buffer Address Register holds tx_cle2 command
    pChain->tx_cle2_dma.bar = pReadSeed->tx_cle2_buf;
    // Setup GPMI bus for second part of Read Command.  Need to set CLE
    // high, then send Read2 command (0x30), then clear CLE.
    pChain->tx_cle2_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber,
                          1, BV_GPMI_CTRL0_ADDRESS_INCREMENT__DISABLED, ASSERT_CS);
    // tt_todo - does CSLock = 1 cause problem here?

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // Once we've received ready, need to receive data.
    pChain->wait_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->sense_dma);
    // Wait for Ready (No transfer count)
    pChain->wait_dma.cmd.U = NAND_DMA_WAIT4RDY_CMD;
    // If there is an error, load Timeout DMA sequence.
    //pChain->wait_dma.bar = 0x00;
    pChain->sense_dma.bar = (apbh_dma_gpmi1_t*)&(pChain->timeout_dma);
    // Send commands Wait for Ready to go high.
    pChain->wait_dma.gpmi_ctrl0.U = NAND_DMA_WAIT4RDY_PIO(u32NandDeviceNumber);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Now psense to see if a timeout has occurred.
    if (bCmdOnly)
        pChain->sense_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->success_dma);
    else
        pChain->sense_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->rx_data_dma);
    // Wait for Ready (No transfer count) - Do not decrement semaphore.
    pChain->sense_dma.cmd.U = NAND_DMA_SENSE_CMD(0);
    // If there is an error, load Timeout DMA sequence.
    pChain->sense_dma.bar = (apbh_dma_gpmi1_t*)&(pChain->timeout_dma);
    // Even though PIO is unused, set it to zero for comparison purposes.
    pChain->sense_dma.gpmi_ctrl0.U = 0;    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // Next step is to disable the ECC.
    pChain->rx_data_dma.nxt = (apbh_dma_gpmi1_t*) &pChain->rx_wait4done_dma;
    
    if (pReadSeed->bEnableHWECC)
    {
        // Configure APBH DMA to NOT read any bytes from the NAND into
        // memory using GPMI.  The ECC will become the Bus Master and 
        // write the read data into memory.
        // Wait for end command from GPMI before next part of chain.
        // Lock GPMI to this NAND during transfer.
        // NO_DMA_XFER - No DMA transfer occurs on APBH - see above.
        // Decrement Semaphore to indicate finished.
        pChain->rx_data_dma.cmd.U = NAND_DMA_RX_CMD_ECC(0, 0);
        // Save Data into buffer.
        pChain->rx_data_dma.bar = 0x00;               // This field isn't used.
        pChain->rx_data_dma.gpmi_compare.U = 0x00;    // This field isn't used.
        // Operate on 4 buffers (2K transfers)  Select which type of Decode - 4 bit or 8 bit.
        pChain->rx_data_dma.gpmi_eccctrl.U = NAND_DMA_ECC_CTRL_PIO(pReadSeed->uiECCMask, 0/*pReadSeed->zNANDEccParams.u32EccType*/);  

        pChain->rx_data_dma.gpmi_ecccount.B.COUNT = pReadSeed->uiReadSize;
        DBG_PRINTF("pChain->rx_data_dma.gpmi_ecccount.B.COUNT 0x%x\n", pChain->rx_data_dma.gpmi_ecccount.U);
    }
    else
    {
        // ECC is disabled. Configure DMA to write directly to memory.
        // Wait for end command from GPMI before next part of chain.
        // Lock GPMI to this NAND during transfer.
        pChain->rx_data_dma.cmd.U = NAND_DMA_RX_NO_ECC_CMD(pReadSeed->uiReadSize, 0);
        // Save Data into buffer.
        pChain->rx_data_dma.bar = (void *)(((uint32_t)pReadSeed->pDataBuffer) & 0xFFFFFFFC); // not sure if this is right...
        pChain->rx_data_dma.gpmi_compare.U = 0x00;    // This field isn't used.
        pChain->rx_data_dma.gpmi_eccctrl.U = 0;    // This field isn't used.
        pChain->rx_data_dma.gpmi_ecccount.U = 0;    // This field isn't used.
        DBG_PRINTF("2 pChain->rx_data_dma.gpmi_ecccount.B.COUNT 0x%x\n", pChain->rx_data_dma.gpmi_ecccount.U);
    }
    // Setup the data buffer.
    pChain->rx_data_dma.gpmi_payload.U = (((uint32_t)pReadSeed->pDataBuffer) & 0xFFFFFFFC);
    // And the Auxiliary buffer here.
    pChain->rx_data_dma.gpmi_auxiliary.U = (((uint32_t)pReadSeed->pAuxBuffer) & 0xFFFFFFFC);
    // Setup GPMI bus for Read Sector Result.  GPMI Read.
    // Read ReadSize words (16 or 8 bit) data
    // Note - althought the GPMI knows more than one byte/word may be
    //        sent, the APBH assumes bytes only.
#ifdef SUPPORT_16BIT_NANDS
    uiGPMITransferSize = (pReadSeed->uiWordSize == BV_GPMI_CTRL0_WORD_LENGTH__8_BIT)  ?
                        pReadSeed->uiReadSize : (pReadSeed->uiReadSize>>1);
    pChain->rx_data_dma.gpmi_ctrl0.U = NAND_DMA_RX_PIO(u32NandDeviceNumber,
                       pReadSeed->uiWordSize, uiGPMITransferSize);
#else
    pChain->rx_data_dma.gpmi_ctrl0.U = NAND_DMA_RX_PIO(u32NandDeviceNumber,
                       BV_GPMI_CTRL0_WORD_LENGTH__8_BIT, pReadSeed->uiReadSize);
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Disable the ECC then load Success DMA sequence.
    pChain->rx_wait4done_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->success_dma);    
    // Configure to send 3 GPMI PIO reads.    
    pChain->rx_wait4done_dma.cmd.U = NAND_DMA_DISABLE_ECC_TRANSFER;
    // Nothing to be sent.
    pChain->rx_wait4done_dma.bar = NULL;
    // Disable the Chip Select and other outstanding GPMI things.
    pChain->rx_wait4done_dma.gpmi_ctrl0.U = NAND_DMA_DISABLE_ECC_PIO(u32NandDeviceNumber);
    // Ignore the compare - we need to skip over it.
    pChain->rx_wait4done_dma.gpmi_compare.U = 0x00; 
    // Disable the ECC Block.
    pChain->rx_wait4done_dma.gpmi_eccctrl.U = 
                    BF_GPMI_ECCCTRL_ENABLE_ECC(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE); 
    // Disable bch mode
    //pChain->rx_wait4done_dma.gpmi_ctrl1.B.BCH_MODE = 0; 

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
    // Initialize the Terminator functions
    // Next function is null.
    pChain->success_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->success_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(0) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to success termination code.
    pChain->success_dma.bar = (void *) SUCCESS;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Next function is null.
    pChain->timeout_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->timeout_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(0) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to timeout termination code.
    pChain->timeout_dma.bar = (void *) ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT;

}


////////////////////////////////////////////////////////////////////////////////
//! \brief Build the abbreviated DMA to send a NAND Read Command to the device.
//!
//! This function builds the DMA descriptor for a NAND Read command to the 
//! device.  This function assumes the DMA has already been setup once so
//! only the parameters that change need to be updated.
//!
//! \param[in]  pChain Pointer to the DMA Chain.
//! \param[in]  u32NandDeviceNumber Physical NAND number to read.
//! \param[in]  pReadSeed Pointer to structure containing the details of the
//!             NAND Read Command (size, address, etc).
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void  rom_nand_hal_BuildQuickReadDma(NAND_dma_read_t* pChain, 
                                uint32_t u32NandDeviceNumber, 
                                NAND_read_seed_t * pReadSeed)
{
    uint32_t    uiGPMITransferSize;
    
    // Setup GPMI bus for first part of Read Command.  Need to set CLE
    // high, then send Read command (0x00), then clear CLE, set ALE high
    // send # address bytes (Column then row) [Type1=3; Type2 = 4].
    // Only thing that needs to be set here is which NAND to talk to.
    //pChain->tx_cle1_addr_dma.gpmi_ctrl0.B.CS = BF_GPMI_CTRL0_CS(u32NandDeviceNumber);        
    pChain->tx_cle1_addr_dma.gpmi_ctrl0.B.CS = u32NandDeviceNumber;        

    // Setup GPMI bus for second part of Read Command.  Need to set CLE
    // high, then send Read2 command (0x30), then clear CLE.
    // Only thing that needs to be set here is which NAND to talk to.
    //pChain->tx_cle2_dma.gpmi_ctrl0.B.CS = BF_GPMI_CTRL0_CS(u32NandDeviceNumber); 
    pChain->tx_cle2_dma.gpmi_ctrl0.B.CS = u32NandDeviceNumber; 
    
    // Only thing that needs to be set here is which NAND to talk to.
    //pChain->wait_dma.gpmi_ctrl0.B.CS = BF_GPMI_CTRL0_CS(u32NandDeviceNumber); 
    pChain->wait_dma.gpmi_ctrl0.B.CS = u32NandDeviceNumber; 
    
    if (pReadSeed->bEnableHWECC)
    {
        pChain->rx_data_dma.gpmi_eccctrl.B.BUFFER_MASK = pReadSeed->uiECCMask;
        pChain->rx_data_dma.gpmi_ecccount.B.COUNT = pReadSeed->uiReadSize;
        DBG_PRINTF("QRDMA pChain->rx_data_dma.gpmi_ecccount.B.COUNT 0x%x\n", pChain->rx_data_dma.gpmi_ecccount.U);

    }
    else
    {
        // ECC is disabled.
        pChain->rx_data_dma.cmd.B.XFER_COUNT = pReadSeed->uiReadSize;
        pChain->rx_data_dma.bar = pReadSeed->pDataBuffer;
        pChain->rx_data_dma.gpmi_eccctrl.U = 0;
        pChain->rx_data_dma.gpmi_ecccount.U = 0;
        DBG_PRINTF("QRDMA 2 pChain->rx_data_dma.gpmi_ecccount.B.COUNT 0x%x\n", pChain->rx_data_dma.gpmi_ecccount.U);
    }

    // Setup the data buffer.
    pChain->rx_data_dma.gpmi_payload.U = (((uint32_t)pReadSeed->pDataBuffer) & 0xFFFFFFFC);
    // And the Auxiliary buffer here.
    pChain->rx_data_dma.gpmi_auxiliary.U = (((uint32_t)pReadSeed->pAuxBuffer) & 0xFFFFFFFC);
    // Setup GPMI bus for Read Sector Result.  GPMI Read.
    // Read ReadSize words (16 or 8 bit) data
    // Note - althought the GPMI knows more than one byte/word may be
    //        sent, the APBH assumes bytes only.
    uiGPMITransferSize = (pReadSeed->uiWordSize == BV_GPMI_CTRL0_WORD_LENGTH__8_BIT)  ?
                        pReadSeed->uiReadSize : (pReadSeed->uiReadSize>>1);
    // Change only those values that need to be changed.
    //pChain->rx_data_dma.gpmi_ctrl0.B.CS = BF_GPMI_CTRL0_CS(u32NandDeviceNumber);   
    //pChain->rx_data_dma.gpmi_ctrl0.B.XFER_COUNT = BF_GPMI_CTRL0_XFER_COUNT(uiGPMITransferSize);   
    pChain->rx_data_dma.gpmi_ctrl0.B.CS = u32NandDeviceNumber;   
    pChain->rx_data_dma.gpmi_ctrl0.B.XFER_COUNT = uiGPMITransferSize;   

    // Disable the Chip Select and other outstanding GPMI things.
    pChain->rx_wait4done_dma.gpmi_ctrl0.B.CS = u32NandDeviceNumber;    
}


#ifdef WRITES_ALLOWED

////////////////////////////////////////////////////////////////////////////////
//! \brief      Build the Complete Program descriptor for the NAND.
//!
//! \fntype     Non-Reentrant
//!
//! Descriptor to write data to the NAND as a complete transaction.  This
//! descriptor sends a Command, Address, Command2 , then the data to the NAND.
//! This may be either a full Sector write (2112 bytes) or a partial sector
//! write depending upon the size passed in with the pSeed structure.
//!
//! \param[in]  pChain - pointer to the descriptor chain that gets filled.
//! \param[in]  u32NandDeviceNumber - Chip Select - NANDs 0-3.
//! \param[in]  u32AddressSize TBD
//! \param[in]  u32DataSize - Number of bytes of data to be written.
//! \param[in]  pNANDEccParams - Bch Ecc parameters.
//! \param[in]  pWriteBuffer - Data buffer to write to NAND.
//! \param[in]  pAuxBuffer - Auxillary buffer for use in write to NAND.
//!
//! \note       branches to Timeout or Success DMA upon completion.
//!             Data is written differently if 3700 is used because the
//!             ECC is tacked on, so ECC is not included here.
//!
//! \todo [PUBS] Define TBD parameter(s)
////////////////////////////////////////////////////////////////////////////////
void  rom_nand_hal_BuildProgramDma(NAND_dma_program_t* pChain, uint32_t u32NandDeviceNumber,
                             uint32_t u32AddressSize, uint32_t u32DataSize,
                             NAND_ECC_Params_t *pNANDEccParams, void* pWriteBuffer, 
                             void* pAuxBuffer)
{
    uint32_t u32EccDataSize = 0;
    uint32_t u32EccMask, u32EccType, u32Temp;

    // CLE1 chain size is # columns + # Rows + CLE command.
    uint32_t iCLE1_Size = u32AddressSize + 1;
    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Send CLE/ALE to the NAND.
    // Next Action - Send Data to the NAND.
    pChain->tx_cle1_addr_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->tx_data_dma);    
    // Configure APBH DMA to push Write command (toggling CLE)
    // into GPMI_CTRL.
    // Transfer CLE1_SIZE (5) bytes to GPMI when GPMI ready.
    // Transfer CLE1 and 4 ADDRESS bytes to GPMI_CTRL0 (see command below)
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    pChain->tx_cle1_addr_dma.cmd.U = NAND_DMA_COMMAND_CMD(iCLE1_Size,0,NAND_LOCK,3);
    // Buffer Address Register holds Read Address command.
    pChain->tx_cle1_addr_dma.bar = pChain->NandProgSeed.tx_cle1_addr_buf;
    // Setup GPMI bus for first part of Write Command.  Need to set CLE
    // high, then send Write command (0x80), then clear CLE, set ALE high
    // send 4 address bytes.
    pChain->tx_cle1_addr_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber,
                       iCLE1_Size, BV_GPMI_CTRL0_ADDRESS_INCREMENT__ENABLED, ASSERT_CS);    

    // Set compare to NULL.
    pChain->tx_cle1_addr_dma.gpmi_compare.U = (reg32_t)NULL;
    // Disable the ECC.
    pChain->tx_cle1_addr_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Send Data to the NAND.
    // Next Action -    3600 - Send the CLE2 command to start the write.
    //                  3700 - Send the Auxillary Data.

    pChain->tx_data_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->tx_auxdata_dma);
    // Configure APBH DMA to write size bytes into the NAND from
    // memory using GPMI.
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    // Calculate the number of 512 byte packets we want.

    // Calculate the ECC Mask for this transaction.  
    // Auxilliary = 0x100 set to request transfer to/from the Auxiliary buffer.
    // Buffer7 = 0x080 set to request transfer to/from buffer7.
    // Buffer6 = 0x040 set to request transfer to/from buffer6.
    // Buffer5 = 0x020 set to request transfer to/from buffer5.
    // Buffer4 = 0x010 set to request transfer to/from buffer4.
    // Buffer3 = 0x008 set to request transfer to/from buffer3.
    // Buffer2 = 0x004 set to request transfer to/from buffer2.
    // Buffer1 = 0x002 set to request transfer to/from buffer1.
    // Buffer0 = 0x001 set to request transfer to/from buffer0.
    // First calculate how many 512 byte buffers fit in here.
    // u32Temp is just data bytes
    u32EccType = BV_GPMI_ECCCTRL_ECC_CMD__ENCODE;
    u32Temp = ((pNANDEccParams->m_u32NumEccBlocksPerPage) * 
                pNANDEccParams->m_u32EccBlockNSize) +
                pNANDEccParams->m_u32EccBlock0Size +
                pNANDEccParams->m_u32MetadataBytes;

    u32EccDataSize = 0;
    
    u32EccMask = BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_PAGE;
    if( pChain->NandProgSeed.bEnableHWECC == false )
    {
        // if no ecc, then datasize should be full 2112 bytes
        u32EccDataSize = 0;
        u32DataSize = pChain->NandProgSeed.NandSizeVars.uiWriteSize;
        u32Temp=pChain->NandProgSeed.NandSizeVars.uiWriteSize;
    }
    // Setup the data buffer.
    pChain->tx_data_dma.gpmi_payload.U = (((uint32_t)pWriteBuffer));
    // And the Auxiliary buffer here.
    pChain->tx_data_dma.gpmi_auxiliary.U = (((uint32_t)pAuxBuffer));

    if(pChain->NandProgSeed.bEnableHWECC)
    {
        pChain->tx_data_dma.cmd.U = NAND_DMA_TXDATA_CMD(0, 0, 6, 0, NO_DMA_XFER);
    }
    else
    {
        pChain->tx_data_dma.cmd.U = NAND_DMA_TXDATA_CMD(u32Temp, 0, 4, 0, DMA_READ);
    }

    // If there is no ECC data to send, skip that descriptor.
    if (!u32EccDataSize)
    {
        pChain->tx_data_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->tx_cle2_dma);
        // Wait for End on this descriptor.
        if(pChain->NandProgSeed.bEnableHWECC)
        {
            pChain->tx_data_dma.cmd.B.WAIT4ENDCMD = 1;
            pChain->tx_data_dma.cmd.B.NANDWAIT4READY = 0;
        }
        else
        {
            pChain->tx_data_dma.cmd.B.WAIT4ENDCMD = 1;
        }
    }
    // Setup GPMI bus for Write Sector.  GPMI Write.
    // Write WriteSize words (16 or 8 bit) data
    // Note - althought the GPMI knows more than one byte/word may be
    //        sent, the APBH assumes bytes only.
    //uiGPMITransferSize = (pSeed->uiWordSize == BV_GPMI_CTRL0_WORD_LENGTH__8_BIT)  ?
    //                    pSeed->uiWriteSize : (pSeed->uiWriteSize>>1);
    if(pChain->NandProgSeed.bEnableHWECC)
    {
        pChain->tx_data_dma.gpmi_ctrl0.U = NAND_DMA_TXDATA_PIO(u32NandDeviceNumber,
                        BV_GPMI_CTRL0_WORD_LENGTH__8_BIT, (0));
    }
    else
    {
        pChain->tx_data_dma.gpmi_ctrl0.U = NAND_DMA_TXDATA_PIO(u32NandDeviceNumber,
                        BV_GPMI_CTRL0_WORD_LENGTH__8_BIT, (u32Temp+u32EccDataSize));
    }
    // Compare isn't used.
    pChain->tx_data_dma.gpmi_compare.U = (reg32_t)NULL;
    // Setup ECC for the appropriate size (4 or 8 bit correction)
    // Setup the mask for the ECC so it knows what to expect.
    if( pChain->NandProgSeed.bEnableHWECC )
    {
        pChain->tx_data_dma.gpmi_eccctrl.U = NAND_DMA_ECC_CTRL_PIO(u32EccMask, u32EccType);
        // Select the total number of bytes being sent.
        pChain->tx_data_dma.gpmi_ecccount.U = u32DataSize;
        
        pChain->tx_data_dma.gpmi_ecccount.U = pNANDEccParams->m_u32PageSize;

        DBG_PRINTF("pChain->tx_data_dma.gpmi_ecccount.U 0x%x\n", pChain->tx_data_dma.gpmi_ecccount.U);
    }
    else
    {
        // no ecc
        pChain->tx_data_dma.gpmi_eccctrl.U = 0;
        pChain->tx_data_dma.gpmi_ecccount.U = 0;
        DBG_PRINTF("2 pChain->tx_data_dma.gpmi_ecccount.U 0x%x\n", pChain->tx_data_dma.gpmi_ecccount.U);
    }
    // Set Buffer Address Register to WriteBuffer.
    pChain->tx_data_dma.bar = pWriteBuffer;
    

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Send Auxillary Data (Metadata/Redundant Area) to the NAND.
    // Next Action - Send the CLE2 command to start the write.
    pChain->tx_auxdata_dma.nxt = (apbh_dma_t*) &(pChain->tx_cle2_dma);
    // Configure APBH DMA to write size bytes into the NAND from
    // memory using GPMI.
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    // there is no auxillary data if bEnableHWECC is false.
    if( pChain->NandProgSeed.bEnableHWECC )
    {
        pChain->tx_auxdata_dma.cmd.U = NAND_DMA_TXDATA_CMD(u32EccDataSize, 0, 0, 1, DMA_READ);
    }
    // Set Buffer Address Register to WriteBuffer.
    pChain->tx_auxdata_dma.bar = pAuxBuffer;    

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Send CLE2 to the NAND.
    // Next Action - Wait for Write to complete.
    pChain->tx_cle2_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->wait_dma);
    // Configure APBH DMA to push Program command (toggling CLE)
    // into GPMI_CTRL.
    // Transfer CLE2 (Program) to GPMI_CTRL0 (see command below)
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    pChain->tx_cle2_dma.cmd.U = NAND_DMA_COMMAND_CMD(1,0,NAND_LOCK,3);
    // Set Buffer Address Register to Program command.
    pChain->tx_cle2_dma.bar = pChain->NandProgSeed.tx_cle2_buf;
    // Setup GPMI bus for first part of Write Command.  Need to set CLE
    // high, then send Program command (0x30), then clear CLE.
    pChain->tx_cle2_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber,
                       1, BV_GPMI_CTRL0_ADDRESS_INCREMENT__DISABLED, ASSERT_CS);
    // Set compare to NULL.
    pChain->tx_cle2_dma.gpmi_compare.U = (reg32_t)NULL;
    // Disable the ECC.
    pChain->tx_cle2_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Wait for the write to the NAND to complete.
    // Next Action - Determine whether timeout occurred.
    pChain->wait_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->sense_dma);
    // Wait for Ready (No transfer count)
    pChain->wait_dma.cmd.U = NAND_DMA_WAIT4RDY_CMD;
    // No Buffer Address Register entry.
    pChain->wait_dma.bar = 0x00;
    pChain->wait_dma.gpmi_ctrl0.U = NAND_DMA_WAIT4RDY_PIO(u32NandDeviceNumber);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Use Sense sequence to wait for ready, timeout if necessary.
    // Next Action - Send Status Command to query write status from NAND.
    pChain->sense_dma.nxt = (apbh_dma_t*)&(pChain->statustx_dma);
    // Wait for Ready (No transfer count)- Decrement semaphore.
    pChain->sense_dma.cmd.U = NAND_DMA_SENSE_CMD(0);
    // If failure occurs, branch to pTimeout DMA.
    //pChain->sense_dma.bar = (apbh_dma_gpmi1_t*)&APBH_PROGRAM_FAILED_DMA; 
    pChain->sense_dma.bar = (apbh_dma_t*)&(pChain->program_failed_dma);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Query the Status of the write command.
    // Next Action - Read the Write Status from the NAND.
    pChain->statustx_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->statusrx_dma);
    // Configure APBH DMA to push CheckStatus command (toggling CLE) 
    // into GPMI_CTRL.
    // Transfer NAND_READ_STATUS_SIZE (1) bytes to GPMI when GPMI ready.
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    pChain->statustx_dma.cmd.U = NAND_DMA_COMMAND_CMD(NAND_READ_STATUS_SIZE,0, NAND_LOCK, 3);
    // Point to structure where NAND Read Status Command is kept.
    pChain->statustx_dma.bar = &(pChain->NandProgSeed.u8StatusCmd);
    // Setup GPMI bus for first part of Read Status Command.  Need to 
    // set CLE high, then send Read Status command (0x70/71), then 
    // clear CLE.
    pChain->statustx_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber, 
          NAND_READ_STATUS_SIZE, BV_GPMI_CTRL0_ADDRESS_INCREMENT__DISABLED, ASSERT_CS);

    // Set compare to NULL.
    pChain->statustx_dma.gpmi_compare.U = (reg32_t)NULL;
    // Disable the ECC.
    pChain->statustx_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Read the Write Status from the NAND.    
    // Next Action - Determine pass or failure.
    pChain->statusrx_dma.nxt = (apbh_dma_gpmi2_t*)&(pChain->statbranch_dma);
    // Send a Read & Compare command to the NAND.
    pChain->statusrx_dma.cmd.U = NAND_DMA_RX_NO_ECC_CMD(1, 0);        
    // No DMA Transfer.
    pChain->statusrx_dma.bar = &(pChain->NandProgSeed.u16Status);
    // GPMI commands.
    pChain->statusrx_dma.gpmi_ctrl0.U = NAND_DMA_RX_PIO(u32NandDeviceNumber, 
                       BV_GPMI_CTRL0_WORD_LENGTH__8_BIT, 1);        

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Branch to appropriate result DMA.
    // Next Action - Success or Failure.
    //pChain->statbranch_dma.nxt = (apbh_dma_t*)&APBH_SUCCESS_DMA;
    pChain->statbranch_dma.nxt = (apbh_dma_t*)&(pChain->success_dma);
    // Based upon above Compare.
    pChain->statbranch_dma.cmd.U = NAND_DMA_SENSE_CMD(0);        
    // Failure.
    //pChain->sense_dma.bar = (apbh_dma_gpmi1_t*)&APBH_PROGRAM_FAILED_DMA; 
    pChain->sense_dma.bar = (apbh_dma_t*)&(pChain->program_failed_dma); 
    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
    // Initialize the Terminator functions
    // Next function is null.
    pChain->success_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->success_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(0) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to success termination code.
    pChain->success_dma.bar = (void *) SUCCESS;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Next function is null.
    pChain->program_failed_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->program_failed_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(0) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to timeout termination code.
    pChain->program_failed_dma.bar = (void *) ERROR_ROM_NAND_DRIVER_PROGRAM_FAILED;

}

////////////////////////////////////////////////////////////////////////////////
//! \brief      Build the Erase Block DMA descriptor for the NAND.
//!
//! \fntype     Non-Reentrant
//!
//! Build descriptor to erase the block.  This is a synchronous call.
//!
//! \param[in]  pChain - pointer to the descriptor chain that gets filled.
//! \param[in]  u32BlockAddressBytes TBD
//! \param[in]  u32NandDeviceNumber - Chip Select - NANDs 0-3.
//!
//! \note       branches to Success DMA upon completion.             
//!
//! \todo [PUBS] Define TBD parameter(s)
////////////////////////////////////////////////////////////////////////////////
void  rom_nand_hal_BuildEraseDma(NAND_dma_block_erase_t* pChain, 
                                 uint32_t u32BlockAddressBytes,
                                 uint32_t u32NandDeviceNumber)
{
    // CLE1 chain size is # Blocks + CLE command.
    uint32_t iCLE1_Size = u32BlockAddressBytes + 1;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Point to next command.
    pChain->tx_cle1_row_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->tx_cle2_dma);
    // Configure APBH DMA to push Erase command (toggling CLE) 
    // into GPMI_CTRL.
    // Transfer CLE1_SIZE (3) bytes to GPMI when GPMI ready.
    // Transfer CLE1 and the row address bytes to GPMI_CTRL0.
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.    
    pChain->tx_cle1_row_dma.cmd.U = NAND_DMA_COMMAND_CMD(iCLE1_Size, 0, NAND_LOCK,3); 
    // Buffer Address Register holds Erase Block command and address.
    pChain->tx_cle1_row_dma.bar = &(pChain->NandEraseSeed.tx_cle1_block_buf);
    // Setup GPMI bus for first part of Write Command.  Need to set CLE 
    // high, then send Write command (0x80), then clear CLE, set ALE high
    // send 4 address bytes. 
    pChain->tx_cle1_row_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber, 
                         iCLE1_Size, BV_GPMI_CTRL0_ADDRESS_INCREMENT__ENABLED, ASSERT_CS); 

    // Set compare to NULL.
    pChain->tx_cle1_row_dma.gpmi_compare.U = (reg32_t)NULL;
    // Disable the ECC.
    pChain->tx_cle1_row_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Setup 2nd transfer.
    // Setup next command - wait.
    pChain->tx_cle2_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->wait_dma);
    // Configure APBH DMA to push 2nd Erase command (toggling CLE) 
    // into GPMI_CTRL.
    // Transfer CLE2_SIZE (1) bytes to GPMI when GPMI ready.
    // Transfer CLE2 byte to GPMI_CTRL0 (see command below)
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    pChain->tx_cle2_dma.cmd.U = NAND_DMA_COMMAND_CMD(1, 0, NAND_LOCK,1);         
    // Buffer Address Register holds tx_cle2 command
    pChain->tx_cle2_dma.bar = &(pChain->NandEraseSeed.tx_cle2_buf);
    // Setup GPMI bus for second part of Erase Command.  Need to set CLE 
    // high, then send Erase2 command (0xD0), then clear CLE.
    pChain->tx_cle2_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber, 
                         1, BV_GPMI_CTRL0_ADDRESS_INCREMENT__DISABLED, ASSERT_CS);        

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Setup Wait for Ready descriptor.
    // Setup success DMA pointer.
    pChain->wait_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->sense_dma);
    // Setup Wait for Ready (No transfer count)
    pChain->wait_dma.cmd.U = NAND_DMA_WAIT4RDY_CMD;        
    // If there is an error, load Timeout DMA sequence.
    pChain->wait_dma.bar = 0x0;
    // Wait for Ready to go high.
    pChain->wait_dma.gpmi_ctrl0.U = NAND_DMA_WAIT4RDY_PIO(u32NandDeviceNumber); 

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Now use Sense sequence to wait for ready.
    pChain->sense_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->statustx_dma);
    // Wait for Ready (No transfer count)- Decrement semaphore.
    pChain->sense_dma.cmd.U = NAND_DMA_SENSE_CMD(0);
    // If failure occurs, branch to pTimeout DMA.
    //pChain->sense_dma.bar = (apbh_dma_gpmi1_t*)&APBH_PROGRAM_FAILED_DMA;
    pChain->sense_dma.bar = (apbh_dma_t*)&(pChain->program_failed_dma);
    // Even though PIO is unused, set it to zero for comparison purposes.
    pChain->sense_dma.gpmi_ctrl0.U = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Next link will Read Status.
    pChain->statustx_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->statusrx_dma);
    // Configure APBH DMA to push CheckStatus command (toggling CLE) 
    // into GPMI_CTRL.
    // Transfer NAND_READ_STATUS_SIZE (1) bytes to GPMI when GPMI ready.
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    pChain->statustx_dma.cmd.U = NAND_DMA_COMMAND_CMD(NAND_READ_STATUS_SIZE,0, NAND_LOCK,1);         
    // Point to structure where NAND Read Status Command is kept.
    pChain->statustx_dma.bar = &(pChain->NandEraseSeed.u8StatusCmd);
    // Setup GPMI bus for first part of Read Status Command.  Need to 
    // set CLE high, then send Read Status command (0x70/71), then 
    // clear CLE.
    pChain->statustx_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber, 
          NAND_READ_STATUS_SIZE, BV_GPMI_CTRL0_ADDRESS_INCREMENT__DISABLED, ASSERT_CS);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // Next Link determines SUCCESS or FAILURE.
    pChain->statusrx_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->statbranch_dma);
    // Send a Read & Compare command to the NAND.
    pChain->statusrx_dma.cmd.U = NAND_DMA_RX_NO_ECC_CMD(NAND_READ_STATUS_RESULT_SIZE, 0);        
    // No DMA Transfer.
    pChain->statusrx_dma.bar = &(pChain->NandEraseSeed.u16Status);
    // GPMI commands.
    pChain->statusrx_dma.gpmi_ctrl0.U = NAND_DMA_RX_PIO(u32NandDeviceNumber, 
                       BV_GPMI_CTRL0_WORD_LENGTH__8_BIT, NAND_READ_STATUS_RESULT_SIZE);  

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // Branch to appropriate result DMA.
    //pChain->statbranch_dma.nxt = (apbh_dma_t*)&APBH_SUCCESS_DMA;
    pChain->statbranch_dma.nxt = (apbh_dma_t*)&(pChain->success_dma);
    // Based upon above Compare.
    pChain->statbranch_dma.cmd.U = NAND_DMA_SENSE_CMD(0);        
    // Failure.
    //pChain->sense_dma.bar = (apbh_dma_gpmi1_t*)&APBH_PROGRAM_FAILED_DMA; 
    pChain->sense_dma.bar = (apbh_dma_t*)&(pChain->program_failed_dma);
    // Even though PIO is unused, set it to zero for comparison purposes.
    pChain->sense_dma.gpmi_ctrl0.U = 0;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
    // Initialize the Terminator functions
    // Next function is null.
    pChain->success_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->success_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(0) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to success termination code.
    pChain->success_dma.bar = (void *) SUCCESS;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Next function is null.
    pChain->program_failed_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->program_failed_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(0) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to timeout termination code.
    pChain->program_failed_dma.bar = (void *) ERROR_ROM_NAND_DRIVER_PROGRAM_FAILED;

}

////////////////////////////////////////////////////////////////////////////////
//! \brief      Build the Read Status DMA descriptor for the NAND.
//!
//! \fntype     Non-Reentrant
//!
//! Build descriptor to read the status.
//!
//! \param[in]  pChain - pointer to the descriptor chain that gets filled.
//! \param[in]  u32NandDeviceNumber - Chip Select - NANDs 0-3.
//! \param[out] pBuffer - Word to put resulting status into.
//!
//! \note       branches to Success DMA upon completion.
//!             Command is held in separate structure because 0x70 (Status)
//!             or 0x71 (Cache Status) may be used.
//!
////////////////////////////////////////////////////////////////////////////////
void  rom_nand_hal_BuildReadStatusDma(NAND_dma_read_status_t* pChain,
                                       uint32_t u32NandDeviceNumber,
                                       void* pBuffer)
{
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    pChain->tx_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->rx_dma);
    // Configure APBH DMA to push CheckStatus command (toggling CLE)
    // into GPMI_CTRL.
    // Transfer NAND_READ_STATUS_SIZE (1) bytes to GPMI when GPMI ready.
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.    
    pChain->tx_dma.cmd.U = NAND_DMA_COMMAND_CMD(NAND_READ_STATUS_SIZE,0,NAND_LOCK,3);
    // Point to byte where NAND Read Status Command is kept.
    pChain->tx_dma.bar = &(pChain->tx_cle1);
    // Setup GPMI bus for first part of Read Status Command.  Need to
    // set CLE high, then send Read Status command (0x70/71), then
    // clear CLE.
    pChain->tx_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber,
           NAND_READ_STATUS_SIZE,BV_GPMI_CTRL0_ADDRESS_INCREMENT__DISABLED, ASSERT_CS);

    // Set compare to NULL.
    pChain->tx_dma.gpmi_compare.U = (reg32_t)NULL;
    // Disable the ECC.
    pChain->tx_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Next dma chain is SUCCESS.
    //pChain->rx_dma.nxt = (apbh_dma_gpmi1_t*)&APBH_SUCCESS_DMA;
    pChain->rx_dma.nxt = (apbh_dma_gpmi1_t*)&(pChain->success_dma);
    // Read back 1 word.
    //pChain->rx_dma.cmd.U = NAND_DMA_RX_CMD(NAND_READ_STATUS_RESULT_SIZE,
    //                                           DECR_SEMAPHORE);
    pChain->rx_dma.cmd.U = NAND_DMA_RX_NO_ECC_CMD(NAND_READ_STATUS_RESULT_SIZE, 0);
    // Put result into pBuffer.
    pChain->rx_dma.bar = pBuffer;
    // Read NAND_STATUS_SIZE bytes from GPMI.
    pChain->rx_dma.gpmi_ctrl0.U = NAND_DMA_RX_PIO(u32NandDeviceNumber,
           BV_GPMI_CTRL0_WORD_LENGTH__8_BIT, NAND_READ_STATUS_RESULT_SIZE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
    // Initialize the Terminator functions
    // Next function is null.
    pChain->success_dma.nxt = (apbh_dma_t*) 0x0;
    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->success_dma.cmd.U = ((reg32_t) 
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(1) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
    // BAR points to success termination code.
    pChain->success_dma.bar = (void *) SUCCESS;
}

#endif      //#ifdef WRITES_ALLOWED
////////////////////////////////////////////////////////////////////////////////
//! \brief Start the DMA.
//!
//! This function kicks off a DMA transaction and records the start time.
//!
//! \param[in]  pDmaChain DMA chain to load and start..
//! \param[in]  u32NandDeviceNumber Physical NAND number to start.
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_StartDma(void *pDmaChain, uint32_t u32NandDeviceNumber)
{
    reg32_t channel_mask = (0x1 << (NAND0_APBH_CH+u32NandDeviceNumber));

#if BRAZO_ROM_FLASH
    // If we are running on brazo, send debugging info about GPMI to
    // CARROLIO port (16 bits) for use with a logic analyzer.  
    HW_CARROLIO_WR(0x00000000);
    HW_CARROLIO_WR(0x0000FEED);
#endif
    
    // soft reset dma chan, load cmd pointer and inc semaphore
    BW_APBH_CHANNEL_CTRL_RESET_CHANNEL(channel_mask);
    // Clear IRQ
    HW_APBH_CTRL1_CLR(channel_mask << BP_APBH_CTRL1_CH0_CMDCMPLT_IRQ);
    // Initialize DMA by setting up NextCMD field
    HW_APBH_CHn_NXTCMDAR_WR(NAND0_APBH_CH+u32NandDeviceNumber,(reg32_t)pDmaChain);
    // Start DMA by incrementing the semaphore.
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(NAND0_APBH_CH+u32NandDeviceNumber,1);

}

////////////////////////////////////////////////////////////////////////////////
//! \brief Wait for the DMA to complete.
//!
//! This function sends spins until the DMA is finished or times out.
//!
//! \param[in]  u32uSecTimeout How many microseconds to wait before timing out.
//! \param[in]  u32NandDeviceNumber Physical NAND number to wait on.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//! \retval ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT DMA timed out.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_WaitDma(uint32_t u32uSecTimeout, 
                                uint32_t u32NandDeviceNumber)
{
    uint32_t u32Start = ROM_GET_TIME_USEC();
    int32_t iComplete;
    reg32_t channel_mask = (0x1 << (NAND0_APBH_CH+u32NandDeviceNumber));

    // end of DMA chain will set IRQ.
    do {
        iComplete = (HW_APBH_CTRL1_RD() & (channel_mask));
    }
    while ((iComplete == 0) && (!ROM_TIME_USEC_EXPIRED(u32Start, u32uSecTimeout)));

    // if timeout return error, else return NXTCMDAR field from last DMA command
    if (iComplete == 0) {
        // abort dma by resetting channel
        BW_APBH_CHANNEL_CTRL_RESET_CHANNEL(channel_mask);
        return ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT;
    }
    else {
        return (RtStatus_t)BF_RDn(APBH_CHn_BAR, NAND0_APBH_CH+u32NandDeviceNumber, ADDRESS);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Get the DMA Status.
//!
//! \fntype Function
//!
//! This gets the status of a currently executing transaction and returns.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to check.
//! \param[in]  u32StartTime Timestamp of when DMA started.
//! \param[in]  u32DmaTimeout Number of usec to wait before timing out.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//! \retval ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT DMA timed out.
//! \retval ERROR_ROM_NAND_DMA_BUSY DMA is still busy.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_GetDmaStatus(uint32_t u32NandDeviceNumber, 
                                     uint32_t u32StartTime, 
                                     uint32_t u32DmaTimeout)
{
    int32_t iComplete;
    reg32_t channel_mask = (0x1 << (NAND0_APBH_CH+u32NandDeviceNumber));

    // end of DMA chain will set IRQ.
    iComplete = (HW_APBH_CTRL1_RD() & (channel_mask));
    
    // if IRQ set return NXTCMDAR field from last DMA command
    if (iComplete) 
    {
        // return status 
        if (BF_RDn(APBH_CHn_BAR, (NAND0_APBH_CH+u32NandDeviceNumber), ADDRESS) 
            != SUCCESS)
        {
            return ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT;  // This is the code for a Timeout.
        } else
            return SUCCESS;
	    
    } else
    {
        if (ROM_TIME_USEC_EXPIRED(u32StartTime, u32DmaTimeout))
        {
            return ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT;  // This is the code for a Timeout.
        }
        return ERROR_ROM_NAND_DMA_BUSY;  // This is the code for Busy.
    }
	
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Build the DMA to send ONFi NAND Read Parameters Page Cmd to device.
//!
//! This function sends a NAND Read Parameters Page command to the device.
//!
//! \param[in]  pChain Pointer to the DMA Chain.
//! \param[in]  u32NandDeviceNumber Physical NAND number to read.
//! \param[in]  pu8Buf Pointer to buffer.
//! \param[in]  u32BufLen Length of pu8Buf.
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_BuildParamsPageDmaDesc(NAND_dma_read_t *pChain, 
    uint32_t u32NandDeviceNumber, uint8_t* pu8Buf, uint32_t u32BufLen)
{
    // Current Action - Send the 1st CLE 
    pChain->tx_cle1_addr_dma.nxt = &(pChain->wait_dma);

    // Configure APBH DMA to push Read command (toggling CLE)
    // into GPMI_CTRL.
    // Transfer CLE1 to GPMI_CTRL0 (see command below)
    // Wait for end command from GPMI before next part of chain.
    // Lock GPMI to this NAND during transfer.
    // DMA_READ - Perform PIO word transfers then transfer
    //            from memory to peripheral for specified # of bytes.
    pChain->tx_cle1_addr_dma.cmd.U = NAND_DMA_COMMAND_CMD(2, 0, NAND_LOCK, 3);

    // Buffer Address Register holds Read Address command.
    pChain->tx_cle1_addr_dma.bar = (void*)pChain->NAND_DMA_Read_Seed.tx_cle1_addr_buf;

    // Setup GPMI bus for first part of Read Command.  Need to set CLE
    // high, then send Read command (0x00), then clear CLE, set ALE high
    // send # address bytes (Column then row) [Type1=2; Type2 = 4].
    pChain->tx_cle1_addr_dma.gpmi_ctrl0.U = NAND_DMA_COMMAND_PIO(u32NandDeviceNumber,
                           2, BV_GPMI_CTRL0_ADDRESS_INCREMENT__ENABLED, ASSERT_CS);

    pChain->tx_cle1_addr_dma.gpmi_compare.U = (reg32_t)NULL;

    pChain->tx_cle1_addr_dma.gpmi_eccctrl.U = NAND_DMA_ECC_PIO(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Setup next command - wait.
    // Next Action - Determine (Sense) whether a timeout occurred.
    // Once we've received ready, need to receive data.
    pChain->wait_dma.nxt = &(pChain->sense_dma);

    // Wait for Ready (No transfer count)
    pChain->wait_dma.cmd.U = NAND_DMA_WAIT4RDY_CMD;

    // If there is an error, load Timeout DMA sequence.
    pChain->sense_dma.bar = &(pChain->timeout_dma);

    // Send commands Wait for Ready to go high.
    pChain->wait_dma.gpmi_ctrl0.U = NAND_DMA_WAIT4RDY_PIO(u32NandDeviceNumber);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Determine (Sense) whether a timeout occurred.
    // Next Action - Receive Data from the NAND.
    // Now psense to see if a timeout has occurred.
    pChain->sense_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->rx_data_dma);

    // Wait for Ready (No transfer count) - Do not decrement semaphore.
    pChain->sense_dma.cmd.U = NAND_DMA_SENSE_CMD(0);

    // If there is an error, load Timeout DMA sequence.
    pChain->sense_dma.bar = &(pChain->timeout_dma);

    // Even though PIO is unused, set it to zero for comparison purposes.
    pChain->sense_dma.gpmi_ctrl0.U = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Receive Data from the NAND.

    // Next step is to disable the ECC.
    pChain->rx_data_dma.nxt = (apbh_dma_gpmi1_t*) &pChain->rx_wait4done_dma;

    pChain->rx_data_dma.cmd.U = NAND_DMA_RX_NO_ECC_CMD(u32BufLen, 0);

    // Save Data into buffer.
    pChain->rx_data_dma.bar = (void*) ((uint32_t)pu8Buf & 0xFFFFFFFC);

    pChain->rx_data_dma.gpmi_ctrl0.U = NAND_DMA_RX_PIO(u32NandDeviceNumber,
                       BV_GPMI_CTRL0_WORD_LENGTH__8_BIT, u32BufLen);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Disable ECC (3700)
    // Next Action - 3700 will return Success.
    // Disable the ECC then load Success DMA sequence.
    pChain->rx_wait4done_dma.nxt = (apbh_dma_gpmi1_t*) &(pChain->success_dma);

    // Configure to send 3 GPMI PIO reads.
    pChain->rx_wait4done_dma.cmd.U = NAND_DMA_DISABLE_ECC_TRANSFER;

    // Nothing to be sent.
    pChain->rx_wait4done_dma.bar = NULL;

    // Disable the Chip Select and other outstanding GPMI things.
    pChain->rx_wait4done_dma.gpmi_ctrl0.U = NAND_DMA_DISABLE_ECC_PIO(u32NandDeviceNumber);

    // Ignore the compare - we need to skip over it.
    pChain->rx_wait4done_dma.gpmi_compare.U = 0x00;

    // Disable the ECC Block.
    pChain->rx_wait4done_dma.gpmi_eccctrl.U =
                    BF_GPMI_ECCCTRL_ENABLE_ECC(BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Success
    // Initialize the Terminator functions
    // Next function is null.
    pChain->success_dma.nxt = (apbh_dma_t*) 0x0;

    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->success_dma.cmd.U = ((reg32_t)
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(0) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));

    // BAR points to success termination code.
    pChain->success_dma.bar = (void *) SUCCESS;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Current Action - Timeout
    // Next function is null.
    pChain->timeout_dma.nxt = (apbh_dma_t*) 0x0;

    // Decrement semaphore, set IRQ, no DMA transfer.
    pChain->timeout_dma.cmd.U = ((reg32_t)
                                 (BF_APBH_CHn_CMD_IRQONCMPLT(1) | \
                                  BF_APBH_CHn_CMD_WAIT4ENDCMD(0) | \
                                  BF_APBH_CHn_CMD_SEMAPHORE(1) | \
                                  BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));

    // BAR points to timeout termination code.
    pChain->timeout_dma.bar = (void *) ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT;
}
// end nand_dma.c
//! @}
