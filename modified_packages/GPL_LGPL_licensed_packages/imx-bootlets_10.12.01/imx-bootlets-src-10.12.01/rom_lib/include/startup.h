////////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    startup.h
//! \brief   startup exported prototypes
//!
////////////////////////////////////////////////////////////////////////////////
#ifndef STARTUP_H
#define STARTUP_H

#ifndef __LANGUAGE_ASM__
//! Setup and enable JTAG
RtStatus_t enable_jtag( uint32_t force_use_serial ) ;

//! Output error code to debug uart and power down chip
void rom_fatal_error( uint32_t error_code ) ;

//! Converts an integer into a hex string. Returns pointer to the string.
char *Int2HexStr( uint32_t number ) ;

//! Output null terminated string to the debug uart
void uart_write( const char *pString ) ;

//! Writes an integer as a string of hex characters to the debug uart
#define UART_WRITE_INT(n) uart_write( Int2HexStr(n) )
#endif

#endif
//! @}

