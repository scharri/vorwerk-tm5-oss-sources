////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_hal
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_hal_api.h
//! \brief Provides API routines for interfacing with the APBH and GPMI
//!                 blocks to NAND in DMA mode.
////////////////////////////////////////////////////////////////////////////////
#ifndef NAND_HAL_API_H
#define NAND_HAL_API_H 1

#include "return_codes.h"
#include "rom_types.h"


////////////////////////////////////////////////////////////////////////////////
//! Definitions
////////////////////////////////////////////////////////////////////////////////
//! This define sets the maximum number of NANDs we need to search for. 
#define MAX_NUMBER_NANDS    1

//! This define sets the typical read size for the HAL function.
#define TYPICAL_NAND_READ_SIZE              2048
#define TYPICAL_NAND_TOTAL_PAGE_SIZE        2112

#define NAND_CMD_COMMON_READ_ID             0x90    //!< Common Read code for all NANDs
#define NAND_CMD_COMMON_RESET               0xFF    //!< Common Reset code for all NANDs

#define NAND_ENABLE_WRITE_PROTECT           0       //!< Negative Logic so 0 
#define NAND_DISABLE_WRITE_PROTECT          1       //!< Negative Logic so 1


//! \brief Nand boot driver efuse structure.
//!
//! This structure is used to track data extracted from the eFuses.  Currently,
//! we track how many NANDs are in use (we only pay attention to the first 2), 
//! how many 64 page chunks to search for the boot blocks, what type of ECC
//! should be used (4bit or 8bit) and whether the Alternate Chip Enables are 
//! required.
#pragma alignvar(4)
typedef struct _efNAND_t
{
    uint32_t efNANDsInUse;              //!< How many NANDs are in the system (only 1st 2 matter)
    uint32_t efNANDBootSearchStride;    //!< What is the search stride (default 64 pages)?
    uint32_t efNANDBootSearchLimit;     //!< How many search stride pages (default 64) should be searched?
    uint32_t efNANDRowAddressBytes;     //!< How many bytes for row address
    uint32_t efNANDColumnAddressBytes;  //!< How many bytes for colimn address
    uint32_t efNANDReadCmdCode1;        //!< What is cmd code1 for read operation
    uint32_t efNANDReadCmdCode2;        //!< What is cmd code2 for read operation
    uint32_t efAltCEPinConfig;          //!< Use alternate Chip Enables instead of Primary CEs and 
    uint32_t efEnableIntPullups;        //! Enable PULLUP_GPMI_CE and PULLUP_GPMI_RDY
    uint32_t efBIPreserve;              //! Switch in OTP to enable or disable swapping for BB
    uint32_t efDisableSecondaryBoot;
} efNAND_t;

//! \brief NAND Timing structure for setting up the GPMI timing.
//!
//! This structure holds the timing for the NAND.  This data is used by
//! rom_nand_hal_GpmiSetNandTiming to setup the GPMI hardware registers.
typedef struct _NAND_Timing
{        
    uint8_t m_u8DataSetup;
    uint8_t m_u8DataHold;
    uint8_t m_u8AddressSetup;
    uint8_t m_u8DSAMPLE_TIME;
// These are for application use only and not for ROM.
    uint8_t m_u8NandTimingState; 
    uint8_t m_u8REA;
    uint8_t m_u8RLOH;
    uint8_t m_u8RHOH;
} NAND_Timing_t;

//! \brief NAND DMA Time structure.
//!
//! This structure is used to keep track of the DMA start time and how long the
//! DMA is expected to last.  This is used to determine if a DMA Timeout has 
//! occurred.
typedef struct _NandDmaTime_t
{
    uint32_t   uStartDMATime;
    uint32_t   uDMATimeout;
} NandDmaTime_t;

////////////////////////////////////////////////////////////////////////////////
//! Prototypes
////////////////////////////////////////////////////////////////////////////////

RtStatus_t rom_nand_hal_Init(void * pNandEFuse);

RtStatus_t rom_nand_hal_HWInit(void * pNandEFuse);

RtStatus_t rom_nand_hal_Shutdown(void);

void rom_nand_hal_GpmiSetNandTiming(void * pNewNANDTiming, uint32_t GpmiPeriod);

void rom_nand_hal_ConfigurePinmux(bool bUse16BitData, uint32_t u32ChipSelectReadyMask,
                                //   uint32_t efAltCEPinConfig,
                                   uint32_t efEnableIntPullups,  bool bUse1_8V_Drive);

void rom_nand_hal_InitReadDma(  void * pReadDmaDescriptor, 
                                uint32_t u32NumRowBytes, 
                                uint32_t u32NumColBytes, 
                                uint32_t u32BusWidth,
                                uint32_t u32ECCSize,
                                uint32_t u32ReadCode1,
                                uint32_t u32ReadCode2);

void rom_nand_hal_UpdateDmaDescriptor(void);

RtStatus_t rom_nand_hal_ReadNand(void * pReadDmaDescriptor, 
                                 uint32_t u32NandDeviceNumber, 
                                 uint32_t u32ColumnOffset, 
                                 uint32_t u32PageNum, 
                                 uint32_t u32ReadSize,
                                 uint8_t *p8PageBuf,
                                 uint8_t *p8AuxillaryBuf);

RtStatus_t rom_nand_hal_ReadPage(uint32_t u32NandDeviceNumber, 
                        uint32_t u32PageNum, uint8_t *p8PageBuf);

RtStatus_t rom_nand_hal_WaitForReadComplete(uint32_t u32NandDeviceNumber,
                                            uint32_t u32EccThreshold);

RtStatus_t rom_nand_hal_CheckECCStatus(uint32_t u32NandDeviceNumber, 
                                       uint32_t u32Threshold);

RtStatus_t rom_nand_hal_CurrentGpmiDmaStatus(uint32_t u32NandDeviceNumber);

RtStatus_t rom_nand_hal_ReadNandID(uint32_t u32NandDeviceNumber, 
                                   uint8_t * pReadIDCode);

RtStatus_t rom_nand_hal_ResetNAND(uint32_t u32NandDeviceNumber);

// Expose DMA start and wait functions to external world
void rom_nand_hal_StartDma(void *pDmaChain, uint32_t u32NandDeviceNumber);

RtStatus_t  rom_nand_hal_WaitDma(uint32_t u32uSecTimeout, uint32_t u32NandDeviceNumber);

bool rom_nand_hal_IsBlockBad(uint32_t u32Block, uint8_t* u8Buf);

#ifdef WRITES_ALLOWED

typedef struct _NANDInfo_t
{
    uint32_t    m_uBlocks;
    uint32_t    m_uSectorsPerBlock;
    int32_t     m_rtLastError;
} NANDInfo_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief Read 512 bytes + ECC from the NAND.
//!
//! \fntype Function
//!
//! This function will write a complete page to the NAND.  It must be able
//! to concatenate 512 chunks into a complete 2K or 4K write.  This function is
//! not actually in the ROM, it used only for verification testing.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to initialize.
//! \param[in]  u32DPageNum Physical page to read..
//! \param[out] p8PageBuf Buffer pointer where the data will be placed.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//! \post If succesful, the data is in pPageBuf.
//!
//!
//! \internal
//! To view function details, see rom_nand_hal_api.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_WriteSector(uint32_t u32NandDeviceNumber, 
                          uint32_t u32PageNum, uint8_t *p8PageBuf);

////////////////////////////////////////////////////////////////////////////////
//! \brief Erase a block the NAND.
//!
//! \fntype Function
//!
//! This function will erase a block on the NAND.  This function is not
//! actually in the ROM, it used only for verification testing.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to initialize.
//! \param[in]  u32BlockNum Physical Block to erase.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//! \post If succesful, the data is in pPageBuf.
//!
//!
//! \internal
//! To view function details, see rom_nand_hal_api.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_EraseBlock(uint32_t u32NandDeviceNumber, uint32_t u32BlockNum);

////////////////////////////////////////////////////////////////////////////////
//! \brief Set or Clear the Write Protect bit.
//!
//! \fntype Function
//!
//! This function sets or clears the Write Protect Enable bit which is 0 if
//! Write Protect is enabled, and 1 if Write Protect is disabled.
//!
//! \param[in]  bDisableEnable 0 if Disable write, 1 if Enable write.
//!
//! \return void
//!
//! \internal
//! To view function details, see rom_nand_hal_gpmi.c.
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_SetClearWriteProtect(uint32_t bDisableEnable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Retrieve the characteristics of the NAND.
//!
//! \fntype Function
//!
//! This function will return the characteristics of the NAND needed for 
//! determining how a boot should proceed.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to initialize.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//!
//! \internal
//! To view function details, see rom_nand_hal_api.c.
////////////////////////////////////////////////////////////////////////////////
NANDInfo_t *rom_nand_hal_GetNandInfo(uint32_t u32NandDeviceNumber);

////////////////////////////////////////////////////////////////////////////////
//! \brief Write a sector to the NAND.
//!
//! \fntype Function
//!
//! This function will write the data to the PageNum in the NAND.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND chip number to write.
//! \param[in]  u32PageNum Physical NAND page number to write.
//! \param[in]  p8PageBuf Buffer to be sent to the NAND.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//!
//! \internal
//! To view function details, see rom_nand_hal_api.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_WriteSectorData(uint32_t u32NandDeviceNumber, 
                                        uint32_t u32PageNum, 
                                        uint8_t *p8PageBuf);

////////////////////////////////////////////////////////////////////////////////
//! \brief Erase a block on the NAND.
//!
//! \fntype Function
//!
//! This function will erase a block on the NAND.
//! determining how a boot should proceed.
//!
//! \param[in]  u32NandDeviceNumber Physical NAND number to erase.
//! \param[in]  u32BlockNum Physical NAND block to erase.
//!
//! \return Status of call or error.
//! \retval 0            If no error has occurred.
//!
//!
//! \internal
//! To view function details, see rom_nand_hal_api.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_Erase_Block(uint32_t u32NandDeviceNumber, uint32_t u32BlockNum);

#endif  //#ifdef WRITES_ALLOWED

#endif

//! @}
