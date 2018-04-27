////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom
//! @{
//!
// Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    itf_plugin.h
//! \brief   Provides rom plugins access to the global rom interface table.
//!
//! This file provides access to the global boot rom interface (ie. patch)
//! table. It should be used by plugins or other code external to the rom
//! that need to call or patch rom boot API's. The following example shows
//! how a plugin can call and patch the rom.
//!
//! \code
//! extern i2c_BootItf_t my_I2cItf = {};
//!
//! my_Patch()
//! {
//!     // Example 1: calling I2c stop function from a plugin
//!     pRomItf->pI2c->Boot.Stop();
//!
//!     // Example 2: patching the I2c next function from a plugin
//!     my_I2cItf = *(pRomItf->pI2c);       // copy interface to ram
//!     my_I2cItf.Boot.Next = my_Next;      // patch desired function
//!     pRomItf->pI2c = &my_I2cItf;         // update interface table
//! }
//! \endcode
////////////////////////////////////////////////////////////////////////////////
#ifndef _ITF_PLUGIN_H
#define _ITF_PLUGIN_H


// Include the boot rom interface definitions
#include "itf_define.h"


// Global pointer to the rom patch table. Plugins will use this pointer to
// access and patch the rom. Rom firmware should not use this pointer, but
// should use the global symbol defined in itf_rom.h.
#ifdef TGT_DILLO
#define pRomItf ((rom_ItfTbl_t **)0x3bff4)
#elif TGT_3700
#define pRomItf ((rom_ItfTbl_t **)0x4001fff4)
#elif TGT_MX28
#define pRomItf ((rom_ItfTbl_t **)0x4001fff4)
#else
#define pRomItf ((rom_ItfTbl_t **)0xFFFFFFF4)
#endif

#endif // _ITF_PLUGIN_H
//! @}
