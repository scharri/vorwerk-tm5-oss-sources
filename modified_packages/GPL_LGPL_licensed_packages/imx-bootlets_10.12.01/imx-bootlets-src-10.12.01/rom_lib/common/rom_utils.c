////////////////////////////////////////////////////////////////////////////////
// Copyright(C) SigmaTel, Inc. 2006
//
// Filename: rom_utils.c
// Description: 
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//   Includes and external references
////////////////////////////////////////////////////////////////////////////////

#include <rom_types.h>
#include "itf_rom.h"
#include "return_codes.h"

#include "stdio.h"
#include "regsrtc.h"
#include "regsdigctl.h"
#include "stdlib.h"
#include "rom_types.h"
#include "rom_utils.h"
#include "cpu_support.h"
#include "icoll.h"
#include "regsicoll.h"
#include "regsclkctrl.h"
#include "clocks.h"
#include "test_utils.h"
#include "efuse.h"
#include "debug.h"

////////////////////////////////////////////////////////////////////////////////
//! \brief  Delay Microseconds
//!
//! \param[in]  Count Number of microseconds to delay
//!
//! \return void 
//!
////////////////////////////////////////////////////////////////////////////////
void DelayUs(uint32_t Count)
{
    uint32_t Start = ROM_GET_TIME_USEC();
    while(!ROM_TIME_USEC_EXPIRED(Start, Count))
    {
        //DBG_PRINTF("DelayUs usec = 0x%x\n", ROM_GET_TIME_USEC());
    }
}

//! @}

