//////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//
//! \file   bootmode.h
//! \brief  Boot mode encodings for ROM
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
#ifndef _BOOTMODE_H
#define _BOOTMODE_H

#include "efuse.h"

//  * * NOTE: Bit 7 is the pulldown on LCD_RS and is not a bootmode bit * *
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------
// |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |                             |
// |     |     |     |     |     |     |     |     |  Boot Mode                  |
// |     |     |     |     |     |     |     |     |                             |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  | TM2 | TM1/| TM0/| BM3 | BM2 | BM1 | BM0 |                             |
// |     |     | BM5 | BM4 |     |     |     |     |                             |
// |-----+ ----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  0  |  0  |  0  |  0  |  USB                        |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  0  |  0  |  0  |  1  |  I2C Master                 |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  0  |  0  |  1  |  0  |  SPI Master@SSP2 (flash)    |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  0  |  0  |  1  |  1  |  SPI Master@SSP3 (flash)    |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  0  |  1  |  0  |  0  |  SD SSP0                    |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  0  |  1  |  0  |  1  |  SD SSP1                    |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  0  |  1  |  1  |  0  |  await JTAG connect in ROM  |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  x  |  x  |  x  |  x  |  0  |  1  |  1  |  1  |  Spare                      |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  1  |  0  |  0  |  0  |  SPI Master@SSP3 (eeprom)   |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  1  |  0  |  0  |  1  |  NAND                       |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  1  |  0  |  1  |  0  |  Spare                      |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  1  |  0  |  1  |  1  |  Spare                      |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  1  |  1  |  0  |  0  |  eMMC fast boot@SSP0        |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  x  |  E  |  x  |  1  |  1  |  0  |  1  |  eMMC fast boot@SSP1        |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  0  |  0  |  0  |  1  |  1  |  1  |  1  |  TESTER LOADER              |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  0  |  0  |  1  |  1  |  1  |  1  |  1  |  BURN-IN                    |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  0  |  1  |  0  |  1  |  1  |  1  |  1  |  ROM CRC                    |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  0  |  1  |  1  |  1  |  1  |  1  |  1  |  RAM BIST                   |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
// |  P  |  1  |  0  |  0  |  1  |  1  |  1  |  1  |  BIST (BUILT-IN SELF-TEST)  |
// |-----+-----+-----+-----+-----+-----+-----+-----+-----------------------------|
//
//                   L E G E N D
//
//   **** Where "V" is 0 for 3.3v devices and 1 for 1.8v devices ****
//   **** Where "x" means "don't care" ****
//   **** Where "E" means enable ETM on Bank 1 pins 7:0  ****
//   **** Where "P" (set to 1) means boot from PINS (in favor of OTP). This also allows
//          using "BM=xx" in simulation to set your bootmode (instead of modifying the
//          OTP bootmode bits in the efuse preload).
//          NOTE: Bit 7 attaches a pulldown to LCD_RS and is not a bootmode bit.
//
///////////////////////////////////////////////////////////////////////////////
// R11 holds the boot mode as represented below:
//
//          |---- Bit 7 is the pulldown on LCD_RS. If set to "1"
//          |       then the startup code will read the bootmode from pins.
//          |       In simulation, this allows you to set your bootmode with the "BM=xx" build option.
//          |
//          |   Bits 6-4 are Test Mode select if bits 3-0 are all 1's
//          |  |-----|
//         7  6  5  4  3  2  1  0
//                |  |  |--------|
//                |  |  Bits 3-0 select boot device. If all 1's then the boot mode
//                |  |  is a mfg test mode which is encoded in bits 6-4
//                |  |
//                |  |---- If not a mfg test mode then "1" means driver should configure
//                |          itself for 1.8v operation
//                |--- If not a mfg test mode then "1" means startup code will enable ETM
//

#define EXT_5V_PRESENT_BIT_POS   13
#define BM0_BIT                  (1 << 0)
#define BM1_BIT                  (1 << 1)
#define BM2_BIT                  (1 << 2)
#define BM3_BIT                  (1 << 3)
#define NAND_ECC_TYPE            BM3_BIT    // if "1", use 8bit ECC, otherwise use 4bit ECC.
#define TM0_BIT                  (1 << 4)
#define VOLTAGE_SEL_BIT          TM0_BIT   // if "1", 1.8v (driver should config itself for 1.8v)  Add 1.8 V device support boot media
#define TM1_BIT                  (1 << 5)
#define ENABLE_ETM_BIT           TM1_BIT   // if "1", ETM enabled in startup
#define TM2_BIT                  (1 << 6)
#define EXT_5V_PRESENT_BIT       (1 << EXT_5V_PRESENT_BIT_POS)   // test boot modes use this, don't change
#define BM_BITS                  0x1F
#define TM_BITS                  0x70
#define TM0_BM4_BIT              (1 << 4)
#define BOOT_MODE_BITS           0x7F
#define BM_DEVICE_BITS           0xF

// Boot Devices
#define BOOT_MODE_USB            0x00
#define BOOT_MODE_I2C            0x01
#define BOOT_MODE_SPI2_FLASH     0x02
#define BOOT_MODE_SPI3_FLASH     0x03
#define BOOT_MODE_NAND_BCH       0x04
#define BOOT_MODE_JTAG           0x06
#define BOOT_MODE_SPI3_EEPROM    0x08
#define BOOT_MODE_SD_SSP0        0x09
#define BOOT_MODE_SD_SSP1        0x0A


// Mfg Test Modes
#define BOOT_MODE_TESTER_LOADER  0x0F
#define BOOT_MODE_BURN_IN        0x1F
#define BOOT_MODE_ROM_CRC        0x2F
#define BOOT_MODE_RAM_TEST       0x3F
#define BOOT_MODE_BIST           0x4F

#ifdef STARTUP_DEBUG  // For TA1 testing - possibly discard on later ROM (remove assoc code from startup.c)
#define TEST_DATA_ABORT         0x7
#define TEST_PREFETCH_ABORT     0x9
#define TEST_UNDEF_EXCEPTION    0xA
#define TEST_SWI_EXCEPTION      0xB
#define TEST_FIQ_EXCEPTION      0xE
#endif

#endif // _BOOTMODE_H
//! @}
