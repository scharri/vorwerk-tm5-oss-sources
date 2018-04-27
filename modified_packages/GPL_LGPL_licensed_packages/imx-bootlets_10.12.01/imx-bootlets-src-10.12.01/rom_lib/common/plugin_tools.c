////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_plugin
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    dillo_tools.c
//! \brief   Routines for using the debug uart from a plugin on dillo target
//!
////////////////////////////////////////////////////////////////////////////////
#if (defined(TGT_DILLO)||defined(TGT_3700)||defined(TGT_MX28))

#include <stdarg.h>
#include "rom_types.h"
#include <regspinctrl.h>
#include <regsuartdbg.h>


////////////////////////////////////////////////////////////////////////////////
//  Definitions
////////////////////////////////////////////////////////////////////////////////
//! Sets the maximum number of arguments for the simulation printf
#define MAX_PRINTF_ARGS 7

// Generate bitmask for use with HW_PINCTRL_MUXSELn registers
#define BM_PINCTRL_MUXSEL(msb, lsb) \
    (uint32_t)(((4 << (2*((msb)-(lsb)))) - 1) << (2*((lsb)&0xF)))

// These baud rate divisors assume a 24 MHz UARTCLK
// Integer Baud Rate Divisors
#define IBRD_460800     3
#define IBRD_230400     6
#define IBRD_115200     13
#define IBRD_57600      26
#define IBRD_38400      39
#define IBRD_19200      78
#define IBRD_14400      104
#define IBRD_9600       156
#define IBRD_4800       312
#define IBRD_2400       625
#define IBRD_1200       1250

// Fractional Baud Rate Divisors
#define FBRD_460800     16
#define FBRD_230400     33
#define FBRD_115200     1
#define FBRD_57600      3
#define FBRD_38400      4
#define FBRD_19200      2
#define FBRD_14400      11
#define FBRD_9600       16
#define FBRD_4800       33
#define FBRD_2400       0
#define FBRD_1200       0


////////////////////////////////////////////////////////////////////////////////
//! \brief  Initializes the debug uart
//!
//! Initializes the debug uart for 8,n,1 @115200 baud.
////////////////////////////////////////////////////////////////////////////////
void dbg_InitUart(void)
{
#ifdef TGT_DILLO
    // Connect external pins to uart1 (by default the pins are gpio's)
    // BANK3[11:10] = 10b, uart1_tx, uart1_rx
    HW_PINCTRL_MUXSEL6_SET(BM_PINCTRL_MUXSEL(11, 10));
    HW_PINCTRL_MUXSEL6_CLR(BM_PINCTRL_MUXSEL(11, 10) & 0x55555555);
#elif defined(TGT_3700)
      HW_PINCTRL_MUXSEL3_SET((0x3 << BP_PINCTRL_MUXSEL3_BANK1_PIN27) | (0x3 << BP_PINCTRL_MUXSEL3_BANK1_PIN26));
      HW_PINCTRL_MUXSEL3_CLR((0x1 << BP_PINCTRL_MUXSEL3_BANK1_PIN27) | (0x1 << BP_PINCTRL_MUXSEL3_BANK1_PIN26));
#elif defined(TGT_MX28)
      HW_PINCTRL_MUXSEL7_SET((0x3 << BP_PINCTRL_MUXSEL7_BANK3_PIN17) | (0x3 << BP_PINCTRL_MUXSEL7_BANK3_PIN16));
      HW_PINCTRL_MUXSEL7_CLR((0x1 << BP_PINCTRL_MUXSEL7_BANK3_PIN17) | (0x1 << BP_PINCTRL_MUXSEL7_BANK3_PIN16));
#endif

    // First, disable everything
    HW_UARTDBGCR_WR(0);
    
    // For now, fix baud rate @115200 baud
    HW_UARTDBGIBRD_WR(IBRD_115200);
    HW_UARTDBGFBRD_WR(FBRD_115200);

    // NOTE: This must happen AFTER setting the baud rate!
    // Set for 8 bits, 1 stop, no parity, enable fifo
    HW_UARTDBGLCR_H_WR(
        BF_UARTDBGLCR_H_WLEN(3) | 
        BF_UARTDBGLCR_H_FEN(1)
        );

    // Start it up
    HW_UARTDBGCR_WR(
        BF_UARTDBGCR_RXE(1) | 
        BF_UARTDBGCR_TXE(1) | 
        BF_UARTDBGCR_UARTEN(1)
        );
}


////////////////////////////////////////////////////////////////////////////////
//! \brief  Writes one character to the debug uart
//!
//! Writes one character to the debug uart. Initializes the uart if not already
//! done. Synchronous function waits for room in the uart fifo before returning.
//!
//! \param[in]  c   Character to write.
////////////////////////////////////////////////////////////////////////////////
void dbg_putchar(int c)
{
//    static bool bInitialized = false;
    
    // Initialize the uart if this is the first call
// MICHAL - image chooser prep already initialized debug UART
//    if (!bInitialized)
//    {
//        bInitialized = true;
//        dbg_InitUart();
//    }

    // Wait for space in FIFO, then write the char
    while (BF_RD(UARTDBGFR, TXFF) != 0)
        ;
    BF_WR(UARTDBGDR, DATA, c);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief  Writes a string to the debug uart
//!
//! Writes a null-terminated string to the debug uart. Outputs "\r\n" for each
//! '\n' character in the string. Returns when all characters of the string have
//! been written to the uart fifo.
//!
//! \param[in]  pStr    Pointer to the string.
////////////////////////////////////////////////////////////////////////////////
void dbg_puts(const char *pStr)
{
    // Write one character at a time
    while (*pStr)
    {
        // If this is a <LF> add a <CR>
        if (*pStr == '\n')
        {
            dbg_putchar('\r');
        }
        dbg_putchar(*pStr++);
    }
}

#include <stdio.h>
#include <stdarg.h>


uint32_t mem[MAX_PRINTF_ARGS];
char string[128];
#if 0
void dillo_printf(const char *fmt, ...)
{    
  va_list argptr;  
  va_start(argptr, fmt);
  vsprintf(string, fmt, argptr);
  va_end(argptr);
  dbg_puts(string);

  return;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//! \brief      terminate simulation run
//!
//! \fntype     Non-Reentrant
//!
//! \param[in]  16-bit return code that is output to simulation log
////////////////////////////////////////////////////////////////////////////////
void dillo_exit( uint32_t rc )
{

  dillo_printf("\ndillo_exit(%d)\n", rc);

  SystemHalt(); 
}

#endif 
// eof dbg_write.c
//! @}
