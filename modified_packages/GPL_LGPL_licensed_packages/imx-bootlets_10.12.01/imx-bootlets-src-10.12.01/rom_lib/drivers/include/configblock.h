//////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_boot
//! @{
//
//! \file   configblock.h
//! \brief  definitions for firmware config block
//
// Copyright (c) Freescale Semiconductor, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary  Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may comprise trade secrets of SigmaTel, Inc.
// or its associates, and any unauthorized use thereof is prohibited.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _CONFIGBLOCK_H
#define _CONFIGBLOCK_H

#define NUM_SYSTEM_DRIVE_BACKUP_COPIES      (10)
#define FIRMWARE_CONFIG_BLOCK_SIGNATURE     (0x00112233) 

typedef struct _DriveInfo_t
{
    uint32_t    u32ChipNum;             //!< Chip Select, ROM does not use it
    uint32_t    u32DriveType;           //!< Always system drive, ROM does not use it
    uint32_t    u32Tag;                 //!< Drive Tag
    uint32_t    u32FirstSectorNumber;	//!< For BA-NAND devices, this number should be divisible by 4, 
                                        //!< Protocol is set to 4 sectors of 512 bytes. 
                                        //!< Firmware can start at sectors 4, 8, 12, 16,....
    uint32_t    u32SectorCount;         //!< Not used by ROM
} DriveInfo_t;

typedef struct _ConfigBlock_t {
    uint32_t    u32Signature;           //!< Signature 0x00112233
    uint32_t    u32PrimaryBootTag;      //!< Primary boot drive identified by this tag
    uint32_t    u32SecondaryBootTag;    //!< Secondary boot drive identified by this tag
    uint32_t    u32NumCopies;           //!< Num elements in aFWSizeLoc array
    DriveInfo_t aDriveInfo[];           //!< Let array aDriveInfo be last in this data 
                                        //!< structure to be able to add more drives in future without changing ROM code
} ConfigBlock_t;

extern RtStatus_t rom_ReadMBR(uint8_t *pMbr, uint32_t *pu32StartLoc);
extern RtStatus_t rom_ReadConfigBlock(uint32_t u32ChipNum, uint8_t *pBuf, uint32_t *pu32StartLoc, uint32_t *pu32SectorCount, uint32_t u32SecondaryBoot);

#ifdef WRITES_ALLOWED

#ifdef __cplusplus
extern "C" {
#endif

void Prepare_BANAND_MBR(uint8_t * pBuf, int FirmwareStartSector, uint32_t u32TotalSectors);

#ifdef __cplusplus
}      // Closing of external "C"
#endif

#endif // WRITES_ALLOWED

#endif //_CONFIGBLOCK_H
//! @}
