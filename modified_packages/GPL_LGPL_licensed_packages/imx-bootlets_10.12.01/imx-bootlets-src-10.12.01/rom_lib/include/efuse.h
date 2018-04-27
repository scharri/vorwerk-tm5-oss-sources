////////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    efuse.h
//! \brief   eFuse definitions
//!
////////////////////////////////////////////////////////////////////////////////

#include "registers/regsocotp.h"


#ifndef __LANGUAGE_ASM__
//! Shadow copies of the rom efuse bank
//extern uint32_t ocotp_efuse_shadows[8] ;

//! Copies the rom efuse bank to shadow registers in OCRAM.
//! Called during startup.
//void ocotp_load_shadows( void ) ;

//! Opens eFuse banks for reading. This function exits with
//! eFuse banks are open, thus consuming power. Close banks
//! by calling rom_otp_close_banks().
int32_t ocotp_load_fuse_data( void ) ;

//! Close eFuse banks (remove power).
void ocotp_close_banks( void );
#endif

/* ----------------------------------------------- */
/*            ROM Fuses (Bank 3)                   */
/* ----------------------------------------------- */

//!  OCOTP_ROM7 (0x1F)
//!  Internal SigmaTel Use
//   * * * This row has internal risk mitigation features. * * *
//    Information in this row must not be given to customers
//    or exposed in the SDK code / documentation.
//
//  For simulations: this is efuse3.preload bits 0-31
//
// If not burnt, then skip reading LCD_RS to determine if a pin boot
// is required and go straight to reading bootmode
// from LCD_D[6:0]. NOTE: if this fuse is NOT burnt then we NEVER
// check OTP for bootmode.
#define BP_OCOTP_ROM7_ENABLE_PIN_BOOT_CHECK      0
#define BM_OCOTP_ROM7_ENABLE_PIN_BOOT_CHECK      0x00000001

//! 1 bit reserved.
#define BP_OCOTP_ROM7_MMU_DISABLE                1
#define BM_OCOTP_ROM7_MMU_DISABLE                0x00000002

// If not burnt, do not enable ARM ICACHE
#define BP_OCOTP_ROM7_ENABLE_ARM_ICACHE          2
#define BM_OCOTP_ROM7_ENABLE_ARM_ICACHE          0x00000004

//! Set I2C speed to 400kHz
#define BP_OCOTP_ROM7_I2C_USE_400kHz             3
#define BM_OCOTP_ROM7_I2C_USE_400kHz             0x00000008

// OCRAM trim value (4-bits)
#define BP_OCOTP_ROM7_OCRAM_TRIM                 4
#define BM_OCOTP_ROM7_OCRAM_TRIM                 0x000000F0 

// Force SSP 12ma drive
#define BP_OCOTP_ROM7_ENABLE_SSP_12MA_DRIVE      8
#define BM_OCOTP_ROM7_ENABLE_SSP_12MA_DRIVE      0x00000100

// reset usb phy at startup
#define BP_OCOTP_ROM7_RESET_USB_PHY_AT_STARTUP   9
#define BM_OCOTP_ROM7_RESET_USB_PHY_AT_STARTUP   0x00000200

// use maximum dataout size usb hid driver can support
#define BP_OCOTP_ROM7_USB_HID_SAFE_DATAOUT       10
#define BM_OCOTP_ROM7_USB_HID_SAFE_DATAOUT       0x00000400

// 1 reserved bits
#define BP_OCOTP_ROM7_HAB_DISABLE                11
#define BM_OCOTP_ROM7_HAB_DISABLE                0x00000800

//! recovery boot mode
#define BP_OCOTP_ROM7_RECOVERY_BOOT_MODE        12
#define BM_OCOTP_ROM7_RECOVERY_BOOT_MODE        0x000FF000

//! HAB config
#define BP_OCOTP_ROM7_HAB_CONFIG                20
#define BM_OCOTP_ROM7_HAB_CONFIG                0x00300000

//! HAB config
#define BP_OCOTP_ROM7_PLL_DISABLE                22
#define BM_OCOTP_ROM7_PLL_DISABLE                0x00400000

//! Disable force recovery 
#define BP_OCOTP_ROM7_FORCE_RECOVERY_DISABLE     23
#define BM_OCOTP_ROM7_FORCE_RECOVERY_DISABLE     0x00800000

//!  OCOTP_ROM6 (0x1E) - SDK USE ONLY
//
//
//! Disable Mfg test Modes (if burnt, disable) 
#define BP_OCOTP_ROM6_DISABLE_TEST_MODES        0
#define BM_OCOTP_ROM6_DISABLE_TEST_MODES        0x00000001

//! Disable Fix DDR_MODE to GPIO (if burnt, disable) 
#define BP_OCOTP_ROM6_DISABLE_FIX_DDR_MODE      1
#define BM_OCOTP_ROM6_DISABLE_FIX_DDR_MODE      0x00000002


//!  OCOTP_ROM5 (0x1D) - SDK USE ONLY
//!  **** OCOTP_ROM4 (0x1C) **** 


//! 4 bits to specify number of nand row address bytes
#define BP_OCOTP_ROM4_NAND_ROW_ADDRESS_BYTES    0
#define BM_OCOTP_ROM4_NAND_ROW_ADDRESS_BYTES    0x0000000F

//! 4 bits to specify number of nand column address bytes
#define BP_OCOTP_ROM4_NAND_COLUMN_ADDRESS_BYTES 4
#define BM_OCOTP_ROM4_NAND_COLUMN_ADDRESS_BYTES 0x000000F0

//! 8 bits for nand read cmd code1
#define BP_OCOTP_ROM4_NAND_READ_CMD_CODE1       8
#define BM_OCOTP_ROM4_NAND_READ_CMD_CODE1       0x0000FF00

//! 8 bits for nand read cmd code2
#define BP_OCOTP_ROM4_NAND_READ_CMD_CODE2       16
#define BM_OCOTP_ROM4_NAND_READ_CMD_CODE2       0x00FF0000


//! 1bit to specify if enabling factory bad block marker preservation
#define BP_OCOTP_ROM4_NAND_BADBLOCK_MARKER_PRESERVE_DISABLE       31
#define BM_OCOTP_ROM4_NAND_BADBLOCK_MARKER_PRESERVE_DISABLE       0x80000000

//!  **** OCOTP_ROM3 (0x1B) ****
//! 1 bit to enable the alternative fast boot mode
#define BP_OCOTP_ROM3_ALT_FAST_BOOT          10
#define BM_OCOTP_ROM3_ALT_FAST_BOOT          0x00000400 


//! 1 bit to enable the fast boot acknowledge
#define BP_OCOTP_ROM3_FAST_BOOT_ACK          11
#define BM_OCOTP_ROM3_FAST_BOOT_ACK          0x00000800


//! 1 bit to enable the nibble_pos bit in SSP_DDR_CTRL register 
#define BP_OCOTP_ROM3_ENABLE_NIBBLE_POS          12
#define BM_OCOTP_ROM3_ENABLE_NIBBLE_POS          0x00001000


//! 3 bits to configure the SSP_DLL_CTRL register 
#define BP_OCOTP_ROM3_SSP_DLL_CTRL          13
#define BM_OCOTP_ROM3_SSP_DLL_CTRL          0x0000E000


//!  **** OCOTP_ROM2 (0x1A) ****

//! 16 Bits USB PID
#define BP_OCOTP_ROM2_USB_PID                   0
#define BM_OCOTP_ROM2_USB_PID                   0x0000FFFF

//! 16 Bits USB PID
#define BP_OCOTP_ROM2_USB_VID                   16
#define BM_OCOTP_ROM2_USB_VID                   0xFFFF0000

//! ****  OCOTP_ROM1 (0x19) ****
// For simulations: this is efuse3.preload bits 192-223

//! 3 Bits representing up to 8 NANDs.
#define BP_OCOTP_ROM1_NUMBER_OF_NANDS         0
#define BM_OCOTP_ROM1_NUMBER_OF_NANDS         0x00000007

//! 1 bit is reserved. in huashan, this bit is NAND_BUS_WIDTH_16
#define BP_OCOTP_ROM1_RESERVED_BIT3           3
#define BM_OCOTP_ROM1_RESERVED_BIT3           0x00000008

//! 4 bits for nand boot search stride
#define BP_OCOTP_ROM1_BOOT_SEARCH_STRIDE      4
#define BM_OCOTP_ROM1_BOOT_SEARCH_STRIDE      0x000000F0

//! 4 Bits representing 2 ^ value 64 page blocks should be read.
#define BP_OCOTP_ROM1_BOOT_SEARCH_COUNT       8
#define BM_OCOTP_ROM1_BOOT_SEARCH_COUNT       0x00000F00

//! 1 Bit representing whether the Alternate pins for SPP1 data lines 4, 5, 6 and 7 should be used.
#define BP_OCOTP_ROM1_RESERVED_BIT12          12
#define BM_OCOTP_ROM1_RESERVED_BIT12          0x00001000

//! 1 bit to disable the 1st init seq in SD
#define BP_OCOTP_ROM1_SD_INIT_SEQ_1_DISABLE   13
#define BM_OCOTP_ROM1_SD_INIT_SEQ_1_DISABLE   0x00002000

//! 1 bit to disable cmd0 in SD
#define BP_OCOTP_ROM1_SD_CMD0_DISABLE         14
#define BM_OCOTP_ROM1_SD_CMD0_DISABLE         0x00004000

//! 1 bit to disable the 2nd init seq in SD
#define BP_OCOTP_ROM1_SD_INIT_SEQ_2_ENABLE   15
#define BM_OCOTP_ROM1_SD_INIT_SEQ_2_ENABLE   0x00008000

//! 1 bit to increase the SD init sequence time
#define BP_OCOTP_ROM1_SD_INCREASE_INIT_SEQ_TIME   16
#define BM_OCOTP_ROM1_SD_INCREASE_INIT_SEQ_TIME   0x00010000

//! 1 bit to indicate external pullups are implemented for SSP0 hardware
#define BP_OCOTP_ROM1_SSP0_EXT_PULLUP         17
#define BM_OCOTP_ROM1_SSP0_EXT_PULLUP         0x00020000

//! 1 bit to indicate external pullups are implemented for SSP1 hardware
#define BP_OCOTP_ROM1_SSP1_EXT_PULLUP         18
#define BM_OCOTP_ROM1_SSP1_EXT_PULLUP         0x00040000

//! 1 bit, is meaningful only for the case if external pullups are 
//! implemented then this bit will be checked to ignore programming internal 
//! pull ups.
#define BP_OCOTP_ROM1_UNTOUCH_INT_SSP_PULLUP          19
#define BM_OCOTP_ROM1_UNTOUCH_INT_SSP_PULLUP          0x00080000

//! If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE0 and GPMI_RDY0
#define BP_OCOTP_ROM1_ENABLE_NAND0_CE_RDY_PULLUP      20
#define BM_OCOTP_ROM1_ENABLE_NAND0_CE_RDY_PULLUP      0x00100000

//! If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE1 and GPMI_RDY1
#define BP_OCOTP_ROM1_ENABLE_NAND1_CE_RDY_PULLUP      21
#define BM_OCOTP_ROM1_ENABLE_NAND1_CE_RDY_PULLUP      0x00200000

//! If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE2 and GPMI_RDY2
#define BP_OCOTP_ROM1_ENABLE_NAND2_CE_RDY_PULLUP      22
#define BM_OCOTP_ROM1_ENABLE_NAND2_CE_RDY_PULLUP      0x00400000

//! If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE3 and GPMI_RDY3
#define BP_OCOTP_ROM1_ENABLE_NAND3_CE_RDY_PULLUP      23
#define BM_OCOTP_ROM1_ENABLE_NAND3_CE_RDY_PULLUP      0x00800000

//To be removed in ER2
//! Mask for OPT NAND CE RDY PULLUP bits
#define BM_OCOTP_ROM1_ENABLE_NAND_CE_RDY_PULLUPS      0x0FF00000

//! If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE4 and GPMI_RDY4
#define BP_OCOTP_ROM1_ENABLE_NAND4_CE_RDY_PULLUP      24          
#define BM_OCOTP_ROM1_ENABLE_NAND4_CE_RDY_PULLUP      0x01000000

//! If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE5 and GPMI_RDY5
#define BP_OCOTP_ROM1_ENABLE_NAND5_CE_RDY_PULLUP      25          
#define BM_OCOTP_ROM1_ENABLE_NAND5_CE_RDY_PULLUP      0x02000000

//! If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE6 and GPMI_RDY6
#define BP_OCOTP_ROM1_ENABLE_NAND6_CE_RDY_PULLUP      26          
#define BM_OCOTP_ROM1_ENABLE_NAND6_CE_RDY_PULLUP      0x04000000

//! If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE7 and GPMI_RDY7
#define BP_OCOTP_ROM1_ENABLE_NAND7_CE_RDY_PULLUP      27          
#define BM_OCOTP_ROM1_ENABLE_NAND7_CE_RDY_PULLUP      0x08000000

//! 1 bit to indicate external pullups are implemented for SSP2 hardware
#define BP_OCOTP_ROM1_SSP2_EXT_PULLUP         28
#define BM_OCOTP_ROM1_SSP2_EXT_PULLUP         0x10000000

//! 1 bit to indicate external pullups are implemented for SSP3 hardware
#define BP_OCOTP_ROM1_SSP3_EXT_PULLUP         29
#define BM_OCOTP_ROM1_SSP3_EXT_PULLUP         0x20000000

//! One bit to disable secondary boot
#define BP_OCOTP_ROM1_DISABLE_SECONDARY_BOOT          30          
#define BM_OCOTP_ROM1_DISABLE_SECONDARY_BOOT          0x40000000

//! Use Alternate Debug UART pins (if burnt, use alternate)
//! Note: Primary means BANK3_PIN[17:16], 
//!       Secondary means BANK3_PIN[3:2]
//!       Third means BANK3[25:24]
#define BP_OCOTP_ROM0_DEBUG_UART_PINS_ALT             0
#define BM_OCOTP_ROM0_DEBUG_UART_PINS_ALT             0x00000003
#define BM_OCOTP_ROM0_DEBUG_UART_PINS_ALT__PWM        0
#define BM_OCOTP_ROM0_DEBUG_UART_PINS_ALT__AUART0     1
#define BM_OCOTP_ROM0_DEBUG_UART_PINS_ALT__I2C0       2

// the location of OCOTP_ROM0_DEBUG_UART_PINS_ALT is moved to the next bit to
// workaround a problem with burnin issue that would set the LSB of each
// OTP bank. As such ROM0.0 will be reserved for 37xxs.
//#define BP_OCOTP_ROM0_DEBUG_UART_PINS_ALT     1
//#define BM_OCOTP_ROM0_DEBUG_UART_PINS_ALT     0x00000002
//! Disable Mfg test Modes (if burnt, disable) 
//#define BP_OCOTP_ROM0_DISABLE_TEST_MODES      1           // moved to register ROM6
//#define BM_OCOTP_ROM0_DISABLE_TEST_MODES      0x00000002  // moved to register ROM6

//! Recovery Mode Allowed (if burnt, not allowed)
#define BP_OCOTP_ROM0_DISABLE_RECOVERY_MODE           2
#define BM_OCOTP_ROM0_DISABLE_RECOVERY_MODE           0x00000004

//! Enable SD MBR BOOT
#define BP_OCOTP_ROM0_RESERVED_BIT3                   3
#define BM_OCOTP_ROM0_RESERVED_BIT3                   0x00000008

//! Enable unencrypted boot modes.
//! (if burnt, unencrypted boot modes allowed)
#define BP_OCOTP_ROM0_ENABLE_UNENCRYPTED_BOOT         4
#define BM_OCOTP_ROM0_ENABLE_UNENCRYPTED_BOOT         0x00000010

//! USB Boot Serial Number Enable (if burnt, enabled)
#define BP_OCOTP_ROM0_USB_BOOT_SERIAL_NUMBER_ENABLE   5
#define BM_OCOTP_ROM0_USB_BOOT_SERIAL_NUMBER_ENABLE   0x00000020

//! Disable NOR fast reads
#define BP_OCOTP_ROM0_DISABLE_SPI_NOR_FAST_READ       6
#define BM_OCOTP_ROM0_DISABLE_SPI_NOR_FAST_READ       0x00000040

//! Blown to enable DDR mode for eMMC4.4
#define BP_OCOTP_ROM0_EMMC_USE_DDR                    7
#define BM_OCOTP_ROM0_EMMC_USE_DDR                    0x00000080 

//! SSP SCK index
//! Bit 11~8
#define BP_OCOTP_ROM0_SSP_SCK_INDEX                   8
#define BM_OCOTP_ROM0_SSP_SCK_INDEX                   0x00000F00
// Add here SCK index description

//! SD bus width
//! 00b = 4-bit
//! 01b = 1-bit
//! 10b = 8-bit
//! 11b = Reserved
#define BP_OCOTP_ROM0_SD_BUS_WIDTH                     12
#define BM_OCOTP_ROM0_SD_BUS_WIDTH                     0x00003000

//! SD power up delay
#define BP_OCOTP_ROM0_SD_POWER_UP_DELAY                14
#define BM_OCOTP_ROM0_SD_POWER_UP_DELAY                0x000FC000

//! SD power gate GPIO
#define BP_OCOTP_ROM0_SD_POWER_GATE_GPIO               20
#define BM_OCOTP_ROM0_SD_POWER_GATE_GPIO               0x00300000

//! SD/MMC boot modes
#define BP_OCOTP_ROM0_SD_MMC_MODE                      22
#define BM_OCOTP_ROM0_SD_MMC_MODE                      0x00C00000 
#define BV_OCOTP_ROM0_SD_MMC_MODE__MBR_BOOT            0x0
#define BV_OCOTP_ROM0_SD_MMC_MODE__BCB_BOOT            0x1
#define BV_OCOTP_ROM0_SD_MMC_MODE__EMMC_FAST_BOOT      0x2
#define BV_OCOTP_ROM0_SD_MMC_MODE__ESD_FAST_BOOT       0x3

//! Boot mode
#define BP_OCOTP_ROM0_BOOT_MODE                        24
#define BM_OCOTP_ROM0_BOOT_MODE                        0xFF000000


/* ----------------------------------------------- */
/*         Operations Fuses  (Bank 2)              */
/* ----------------------------------------------- */


/* ----------------------------------------------- */
/*         Capability Fuses (Bank 1)               */
/* ----------------------------------------------- */


/* ----------------------------------------------- */
/*         Customer Fuses (Bank 0)                 */
/* ----------------------------------------------- */


// eof efuse.h
//! @}
