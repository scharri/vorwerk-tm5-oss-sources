////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_boot
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_api.h
//! \brief Provides API routines for interfacing with the NAND boot driver.
////////////////////////////////////////////////////////////////////////////////
#ifndef __ROM_NAND_API_H
#define __ROM_NAND_API_H

#include "return_codes.h"
#include "itf_define.h"
#include "rom_nand_internal.h"

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

int rom_nand_BootInit(rom_BootInit_t *);
chunk_t * rom_nand_BootNext(int *pCount);
int rom_nand_BootSkip(int count);
int rom_nand_BootCtrl(rom_BootAction_t, void *);
int rom_nand_BootStop(void);
int rom_nand_BootDriverSetupDriver(rom_BootItf_t *pDriver);

RtStatus_t rom_nand_BootBlockSearch(uint32_t u32NandDeviceNumber, 
                                    const FingerPrintValues *pFingerPrintValues,
                                    uint32_t * p32SearchSector,
                                    uint8_t * pBuffer);
bool rom_nand_BlockIsInBadBlockTable(uint32_t u32BlockToMatch);
RtStatus_t rom_nand_FindFCB(uint32_t u32CurrentNAND, uint32_t * pReadSector, uint8_t * pBuffer);
RtStatus_t rom_nand_ReadBadBlockTable(uint32_t u32CurrentNAND, uint32_t * p32Sector, uint8_t * pBuffer);
RtStatus_t rom_nand_FindBootControlBlocks(uint8_t * pBuffer);
RtStatus_t rom_nand_WaitOnRead(void);
RtStatus_t rom_nand_StartSectorRead(uint8_t * pNewBuffer);
RtStatus_t rom_nand_SkipSectors(unsigned uSectorCount, uint8_t * pNewBuffer);
RtStatus_t rom_nand_FindNextGoodBlock(uint8_t * pNewBuffer);
RtStatus_t rom_ba_nand_FindBootControlBlocks(uint8_t * pBuffer);
RtStatus_t rom_ba_nand_hal_SendReadCmd(uint32_t u32NandDeviceNumber, uint32_t u32SectorCount, uint32_t u32SectorAddress, uint8_t *p8PageBuf);
RtStatus_t rom_ba_nand_hal_Abort(uint32_t u32NandDeviceNumber);
RtStatus_t rom_nand_hal_ReadParameterPage(uint32_t u32NandDeviceNumber, uint8_t *p8PageBuf);
RtStatus_t rom_nand_VerifyFCB(uint8_t * pBuffer, void **pFCBGoodCopy);
uint32_t rom_nand_GetBCBChecksum(void * pBuffer, int u32Size);
#endif //__ROM_NAND_API_H
//! @}
