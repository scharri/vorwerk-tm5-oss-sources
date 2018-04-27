////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom
//! @{
//!
// Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    itf_rom.h
//! \brief   Provides rom firmware access to the global interface table.
//!
//! This file provides access to the global boot rom interface (ie. patch)
//! table. It should be used by rom firmware (inluding unit tests) only, and
//! not by plugins or other code external to the rom.
//!
//! For an interface function to be patchable, all rom firmware must call that
//! function through this global table. This file provides #defines that can be
//! used to shorten the syntax. For example, to call the I2C boot driver stop
//! function, use either of the following:
//!
//! \code
//! Foo()
//! {
//!     g_RomItf.pI2c->Boot.Stop(); // calling I2c stop function in the rom
//!     g_pI2c->Boot.Stop();        // shorthand using the #define
//! }
//! \endcode
////////////////////////////////////////////////////////////////////////////////
#ifndef _ITF_ROM_H
#define _ITF_ROM_H


// Include the boot rom interface definitions
#include "itf_define.h"


//! \brief The global boot rom interface table.
//!
//! This table is located in OCRAM, but is not anchored to a known address. Thus
//! its location can move with each build. ROM firmware can and should reference
//! the table through its name, because the table will be linked with the rom.
//!
//! To avoid linking the entire rom, unit tests should create and initialize
//! their own copy of this table. For example, a unit test for I2C could do
//! the following:
//!
//! \code
//! extern i2c_BootItf_t i2c_Itf;
//! rom_ItfTbl_t g_RomItf = {};
//!
//! i2c_Test()
//! {
//!     g_RomItf.pI2c = &i2c_Itf;    // initialize the rom interface table
//! }
//! \endcode
//extern rom_ItfTbl_t g_RomItf;
extern nand_BootItf_t rom_NandBootItf;


// These defines provide shorthand for accessing the boot rom interfaces.
// See the example in the file header. Their use is optional.

//#define g_pLdr      ((loader_Itf_t *)g_RomItf.pLdr)
//#define g_pUsb      g_RomItf.pUsb
//#define g_pI2c      g_RomItf.pI2c
////#define g_pNand     g_RomItf.pNand
#define g_pNand     ((nand_BootItf_t *)&rom_NandBootItf)
//#define g_pNor      g_RomItf.pNor
//#define g_pSSP      g_RomItf.pSsp
//#define g_pSsp      g_RomItf.pSsp
//#define g_pSpi      g_RomItf.pSpi
//#define g_pSd       g_RomItf.pSd


#endif // _ITF_ROM_H
//! @}

