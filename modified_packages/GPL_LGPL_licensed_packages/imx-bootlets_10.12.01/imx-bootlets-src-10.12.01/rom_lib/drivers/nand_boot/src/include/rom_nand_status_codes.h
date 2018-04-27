//////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//
//! \file rom_nand_status_codes.h
//! \brief Status return values for the NAND driver.
//! \note All positive values (bit 31 not set) are status codes.
//! \note See return_codes.h header file for additional information.
//!
//! 2048 Major groups:  1024 for Sigmatel, 1024 Major groups for customers
//!   (Sigmatel uses low-order 11 bits)
//!  256 Minor groups per Major group
//! 4096 errors per Minor group
//!
//! Bit            3322 2222 2222 1111 1111 11
//!                1098 7654 3210 9876 5432 1098 7654 3210
//!                ---------------------------------------
//! Major Groups:  EMMM MMMM MMMM ---- ---- ---- ---- ----
//! Minor Groups:  ---- ---- ---- mmmm mmmm ---- ---- ----
//! Return Code:   ---- ---- ---- ---- ---- eeee eeee eeee
//
// Copyright (c) SigmaTel, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary  Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may comprise trade secrets of SigmaTel, Inc.
// or its associates, and any unauthorized use thereof is prohibited.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _ROM_NAND_STATUS_CODES_H
#define _ROM_NAND_STATUS_CODES_H

////////////////////////////////////////////////////////////////////////////////
//! Definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//                            ROM GROUP
////////////////////////////////////////////////////////////////////////////////
// The ROM Major group is 5
//#define ROM_GROUP                       0x00500000
//#define STATUS_ROM_NAND_DRIVER_GROUP    0x00008000


// RETURN_CODE_ROM_NAND_DMA_BUSY (0x80608001) - The DMA is still running.
//#define RETURN_CODE_ROM_NAND_DMA_BUSY                       (ROM_GROUP | STATUS_ROM_NAND_DRIVER_GROUP | 0x1)
// RETURN_CODE_ROM_NAND_DRIVER_ECC_THRESHOLD (0x80608002) - During the NAND read, the ECC threshold was exceeded.
//#define RETURN_CODE_ROM_NAND_ECC_THRESHOLD                  (ROM_GROUP | STATUS_ROM_NAND_DRIVER_GROUP | 0x2)



#endif // _RETURN_CODES_H
////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
