////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_boot
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_internal.h
//! \brief Provides the NAND ROM internal structures and function calls.
////////////////////////////////////////////////////////////////////////////////
#ifndef __ROM_NAND_INTERNAL_H
#define __ROM_NAND_INTERNAL_H

#include "rom_nand_hal_structs.h"
#include "../hal/include/rom_nand_hal_dma.h"

// NAND stubs take too long if the sector size is too large.
//! Sets the number of pages to skip between FCB and DBBT searches.
#ifdef USE_NAND_STUBS
#define NAND_SEARCH_STRIDE      8  //!< Skip 8 pages between searches.
#else
#define NAND_SEARCH_STRIDE      64  //!< Skip 64 pages between searches.
#endif

// Need the following for the NAND Timings declaration.
#include "rom_nand_hal_api.h"

//! Sets the number of buffers used to feed the NAND data to the Loader.
#define MAX_NAND_BUFFERS 2

//! This define sets the size of each NAND buffer (data + ECC scratchpad).  
////#define NAND_READ_BUFFER_SIZE       17500
#define NAND_READ_BUFFER_SIZE       3000

//! This define sets the index where the Auxillary buffer begins.  
////#define NAND_AUX_BUFFER_INDEX    16500
#define NAND_AUX_BUFFER_INDEX    2500

//! Sets the number of bytes in the auxillary data area for ECC operations.  
#define NAND_AUX_BUFFER_SIZE    256

//! Sets the maximum number of bad blocks in this table.  
#define MAX_BB_TABLE_SIZE       425

//! Sets the Bad Block Header Size in 2K chunks (8K = 4).  
#define BB_HEADER_SIZE_IN_2K       4

//! \brief enum to define ECC Types.
//!
//! enum listing all ecc modes supported
typedef enum _nand_ecc_levels 
{
    BCH_Ecc_0bit=0,
    BCH_Ecc_2bit=2,
    BCH_Ecc_4bit=4,
    BCH_Ecc_6bit=6,
    BCH_Ecc_8bit=8,
    BCH_Ecc_10bit=10,
    BCH_Ecc_12bit=12,
    BCH_Ecc_14bit=14,
    BCH_Ecc_16bit=16,
    BCH_Ecc_18bit=18,
    BCH_Ecc_20bit=20,
    BCH_Ecc_Max_Levels=11
} nand_ecc_levels_t;

#define MAX_2K_SECTORS_PER_PAGE (32) // to go upto 64K page sizes


// TM41
struct sw_version {
    char version_date[20];
    char version_comment[30];
};

//! \brief Structure defining where FCB and DBBT parameters are located.
//!
//! This structure defines the basic fingerprint template for both the Firmware
//! Control Block (FCB) and the Discovered Bad Block Table (DBBT).  This
//! template is used to determine if the sector read is a Boot Control Block.
//! This structure defines the Firmware Control Block (FCB).  This block
//! contains information describing the timing for the NAND, the number of
//! NANDs in the system, the block size of the NAND, the page size of the NAND,
//! and other criteria for the NAND.  This is information that is
//! required just to successfully communicate with the NAND.
//! This block contains information describing the version as well as the layout of
//! the code and data on the NAND Media.  For the ROM, we're only concerned
//! with the boot firmware start.  Additional information may be stored in
//! the Reserved area.  This area will be of interest to the SDK.
//! This structure also defines the Discovered Bad Block Table (DBBT) header.  
//! This block contains the information used for parsing the bad block tables
//! which are stored in subsequent 2K sectors.  The DBBT header is 8K, followed
//! by the first NANDs entries on a subsequent 2K page (determined by how many 
//! 2K pages the first nand requires)
typedef struct _BootBlockStruct_t
{
    uint32_t    m_u32Checksum;         //!< First fingerprint in first byte.
    uint32_t    m_u32FingerPrint;      //!< 2nd fingerprint at byte 4.
    uint32_t    m_u32Version;          //!< 3rd fingerprint at byte 8.
    union
    {
        struct
        {            
            NAND_Timing_t   m_NANDTiming;                   //!< Optimum timing parameters for Tas, Tds, Tdh in nsec.
            uint32_t        m_u32DataPageSize;              //!< 2048 for 2K pages, 4096 for 4K pages.
            uint32_t        m_u32TotalPageSize;             //!< 2112 for 2K pages, 4314 for 4K pages.
            uint32_t        m_u32SectorsPerBlock;           //!< Number of 2K sections per block.
            uint32_t        m_u32NumberOfNANDs;             //!< Total Number of NANDs - not used by ROM.
            uint32_t        m_u32TotalInternalDie;          //!< Number of separate chips in this NAND.
            uint32_t        m_u32CellType;                  //!< MLC or SLC.
            uint32_t        m_u32EccBlockNEccType;         //!< Type of ECC, can be one of BCH-0-20
            uint32_t        m_u32EccBlock0Size;             //!< Number of bytes for Block0 - BCH
            uint32_t        m_u32EccBlockNSize;             //!< Block size in bytes for all blocks other than Block0 - BCH
            uint32_t        m_u32EccBlock0EccType;         //!< Ecc level for Block 0 - BCH
            uint32_t        m_u32MetadataBytes;             //!< Metadata size - BCH
            uint32_t        m_u32NumEccBlocksPerPage;       //!< Number of blocks per page for ROM use - BCH
            uint32_t        m_u32EccBlockNEccLevelSDK;      //!< Type of ECC, can be one of BCH-0-20
            uint32_t        m_u32EccBlock0SizeSDK;          //!< Number of bytes for Block0 - BCH
            uint32_t        m_u32EccBlockNSizeSDK;          //!< Block size in bytes for all blocks other than Block0 - BCH
            uint32_t        m_u32EccBlock0EccLevelSDK;      //!< Ecc level for Block 0 - BCH
            uint32_t        m_u32NumEccBlocksPerPageSDK;    //!< Number of blocks per page for SDK use - BCH
            uint32_t        m_u32MetadataBytesSDK;          //!< Metadata size - BCH
            uint32_t        m_u32EraseThreshold;            //!< To set into BCH_MODE register.
            uint32_t        m_u32BootPatch;                 //!< 0 for normal boot and 1 to load patch starting next to FCB.
            uint32_t        m_u32PatchSectors;              //!< Size of patch in sectors.
            uint32_t        m_u32Firmware1_startingSector;  //!< Firmware image starts on this sector.
            uint32_t        m_u32Firmware2_startingSector;  //!< Secondary FW Image starting Sector.
            uint32_t        m_u32SectorsInFirmware1;        //!< Number of sectors in firmware image.
            uint32_t        m_u32SectorsInFirmware2;        //!< Number of sector in secondary FW image.
            uint32_t        m_u32DBBTSearchAreaStartAddress;//!< Page address where dbbt search area begins
            uint32_t        m_u32BadBlockMarkerByte;        //!< Byte in page data that have manufacturer marked bad block marker, this will
                                                            //!< bw swapped with metadata[0] to complete page data.
            uint32_t        m_u32BadBlockMarkerStartBit;    //!< For BCH ECC sizes other than 8 and 16 the bad block marker does not start 
                                                            //!< at 0th bit of m_u32BadBlockMarkerByte. This field is used to get to the 
                                                            //!< start bit of bad block marker byte with in m_u32BadBlockMarkerByte.
            uint32_t        m_u32BBMarkerPhysicalOffset;    //!< FCB value that gives byte offset for bad block marker on physical NAND page.
        } FCB_Block;
        struct
        {                            
            uint32_t        m_u32NumberBB;		            //!< # Bad Blocks stored in this table for NAND0.
            uint32_t        m_u32Number2KPagesBB;           //!< Bad Blocks for NAND0 consume this # of 2K pages.   
        } DBBT_Block;
    };
    struct
    {
	uint32_t m_u32UpdateStatus;			// if update is pending
	struct sw_version images[2];
    } TM41_Block;
} BootBlockStruct_t;

//! \brief Structure of the Bad Block Entry Table in NAND.
//!
//! This structure defines the Discovered Bad Block Table (DBBT) entries.  This 
//! block contains a word holding the NAND number then a word describing the number 
//! of Bad Blocks on the NAND and an array containing these bad blocks.  The ROM 
//! will use these entries in the Bad Block table to correctly index to the next 
//! sector (skip over bad blocks) while reading from the NAND. 
//! Blocks are not guaranteed to be sorted in this table.
//! \todo   Does this need to be larger than 500 blocks?  I don't think so.
typedef struct _BadBlockTableNand_t
{
    uint32_t      uNAND;		        //!< Which NAND this table is for.
    uint32_t      uNumberBB;		    //!< Number of Bad Blocks in this NAND.
    // Divide by 4 because of 32 bit words.  Subtract 2 because of the 2 above
    // 32 bit words.
	uint32_t      u32BadBlock[(TYPICAL_NAND_READ_SIZE/4)-2];		//!< Table of the Bad Blocks.  
} BadBlockTableNand_t;

//!
//! \brief Nand boot driver alternate CE3 pin options
//!
typedef enum _efNANDCE3_t
{
    GPMI_D15=0,
    LCD_RESET,
    SSP1_DETECT,
    ROTARYB
} efNANDCE3_t;

//!
//! \brief Nand boot driver alternate RDY3 pin options
//!
typedef enum _efNANDRDY3_t
{
    GPMI_RDY3=0,
    PWM2,
    LCD_DOTCK
} efNANDRDY3_t;
//! \brief Nand Serializer State Machine.
//!
//! This structure is used to keep track of the current sector and place
//! in the block.  During NAND reads, we need to track when we are 
//! at a block boundary.  It is also important to know where the firmware
//! starts which is gathered from the FCB.
typedef struct _SerializerState
{
    uint32_t m_uSectorsPerBlock;        //!< Used to determine block boundary and BB search.

    uint32_t m_uSectorsToRead; //!< Counts down to 0.
    uint32_t m_uSectorsRead;    //!< Track the number of sectors read during boot load.
    uint32_t m_uCurrentNand;    //!< Track which NAND is being accessed.
    uint32_t m_uCurrentBlock;   //!< Track which block is being accessed.
//    uint32_t m_uCurrentSectorInBlock;   //!< Track the current sector in block.
    uint32_t m_uCurrentSector;  //!< Track the current sector number.

    uint32_t m_u32PageDataSize; //!< Track the Page Data size.
    uint32_t m_u32SectorDataRead; //!< Track the data read from sector

    uint32_t m_u32BBAreaStartPage;  //!< Remember the Bad Block Table sector.
    uint32_t m_u32NumberOfBadBlocks;    //!< Remember the number of Bad Blocks in Table.
    uint32_t m_u32UseSecondaryBoot;     //!< 0 = primary, 1 = Secondary.
    uint32_t m_u32BBMarkByteOffsetInPageData; //!< FCB value that gives byte offset for bad block marker in data area
    uint32_t m_u32BBMarkBitOffset; //!< FCB value that gives starting bit offset within m_u32BBMarkByteOffsetInPageData
    uint32_t m_u32BBMarkerPhysicalOffset; //!< FCB value that gives byte offset for bad block marker on physical NAND page.
} SerializerState_t;

//! \brief Nand Fingerprint structure.
//!
//! Fingerprints are used in conjunction with the ECC to determine
//! whether or not a block is valid.  They are strategically placed
//! in both the FCB and DBBT blocks.
typedef struct _FingerPrintValues
{
    uint32_t    m_u32FingerPrint;
    uint32_t    m_u32Version;
} FingerPrintValues;

//! \brief ONFi BA NAND's features supported data structure as defined in ONFi_BA_NAND spec.
//!
typedef struct _BA_NAND_Features_Supported_t{
    union {
        struct {
            uint16_t Reserved1 : 7;
            uint16_t SupportsBlockAbstractedAccessMode : 1;
            uint16_t Reserved2 : 8;
        } u16FS;
        uint16_t u16;
    };
} BA_NAND_Features_Supported_t;

typedef struct _BA_NAND_Optional_Commands_Supported_t{
    uint16_t Reserved1 : 5;
    uint16_t SupportsReadUniqueId : 1;
    uint16_t Reserved2 : 10;
} BA_NAND_Optional_Commands_Supported_t;
//! \brief ONFi BA NAND's Parameter Page as defined in ONFi_BA_NAND spec.
//!
#pragma pack(1)
typedef struct _BA_NAND_Parameter_Page_t {
    uint32_t                              u32ONFiSignature;
    uint16_t                              u16Revision;
    BA_NAND_Features_Supported_t          u16FeaturesSupported;
    BA_NAND_Optional_Commands_Supported_t u16OptionalCommandsSupported;
    uint8_t                               Reserved1[22];
    uint8_t                               Manufacturer[12];
    uint8_t                               Model[20];
    uint8_t                               u8JedecMfgId;
    uint16_t                              u16DateCode;
    uint8_t                               Reserved2[13];
    uint64_t                              u64NumberOfLBAs;
    uint16_t                              u16SectorSize;
    uint16_t                              u16SectorMultiple;
    uint8_t                               u8MetadataBytesPerSector;
    uint8_t                               Reserved3[35];
    uint8_t                               u8MaxPinCap;
    uint16_t                              u16TimingModeSupport;
    uint8_t                               Reserved4[2];
    uint16_t                              u16MaxLBAReadTime;
    uint16_t                              u16MaxLBAWriteTime;
    uint16_t                              u16MaxLBAFlushTime;
    uint8_t                               Reserved5[25];
} BA_NAND_Parameter_Page_t;
#pragma pack()
//! \brief Nand boot driver context variable.
//!
//! This structure packages the NAND Driver and HAL variables into
//! a defined space.
//! It is assumed that bufA will start on a 32 bit boundary.
typedef struct _rom_nand_Context
{
    uint8_t bufA[NAND_READ_BUFFER_SIZE];   //!< First ping-pong buffer.
    uint8_t bufB[NAND_READ_BUFFER_SIZE];   //!< Second ping-pong buffer.
    uint8_t * pDmaBuf;              //!< Points to the buffer owned by DMA.
    uint8_t * pLoaderBuf;           //!< Points to buffer owned by the loader.
    unsigned nextChunk;             //!< Index of next chunk to return from within #pLoaderBuf.
    unsigned skipCount;             //!< Number of chunks remaining to skip.
    unsigned chunksPerRead;      //!< Number of 16 byte loader packets in current read
    efNAND_t zNandEFuse;            //!< Temp variable holding values of eFuse.
    NAND_ECC_Params_t zNANDEccParams;   //!< Temp variable holding values of ecc after reading from FCB.
    SerializerState_t zSerializerState; //!< Structure to track execution.
    NAND_dma_read_t DmaReadDma;     //!< Read DMA descriptor
    NAND_Descriptor_t zNANDDescriptor;  //!< NAND descriptor - page size, # rows, etc.
    uint32_t u32Use1_8V;            //!< Flag for using 1.8V.
    NandDmaTime_t  zNandDmaTime;    //!< Track DMA start time and timeout.
    bool bEnableHWECC;                //!< false to read raw data
    bool bBootPatch;                 //! indicating this is a patch boot.
    // Always leave this table at the end so it can grow to fit the available size.
    uint16_t u16BadBlockTable[MAX_BB_TABLE_SIZE]; //!< Bad Block Table.    
} rom_nand_Context_t;

#define ONFI_SIGNATURE      0x49464E4F    //!< 'ONFI'

#define FCB_FINGERPRINT     0x20424346    //!< 'FCB<space>' - NAND Control Block
#define FCB_VERSION         0x01000000    //!< FCB Version

#define DBBT_FINGERPRINT    0x54424244    //!< 'DBBT' - Discovered Bad Block Table.
#define DBBT_VERSION        0x01000000    //!< DBBT Version

#endif      // ifdef __ROM_NAND_INTERNAL_H
//! @}
