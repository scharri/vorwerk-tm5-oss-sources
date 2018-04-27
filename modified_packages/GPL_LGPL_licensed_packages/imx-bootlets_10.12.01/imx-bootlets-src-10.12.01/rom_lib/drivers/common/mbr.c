////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom
//! @{
//
// Copyright (c) 2009 Freescale Semiconductors.
//
//! \file       mbr.c
//! \brief      Definitions and API used by rom drivers to decrypt MBR
//!
//! \version    0.1
//! \date       1/22/09
//!
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include <rom_types.h>
#include <string.h>
#include "return_codes.h"
////////////////////////////////////////////////////////////////////////////////
// definitions
////////////////////////////////////////////////////////////////////////////////

// definitions used to discover the MBR signature
#define MBR_BLOCK_POSITION                          0
#define MBR_PART_TABLE                              0x01BE 
#define MBR_SIGNATURE                               0x01FE 
// partition table system id for the SigmaTel partition
#define MBR_SIGMATEL_ID                             'S'
// MBR partition table field offsets
#define PTBL_SYSTEM_ID                              0x04
#define PTBL_REL_SECTOR                             0x08
#define PTBL_ENTRY_SIZE                             0x10
#define PTBL_MAX_NUM_ENTRIES                        0x04
#define PART_SIGNATURE		                    0xaa55



////////////////////////////////////////////////////////////////////////////////
// code
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//! \brief      Loads an unaligned, little-endian, 32-bit word from address
//!                 p, and returns the result as an unsigned integer.
//!
//! \fntype     Non-Reentrant Function
//!
//! Continues the Identify card process:
//!         Determine supported voltage levels for device, based on CMD1/APCMD41
//!         CMD2 - Send CID
//!         CMD3 - Set/Send Relative Card Address
//! 
//! \param[in]  p:    byte pointer to load address.
//!
//! \retval     unsigned 32-bit value loaded from address p
////////////////////////////////////////////////////////////////////////////////
static uint32_t LoadUINT32(uint8_t *p)
{
    return (uint32_t)((p[3]<<24) | (p[2]<<16) | (p[1]<<8) | (p[0]));
}

////////////////////////////////////////////////////////////////////////////////
//! \brief      Returns start location of firmware by reading MBR
//!
//! \fntype     Non-Reentrant Function
//!
//! \param[in]  pMbr:    pointer to buffer having Master Boot Record
//!
//! \param[out]  pu32StartLoc:    pointer to return start location of 
//!              firmware block
//!
//! \retval     SUCCESS
//!                              
//! \retval     ERROR_ROM_COMMON_DRIVER_INVALID_MBR: Master boot record is not valid
//!                              
////////////////////////////////////////////////////////////////////////////////

RtStatus_t rom_ReadMBR(uint8_t *pMbr, uint32_t *pu32StartLoc)
{
    uint8_t *pSig = &pMbr[MBR_SIGNATURE];   // ptr to 16-bit mbr signature word
    uint8_t *pTbl = &pMbr[MBR_PART_TABLE];  // ptr to first partition table entry

    //
    // Check the MBR signature (0x55AA).  Buffer alignment is uknown
    // so do this one byte at a time.
    //
    if ((pSig[0] == 0x55) && (pSig[1] == 0xAA))             
    {
        //
        // Search entire partition table for an entry with the SigmaTel system id
        //
        while (pTbl < pSig)
        {
            if (pTbl[PTBL_SYSTEM_ID] == MBR_SIGMATEL_ID)
            {
                //
                // Found it! Boot image LBA is a fixed sector offset from the
                // start of the SigmaTel partition. Signal success and return.
                //
                *pu32StartLoc = LoadUINT32(&pTbl[PTBL_REL_SECTOR]);
                return SUCCESS;
            }
            pTbl += PTBL_ENTRY_SIZE;
        }
    }

    // the block is not a valid MBR
    return ERROR_ROM_COMMON_DRIVER_INVALID_MBR;                    
}

#ifdef WRITES_ALLOWED

#pragma pack(1)
//
// CHS
//
typedef struct _CHS
{
	uint16_t	Head;
	uint8_t		Sector;
	uint16_t	Cylinder;
} CHS, *PCHS;

//
// CHS Packed
//
typedef struct _CHS_PACKED
{
	uint8_t		Head;
	uint8_t		Sector;
	uint8_t		Cylinder;
} CHS_PACKED, *PCHS_PACKED;

typedef struct _PART_ENTRY
{
    uint8_t     BootDescriptor;				// 0=nonboot, 0x80=bootable
    CHS_PACKED  StartCHSPacked;
    uint8_t     FileSystem;					// 1=fat12, 6=fat16
    CHS_PACKED  EndCHSPacked;
    uint32_t    FirstSectorNumber;			// relative to beginning of device
    uint32_t    SectorCount;
} PART_ENTRY, *PPART_ENTRY;

typedef struct _PARTITION_TABLE
{
	uint8_t		ConsistencyCheck[MBR_PART_TABLE];		// not used
	PART_ENTRY	Partitions[PTBL_MAX_NUM_ENTRIES];
	uint16_t    Signature;					// 0xaa55
} PARTITION_TABLE, *PPARTITION_TABLE;

#pragma pack()

void Prepare_BANAND_MBR(uint8_t * pBuf, int FirmwareStartSector, uint32_t u32TotalSectors)
{
    PARTITION_TABLE *pMbr = (PARTITION_TABLE *)pBuf;

    pMbr->Signature = PART_SIGNATURE;
    pMbr->Partitions[0].BootDescriptor = 0;
    pMbr->Partitions[0].FileSystem = MBR_SIGMATEL_ID;
    pMbr->Partitions[0].FirstSectorNumber = FirmwareStartSector;
    pMbr->Partitions[0].SectorCount = u32TotalSectors;
}

#endif //WRITES_ALLOWED
