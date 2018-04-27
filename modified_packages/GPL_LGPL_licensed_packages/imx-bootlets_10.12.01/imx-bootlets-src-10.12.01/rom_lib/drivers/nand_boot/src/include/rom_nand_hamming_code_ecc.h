////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_boot
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_hamming_code_ecc.h
//! \brief This file provides header info for hamming code ecc.
//!
////////////////////////////////////////////////////////////////////////////////
#ifndef _ROM_NAND_HAMMING_CODE_ECC_H
#define _ROM_NAND_HAMMING_CODE_ECC_H

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
//!< Bytes per FCB data block
#define NAND_HC_ECC_SIZEOF_DATA_BLOCK_IN_BYTES        (512) 
//! Size of a parity block in bytes for all 16-bit data blocks present inside one 512 byte FCB block.
#define NAND_HC_ECC_SIZEOF_PARITY_BLOCK_IN_BYTES      (512) 
//! Offset to first copy of FCB in a NAND page
#define NAND_HC_ECC_OFFSET_DATA_COPY            (12) // make sure this value is divisible by 4 for buffers to be word aligned 
//! Offset to first copy of Parity block in a NAND page
#define NAND_HC_ECC_OFFSET_PARITY_COPY          (NAND_HC_ECC_OFFSET_DATA_COPY+NAND_HC_ECC_SIZEOF_DATA_BLOCK_IN_BYTES)

#define BITMASK_HAMMINGCHECKED_ALL_THREE_COPIES 0x7 //!< to indicate all three copies of FCB in first page are processed with Hamming codes.
#define BITMASK_HAMMINGCHECKED_FIRST_COPY       0x1 //!< to indicate first copy of FCB is processed with Hamming codes.
#define BITMASK_HAMMINGCHECKED_SECOND_COPY      0x2 //!< to indicate second copy of FCB is processed with Hamming codes.
#define BITMASK_HAMMINGCHECKED_THIRD_COPY       0x4 //!< to indicate third copy of FCB is processed with Hamming codes.

extern const uint8_t au8SyndTable[];

RtStatus_t TripleRedundancyCheck(uint8_t* pFCBCopy1, uint8_t* pFCBCopy2, uint8_t* pFCBCopy3, 
                            uint8_t * pu8HammingCopy);
bool IsNumOf1sEven(uint8_t u8);

void CalculateParity(uint8_t d, uint8_t * p);

RtStatus_t TableLookupSingleErrors(uint8_t u8Synd, uint8_t * pu8BitToFlip);

RtStatus_t HammingCheck(uint8_t * pFCB, uint8_t * pParityBlock);

#endif
////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
// @}
