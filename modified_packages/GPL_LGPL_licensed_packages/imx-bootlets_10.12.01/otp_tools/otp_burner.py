#!/usr/bin/env python

# Copyright (c) 2008-2010 Freescale Semiconductor, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# o Redistributions of source code must retain the above copyright notice, this list
#   of conditions and the following disclaimer.
#
# o Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimer in the documentation and/or
#   other materials provided with the distribution.
#
# o Neither the name of Freescale Semiconductor, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import sys, os, string, struct, optparse
from sgtl.utility import lexerbase
from sgtl.utility import bufferedstream
from sgtl import build

##
# @main
#
# Version history:
#
# 1.3.2:
# - New version of pitc_otp_mfg_mx28 that fixes a problem where it was checking the wrong lock bit
#   for srk registers and hence skipping some of them.
# - Corrected duplicate 'lock_hwsw_shadow' bitfield name for MX28 (was missing 'lock_hwsw').
# - Updated chip names in the accompanying bit_settings_example.txt file.
#
# 1.3.1:
# - Updated bit field definitions from final MX28 reference manual rev 1.
#
# 1.3.0:
# - Added --hab and --no-hab command line options.
# - Setting chip family in elftosb command line.
#
# 1.2.1:
# - Changed license to BSD.
# - The .dat file is now correctly opened as binary.
# - Added --version command line option.
#
# 1.2.0:
# - Added mx28 family and related bitfields and constants.
# - Added super root key file loading support for mx28.
#
# 1.1.0:
# - Added 3770 and 3780 support.

# Tool version number and copyright string.
kToolVersion = "1.3.2"
kToolCopyright = "Copyright (c) 2008-2012 Freescale Semiconductor, Inc. All rights reserved."

##
# @name Chip families

#@{

## 3700 chip family constant.
k3700Family = '3700'

## 3770 chip family constant.
k3770Family = '3770'

## 3780 chip family constant.
k3780Family = '3780'

## i.MX28 chip family constant.
kMX28Family = 'mx28'

#@}

##
# @name Bit fields

#@{

##
# @brief Table of named OTP bit fields common to all chips.
#
# Some bit fields represent entire OTP registers, while others are individual
# fields within a single register.
#
# The table is an array containing tuples that all have the same elements:
#	- Bit field name
#	- OTP bank
#	- OTP word
#	- Upper bit number
#	- Lower bit number
kOTPBitFieldsCommon = [
		# Bank 0
		('hw_ocotp_cust0', 0, 0, 31, 0),
		('hw_ocotp_cust1', 0, 1, 31, 0),
		('hw_ocotp_cust2', 0, 2, 31, 0),
		('hw_ocotp_cust3', 0, 3, 31, 0),
		('hw_ocotp_crypto0', 0, 4, 31, 0),
		('hw_ocotp_crypto1', 0, 5, 31, 0),
		('hw_ocotp_crypto2', 0, 6, 31, 0),
		('hw_ocotp_crypto3', 0, 7, 31, 0),

		# Bank 1
		('hw_ocotp_hwcap0', 1, 0, 31, 0),
		('hw_ocotp_hwcap1', 1, 1, 31, 0),
		('hw_ocotp_hwcap2', 1, 2, 31, 0),
		('hw_ocotp_hwcap3', 1, 3, 31, 0),
		('hw_ocotp_hwcap4', 1, 4, 31, 0),
		('hw_ocotp_hwcap5', 1, 5, 31, 0),
		('hw_ocotp_swcap', 1, 6, 31, 0),
		('hw_ocotp_custcap', 1, 7, 31, 0),

		# Bank 2
		('hw_ocotp_lock', 2, 0, 31, 0),
		('hw_ocotp_ops0', 2, 1, 31, 0),
		('hw_ocotp_ops1', 2, 2, 31, 0),
		('hw_ocotp_ops2', 2, 3, 31, 0),
		('hw_ocotp_ops3', 2, 4, 31, 0),
		('hw_ocotp_un0', 2, 5, 31, 0),
		('hw_ocotp_un1', 2, 6, 31, 0),
		('hw_ocotp_un2', 2, 7, 31, 0),

		# Bank 3
		('hw_ocotp_rom0', 3, 0, 31, 0),
		('hw_ocotp_rom1', 3, 1, 31, 0),
		('hw_ocotp_rom2', 3, 2, 31, 0),
		('hw_ocotp_rom3', 3, 3, 31, 0),
		('hw_ocotp_rom4', 3, 4, 31, 0),
		('hw_ocotp_rom5', 3, 5, 31, 0),
		('hw_ocotp_rom6', 3, 6, 31, 0),
		('hw_ocotp_rom7', 3, 7, 31, 0),

		# HW_OCOTP_CUSTCAP: Bank 1 Word 7
		('rtc_xtal_32768_present', 1, 7, 2, 2),
		('rtc_xtal_32000_present', 1, 7, 1, 1),
		('lba_nand_populated', 1, 7, 5, 5),     # This was a custom field for a customer project.

		# HW_OCOTP_LOCK: Bank 2 Word 0
		('lock_cust0', 2, 0, 0, 0),
		('lock_cust1', 2, 0, 1, 1),
		('lock_cust2', 2, 0, 2, 2),
		('lock_cust3', 2, 0, 3, 3),
		('lock_cryptokey', 2, 0, 4, 4),
		('lock_cryptodcp', 2, 0, 5, 5),
		('lock_custcap_shadow', 2, 0, 7, 7),
		('lock_custcap', 2, 0, 9, 9),
		('lock_cryptokey_alt', 2, 0, 21, 21),
		('lock_cryptodcp_alt', 2, 0, 22, 22),
		('lock_rom0', 2, 0, 24, 24),
		('lock_rom1', 2, 0, 25, 25),
		('lock_rom2', 2, 0, 26, 26),
		('lock_rom3', 2, 0, 27, 27),
		('lock_rom4', 2, 0, 28, 28),
		('lock_rom5', 2, 0, 29, 29),
		('lock_rom6', 2, 0, 30, 30),
		('lock_rom7', 2, 0, 31, 31),

		# HW_OCOTP_ROM0: Bank 3 Word 0
		('boot_mode', 3, 0, 31, 24),
		('sd_power_gate_gpio', 3, 0, 21, 20),
		('sd_power_up_delay', 3, 0, 19, 14),
		('sd_bus_width', 3, 0, 13, 12),
		('ssp_sck_index', 3, 0, 11, 8),
		('disable_spi_nor_fast_read', 3, 0, 6, 6),
		('enable_usb_boot_serial_num', 3, 0, 5, 5),
		('enable_unencrypted_boot', 3, 0, 4, 4),
		('disable_recovery_mode', 3, 0, 2, 2),

		# HW_OCOTP_ROM1: Bank 3 Word 1
		('sd_increase_init_seq_time', 3, 1, 16, 16),
		('sd_init_seq_2_enable', 3, 1, 15, 15),
		('sd_cmd0_disable', 3, 1, 14, 14),
		('sd_init_seq_1_disable', 3, 1, 13, 13),
		('boot_search_count', 3, 1, 11, 8),
		('number_of_nands', 3, 1, 2, 0),

		# HW_OCOTP_ROM2: Bank 3 Word 2
		('usb_vid', 3, 2, 31, 16),
		('usb_pid', 3, 2, 15, 0),

		# HW_OCOTP_ROM6: Bank 3 Word 6
		('disable_test_modes', 3, 6, 0, 0),

		# HW_OCOTP_ROM7: Bank 3 Word 7
		('reset_usb_phy_at_startup', 3, 7, 9, 9),
		('enable_ssp_12ma_drive', 3, 7, 8, 8),
		('i2c_use_400khz', 3, 7, 3, 3),
		('enable_arm_icache', 3, 7, 2, 2),
		('enable_pin_boot_check', 3, 7, 0, 0)
	]

## Bit fields common to 37xx families, but not mx28.
kOTPBitFieldsCommon37xx = [
		# HW_OCOTP_CUSTCAP: Bank 1 Word 7
		('cust_jtag_lockout', 1, 7, 0, 0),

		# HW_OCOTP_ROM0: Bank 3 Word 0
		('enable_pjtag_12ma_drive', 3, 0, 23, 23),
		('use_parallel_jtag', 3, 0, 22, 22),
		('sd_mbr_boot', 3, 0, 3, 3),

		# HW_OCOTP_ROM1: Bank 3 Word 1
		('ssp2_ext_pullup', 3, 1, 18, 18),
		('ssp1_ext_pullup', 3, 1, 17, 17),

		# HW_OCOTP_ROM3: Bank 3 Word 3
		('osc_trim', 3, 3, 9, 0),

		# HW_OCOTP_ROM7: Bank 3 Word 7
		('ocram_ss_trim', 3, 7, 7, 4),
	]

## Bit fields unique to 3700.
kOTPBitFields3700 = [
		# HW_OCOTP_ROM0: Bank 3 Word 0
		('use_alt_debug_uart_pins', 3, 0, 1, 1),

		# HW_OCOTP_ROM1: Bank 3 Word 1
		('use_alternate_ce', 3, 1, 12, 12)
	]

## Additional bit fields for 3770.
kOTPBitFields3770 = [
		# HW_OCOTP_ROM0: Bank 3 Word 0
		('use_alt_debug_uart_pins', 3, 0, 1, 1),

		# HW_OCOTP_ROM1: Bank 3 Word 1
		('enable_nand3_ce_rdy_pullup', 3, 1, 23, 23),
		('enable_nand2_ce_rdy_pullup', 3, 1, 22, 22),
		('enable_nand1_ce_rdy_pullup', 3, 1, 21, 21),
		('enable_nand0_ce_rdy_pullup', 3, 1, 20, 20),
		('untouch_int_pullup', 3, 1, 19, 19),
		('use_alternate_ce', 3, 1, 12, 12)
	]

## Additional bit fields for 3780.
kOTPBitFields3780 = [
		# Bank 2
		('hw_ocotp_ops6', 2, 7, 31, 0), # Replaces hw_ocotp_un2

		# HW_OCOTP_CUSTCAP: Bank 1 Word 7
		('enable_sjtag_12ma_drive', 1, 7, 4, 4),
		('use_parallel_jtag', 1, 7, 3, 3),

		# HW_OCOTP_ROM1: Bank 3 Word 1
		('use_alt_gpmi_rdy3', 3, 1, 29, 28),
		('use_alt_gpmi_ce3', 3, 1, 27, 26),
		('use_alt_gpmi_rdy2', 3, 1, 25, 25),
		('use_alt_gpmi_ce2', 3, 1, 24, 24),
		('enable_nand3_ce_rdy_pullup', 3, 1, 23, 23),
		('enable_nand2_ce_rdy_pullup', 3, 1, 22, 22),
		('enable_nand1_ce_rdy_pullup', 3, 1, 21, 21),
		('enable_nand0_ce_rdy_pullup', 3, 1, 20, 20),
		('untouch_internal_ssp_pullup', 3, 1, 19, 19),
		('use_alt_ssp1_data4_7', 3, 1, 12, 12),

		# HW_OCOTP_ROM7: Bank 3 Word 7
		('recovery_boot_mode', 3, 7, 19, 12)
	]

## Bit fields unique to the mx28.
kOTPBitFieldsMX28 = [
		# Bank 4 - Super Root Key
		('hw_ocotp_srk0', 4, 0, 31, 0),
		('hw_ocotp_srk1', 4, 1, 31, 0),
		('hw_ocotp_srk2', 4, 2, 31, 0),
		('hw_ocotp_srk3', 4, 3, 31, 0),
		('hw_ocotp_srk4', 4, 4, 31, 0),
		('hw_ocotp_srk5', 4, 5, 31, 0),
		('hw_ocotp_srk6', 4, 6, 31, 0),
		('hw_ocotp_srk7', 4, 7, 31, 0),

		# HW_OCOTP_LOCK
		('lock_hwsw_shadow', 2, 0, 6, 6),
		('lock_hwsw', 2, 0, 8, 8),
		('lock_rom_shadow', 2, 0, 10, 10),
		('lock_srk_shadow', 2, 0, 11, 11),
		('lock_srk', 2, 0, 15, 15),
		('lock_un0', 2, 0, 16, 16),
		('lock_un1', 2, 0, 17, 17),
		('lock_un2', 2, 0, 18, 18),
		('lock_ops', 2, 0, 19, 19),
		('lock_pin', 2, 0, 20, 20),
		('lock_hwsw_shadow_alt', 2, 0, 23, 23),

		# HW_OCOTP_ROM0: Bank 3 Word 0
		('sd_mmc_mode', 3, 0, 23, 22),
		('emmc_use_ddr', 3, 0, 7, 7),
		('use_alt_debug_uart_pins', 3, 0, 1, 0),

		# HW_OCOTP_ROM1: Bank 3 Word 1
		('ssp3_ext_pullup', 3, 1, 29, 29),
		('ssp2_ext_pullup', 3, 1, 28, 28),
		('enable_nand7_ce_rdy_pullup', 3, 1, 27, 27),
		('enable_nand6_ce_rdy_pullup', 3, 1, 26, 26),
		('enable_nand5_ce_rdy_pullup', 3, 1, 25, 25),
		('enable_nand4_ce_rdy_pullup', 3, 1, 24, 24),
		('enable_nand3_ce_rdy_pullup', 3, 1, 23, 23),
		('enable_nand2_ce_rdy_pullup', 3, 1, 22, 22),
		('enable_nand1_ce_rdy_pullup', 3, 1, 21, 21),
		('enable_nand0_ce_rdy_pullup', 3, 1, 20, 20),
		('untouch_internal_ssp_pullup', 3, 1, 19, 19),
		('ssp1_ext_pullup', 3, 1, 18, 18),
		('ssp0_ext_pullup', 3, 1, 17, 17),
		('boot_search_stride', 3, 1, 7, 4),

		# HW_OCOTP_ROM3: Bank 3 Word 3
		('fast_boot_ack', 3, 3, 11, 11),
		('alt_fast_boot', 3, 3, 10, 10),

		# HW_OCOTP_ROM4: Bank 3 Word 4
		('nand_badblock_marker_reserve', 3, 4, 31, 31),
		('nand_read_cmd_code2', 3, 4, 23, 16),
		('nand_read_cmd_code1', 3, 4, 15, 8),
		('nand_column_address_bytes', 3, 4, 7, 4),
		('nand_row_address_bytes', 3, 4, 3, 0),

		# HW_OCOTP_ROM7: Bank 3 Word 7
		('force_recovery_disable', 3, 7, 23, 23),
		('arm_pll_disable', 3, 7, 22, 22),
		('hab_config', 3, 7, 21, 20),
		('recovery_boot_mode', 3, 7, 19, 12),
		('hab_disable', 3, 7, 11, 11),
		('mmu_disable', 3, 7, 1, 1),
	]

#@}

##
# @name Bit Field Value Constants

#@{

## Constant names and values.
kOTPConstantsCommon = {
		# Generic constants.
		'yes' : 1,
		'true' : 1,
		'on' : 1,
		'blown' : 1,
		'no' : 0,
		'false' : 0,
		'off' : 0,
		'unblown' : 0,

		# SD_BUS_WIDTH constants.
		'sd_bus_width_4bit' : 0,
		'sd_bus_width_1bit' : 1,
		'sd_bus_width_8bit' : 2,
	}

## Constants for 3700 and 3770.
kOTPConstants37xx = {
		# SD_POWER_GATE_GPIO constants.
		'sd_power_gate_gpio_pwm3' : 0x0,
		'sd_power_gate_gpio_pwm4' : 0x1,
		'sd_power_gate_gpio_rotarya' : 0x2,
		'sd_power_gate_gpio_no_gate' : 0x3,

		# BOOT_MODE constants.
		'boot_mode_usb' : 0x00,
		'boot_mode_i2c_3v3' : 0x01,
		'boot_mode_i2c_1v8' : 0x11,
		'boot_mode_spi_flash_ssp1_3v3' : 0x02,
		'boot_mode_spi_flash_ssp1_1v8' : 0x12,
		'boot_mode_spi_flash_ssp2_3v3' : 0x03,
		'boot_mode_spi_flash_ssp2_1v8' : 0x13,
		'boot_mode_nand_ecc4_3v3' : 0x04,
		'boot_mode_nand_ecc4_1v8' : 0x14,
		'boot_mode_nor_16bit_3v3' : 0x05,
		'boot_mode_nor_16bit_1v8' : 0x15,
		'boot_mode_jtag_wait' : 0x06,
		'boot_mode_spi_eeprom_ssp2_3v3' : 0x08,
		'boot_mode_spi_eeprom_ssp2_1v8' : 0x18,
		'boot_mode_sdmmc_ssp1_3v3' : 0x09,
		'boot_mode_sdmmc_ssp1_1v8' : 0x19,
		'boot_mode_sdmmc_ssp2_3v3' : 0x0a,
		'boot_mode_sdmmc_ssp2_1v8' : 0x1a,
		'boot_mode_nand_ecc8_3v3' : 0x0c,
		'boot_mode_nand_ecc8_1v8' : 0x1c,
		'boot_mode_nor_8bit_3v3' : 0x0d,
		'boot_mode_nor_8bit_1v8' : 0x1d
	}

## Extra constants for 3780.
kOTPConstants3780 = {
		# SD_POWER_GATE_GPIO constants.
		'sd_power_gate_gpio_pwm0' : 0x0,
		'sd_power_gate_gpio_lcd_dotclk' : 0x1,
		'sd_power_gate_gpio_pwm3' : 0x2,
		'sd_power_gate_gpio_no_gate' : 0x3,

		# USE_ALT_GPMI_RDY3 constants.
		'use_alt_gpmi_rdy3_gpmi_rdy3' : 0x0,
		'use_alt_gpmi_rdy3_pwm2' : 0x1,
		'use_alt_gpmi_rdy3_lcd_dotclk' : 0x2,

		# USE_ALT_GPMI_CE3 constants.
		'use_alt_gpmi_ce3_gpmi_d15' : 0x0,
		'use_alt_gpmi_ce3_lcd_reset' : 0x1,
		'use_alt_gpmi_ce3_ssp_detect' : 0x2,
		'use_alt_gpmi_ce3_rotaryb' : 0x3,

		# BOOT_MODE constants.
		'boot_mode_usb' : 0x00,
		'boot_mode_i2c' : 0x01,
		'boot_mode_spi_flash_ssp1' : 0x02,
		'boot_mode_spi_flash_ssp2' : 0x03,
		'boot_mode_nand' : 0x04,
		'boot_mode_jtag_wait' : 0x06,
		'boot_mode_spi_eeprom_ssp2' : 0x08,
		'boot_mode_sdmmc_ssp1' : 0x09,
		'boot_mode_sdmmc_ssp2' : 0x0a
	}

## Extra constants for i.MX28.
kOTPConstantsMX28 = {
		# SD_POWER_GATE_GPIO constants.
		'sd_power_gate_gpio_pwm3' : 0x0,
		'sd_power_gate_gpio_pwm4' : 0x1,
		'sd_power_gate_gpio_lcd_dotclk' : 0x2,
		'sd_power_gate_gpio_no_gate' : 0x3,

		# SD_MMC_MODE constants.
		'sd_mmc_mode_mbr' : 0x0,
		'sd_mmc_mode_emmc_fast_boot' : 0x2,
		'sd_mmc_mode_esd_fast_boot' : 0x3,

		# HAB_CONFIG constants.
		'hab_config_hab_fab' : 0x0,
		'hab_config_hab_open' : 0x1,
		'hab_config_hab_close' : 0x2,

		# BOOT_MODE constants.
		'boot_mode_usb' : 0x00,
		'boot_mode_i2c_3v3' : 0x01,
		'boot_mode_i2c_1v8' : 0x11,
		'boot_mode_spi_flash_ssp2_3v3' : 0x02,
		'boot_mode_spi_flash_ssp2_1v8' : 0x12,
		'boot_mode_spi_flash_ssp3_3v3' : 0x03,
		'boot_mode_spi_flash_ssp3_1v8' : 0x13,
		'boot_mode_nand_3v3' : 0x04,
		'boot_mode_nand_1v8' : 0x14,
		'boot_mode_jtag_wait' : 0x06,
		'boot_mode_spi_eeprom_ssp3_3v3' : 0x08,
		'boot_mode_spi_eeprom_ssp3_1v8' : 0x18,
		'boot_mode_sdmmc_ssp0_3v3' : 0x09,
		'boot_mode_sdmmc_ssp0_1v8' : 0x19,
		'boot_mode_sdmmc_ssp1_3v3' : 0x0a,
		'boot_mode_sdmmc_ssp1_1v8' : 0x1a,

	}

#@}

##
# @name Token classes
#
# Instances of these classes will be returned by OTPFileLexer.

#@{

# Tokens produced by the boolean expression lexical analyser.
class IdentifierToken(lexerbase.TokenBase): pass
class OpenBracketToken(lexerbase.TokenBase): pass
class CloseBracketToken(lexerbase.TokenBase): pass
class ColonToken(lexerbase.TokenBase): pass
class EqualsToken(lexerbase.TokenBase): pass
class StarToken(lexerbase.TokenBase): pass

class IntegerLiteralToken(lexerbase.TokenBase):
	def __init__(self, numberString, base=10):
		self._base = base
		self._string = numberString
		self._value = string.atoi(numberString, base)

#@}

## Class of exception raised by the lexer and parser.
class ParseException(Exception):
	def __init__(self, msg, line):
		self.msg = msg
		self.line = line

	def __str__(self):
		return "%s (line %d)" % (self.msg, self.line)

##
# @brief Lexical analyser for OTP bit settings files.
#
# Comments start with a '#' and run to the end of line. Any end of line
# sequences are supported: CR, LF, and CRLF. And even mixed EOL types are
# allowed.
#
class OTPFileLexer(lexerbase.LexerBase):
	def readNextToken(self):
		while True:
			c = self.get()
			if not c:
				break

			if c in lexerbase.lettersAndUnderscore:
				ident = c + self.readIdentifier()
				return IdentifierToken(ident)
			elif c in string.digits:
				numberString = ''
				base = 10
				baseDigits = string.digits

				# Handle a leading zero and look for a base prefix.
				if c == '0':
					pc = self.get()
					if pc == 'x':
						base = 16
						baseDigits = string.hexdigits
					elif pc == 'b':
						base = 2
						baseDigits = '01'
					elif pc in string.digits:
						numberString += '0' + pc
					else:
						self.put(pc)
						return IntegerLiteralToken('0')
				else:
					numberString += c

				# Build up the string representation of the number character by character.
				c = self.get()
				if not c:
					if len(numberString):
						# File ended with an integer literal
						return IntegerLiteralToken(numberString, base)
					else:
						# Incomplete integer literal
						break
				while c in baseDigits:
					numberString += c
					c = self.get()
					if not c:
						break

				# Put back the last character since we know it didn't match
				if c:
					self.put(c)

				return IntegerLiteralToken(numberString, base)

			elif c == '[':
				return OpenBracketToken()
			elif c == ']':
				return CloseBracketToken()
			elif c == ':':
				return ColonToken()
			elif c == '=':
				return EqualsToken()
			elif c == '*':
				return StarToken()
			elif c == '#':
				# skip through the EOL to pass comments
				c = self.get()
				while c not in lexerbase.CRLF:
					c = self.get()
				self.scanEOL(c)
			elif c in lexerbase.CRLF:
				self.scanEOL(c)
			elif c in lexerbase.whitespace:
				pass
			else:
				raise ParseException("Unexpected character '%s'" % c, self.getLineNumber())

		return None

##
# @brief Class that defines the methods that the OTP file parser invokes.
#
class OTPFileParserDelegate:
	##
	# @brief Process a pragma.
	#
	# Pragmas have no arguments, just their own intrinisic value.
	#
	# @param self
	# @param name The pragma name.
	def handlePragma(self, name): pass

	##
	# @brief Process a bit field assignment statement.
	#
	# @param self
	# @param identifier A string of the name of the bit field being assigned a value.
	# @param bitSlice Either None if the entire bit field is being assigned, or
	#		a bi-tuple containing the upper and lower bit numbers (inclusive) of
	#		the slice of bits to assign, in that order.
	# @param value The value being assigned to the bit field, or slice of the bit field.
	#		This will either be an integer value, or will be an instance of
	#		the class OTPFileConstantRef.
	def handleAssignment(self, identifier, bitSlice, value): pass

##
# @brief Represents the usage of a constant in the OTP bit settings file.
#
# A constant reference is passed for the @a value argument of
# OTPFileParserDelegate::handleAssignment() when the value of the bit field
# assignment is an identifier instead of an integer literal.
class OTPFileConstantRef:
	def __init__(self, name):
		self._name = name

	def getName(self):
		return self._name

	def __str__(self):
		return "<const: %s>" % self._name

##
# @brief Parser class for OTP bit settings files.
#
# The OTP bit settings file format's grammar is:
#
# @code
#		input			::=		statement*
#
#		statement		::=		pragma
#						|		assignment
#
#		pragma			::=		'*' IDENT '*'
#
#		assignment		::=		IDENT bit-slice? '=' value
#
#		bit-slice		::=		'[' INTEGER ( ':' INTEGER )? ']'
#
#		value			::=		INTEGER
#						|		IDENT
# @endcode
class OTPFileParser:
	def __init__(self, lexer, delegate):
		self._lexer = lexer
		self._delegate = delegate

	## @brief Parses the entire input.
	#
	# Grammar:
	# @code
	#		input			::=		statement*
	# @endcode
	def parseInput(self):
		while True:
			# Check if there are any more tokens.
			token = self._lexer.nextToken();
			if token is None:
				return
			else:
				self._parseStatement(token)

	## @brief Parse a statement non-terminal.
	#
	# Grammar:
	# @code
	#		statement		::=		pragma
	#						|		assignment
	# @endcode
	def _parseStatement(self, token):
		# Pragma always starts with an asterisk.
		if isinstance(token, StarToken):
			self._parsePragma()
		# Assignment always starts with the identifier.
		elif isinstance(token, IdentifierToken):
			self._parseAssignment(token)
		else:
			raise ParseException("Expected statement", self._lexer.getLineNumber())

	## @brief Parse a pragma statement.
	#
	# Grammar:
	# @code
	#		pragma			::=		'*' IDENT '*'
	# @endcode
	def _parsePragma(self):
		token = self._lexer.nextToken()
		if not isinstance(token, IdentifierToken):
			raise ParseException("Expected identifier", self._lexer.getLineNumber())

		# Handle pragma.
		if self._delegate:
			self._delegate.handlePragma(token.getValue())

		token = self._lexer.nextToken()
		if not isinstance(token, StarToken):
			raise ParseException("Expected '*'", self._lexer.getLineNumber())

	## @brief Parse an assignment statement.
	#
	# Grammar:
	# @code
	#		assignment		::=		IDENT bit-slice? '=' value
	# @endcode
	def _parseAssignment(self, identToken):
		bitSlice = None
		token = self._lexer.nextToken()

		# Check for a bit slice nonterminal.
		if isinstance(token, OpenBracketToken):
			bitSlice = self._parseBitSlice()

			# The next token read after the ']' should be the equals.
			token = self._lexer.nextToken()

		# Next is the equals sign.
		if not isinstance(token, EqualsToken):
			raise ParseException("Expected '='", self._lexer.getLineNumber())

		# Parse the value.
		value = self._parseValue()

		# And let the delegate handle this.
		if self._delegate:
			self._delegate.handleAssignment(identToken.getValue(), bitSlice, value)

	## @brief Parse a bit slice definition.
	#
	# Grammar:
	# @code
	#		bit-slice		::=		'[' INTEGER ( ':' INTEGER )? ']'
	# @endcode
	#
	# @return A bi-tuple is returned containing two elements, the upper and
	# lower bit numbers (or lexically right and left).
	def _parseBitSlice(self):
		fromToken = self._lexer.nextToken()
		if not isinstance(fromToken, IntegerLiteralToken):
			raise ParseException("Expected integer", self._lexer.getLineNumber())

		token = self._lexer.nextToken()
		if isinstance(token, CloseBracketToken):
			# Only one integer was within the brackets.
			return (fromToken.getValue(), fromToken.getValue())
		elif not isinstance(token, ColonToken):
			raise ParseException("Expected ':'", self._lexer.getLineNumber())

		toToken = self._lexer.nextToken()
		if not isinstance(toToken, IntegerLiteralToken):
			raise ParseException("Expected integer", self._lexer.getLineNumber())

		token = self._lexer.nextToken()
		if not isinstance(token, CloseBracketToken):
			raise ParseException("Expected ']'", self._lexer.getLineNumber())

		return (fromToken.getValue(), toToken.getValue())

	## @brief Parse a value.
	#
	# Grammar:
	# @code
	#		value			::=		INTEGER
	#						|		IDENT
	# @endcode
	def _parseValue(self):
		token = self._lexer.nextToken()
		if isinstance(token, IntegerLiteralToken):
			return token.getValue()
		elif isinstance(token, IdentifierToken):
			identName = token.getValue()
			return OTPFileConstantRef(identName)

		# Raise exception for unexpected token class
		raise ParseException("Expected value", self._lexer.getLineNumber())

##
# @brief Test function for the OTPFileParser.
def testParser():
	print 'testing parser...'

	class ParsePrinter(OTPFileParserDelegate):
		def handlePragma(self, identifier):
			print 'handlePragma: ' + str(identifier)

		def handleAssignment(self, identifier, bitSlice, value):
			print 'assignment: ' + str(identifier) + ', ' + str(bitSlice) + ', ' + str(value)

	path = 'example.otp'
	stream = bufferedstream.BufferedCharacterStream(open(path))
	lex = OTPFileLexer(stream)
	parse = OTPFileParser(lex, ParsePrinter())
	parse.parseInput()

##
# @brief Representation of an OTP register.
#
# An OTP register is a 32-bit value that belongs to a bank of registers,
# forming a 256-bit array of OTP bits.
class OTPRegister:
	##
	# @brief Initialiser.
	#
	# The register's value is initialised to 0.
	#
	# @param self
	# @param bank Index of the OTP bank to which this register belongs.
	# @param word Word index within the bank for this register.
	def __init__(self, bank, word):
		self._bank = bank
		self._word = word
		self._value = 0

	##
	# @brief Simply returns the current 32-bit register value.
	#
	# @return The current integer value of the register.
	def getValue(self):
		return self._value

	##
	# @brief Sets the value of a given range of bits in the OTP register.
	#
	# Use this method to set the value of one or more bits in a range within
	# the 32-bit OTP register. The value passed into the @a bits argument is
	# the non-shifted value of the bit range. For instance, if the caller wants
	# to set bits 13 through 12, a two-bit range, to the value 0b11, then the
	# @a bits argument should be set to 0x3 and not 0x3000.
	#
	# @param self
	# @param bits The value of the bits. This value should not be shifted up to the
	#		@a lower bit number.
	# @param upper Number of the upper bit in the range being set. This value must
	#		range from 0 through 31, and must be higher than @a lower.
	# @param lower Specifies the lower bounds of the range of bits being modified.
	#		This value must be at least 0, and cannot be greater than @a upper.
	def setBits(self, bits, upper, lower):
		assert(upper >= lower)
		assert(upper >= 0 and upper <= 31)
		assert(lower >= 0 and lower <= 31)

		bitcount = upper - lower + 1

		# Build up a mask.
		mask = 0
		for i in range(lower, upper + 1):
			mask |= 1 << i

		maskedValue = self._value & ~mask
		self._value = maskedValue | ((bits << lower) & mask)

	def __repr__(self):
		return "<B%dW%d=0x%08x>" % (self._bank, self._word, self._value)

##
# @brief Representation of the OTP bit array.
#
# The MX28 has 5 banks while other chips have 4 banks, so we want to start off
# with enough registers to cover all chips. To change the number of register banks
# after the bit array object is created, call the setBankCount() method.
class OTPBitArray:
	## Total number of OTP banks.
	kOTPBankCount = 5

	## Number of 32-bit registers per OTP bank.
	kOTPRegisterCount = 8

	## @brief Initialiser.
	def __init__(self):
		## Values for each of the 32 OTP registers, for 1kbit in total.
		self._otp = []

		## An alternate way to access register objects, through their bank and word index.
		self._otpBanks = []

		## Number of banks.
		self._bankCount = OTPBitArray.kOTPBankCount

		# Create the register objects.
		for bank in range(OTPBitArray.kOTPBankCount):
			bankRegs = []

			for word in range(OTPBitArray.kOTPRegisterCount):
				reg = OTPRegister(bank, word)
				bankRegs.append(reg)
				self._otp.append(reg)

			self._otpBanks.append(bankRegs)

	## @brief Set the actual number of OTP register banks.
	#
	# The number of banks set with this method overrides the default number of banks set
	# when the bit array object is created.
	def setBankCount(self, newBankCount):
		self._bankCount = newBankCount

	## @brief Returns the actual number of register banks.
	def getBankCount(self):
		return self._bankCount

	## @brief Access a single register by register index.
	def getRegister(self, registerIndex):
		return self._otp[registerIndex]

	## @brief Generate the buffer containing the binary values of all OTP registers.
	def buildBinaryImage(self):
		result = ''
		for i in range(self._bankCount):
			result += self.buildBinaryBank(i)

		return result

	## @brief Generate the binary representation of a single OTP bank.
	#
	# Each OTP bank, of which there are four, consists of eight thirty-two bit registers
	# stacked in order from 0 to 7. Each register is in little endian byte order.
	def buildBinaryBank(self, bankIndex):
		assert(bankIndex >= 0 and bankIndex < OTPBitArray.kOTPBankCount)
		bank = self._otpBanks[bankIndex]
		bankValue = ''

		for i in range(len(bank)):
			bankValue += struct.pack('<L',	bank[i].getValue())

		return bankValue

	## @brief Set the crypto key register values.
	#
	# If the bytes in the AES-128 key are in this order:
	#
	#	- 00010203040506070809101112131415
	#
	# Then the bytes in the crypto key registers are in the following order:
	#
	#	- HW_OCOTP_CRYPTO0 = 0x03020100
	#	- HW_OCOTP_CRYPTO1 = 0x07060504
	#	- HW_OCOTP_CRYPTO2 = 0x11100908
	#	- HW_OCOTP_CRYPTO3 = 0x15141312
	#
	# To burn the key correctly as four words, the byte order needs to be reversed
	# in each word. We read the bytes as little endian from the word string. Since
	# the word string bytes are effectively in big endian order, this reverses
	# the bytes as necessary.
	#
	# @param self
	# @param cryptoKey The AES-128 key as a string of 16 bytes.
	def fillInCryptoKey(self, cryptoKey):
		assert(len(cryptoKey) == 16)

		for i in range(4):
			# Crypto key is bank 0, word 4-7.
			reg = self._otp[4 + i]

			# Read the key word from the full key string. The bytes in the key array
			# are in byte to byte order, and so are effectively big endian.
			keyWordBytes = cryptoKey[i * 4:(i + 1) * 4]

			# Convert to a long integer and reverse byte order a the same time.
			keyWordValue, = struct.unpack('<L', keyWordBytes)

			# Set the register's value.
			reg.setBits(keyWordValue, 31, 0)

		# The user has provided a crypto key, so we also need to make sure that
		# the crypto key lock bit and it's alternate bit are set.
		#
		# The lock register is bank 2 word 0. cryptokey is bit 4, cryptokey_alt
		# is bit 21.
# MICHAL: disabled for testing and for production (set those bits explicitly in bit_settings.txt file)
#		lockReg = self._otp[16]
#		lockReg.setBits(1, 4, 4)
#		lockReg.setBits(1, 21, 21)

	## @brief Sets the SRK registers from the contents of a binary file.
	#
	# @param self
	# @param srkPath Path to a binary file holding the super root key hash.
	#
	# @todo Figure out the order that the SRK bytes and words need to be in.
	def fillInSuperRootKey(self, srkPath):
		try:
			# Open the file in binary mode.
			srkFile = open(srkPath, 'rb')

			try:
				# Read the 256-bit value from the file.
				srk = srkFile.read(32)

				if len(srk) < 32:
					print "Warning: SRK file '%s' is too small" % srkPath
					return

				for i in range(8):
					# SRK registers are in bank 4.
					reg = self._otpBanks[4][i]

					# Extract one word from the entire SRK and convert it to a long integer.
					srkWordBytes = srk[i * 4:(i + 1) * 4]
					srkWordValue, = struct.unpack('L', srkWordBytes)

					# Set the SRK register's value.
					reg.setBits(srkWordValue, 31, 0)
			finally:
				srkFile.close()

		except IOError, e:
			print "Error reading SRK file:", e
			#print "Error: error reading SRK file '%s': %s [%d]" % (srkPath, os.strerror(e.errno), e.errno)

##
# @brief Representation of a bit field.
class OTPBitField:
	def __init__(self, name, register, bank, word, upper, lower):
		assert(register)
		self._name = name
		self._register = register
		self._bank = bank
		self._word = word
		self._upper = upper
		self._lower = lower

	def setBits(self, bits, upper=None, lower=None):
		assert(self._register)

		# If upper and lower are defaults then we're setting the entire field.
		if upper is None and lower is None:
			print "setting %d:%d of %s to 0x%08x" % (self._upper, self._lower, self._name, bits)
			self._register.setBits(bits, self._upper, self._lower)
		else:
			# Range check the slice.
			if (upper - lower + 1) > (self._upper - self._lower + 1):
				raise Exception("Slice too wide")
			elif (upper > (self._upper - self._lower)) or (lower < 0):
				raise Exception("Slice out of range")

			# Convert bit numbers to relative to register.
			actualLower = self._lower + lower
			actualUpper = actualLower + (upper - lower)
			print "setting %d:%d of %s to 0x%08x" % (actualUpper, actualLower, self._name, bits)
			self._register.setBits(bits, actualUpper, actualLower)

	def getUpperBit(self):
		return self._upper

	def getLowerBit(self):
		return self._lower

	def getRegister(self):
		return self._register

	def __repr__(self):
		return "<%s:%d:%d>" % (self._name, self._upper, self._lower)

##
# @brief Reads an OTP bit settings file to build an OTP value table.
#
# Takes a file object provided by the caller and reads it to generate a
# binary image of the OTP register values.
class OTPFileReader(OTPFileParserDelegate):
	##
	# @brief Initialiser.
	#
	# @param self
	# @param theFile A file object for the input bit settings file.
	# @param bits An instance of OTPBitArray. Parsing the input file causes the
	#	registers in this object to be assigned values.
	def __init__(self, theFile, bitArray):
		## The file object for the input file.
		self._file = theFile

		## The object representation of the OTP bits.
		self._bits = bitArray

		## Chip family.
		self._family = None

		# Now set up the bit field and constant dicts for the default chip family (3700).
		self._resetDicts()
		self._addBitFields(kOTPBitFieldsCommon)
		self._addBitFields(kOTPBitFields3700)
		self._addConstants(kOTPConstantsCommon)
		self._addConstants(kOTPConstants37xx)

	##
	# @brief Accessor for the OTP bit array.
	def getBitArray(self):
		return self._bits

	##
	# @brief Accessor for the chip family.
	#
	# The chip family is specified in the bit settings file through the use of
	# a pragma statement.
	def getChipFamily(self):
		return self._family

	##
	# @brief Clears the bitfield and constants dictionaries.
	def _resetDicts(self):
		## Dictionary of OTP bit fields.
		self._bitfields = {}

		## Constants table to use.
		self._constants = {}

	##
	# @brief Adds in a dictionary of constants.
	def _addConstants(self, constantsDict):
		self._constants.update(constantsDict)

	##
	# @brief Builds a dictionary of bit field objects.
	#
	# The @a bitfieldDefinitions list of bit field definitions is processed into a
	# dictionary that maps each bit field's name to an instance of OTPBitField.
	# The OTPBitField objects have a reference to the OTP register that they
	# belong to.
	def _addBitFields(self, bitfieldDefinitions):
		for field in bitfieldDefinitions:
			regIndex = field[1] * 8 + field[2]
			reg = self._bits.getRegister(regIndex)
			self._bitfields[field[0]] = OTPBitField(field[0], reg, *field[1:])

	##
	# @brief Process a pragma.
	#
	# Pragmas have no arguments, just their own intrinisic value. The currently supported
	# pragmas are:
	#	- chip-family-3700
	#	- chip-family-3770
	#	- chip-family-3780
	#	- chip-family-mx23
	#	- chip-family-mx28
	#
	# @param self
	# @param name The pragma name.
	def handlePragma(self, name):
		lowerName = name.lower()

		# Match the family pragma and set up the bit fields dictionary and constants
		# table to match.
		if lowerName == 'chip-family-3700':
			self._family = k3700Family
			self._resetDicts()
			self._addBitFields(kOTPBitFieldsCommon)
			self._addBitFields(kOTPBitFieldsCommon37xx)
			self._addBitFields(kOTPBitFields3700)
			self._addConstants(kOTPConstantsCommon)
			self._addConstants(kOTPConstants37xx)
			self._bits.setBankCount(4)
		elif lowerName == 'chip-family-3770':
			self._family = k3770Family
			self._resetDicts()
			self._addBitFields(kOTPBitFieldsCommon)
			self._addBitFields(kOTPBitFieldsCommon37xx)
			self._addBitFields(kOTPBitFields3770)
			self._addConstants(kOTPConstantsCommon)
			self._addConstants(kOTPConstants37xx)
			self._bits.setBankCount(4)
		elif lowerName == 'chip-family-3780' or lowerName == 'chip-family-mx23':
			self._family = k3780Family
			self._resetDicts()
			self._addBitFields(kOTPBitFieldsCommon)
			self._addBitFields(kOTPBitFieldsCommon37xx)
			self._addBitFields(kOTPBitFields3780)
			self._addConstants(kOTPConstantsCommon)
			self._addConstants(kOTPConstants3780)
			self._bits.setBankCount(4)
		elif lowerName == 'chip-family-mx28':
			self._family = kMX28Family
			self._resetDicts()
			self._addBitFields(kOTPBitFieldsCommon)
			self._addBitFields(kOTPBitFieldsMX28)
			self._addConstants(kOTPConstantsCommon)
			self._addConstants(kOTPConstantsMX28)
			self._bits.setBankCount(5)
		else:
			raise ParseException("Unknown pragma name '%s'" % name, self._lex.getLineNumber())

	##
	# @brief Process a bit field assignment statement.
	#
	# @param self
	# @param identifier A string of the name of the bit field being assigned a value.
	# @param bitSlice Either None if the entire bit field is being assigned, or
	#		a bi-tuple containing the upper and lower bit numbers (inclusive) of
	#		the slice of bits to assign.
	# @param value The value being assigned to the bit field, or slice of the bit field.
	#		This will either be an integer value, or will be an instance of
	#		the class OTPFileConstantRef.
	def handleAssignment(self, identifier, bitSlice, value):
		# Convert constants to their value.
		if isinstance(value, OTPFileConstantRef):
			try:
				lowerName = value.getName().lower()
				value = self._constants[lowerName]
			except KeyError:
				raise ParseException("Unknown constant '%s'" % value.getName(), self._lex.getLineNumber())

		# Lookup bit field.
		try:
			lowerIdentifier = identifier.lower()
			bitfield = self._bitfields[lowerIdentifier]
		except KeyError:
			raise ParseException("Unknown bit field '%s'" % identifier, self._lex.getLineNumber())

		# If no bit slice was used, we're setting all bits.
		if bitSlice is None:
			bitfield.setBits(value)
		else:
			bitfield.setBits(value, *bitSlice)

	## @brief Initiate processing of the input bit settings file.
	#
	# @exception ParseException Any error encountered while reading the input file will result
	#	in an instance of ParseException being raised. The exception object will have an
	#	appropriate error message and the line number of the input file that caused the error.
	#	However, the line number is not guaranteed to be entirely accurate.
	def parse(self):
		stream = bufferedstream.BufferedCharacterStream(self._file)

		## The lexer instance, used to get line numbers for error messages.
		self._lex = OTPFileLexer(stream)

		parser = OTPFileParser(self._lex, self)
		parser.parseInput()

	## @brief Access a bit field by name.
	#
	# @param self
	# @param name Name of the bit field to return. Case is ignored.
	def getBitField(self, name):
		return self._bitfields[name.lower()]

## @brief Simple test function for the OTPFileReader class.
def testReader():
	print "testing reader..."
	path = 'example.otp'
	bits = OTPBitArray()
	reader = OTPFileReader(open(path), bits)
	reader.parse()
	print bits._otp
	print repr(bits.buildBinaryOTPBuffer())

##
# @brief Reader for key files used by elftosb.
#
# Key files contain one AES128 key per line, each key encoded in ASCII hex. Since each
# key is 128 bits, or 16 bytes, each line has 32 hexadecimal characters on it.
class AES128KeyFile(file):
	## @brief Count the number of keys in the file.
	#
	# The number of keys from the current position in the file until the EOF are
	# counted. Afterwards the current position is reset to the beginning of the file.
	def countKeys(self):
		count = 0
		k = self.readNextKey()
		while k is not None:
			count += 1
			k = self.readNextKey()
		self.seek(0)
		return count

	## @brief Returns the next key in the file.
	#
	# @return Either the next key or None is returned. Keys are returned as strings containing
	#	the 16-byte binary data of the key. As the key data is packed byte by byte, it is
	#	effectively in big endian byte order if you choose to view the key as four 32-bit words.
	#
	# @post The file's current position is advanced by one key, or effectively one line,
	#	every call to this method.
	def readNextKey(self):
		k = self.readline().strip()
		if len(k) != 32:
			return None

		result = ''
		while len(k):
			byte = string.atoi(k[:2], 16)
			result += struct.pack('B', byte)
			k = k[2:]

		return result

##
# @brief Class to help printing columns of text.
#
# Create an instance of this class and pass the width of each column to the init
# method. Then for each row, generate the string for that row of each column. Pass
# those strings to printRow() as separate arguments. Each row string will be printed
# with enough trailing whitespace (except for the last row) to line the columns
# up nicely.
class ColumnPrinter:
	##
	# @brief Init method.
	# @param self
	# @param columnWidth Width of the columns in characters.
	def __init__(self, columnWidth):
		self._width = columnWidth

	##
	# @brief Prints one row of a series of columns.
	# @param self
	# @param cols All arguments to this function wind up in this argument as an array.
	def printRow(self, *cols):
		output = ""
		colCount = len(cols)
		n = 0
		for thisCol in cols:
			whitespaceCount = self._width - len(thisCol)
			output += thisCol
			# Don't add trailing whitespace to last column.
			if n + 1 != colCount:
				output += " " * whitespaceCount
			n += 1
		print output

## @brief Command line interface for OTP processor.
#
# Create an instance of this class and invoke the run() method.
class OTPGeneratorTool:
	## @brief Prints OTP register values nicely formatted.
	#
	# @param self
	# @param otp A string containing all OTP registers.
	# @param bankCount Number of 8-register OTP banks represented by the @a otp parameter.
	def _printOTP(self, otp, bankCount):
		cols = ColumnPrinter(16)
		rowTitles = ["Bank 0", "Bank 1", "Bank 2", "Bank 3"]
		if bankCount > 4:
			rowTitles.append("Bank 4")
		cols.printRow(*rowTitles)

		for word in range(8):
			words = []
			for bank in range(bankCount):
				offset = (bank * 8 * 4) + (word * 4)
				registerSlice = otp[offset:offset + 4]
				registerValue, = struct.unpack("<L", registerSlice)
				words.append("%d: 0x%08x" % (word, registerValue))
			cols.printRow(*words)

	## @brief Read the chosen crypto key.
	#
	# The @a keyFilePath and @a keyIndex attributes of the @a options argument
	# are used to find the crypto key the user wants. The key is read into a
	# string and returned. If no key was specified, None is returned instead.
	#
	# @param self
	# @param options Command line options dictionary.
	# @return Either None or a string containing the AES-128 key bytes.
	def _readCryptoKey(self, options):
		cryptoKey = None

		if options.keyFilePath:
			keys = AES128KeyFile(options.keyFilePath)
			count = keys.countKeys()
			print "%d keys in %s, using key %d" % (count, options.keyFilePath, options.keyIndex)
			remaining = options.keyIndex + 1
			while remaining > 0:
				cryptoKey = keys.readNextKey()
				remaining -= 1

		return cryptoKey

	## @brief Returns the path to the PITC to use for the given chip family.
	def _getPitcForChipFamily(self, family):
		if family in [k3700Family, k3770Family, k3780Family]:
			return "pitc_otp_mfg"
		elif family in [kMX28Family]:
			return "pitc_otp_mfg_mx28"
		else:
		    raise Exception("Unknown chip family " + str(family))

	## @brief Converts our chip family constant to the elftosb chip family name.
	def _getElftosbChipFamily(self, family):
		if family in [k3700Family, k3770Family, k3780Family]:
			return "37xx"
		elif family in [kMX28Family]:
			return "mx28"
		else:
		    raise Exception("Unknown chip family " + str(family))

	def _printVersion(self):
	    print os.path.basename(sys.argv[0]), kToolVersion
	    print kToolCopyright

	## @brief Read the command line and generate the output file.
	#
	# This method is the meat of the class; all other methods are here to support this one.
	#
	def run(self):
		# Process command line options.
		options, args = self._readOptions()

		# Check for version option.
		if options.showVersion:
		    self._printVersion()
		    return

		# Verify that there is input.
		if options.inputFilePath is None:
			print "Error: no input OTP settings file was provided"
			return

		# Pick out the crypto key.
		cryptoKey = self._readCryptoKey(options)

		try:
			# Create the bit array.
			bits = OTPBitArray()

			# Read the input file.
			reader = OTPFileReader(open(options.inputFilePath), bits)
			reader.parse()

			# Fill in the crypto key registers, by breaking the key into
			# one word at a time.
			if cryptoKey is not None:
				bits.fillInCryptoKey(cryptoKey)

			# Fill in the super root key if provided.
			if reader.getChipFamily() is kMX28Family and options.superRootKeyPath is not None:
				bits.fillInSuperRootKey(options.superRootKeyPath)

			# Get the output.
			buffer = bits.buildBinaryImage()
			if options.printResult:
				self._printOTP(buffer, bits.getBankCount())

			# The intermediate file contains the binary OTP data. It's name is
			# just the input file with a .dat extension. However, if we're not calling
			# elftosb and the user has provided an explicit output file name, then
			# use that instead.
			if not options.runElftosb and options.outputFilePath:
				intermediateFilePath = options.outputFilePath
			else:
				intermediateFilePath = os.path.splitext(options.inputFilePath)[0] + '.dat'

			# Write the data.
			outputFile = open(intermediateFilePath, 'w+b')
			outputFile.write(buffer)
			outputFile.close()

			# Flush stdout before calling elftosb, so that log output shows up in the
			# expected order. If we don't do this, elftosb's output shows up before our own!
			sys.stdout.flush()

			if options.runElftosb:
				# Look up the PITC to use.
				pitcElfFilePath = self._getPitcForChipFamily(reader.getChipFamily())

				# Generate output filename if none was provided.
				if not options.outputFilePath:
					options.outputFilePath = os.path.join(os.path.dirname(options.inputFilePath), 'OtpInit.sb')

				# Select the boot descriptor file. The MX28 uses a different .bd if outputting
				# a HAB enabled boot image.
				if reader.getChipFamily() is kMX28Family and options.useHab:
				    bdFile = 'otp_burner_hab.bd'
				else:
				    bdFile = 'otp_burner.bd'

				# Call elftosb and copy its output to stdout.
				if cryptoKey is not None:
					# Add the key file to elftosb args so the burner .sb can be executed again
					# if needed, after burning the crypto key.
					elftosbArgs = ['-k', options.keyFilePath]
				else:
					elftosbArgs = []

				# Set the chip family in elftosb.
				elftosbArgs += ['-f', self._getElftosbChipFamily(reader.getChipFamily())]

				elftosbArgs += ['-Vz', '-c', bdFile, '-o', options.outputFilePath, pitcElfFilePath, intermediateFilePath]
				elftosb = build.tool('elftosb', elftosbArgs)
				elftosb.copyOutputToLog(elftosb.executeCommand())
		except ParseException, e:
			print "Error: " + str(e)

	## @brief Parse command line options into an options dictionary.
	#
	# The options dictionary that is the first element in the tuple returned from this
	# method has the following attributes:
	#	- inputFilePath: Path to the input OTP bit settings file.
	#	- outputFilePath: Path to the output file, which will either be the .dat file
	#		containing the binary OTP bit data or the .sb file, depending on whether
	#		the user wants to run elftosb or not. This attribute may be None, in
	#		which case the user expects output file names to be based on the input
	#		file name.
	#	- keyFilePath: Optional path to the elftosb-style crypto key file.
	#	- keyIndex: Index of the key to use within the crypto key file specified by keyFilePath.
	#	- superRootKeyPath: Optional path to the super root key binary file.
	#	- printResult: A boolean indicating whether to print OTP values before exiting.
	#	- runElftosb: Boolean indicating if elftosb should be executed. This attribute also
	#		modifies the meaning of outputFilePath. If false, the output file is the binary
	#		.dat file that contains raw OTP bit values. Otherwise the output file is the .sb
	#		file generated by elftosb.
	#   - showVersion: Boolean for whether to show the tool version and exit.
	#   - useHab: whether to enable HAB support on MX28
	#
	# @return A bi-tuple is returned that contains the command line options value dictionary
	#	and any leftover command line positional arguments.
	def _readOptions(self):
		usage = "%prog [options] [-k FILE] [-r FILE] -i FILE [-o FILE]"

		# Build option parser.
		parser = optparse.OptionParser(usage=usage, formatter=optparse.TitledHelpFormatter())

		parser.add_option("-V", "--version", action="store_true", dest="showVersion", default=False, help="Show version information.")

		parser.add_option("-i", "--input", action="store", type="string", dest="inputFilePath", default=None, metavar="PATH", help="Specify the input OTP bit settings file.")

		parser.add_option("-o", "--output", action="store", type="string", dest="outputFilePath", default=None, metavar="PATH", help="Write output to this file. Optional; if not provided then the output file name is generated from the input file name.")

		parser.add_option("-k", "--key", action="store", type="string", dest="keyFilePath", default=None, metavar="PATH", help="Specify the input crypto key file.")

		parser.add_option("-n", "--key-number", action="store", type="int", dest="keyIndex", default=0, metavar="NUM", help="Use key number N from the crypto key file (default 0).")

		parser.add_option("-r", "--srk", action="store", type="string", dest="superRootKeyPath", default=None, metavar="PATH", help="Specify the binary super root key hash file.")

		parser.add_option("-p", "--print-otp", action="store_true", dest="printResult", default=False, help="Print the resulting OTP registers.")

		parser.add_option("-e", "--elftosb", action="store_true", dest="runElftosb", default=True, help="Run elftosb to generate the .sb file (the default).")

		parser.add_option("-E", "--no-elftosb", action="store_false", dest="runElftosb", default=True, help="Do not run elftosb.")

		parser.add_option("-a", "--hab", action="store_true", dest="useHab", default=True, help="generate a HAB compatible .sb (MX28 only; default)")

		parser.add_option("-A", "--no-hab", action="store_false", dest="useHab", default=True, help="generate a non-HAB .sb (MX28 only)")

		#Retrieve agruments(list) and options(dictionary)
		return parser.parse_args()


# Are we being executed directly?
if __name__ == '__main__':
	#testParser()
	#testReader()

	OTPGeneratorTool().run()
