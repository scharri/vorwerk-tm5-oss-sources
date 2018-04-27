////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_loader
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    ldr_config.h
//! \brief   Boot loader compile time options.
//!
////////////////////////////////////////////////////////////////////////////////
#ifndef _LDR_CONFIG_H
#define _LDR_CONFIG_H


////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
#ifdef TGT_SIM
//! Defines microseconds per milliseconds for the recovery switch timer values.
//! To allow simulations to run faster, this is set to a small value for TGT_SIM.
#define RSWITCH_MSEC 1
#else
#define RSWITCH_MSEC 1000
#endif

#ifndef RSWITCH_MAX_ON_USEC
//! Defines maximum time to wait for usb connect if the recovery switch is held.
//! Specified in microseconds. Default is 5 seconds.
#define RSWITCH_MAX_ON_USEC (5*1000*RSWITCH_MSEC)
#endif

#ifndef RSWITCH_MIN_ON_USEC
//! Defines minimum time the recovery switch must be held before checking for
//! usb connect. Specified in microseconds. Default is 2 seconds.
#define RSWITCH_MIN_ON_USEC (2*1000*RSWITCH_MSEC)
#endif

#ifndef RSWITCH_OFF_USEC
//! Defines debounce time for detecting the recovery switch released.
//! Specified in microseconds. Default is 20 milliseconds.
#define RSWITCH_OFF_USEC (20*RSWITCH_MSEC)
#endif

#ifndef FILE_MAJOR_VERSION
//! Defines boot image file version compatibility. Image files with the major
//! version less than or equal to this value will be loaded.
#define FILE_MAJOR_VERSION 1
#endif

#ifndef ROM_DCP_CHAN
//! Defines which dcp channel the rom loader will use.
#define ROM_DCP_CHAN 3
#endif

#ifndef ROM_DCP_TIMEOUT
//! Defines a timeout value for rom dcp transactions.
//! Specified in microseconds. Default is 500 milliseconds.
#define ROM_DCP_TIMEOUT 500*1000
#endif


#endif // _LDR_CONFIG_H
//! @}
