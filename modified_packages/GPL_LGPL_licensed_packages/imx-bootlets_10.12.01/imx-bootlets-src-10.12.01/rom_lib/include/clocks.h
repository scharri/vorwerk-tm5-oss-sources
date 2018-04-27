////////////////////////////////////////////////////////////////////////////////
// Copyright(C) SigmaTel, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary & Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may comprise trade secrets of SigmaTel, Inc.
// or its associates, and any use thereof is subject to the terms and
// conditions of the Confidentail Disclosure Agreement pursuant to which this
// source code was originally received.
////////////////////////////////////////////////////////////////////////////////
// File : clocks.h
// Description:  3600 Definitions for clock dividers
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLOCKS_H__
#define __CLOCKS_H__

#if (defined(TGT_CHIP) || defined(TGT_SIM))
#define PLL_LOCK_TIMEOUT (15)
#define CLK_BUSY_TIMEOUT 1000000
#define USB_WAIT_FOR_CONNECT_TIMEOUT (20*1000*1000)
#define USB_WAIT_FOR_CONFIGURE_TIMEOUT (20*1000*1000)
#endif

#ifdef TGT_DILLO
#define PLL_LOCK_TIMEOUT 1000000
#define CLK_BUSY_TIMEOUT 1000000
#define USB_WAIT_FOR_CONNECT_TIMEOUT (5*1000*1000)
#define USB_WAIT_FOR_CONFIGURE_TIMEOUT (5*1000*1000)

// ROM drivers want the following
// frequencies:
#define XCLK_DRIVER_FREQ 24  // Mhz
#define GPMI_DRIVER_FREQ 24
#define EMI_DRIVER_FREQ  24
#define PCLK_DRIVER_FREQ 24
#define HCLK_DRIVER_FREQ 24

// For running from the 24mhz crystal
// (PLL bypass)
#define XCLK_24MHZ_DIVIDER 1
#define GPMI_24MHZ_DIVIDER 1
#define EMI_24MHZ_DIVIDER 1
#define PCLK_24MHZ_DIVIDER 1
#define HCLK_24MHZ_DIVIDER 1


// "Safe" setting for XCLK is below 100 kHz, so that it will always be < HCLK (min 100 kHz).
// Only leave it here for a small number of instructions, as it slows down clock control, itself.
#define XCLK_SAFE_24MHZ_DIVIDER 250

// "Safe" setting for PCLK. This divisor should be safe in both PLL and XTAL (PLL bypass) modes, 
// for any HCLK divider, as long as HCLK AUTO_SLOW_DOWN is disabled and XCLK is at 
// XCLK_SAFE_24MHZ_DIVIDER. (Any PCLK divider between 5 and 14, inclusive, would work here.)
#define PCLK_SAFE_DIVIDER 10
#endif

#endif
//! @}
