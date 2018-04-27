//////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//!
// Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    debug.h
//! \brief   Defines encore ROM debug functions.
//!
//! This file provides definitions for various ROM test and debug functions.
//! The definitions are conditional on the selected target environment.
//!
///////////////////////////////////////////////////////////////////////////////
#ifndef _DEBUG_H
#define _DEBUG_H

#if defined(TGT_SIM)

  //#include "rom_types.h"
  #include "regssimdbg.h"

////////////////////////////////////////////////////////////////////////////////
//! \brief      simulation printf function writes to simulation log
//!
//! \fntype     Non-Reentrant
//!
//! \param[in]  pointer to null terminated format string
////////////////////////////////////////////////////////////////////////////////
void sim_printf(const char *fmt, ...);

////////////////////////////////////////////////////////////////////////////////
//! \brief      terminate simulation run
//!
//! \fntype     Non-Reentrant
//!
//! \param[in]  16-bit return code that is output to simulation log
////////////////////////////////////////////////////////////////////////////////
void sim_exit( uint32_t rc );


  #define DBG_PRINTF(...) sim_printf(__VA_ARGS__)
  #define DILLO_PRINTF(...)
  #define SIM_PRINTF(...) sim_printf(__VA_ARGS__)
  #define DBG_EXIT(rc) sim_exit(rc)
  #define DBG_DUMP_OCRAM(n) sim_printf("%M", n)
  #define SystemHalt() sim_exit((uint32_t)(-1))

#elif ( defined(TGT_3700) || defined(TGT_MX28))

  #include <stdio.h>
  #include <stdlib.h>

  #define DBG_PRINTF(...) printf(__VA_ARGS__)
  #define DILLO_PRINTF(...) printf(__VA_ARGS__)
  #define SIM_PRINTF(...)
  #define DBG_EXIT(rc) exit(rc)
  #define DBG_DUMP_OCRAM(n)
  #define SystemHalt() __asm(" BKPT 0");
  #include <string.h>
  #include "dbg_plugin.h"

#else

  #define DBG_PRINTF(...)
  #define DILLO_PRINTF(...)
  #define SIM_PRINTF(...)
  #define DBG_EXIT(rc)
  #define DBG_DUMP_OCRAM(n)
  #define SystemHalt()

#endif

#endif // _DEBUG_H
//! @}
