//////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom
//! @{
//
//! \file   sim_tools.c
//! \brief  simulation printing and termination functions
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

#ifdef TGT_SIM // SIMULATION

#include <stdarg.h>
#include "rom_types.h"
#include "regssimdbg.h"

//! Sets the maximum number of arguments for the simulation printf
#define MAX_PRINTF_ARGS 7


////////////////////////////////////////////////////////////////////////////////
//! \brief      simulation printf function writes to simulation log
//!
//! \fntype     Non-Reentrant
//!
//! \param[in]  pointer to null terminated format string
////////////////////////////////////////////////////////////////////////////////
void sim_printf(const char *fmt, ...)
{
    va_list ap;
    uint32_t mem[MAX_PRINTF_ARGS];
    int c, i = 0;

    // Create an array of 32-bit values for the simulation debug mechanism.
    // The first value in the array is the address of the format string.
    mem[i++] = (uint32_t)fmt;

    // The remaining values are taken from the argument list.
    va_start(ap, fmt);
    while (MAX_PRINTF_ARGS > i && '\0' != (c = *fmt++))
    {
        if ('%' == c)
        {
            if ('\0' == (c = *fmt++))
                break;

            switch(c)
            {
              case 's':
              case 'S':
              case 'c':
              case 'N':
              case 'M':
              case 'd':
              case 'x':
              case 'X':
                mem[i++] = va_arg(ap, uint32_t);
                break;
            }
        }
    }
    va_end(ap);

    // Write simulation debug registers.
    *((uint32_t *)HW_SIMDBG_PTR_REG_ADDR) = (uint32_t) mem;
    *((uint32_t *)HW_SIMDBG_CS_REG_ADDR) = (uint32_t) i;
    return;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief      terminate simulation run
//!
//! \fntype     Non-Reentrant
//!
//! \param[in]  16-bit return code that is output to simulation log
////////////////////////////////////////////////////////////////////////////////
void sim_exit( uint32_t rc )
{
  *((uint32_t *)HW_SIMDBG_TERM_REG_ADDR) = (uint32_t) ((rc << 16) | 1);

  // End XACTOR recording
  // fix for encore   .... HW_UARTDBGDR_WR( 0x16 ) ;

  for (;;) ;
}


#endif //TGT_SIM

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

