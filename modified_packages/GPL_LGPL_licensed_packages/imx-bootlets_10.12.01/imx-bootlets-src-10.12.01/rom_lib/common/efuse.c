////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_common
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file   efuse.c
//! \brief  Implements various efuse utilities.
//!
////////////////////////////////////////////////////////////////////////////////
#include <rom_types.h>
#include <efuse.h>
#include <debug.h>


////////////////////////////////////////////////////////////////////////////////
// Data
////////////////////////////////////////////////////////////////////////////////

//! Shadow copies of the rom efuse bank
//uint32_t ocotp_efuse_shadows[8];


////////////////////////////////////////////////////////////////////////////////
//! \brief  Loads the rom efuse bank to shadow registers in ocram
//!
//! Copies the contents of the rom efuse bank to a set of shadow registers in
//! ocram. Closes the efuse hardware when done to conserve power.
//!
//! \post   ocotp_efuse_shadows[] holds a copy of the rom efuse bank.
////////////////////////////////////////////////////////////////////////////////
/*void ocotp_load_shadows(void)
{
#ifndef TGT_DILLO
    int i ;

    DBG_PRINTF("ocotp_load_shadows()\n");

    // Open eFuses for reading
    ocotp_load_fuse_data( );

    // Copy data into ROM shadow memory
    for ( i=0; i<HW_OCOTP_ROMn_COUNT; i++ )
    {
        ocotp_efuse_shadows[i] = HW_OCOTP_ROMn_RD(i) ;
    }

    // Done with efuse, conserve power
    ocotp_close_banks( );
#endif
}
*/

// eof efuse.c
//! @}
