////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_hal
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_hal_structs.h
//! \brief Structure definitions for NAND descriptors
////////////////////////////////////////////////////////////////////////////////

#ifndef ROM_NAND_HAL_STRUCTS_H
#define ROM_NAND_HAL_STRUCTS_H  1

///////////////////////////////////////////////////////////////////////////////
// Structure for easily manipulating ReadID code.
//////////////////////////////////////////////////////////////////////////////

//! \brief Read ID Code structure.
//!
//! This structure packs the ReadID result into 6 bytes.
typedef struct _ReadIDCode{
  union{
      struct {
          uint8_t btManufacturerCode;
          uint8_t btDeviceCode;          
      };      
      uint16_t usDeviceID;
  };
   // Read ID Byte 3
  uint32_t InternalChipNumber     : 2;        //!> Number of die = (1 << n)
  uint32_t CellType               : 2;        //!> Number of bits per memory cell = ( 1 << (n+1) )
  uint32_t VendorSpecific0        : 3;
  uint32_t CacheProgram           : 1;        //!> 0 == Not supported
    // Read ID Byte 4
  uint32_t PageSize               : 2;        //!> Page size in bytes = (1 << n) * 1024
  uint32_t RedundantAreaSize      : 1;        //!> Redundant area bytes per 512 data bytes = 8 * (1 << n)
  uint32_t Reserved0              : 1;
  uint32_t BlockSize              : 2;        //!> Block size in bytes = 64 * 1024 * (1 << n)
  uint32_t Organization           : 1;        //!> 0 == x8, 1 == x16
  uint32_t SamsungHSSerialAccess  : 1;        //!> 0 == 50/30ns, 1 == 25ns
  // Read ID Byte 5
  uint32_t VendorSpecific1        : 2;
  uint32_t PlaneNumber            : 2;        //!> # of planes total (see note below) = (1 << n)
  uint32_t PlaneSize              : 3;        //!> # of bytes per plane = 64 * 1024 * 1024 * (1 << n)
  uint32_t Reserved4              : 1;
    // Read ID Byte 6
  uint32_t Reserved5              : 3;
  uint32_t ToshibaHighSpeedMode   : 1;        //!> 0 == Not supported
  uint32_t Reserved6              : 4;
} ReadIDCode;

#define NAND_READ_ID_RESULT_SIZE  6    // Reading 6 bytes back.

///////////////////////////////////////////////////////////////////////////////
// Sector/Page Descriptor

//! \brief Sector definitions.  Size and masks.
//!
//! This structure packs the ReadID result into 6 bytes.
typedef struct _NANDSectorDescriptorStruct_t {
  uint32_t    u32TotalPageSize;	    // Total Page size - if >2K page use actual size.  
  //uint16_t    u16SectorInPageMask;  // Sectors per page mask
  //uint16_t    u16SectorToPageShift; // Number of bits to shift.
  uint16_t    u16SectorSize; // Sector Size field from BA NAND's parameters page.
  //uint16_t    u16SectorMultiple; // Sector Multiple field from BA NAND's parameters page.
#ifdef WRITES_ALLOWED
  uint32_t    u32PagesPerBlock;     // Number of pages per Block
#endif
} NANDSectorDescriptorStruct_t;

///////////////////////////////////////////////////////////////////////////////
// NAND Command Codes Descriptor
typedef struct _NANDCommandCodesStruct_t {

  // NOTE: Command are a single byte, in the LSByte.  A value of -1 (0xff) indicates
  //       a code is not available for the current device.  This may cause problems
  //       because Reset typeically uses 0xFF.

  uint8_t btReadIDCode;                    // ReadID
  uint8_t btResetCode;                     // Reset

  uint8_t btRead1Code;                     // Read (Mode 1)
  uint8_t btRead1_2ndCycleCode;            // Second Cycle for Read (Type 2 NANDs)

//  uint8_t btRandomReadStartCode;           // Random Read Start Type2 - (Mode 2) for Type1
//  union 
//  {
//      uint8_t btRead3Code;                     // (Mode 3) for Type1
//      uint8_t btRandomReadFinishCode;          // Random Read Finish Type2 - (Mode 3) for Type1
//  };

#ifdef WRITES_ALLOWED
  uint8_t btReadStatusCode;	                // Read Status
  uint8_t btSerialDataInputCode;           // Serial Data Input
  uint8_t btPageProgramCode;               // Page Program

  uint8_t btBlockEraseCode;                // Block Erase
  uint8_t btBlockErase2Code;               // Block Erase 2

#endif
} NANDCommandCodesStruct_t;


typedef struct _NAND_Descriptor{
  NANDSectorDescriptorStruct_t stc_NANDSectorDescriptor;
  NANDCommandCodesStruct_t stc_NANDDeviceCommandCodes;  
  uint8_t	btNumRowBytes;		// Number of Row Address bytes required
  uint8_t	btNumColBytes;		// Number of Row Address bytes required
  uint8_t	btNANDDataBusWidth;	// BusWidth = 8 or 16 bits
						// Use this parameter only to initialize the global CurrentNANDBusWidth
						// Some NANDs required more real time process to determine  their bus
						// width. (see CurrentNANDBusWidth declaration Note for further information)
  uint32_t  uiNANDTotalBlocks;
  uint32_t  u32NumLBAPerRead; // Number of addressed LBAs per 2K bytes
  bool      bBA_NAND;
} NAND_Descriptor_t;

//! \brief   Nand ECC Parameters structure.
//!
//! This structure is to list parameters used to program ecc engine.  
typedef struct _NAND_ECC_Params_t
{
    uint32_t m_u32EccBlockNEccLevel;        //!< Type of ECC - BCH0, BCH2, ... BCH-20
    uint32_t m_u32EccBlock0Size;            //!< Number of bytes for Block0 - BCH
    uint32_t m_u32EccBlockNSize;            //!< Block size in bytes for all blocks other than Block0 - BCH
    uint32_t m_u32EccBlock0EccLevel;        //!< Ecc level for Block 0 - BCH
    uint32_t m_u32NumEccBlocksPerPage;      //!< Number of blocks per page - BCH
    uint32_t m_u32MetadataBytes;            //!< Metadata size - BCH
    uint32_t m_u32PageSize;                 //!< Size of page including redundant area
    uint32_t m_u32EraseThreshold;           //!< To set into BCH_MODE register.
} NAND_ECC_Params_t;


#define TYPE1_NAND  1
#define TYPE2_NAND  2
#define TYPE3_NAND  3
#define TYPE4_NAND  4
#define TYPE5_NAND  5
#define TYPE6_NAND  6
#define TYPE7_NAND  7

#define NAND_64MB_3_3V      0xF0    //!< Device ID code for 2 row byte device.
#define NAND_64MB_1_8V      0xA0    //!< Device ID code for 2 row byte device.
#define NAND_128MB_3_3V     0xF1    //!< Device ID code for 2 row byte device.
#define NAND_128MB_1_8V     0xA1    //!< Device ID code for 2 row byte device.
#define NAND_64MB_3_3V2     0xF2    //!< Device ID code for 2 row byte device.

#endif    // #ifdef NAND_STRUCT_H
//! @}
