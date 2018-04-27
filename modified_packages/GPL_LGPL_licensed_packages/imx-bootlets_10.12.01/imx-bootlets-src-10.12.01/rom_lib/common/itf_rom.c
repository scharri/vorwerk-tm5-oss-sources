////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    itf_rom.c
//! \brief   Implements the global rom interface table.
//!
////////////////////////////////////////////////////////////////////////////////
#include "itf_define.h"
#include "loader.h"

//! \todo These should be imported through a header file
extern const usb_BootItf_t  rom_UsbBootItf;
extern const i2c_BootItf_t  rom_i2c_BootItf;
extern const nand_BootItf_t rom_NandBootItf;
//extern const nor_BootItf_t  rom_nor_BootItf;
extern const spi_BootItf_t  rom_spi_BootItf;
extern const ssp_BootItf_t  rom_ssp_BootItf;
extern const sd_BootItf_t   rom_sd_BootItf;

////////////////////////////////////////////////////////////////////////////////
// The ROM Interface Patch Table
////////////////////////////////////////////////////////////////////////////////
rom_ItfTbl_t g_RomItf =
{
    &rom_LoaderItf,                         //!< Boot loader
    (usb_BootItf_t *) &rom_UsbBootItf,      //!< USB boot driver
    (i2c_BootItf_t *) &rom_i2c_BootItf,     //!< I2C boot driver
    (nand_BootItf_t *)&rom_NandBootItf,     //!< NAND boot driver
//    (nor_BootItf_t *) &rom_nor_BootItf,     //!< NOR boot driver
    (spi_BootItf_t *) &rom_spi_BootItf,     //!< SPI boot driver
    (ssp_BootItf_t *) &rom_ssp_BootItf,     //!< SSP HAL
    (sd_BootItf_t *)  &rom_sd_BootItf       //!< SD HAL
};


// eof itf_rom.c
//! @}
