////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom
//! @{
//!
// Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    itf_define.h
//! \brief   Defines all public Encore ROM interfaces.
//!
//! This file provides definitions for all public ROM interfaces that are
//! accessed through the ROM patch table.
//!
////////////////////////////////////////////////////////////////////////////////
#ifndef _ITF_DEFINE_H
#define _ITF_DEFINE_H

#include <stdint.h>
#include <stdbool.h>

//! This structure defines the parameter passed to the rom_BootInit function.
//! 
typedef struct
{
    void *pMem;         //!< Pointer to memory region for use by the driver.
    int size;           //!< Size of the memory region.
} MemoryPool_t ;

//------------------------------------------------------------------------------
// Generic Boot Driver Interface
//------------------------------------------------------------------------------

//! Defines the number of bytes in a cipher block (chunk). This is dictated by
//! the encryption algorithm.
#define BYTES_PER_CHUNK 16

//! Defines the unit of data that is returned by the rom_BootNext function.
//! From the boot driver's view, an image file looks like an array of these
//! data units (chunks). The size of a chunk is dictated by the cipher algorithm.
//! The Encore ROM will use AES-128 block encryption. Thus a chunk is defined as
//! 128 bits or 16 bytes. While any alignment is supported, performance is best
//! if chunks are word (32-bit) aligned.
typedef uint8_t chunk_t[BYTES_PER_CHUNK];

//! This structure defines the parameter passed to the rom_BootInit function.
//! \note   The memory region pMem[size] is word aligned. Boot drivers must
//!         handle stricter alignment requirements internally.
typedef struct
{
    void *pMem;         //!< Pointer to memory region for use by the driver.
    int size;           //!< Size of the memory region.
    uint32_t mode;      //!< Current boot mode.
    uint32_t id;        //!< ID of the boot image. Always 0 on the first call.
}
rom_BootInit_t;

//! This enumeration defines the first parameter passed to the rom_BootControl
//! function. It specifies the requested driver control action.
typedef enum
{
    BOOT_PAUSE,         //!< Pause all active operations
    BOOT_RESUME         //!< Update driver settings and resume operations
}
rom_BootAction_t;

//! Standard boot driver API. All boot devices \b must include an instance of
//! this structure as the first element of it's interface structure.
typedef struct _rom_BootItf
{
    int (*Init)(rom_BootInit_t *);
    chunk_t * (*Next)(int *);
    int (*Skip)(int);
    int (*Stop)(void);
    int (*Control)(rom_BootAction_t, void *);
    int nRedundantBootSupported;
}
rom_BootItf_t;


//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

struct _i2c_Context;
struct _nor_Context;
struct _rom_nand_Context;
struct _ssp_Context;
struct _i2c_dma_cmd;
struct _ssp_Cfg_t;              // Configuration struct
struct _ssp_ClockConfig;         // Table Lookup
struct _spi_Context;
struct _sd_Context;

//------------------------------------------------------------------------------
// USB Boot Driver Interface
//------------------------------------------------------------------------------

//! USB boot driver interface definition.
typedef struct _usb_BootItf
{
    rom_BootItf_t Boot;                 //!< First member must be the standard API
    int (*install_pitc)(void *arg);     //!< Provides PITC access to BLTC 
    int (*uninstall_pitc)(void *arg);   //!< Provides PITC access to BLTC 
    uint16_t *** pppStringList;         //!< For patching the string tables
}
usb_BootItf_t;


//------------------------------------------------------------------------------
// I2C Boot Driver Interface
//------------------------------------------------------------------------------

//! I2C boot driver interface definition.
typedef struct _i2c_BootItf
{
    rom_BootItf_t Boot;                 //!< First member must be the standard API
    struct _i2c_Context ** pContext;        //!< Pointer to context pointer global.
    int (*I2CHAL_Init)(void);
    int (*I2CHAL_EnableDMA)(void);
    int (*I2CHAL_SetReadAddr)(int iStartByte);
    int (*I2CHAL_SetupAndStartDMA)(struct _i2c_dma_cmd * pChain);
    int (*I2CHAL_IsDMADone)(void);
    int (*I2CHAL_WaitForDMADone)(void);
    int (*I2CHAL_QueueRead)(int iStartByte, char * pData, int iSize);
    int (*I2CHAL_Disable)(void);
}
i2c_BootItf_t;

//------------------------------------------------------------------------------
// SSP Boot Driver Interface
//------------------------------------------------------------------------------

//! SSP boot driver interface definition.
typedef struct _ssp_BootItf
{
    struct _ssp_Context ** pContext;      //!< Pointer to context pointer global.
    struct _ssp_ClockConfig ** pSckTable;   //!< Table of 16 SCK entries

    // HAL API
    int (*Init)(struct _ssp_Cfg_t * pConfig);
    int (*Shutdown)(void);
    int (*Ctrl)(int command, void * pParameter);

    // CNTL_IO Functions
    int (*Clock)(void*pParam);
    int (*ClockCfg)(void*pParam);
    int (*Drive)(void*pParam);
    int (*Timeout)(void*pParam);
    int (*BusWidth)(void*pParam);
    int (*SdStatus)(void*pParam);
    int (*CmdPullups)(void*pParam);
    int (*DataPullups)(void*pParam);
    int (*DmaStart)(void*pParam);
    int (*DmaEnd)(void*pParam);
    int (*DmaWait)(void*pParam);
    int (*DmaStatus)(void*pParam);
    int (*ClockFindIndex)(void*pParam);
    int (*DmaAdd)(void*pParam);
}
ssp_BootItf_t;

//------------------------------------------------------------------------------
// SPI Boot Driver Interface
//------------------------------------------------------------------------------

//! SPI boot driver interface definition.
typedef struct _spi_BootItf
{
    rom_BootItf_t Boot;                 //!< First member must be the standard API
    int (*IoCtrl)(int command, void * pParameter);
    struct _spi_Context ** pContext;    //!< Pointer to context pointer global.
}
spi_BootItf_t;

//------------------------------------------------------------------------------
// SD Boot Driver Interface
//------------------------------------------------------------------------------

//! SD boot driver interface definition.
typedef struct _sd_BootItf
{
    rom_BootItf_t Boot;                 //!< First member must be the standard API
    struct _sd_Context ** pContext;    //!< Pointer to context pointer global.
    int (*fpSd_ddi_ProbeForDev)(void * pDevice);
    int (*fpSd_ddi_SetBusWidth)(void * pDevice, int Width, bool bDDR_enable);
    int (*fpSd_ddi_SelectBusWidth)(void * pDevice, bool bDDR_enable);
    int (*fpSd_ddi_Identify)(void * pDevice);
    int (*fpSd_ddi_CheckOpCondVoltageRange)(void * pDevice);
    int (*fpSd_ddi_SetDataSpeed)(void * pDevice, bool highspeedready);
    int (*fpSd_ddi_MultiBlockRead)(void * pDevice, uint64_t u64ByteAddress, uint8_t * pBuffer);
    int (*fpSd_ddi_MultiBlockReadStop)(void * pDevice);
    int (*fpSd_ddi_MultiBlockReadStatus)(void * pDevice, int * MbrStatus);
    int (*fpSd_hal_InitSequence)(void * pDevice);
    int (*fpSd_hal_Cmd0GoIdle)(void * pDevice);               
    int (*fpSd_hal_Cmd2SendCid)(void * pDevice);
    int (*fpSd_hal_Cmd3SdSendRelAddr)(void * pDevice);          
    int (*fpSd_hal_Cmd3SetRelAddr)(void * pDevice, uint32_t Rca);
    int (*fpSd_hal_Cmd6Switch)(void * pDevice, void * SwitchArg);
    int (*fpSd_hal_Cmd7Select)(void * pDevice, bool bSelectCard);
    int (*fpSd_hal_Cmd8SendExtCsd)(void * pDevice);
    int (*fpSd_hal_Cmd8SendIfCond)(void * pDevice);
    int (*fpSd_hal_Cmd9SendCsd)(void * pDevice);
    int (*fpSd_hal_Cmd12StopTransmission)(void * pDevice);
    int (*fpSd_hal_Cmd13SendStatus)(void * pDevice);
    int (*fpSd_hal_Cmd55AppCmd)(void * pDevice, uint16_t u16Rca, void * pResponse);
    int (*fpSd_hal_SendOpCond)(void * pDevice, uint32_t u32CmdArg, int Type, void * sResponse);
    int (*fpSd_hal_Acmd51SdSendScr)(void * pDevice);
    int (*fpSd_hal_MmcSetHighSpeed)(void * pDevice);
    int (*fpSd_hal_SdSetHighSpeed)(void * pDevice);
    int (*fpSd_hal_SetBusWidth)(void * pDevice, int Width, bool bDDR_enable);
    int (*fpSd_hal_CmdR1)(void * pDevice, uint32_t u32Cmd, uint32_t u32Arg, bool bWaitForIrq, bool bWaitForCmd, void*  sResp);
    int (*fpSd_hal_CmdR2)(void * pDevice, uint32_t u32Cmd, uint32_t u32Arg, bool bWait, void*  pResp);
    int (*fpSd_hal_DmaR1Read)(void * pDevice, uint32_t u32Cmd, uint32_t u32Arg, bool bWait, void*  sResp, uint32_t u32WordLength, uint32_t * pWordBuffer);
    int (*fpSd_hal_BlockRead)(void * pDevice, uint32_t u32BlockAddr, uint32_t * pDataBuffer, uint32_t u32WordLength, void* pResponse);
    int (*fpSd_hal_StartDma)(void * pDevice, bool bIgnoreCrc, bool bRead, bool bCmd, uint16_t  u16ByteCount, uint32_t  u32Cmd, uint32_t  u32Arg, uint32_t *p32Buffer);
    int (*fpSd_hal_CheckErrors)(void * pDevice);
    int (*fpSd_hal_ClearErrors)(void * pDevice);
    int (*fpeMMC_ddi_Fastboot_BlockRead)(void * pDevice, uint64_t u64BlockCount, bool skip, uint8_t * pBuffer);
    int (*fpsd_ddi_ReadRegisters)(void * pDevice);
    int (*fpSd_ddi_Read_SCR_register)(void * pDevice);
    int (*fpsd_hal_Cmd43SelectPartition)(void * pDevice);
    int (*fpsd_hal_MmcSetbootpartition)(void * pDevice, uint32_t bootpart);
}
sd_BootItf_t;

//------------------------------------------------------------------------------
// NAND Boot Driver Interface
//------------------------------------------------------------------------------

//! NAND boot driver interface definition.
typedef struct _nand_BootItf_t
{
    rom_BootItf_t Boot;                 //!< First member is always the standard API
    struct _rom_nand_Context ** pContext;        //!< Pointer to context pointer global.
    int (*BootBlockSearch)(uint32_t u32NandDeviceNumber, const void *pFingerPrintValues, uint32_t * p32SearchSector, uint8_t * pBuffer);
    bool (*BlockIsInBadBlockTable)(uint32_t u32BlockToMatch);
    int (*FindFCB)(uint32_t u32CurrentNAND, uint32_t * pReadSector, uint8_t * pBuffer);
    int (*ReadBadBlockTable)(uint32_t u32CurrentNAND, uint32_t * pSector, uint8_t * pBuffer);
    int (*ReadMediaInit)(uint8_t * pBuf);
    int (*ReadMedia)(uint8_t * pBuf);   //!< Start the NAND Read.
    int (*ReadComplete)(void);          //!< Wait for Read Complete
    int (*SkipSectors)(unsigned uSectorCount, uint8_t* pu8Buf);  //!< Skip a number of sectors.
    int (*FindNextGoodBlock)(uint8_t*);
    int (*HalInit)(void*);         //!< Initialize the HAL Layer
    int (*HalGpmiHWInit)(void *);     //!< Initialize the HAL GPMI hardware.
    void (*HalGpmiConfigurePinmux)(bool bUse16BitData, 
                                  uint32_t u32NumberOfNANDs,
                                  // uint32_t efAltCEPinConfig,
                                   uint32_t efEnableIntPullups,
                                   bool bUse1_8V_Drive); //!< Configure NAND Pinmux (GPMI)
    int (*HalReadPage)(uint32_t, uint32_t, 
                   uint8_t * pBuf);     //!< Read 2K bytes from NAND

    void (*HalInitReadDma)(  void * pReadDmaDescriptor, 
                            uint32_t u32NumRowBytes, 
                            uint32_t u32NumColBytes, 
                            uint32_t u32BusWidth,
                            uint32_t u32ECCSize,
                            uint32_t u32ReadCode1,
                            uint32_t u32ReadCode2); //!< Inititialize Read DMA descriptor.
    int (*HalReadNand)(void * pReadDmaDescriptor, 
                         uint32_t u32NandDeviceNumber, 
                         uint32_t u32ColumnOffset, 
                         uint32_t u32PageNum, 
                         uint32_t u32ReadSize,
                         uint8_t *p8PageBuf,
                         uint8_t *p8AuxillaryBuf);  //!< Generic NAND Read Function
    int (*HalWaitForReadComplete)(uint32_t u32NandDeviceNumber, //!< Wait for the Read to complete
                                  uint32_t u32EccThreshold);     
    int (*HalDmaStatus)(uint32_t);      //!< Return the current DMA status
    void (*HalUpdateGPMITiming)(void * pTiming, //!< Update the GPMI to new settings.
                          uint32_t GpmiClkPeriod); 
    int (*HalECCStatus)(uint32_t, uint32_t); //!< Check the ECC Status after a read.
    int (*HalReset)(uint32_t);               //!< Send the NAND Reset command.
    int (*HalReadID)(uint32_t, uint8_t *);   //!< Send the common NAND Read ID.
    void (*HalStartDma)(void * pDmaChain,    //!< Kick off a NAND Dma.
                        uint32_t u32NandDeviceNumber);
    int (*HalWaitDma)(uint32_t u32uSecTimeout, //!< Wait for NAND Dma to finish.
                      uint32_t u32NandDeviceNumber);
    void (*HalUpdateDmaDescriptor)(void);   //!< Update Read Dma Descriptor with new values.
    int (*HalShutdown)(void);           //!< Shutdown the HAL layer
    int (*BANandHalSendReadCmd)( uint32_t u32NandDeviceNumber, 
                            uint32_t u32SectorCount, 
                            uint32_t u32SectorAddress, 
                            uint8_t *p8PageBuf); //!< Send read command
//    int (*BANandHalRead2k)( uint32_t u32NandDeviceNumber, 
//                            uint32_t u32SectorAddress, 
//                            uint8_t *p8PageBuf); //!< Read 2k from sector specified in SendReadCmd
    int (*BANandReadMediaInit)( uint8_t * pBuf); //!< Find config blocks for ba nand
//    int (*BANandHalAbort)( uint32_t u32NandDeviceNumber); //!< Abort last read command for ba nand
    int (*ReadMBR)(uint8_t *pMbr, uint32_t *pu32StartLoc); //!< Reads MBR, first block and validates it.
    int (*ReadConfigBlock)(uint32_t wChipNum, 
                                uint8_t *pBuf, 
                                uint32_t *pu32StartLoc, 
                                uint32_t *pu32SectorCount, 
                                uint32_t u32SecondaryBoot); //!< Reads Firmware Config block validates it.
    int (*HalReadParameterPage)(uint32_t u32NandDeviceNumber, uint8_t *p8PageBuf); //!< Reads parameter page from ONFi NAND
    bool (*HalIsBlockBad)(uint32_t u32Block, uint8_t* pu8Buf); //!< checks if given block is bad
    int (*VerifyFCB)(uint8_t * pBuffer, void **pFCBGoodCopy);//!< Verifies FCB by running software ECC
    uint32_t (*GetBCBChecksum)(void* pBuffer, int size); //!< Api to run BCB checksum algorithm
}
nand_BootItf_t;


//------------------------------------------------------------------------------
// NOR Boot Driver Interface
//------------------------------------------------------------------------------

//! NOR boot driver interface definition.
//typedef struct _nor_BootItf
//{
//    rom_BootItf_t Boot;                 //!< First member must be the standard API
//    struct _nor_Context ** pContext;        //!< Pointer to context pointer global.
//    int (*nor_InitEmi)(uint32_t bootMode);   //!< Initialize EMI for NOR
//}
//nor_BootItf_t;

//------------------------------------------------------------------------------
// ROM Interface Table
//------------------------------------------------------------------------------

//! ROM interface table definition.
typedef struct _rom_ItfTbl
{
    const void      *pLdr;      //!< Boot loader
    usb_BootItf_t   *pUsb;      //!< USB boot driver
    i2c_BootItf_t   *pI2c;      //!< I2C boot driver
    nand_BootItf_t  *pNand;     //!< NAND boot driver
    //nor_BootItf_t   *pNor;      //!< NOR boot driver
    spi_BootItf_t   *pSpi;      //!< SPI boot driver
    ssp_BootItf_t   *pSsp;      //!< SSP boot driver
    sd_BootItf_t    *pSd;       //!< SD boot driver
}
rom_ItfTbl_t;


#endif // _ITF_DEFINE_H
//! @}
