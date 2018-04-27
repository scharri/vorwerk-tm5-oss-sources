//////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//
//! \file   return_codes.h
//! \brief  Error and Status return values
//!
//! Errors (only) have bit 31 set.
//! Zero is a "generic" success return value.
//! 0xFFFFFFFF is a "generic" failure return value.
//! All positive values (bit 31 not set) are status codes.
//!
//! 2048 Major groups:  1024 for Sigmatel, 1024 Major groups for customers
//!   (Sigmatel uses low-order 11 bits)
//!  256 Minor groups per Major group
//! 4096 errors per Minor group
//!
//! Bit            3322 2222 2222 1111 1111 11
//!                1098 7654 3210 9876 5432 1098 7654 3210
//!                ---------------------------------------
//! Major Groups:  EMMM MMMM MMMM ---- ---- ---- ---- ----
//! Minor Groups:  ---- ---- ---- mmmm mmmm ---- ---- ----
//! Return Code:   ---- ---- ---- ---- ---- eeee eeee eeee
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
#ifndef _RETURN_CODES_H
#define _RETURN_CODES_H

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef int RtStatus_t;
#endif

//! Generic return codes
#define SUCCESS                 0
#define ERROR_ROM               0xFFFFFFFF

//! Plugin return codes
#define ROM_BOOT_SECTION_ID     1
#define ROM_BOOT_IMAGE_ID       2


////////////////////////////////////////////////////////////////////////////////
//                            ROM GROUP
////////////////////////////////////////////////////////////////////////////////

//! The DDI major group is 2
#define ERROR_DDI_GROUP         0x80200000

//! The ROM Major group is 5
#define ROM_GROUP               0x00500000
//! The ROM Major group with error bit set
#define ERROR_ROM_GROUP         0x80500000


//  ***************************
//  **** ROM STARTUP GROUP ****
//  ***************************

//! The ROM Startup group is 0x00004000
#define ERROR_ROM_STARTUP_GROUP 0x00004000

//! 0x80504001
//! An Invalid boot mode was decoded
#define ERROR_ROM_STARTUP_UNKNOWN_BOOT_MODE                    (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x1)

//! 0x80504002
//! Decoded boot mode is known, but not supported in this ROM
#define ERROR_ROM_STARTUP_UNSUPPORTED_BOOT_MODE                (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x2)

//! 0x80504003
//! Initial 'C' function in loader returned unexpectedly to startup
#define ERROR_ROM_STARTUP_UNEXPECTED_RETURN_FROM_LOADER        (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x3)

//! 0x80504004
//! Unexpected return to ROM startup from mfg tester/loader
#define ERROR_ROM_STARTUP_UNEXPECTED_RETURN_FROM_TESTER_LOADER (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x4)

//! 0x80504005
//! Unexpected return to ROM startup from mfg burnin code
#define ERROR_ROM_STARTUP_UNEXPECTED_RETURN_FROM_BURN_IN       (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x5)

//! 0x80504006
//! Unexpected return to ROM startup from mfg CRC test
#define ERROR_ROM_STARTUP_UNEXPECTED_RETURN_FROM_ROM_CRC       (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x6)

//! 0x80504007
//! Unexpected return to ROM startup from mfg RAM test
#define ERROR_ROM_STARTUP_UNEXPECTED_RETURN_FROM_RAM_TEST      (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x7)

//! 0x80504008
//! Unexpected return to ROM from mfg bist test
#define ERROR_ROM_STARTUP_UNEXPECTED_RETURN_FROM_BIST          (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x8)

//! 0x80504009
//! Timed out trying to read persistent bits
#define ERROR_ROM_STARTUP_PERSISTENT_BIT_READY_TIMEOUT         (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x9)

//! 0x80504010
//! Timed out waiting for eFuse data to load
#define ERROR_ROM_STARTUP_EFUSE_READY_TIMEOUT                  (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x10)

//! 0x80504011
//! Initial 'C' function returned unexpectedly to early reset function 
#define ERROR_ROM_STARTUP_UNEXPECTED_RETURN_FROM_START         (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x11)

//! 0x80504012
//! A call to mfg mode ram test detected in startup.c
#define ERROR_ROM_STARTUP_UNEXPECTED_CALL_TO_RAM_TEST          (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x12)

//! 0x80504013
//! JTAG has been permanently disabled yet bootmode was JTAG
#define ERROR_ROM_STARTUP_JTAG_DISABLED                        (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x13)

//! 0x80504014
//! A needed GPIO bank has been disabled and ROM cannot proceed.
#define ERROR_ROM_STARTUP_GPIO_BANK_DISABLED                   (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x14)

//! 0x80504015
//! Error loading the Persistent Words.
#define ERROR_ROM_STARTUP_UNABLE_TO_LOAD_PERSISTENT_WORD       (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x15)

//! 0x80504016
//! Error setting the Persistent Words.
#define ERROR_ROM_STARTUP_UNABLE_TO_SET_PERSISTENT_WORD        (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x16)

//! 0x80504017
//! Jump to NOR returned to startup
#define ERROR_ROM_STARTUP_UNEXPECTED_RETURN_FROM_NOR  (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x17) 

//! 0x80504018
//! Mfg test modes disabled
#define ERROR_ROM_STARTUP_TEST_MODES_DISABLED  (ERROR_ROM_GROUP | ERROR_ROM_STARTUP_GROUP | 0x18) 

//  *****************************
//  ******** LOADER GROUP *******
//  *****************************

#define ERROR_ROM_LOADER_GROUP          0x00001000

//! 0x80501001
//! DCP reported a status error.
#define ERROR_ROM_LDR_DCP_STATUS                (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x1)

//! 0x80501002
//! DCP transaction timed out.
#define ERROR_ROM_LDR_DCP_TIMEOUT               (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x2)

//! 0x80501003
//! The file signature or file version is incorrect.
#define ERROR_ROM_LDR_SIGNATURE                 (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x3)

//! 0x80501004
//! A section length is out of range.
#define ERROR_ROM_LDR_SECTION_LENGTH            (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x4)

//! 0x80501005
//! Cannot load unencrypted sb image file.
#define ERROR_ROM_LDR_ENCRYPTED_ONLY            (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x5)

//! 0x80501006
//! Key dictionary lookup failed.
#define ERROR_ROM_LDR_KEY_NOT_FOUND             (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x6)

//! 0x80501007
//! Boot command checksum failed.
#define ERROR_ROM_LDR_CHECKSUM                  (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x7)

//! 0x80501008
//! Unknown boot command.
#define ERROR_ROM_LDR_UNKNOWN_COMMAND           (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x8)

//! 0x80501009
//! Reached the end of the sb image file.
#define ERROR_ROM_LDR_EOF_REACHED               (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x9)

//! 0x8050100A
//! Did not find requested section ID.
#define ERROR_ROM_LDR_ID_NOT_FOUND              (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0xA)

//! 0x8050100B
//! Load command payload CRC failed.
#define ERROR_ROM_LDR_PAYLOAD_CRC               (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0xB)

//! 0x8050100C
//! Plugin returned from a jump command.
#define ERROR_ROM_LDR_JUMP_RETURNED             (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0xC)

//! 0x8050100D
//! Requested data beyond the end of a section.
#define ERROR_ROM_LDR_SECTION_OVERRUN           (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0xD)

//! 0x8050100E
//! Requested data beyond the end of a section.
#define ERROR_ROM_LDR_DCD_FAILED                (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0xE)

//! 0x8050100F
//! Requested data beyond the end of a section.
#define ERROR_ROM_LDR_CALL_FAILED               (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0xF)

//! 0x80501010
//! Requested data beyond the end of a section.
#define ERROR_ROM_LDR_CHECK_TARGET_FAILED       (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x10)

//! 0x80501011
//! OTP reload timed out.
#define ERROR_ROM_LDR_OTP_TIMEOUT               (ERROR_ROM_GROUP | ERROR_ROM_LOADER_GROUP | 0x11)

//  ***************************
//  ******** USB GROUP ********
//  ***************************

#define ERROR_ROM_USB_DRIVER_GROUP      0x00002000

//! 0x80502001 
//! No registered service found 
#define ERROR_ROM_USB_NO_SERVICE                (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x1)

//! 0x80502002
//! Number of bytes to transfer is too large
#define ERROR_ROM_USB_SIZE_TOO_LARGE            (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x2)

//! 0x80502003
//! Endpoint init fail (end point already in use)
#define ERROR_ROM_USB_EP_INIT_FAILED            (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x3)

//! 0x80502004
//! Error reported by the send/recv if the device is not yet configured
#define ERROR_ROM_USB_DEVICE_NOT_CONFIGURED     (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x4)

//! 0x80502005
//! Fail to initialize the USB API
#define ERROR_ROM_USB_INIT_FAILED               (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x5)

//! 0x80502006
//! Ran out of transfer structures
#define ERROR_ROM_USB_NO_TRANSFER_STRUCTURES    (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x6)

//! 0x80502007
#define ERROR_ROM_USB_PLL_LOCK_TIMEOUT        (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x7)

//! 0x80502008
#define ERROR_ROM_USB_CONNECT_TIMEOUT        (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x8)

//! 0x80502009
#define ERROR_ROM_USB_DISCONNECTED        (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0x9)

//! 0x8050200A
#define ERROR_USB_ARC_RESET_TIMEOUT        (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0xA)

//! 0x8050200B
#define ERROR_USB_HARDWARE_NOT_PRESENT     (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0xB)

//! 0x8050200C
#define ERROR_USB_SUSPEND_WAIT_FOR_LINESTATE_TIMEOUT     (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0xC)

//! 0x8050200D
#define ERROR_USB_RECOVERY_DISABLED     (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0xD)

//! 0x8050200E
#define ERROR_USB_INSUFFICIENT_MEM_POOL     (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0xE)

//! 0x8050200F
#define ERROR_USB_WAIT_FOR_LINESTATE_TIMEOUT     (ERROR_ROM_GROUP | ERROR_ROM_USB_DRIVER_GROUP | 0xF)

//  ***************************
//  ******** NAND GROUP *******
//  ***************************

#define ERROR_ROM_NAND_DRIVER_GROUP     0x00008000

//! 0x80508001
//! Unable to find one of the Boot Control Blocks (FCB or DBBT)
#define ERROR_ROM_NAND_DRIVER_NO_BCB                            (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x1)

//! 0x80508002
//! Unable to find FCB
#define ERROR_ROM_NAND_DRIVER_NO_FCB                            (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x2)

//! 0x80508003 
//! Reserved, earlier it used to be ERROR_ROM_NAND_DRIVER_NO_LDLB                           

//! 0x80508004
//! Unable to find Discovered Bad Block Table.
#define ERROR_ROM_NAND_DRIVER_NO_DBBT                           (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x4)

//! 0x80508005
//! Unexpected fatal error in NAND.
#define ERROR_ROM_NAND_DRIVER_FATAL_ERROR                       (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x5)

//! 0x80508006
//! Unable to complete the NAND Initialization.
#define ERROR_ROM_NAND_DRIVER_NAND_INIT_FAILED                  (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x6)

//! 0x80508007
//! Search for BCB failed.
#define ERROR_ROM_NAND_DRIVER_SEARCH_FAILED                     (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x7)

//! 0x80508008
//! NAND DMA timed out.
#define ERROR_ROM_NAND_DRIVER_DMA_TIMEOUT                       (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x8)

//! 0x80508009
//! NAND Reset command failed.
#define ERROR_ROM_NAND_DRIVER_RESET_FAILED                      (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x09)

//! 0x8050800A
//! Tried to check DMA status but no Read is in progress.
#define ERROR_ROM_NAND_DRIVER_NO_READ_IN_PROGRESS               (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x0A)

//! 0x8050800B
//! The GPMI block is not present on this chip.
#define ERROR_ROM_NAND_DRIVER_NO_GPMI_PRESENT                   (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x0B)

//! 0x8050800C
//! The NAND program operation failed.
#define ERROR_ROM_NAND_DRIVER_PROGRAM_FAILED                    (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x0C)

//! 0x8050800D
//! The NAND erase operation failed.
#define ERROR_ROM_NAND_DRIVER_ERASE_FAILED                      (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x0D)

//! 0x8050800E
//! Invalid request for more data - the NAND has loaded all known sectors.
#define ERROR_ROM_NAND_LOAD_COMPLETED                           (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x0E)

//! 0x8050800F
//! ECC failed - data is not valid.
#define ERROR_ROM_NAND_ECC_FAILED                               (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x0F)

//! 0x80508010
//! The ECC8 block required is not present on this chip.
#define ERROR_ROM_NAND_DRIVER_NO_ECC_PRESENT                    (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x10)

//! 0x80508011
//! Triple redundancy check failed for FCB block.
#define ERROR_ROM_NAND_DRIVER_FCB_TRIPLE_RED_CHK_FAILED         (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x11)

//! 0x80508012
//! Discovered hamming double bit error.
#define ERROR_ROM_NAND_DRIVER_FCB_HAMMING_DOUBLE_ERROR         (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x12)

//! 0x80508013
//! syndrome table error for hamming code check.
#define ERROR_ROM_NAND_DRIVER_FCB_SYNDROME_TABLE_MISMATCH      (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x13)

//! 0x80508014
//! invalid/unsupported ecc specified in FCB.
#define ERROR_ROM_NAND_DRIVER_FCB_INVALID_ECC                  (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x14)

//! 0x80508015
//! Block read is erased.
#define ERROR_ROM_NAND_ECC_ALL_ONES                            (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x15)

//! 0x80508016
//! ERROR_ROM_NAND_DMA_BUSY - The DMA is still running.
#define ERROR_ROM_NAND_DMA_BUSY                                (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x16)

//! 0x80508017
//! During the NAND read, the ECC threshold has reached.
#define ERROR_ROM_NAND_ECC_THRESHOLD                           (ERROR_ROM_GROUP | ERROR_ROM_NAND_DRIVER_GROUP | 0x17)


//  ***************************
//  ******** I2C GROUP ********
//  ***************************

//! The I2C driver minor group.
#define ERROR_DDI_I2C_GROUP     (0x00007000)

//! 0x80207000
//! Generic I2C driver error.
#define ERROR_DDI_I2C_GENERAL                   (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x0)

//! 0x80207001
//! A read of an unsupported size was attempted.
#define ERROR_DDI_I2C_INVALID_READ_SIZE         (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x1)

//! 0x80207002
//! The DMA transaction timed out.
#define ERROR_DDI_I2C_DMA_SEMAPHORE_TIMEOUT     (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x2)

//! 0x80207003
//! Timed out white resetting the I2C DMA channel.
#define ERROR_DDI_I2C_DMA_RESET_TIMEOUT         (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x3)

//! 0x80207004
//! Reached the end of the EEPROM.
#define ERROR_DDI_I2C_EOF                       (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x4)

//! 0x80207005
//! The EEPROM slave device did not respond.
#define ERROR_DDI_I2C_NO_SLAVE_ACK              (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x5)

//! 0x80207006
//! Slave device terminated communications early.
#define ERROR_DDI_I2C_EARLY_TERM                (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x6)

//! 0x80207007
//! Lost arbitration to another master.
#define ERROR_DDI_I2C_MASTER_LOSS               (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x7)

//! 0x80207008
//!
#define ERROR_DDI_I2C_BUFFER_NOT_FOUND          (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x8)

//! 0x80207009
//! The I2C DMA is still being processed.
#define ERROR_DDI_I2C_DMA_IN_FLIGHT             (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0x9)

//! 0x8020700a
//! The I2C master is not available in this device.
#define ERROR_DDI_I2C_MASTER_NOT_PRESENT        (ERROR_DDI_GROUP | ERROR_DDI_I2C_GROUP | 0xa)


//  *****************************
//  ******** NOR GROUP *******
//  *****************************

#define ERROR_DDI_NOR_DRIVER_GROUP     0x00022000

//! 0x80222001
//! NOR Controller has been fused out (is not present) in device
#define  ERROR_DDI_NOR_CONTROLLER_NOT_PRESENT       (ERROR_DDI_GROUP | ERROR_DDI_NOR_DRIVER_GROUP | 0x0)  

//! 0x80222001
//! NOR not supported in 100pin package
#define  ERROR_DDI_NOR_UNSUPPORTED_IN_100_PIN_PKG   (ERROR_DDI_GROUP | ERROR_DDI_NOR_DRIVER_GROUP | 0x1)  

//! 0x80222002
//! Unencrypted jump to NOR not allowed by eFuse
#define  ERROR_DDI_NOR_JUMP_NOT_ALLOWED             (ERROR_DDI_GROUP | ERROR_DDI_NOR_DRIVER_GROUP | 0x2)  

//! 0x80222003
//! Unencrypted jump to NOR returned to ROM
#define ERROR_DDI_NOR_RETURNED_FROM_JUMP_TO_NOR     (ERROR_DDI_GROUP | ERROR_DDI_NOR_DRIVER_GROUP | 0x3) 


//  *****************************
//  ******** CLK GROUP *******
//  *****************************

#define ERROR_DDI_CLK_GROUP     0x00005000

//! 0x80205001
//! Invalid Divisor setting
#define  ERROR_DDI_DIVISOR_VALUE                    (ERROR_DDI_GROUP | ERROR_DDI_CLK_GROUP | 0x1)  

//! 0x80205002
//! Attempt to select a Clock not enabled
#define  ERROR_DDI_CLK_NOT_ENABLED                  (ERROR_DDI_GROUP | ERROR_DDI_CLK_GROUP | 0x2)  


//  *****************************
//  ******** SSP GROUP *******
//  *****************************

#define ERROR_DDI_SSP_DRIVER_GROUP     0x00006000

//! 0x80206000
//! SSP Controller has been fused out (is not present) in device
#define  ERROR_DDI_SSP_CONTROLLER_NOT_PRESENT       (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x0)  

//! 0x80206001
//! SSP Request not supported in 100pin package
#define  ERROR_DDI_SSP_UNSUPPORTED_IN_100_PIN_PKG   (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x1)  

//! 0x80206002
//! Can not sumbit request until DMA has finished.
#define  ERROR_DDI_SSP_DMA_ACTIVE                   (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x2)  

//! 0x80206003
//! Invalid Configuration parameter
#define ERROR_DDI_SSP_CONFIG                        (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x3) 

//! 0x80206004
//! NULL Pointer
#define ERROR_DDI_SSP_NULL_PTR                      (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x4) 

//! 0x80206005
//! Invalid Command (IOCNTL)
#define ERROR_DDI_SSP_CMD                           (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x5) 

//! 0x80206006
//! IOCNTL Function is known, but not supported
#define ERROR_DDI_SSP_UNSUPPORTED                   (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x6) 

//! 0x80206007
//! Invalid SCK table index number
#define ERROR_DDI_SSP_SCK_INDEX                     (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x7) 

//! 0x80206008
//! DMA Timed out
#define ERROR_SSP_DRIVER_DMA_TIMEOUT                (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x8) 

//! 0x80206009
//! Incorrect channel specified (0,1)
#define ERROR_SSP_CHANNEL_INVALID                   (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0x9) 

//! 0x8020600A
//! Incorrect channel specified (0,1)
#define ERROR_SSP_HW_TIMEOUT                   (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0xA) 

//! 0x8020600B
//! Incorrect channel specified (0,1)
#define ERROR_SSP_INVALID_SCK_FREQ                   (ERROR_DDI_GROUP | ERROR_DDI_SSP_DRIVER_GROUP | 0xB) 

//  *****************************
//  ******** SPI GROUP *******
//  *****************************

#define ERROR_DDI_SPI_DRIVER_GROUP     0x00009000

//! 0x80209000
//! Invalid spi_IoCtrl() command
#define  ERROR_DDI_SPI_INVALID_IOCTRL_COMMAND       (ERROR_DDI_GROUP | ERROR_DDI_SPI_DRIVER_GROUP | 0x0)  

//! 0x80209001
//! Invalid boot mode passed to spi_Init()
#define  ERROR_DDI_SPI_INVALID_BOOT_MODE            (ERROR_DDI_GROUP | ERROR_DDI_SPI_DRIVER_GROUP | 0x1)  

//! 0x80209002
//! Invalid start address in media config block
#define  ERROR_DDI_SPI_INVALID_CFGBLK_START_ADDR    (ERROR_DDI_GROUP | ERROR_DDI_SPI_DRIVER_GROUP | 0x2)  

//! 0x80209003
//! Invalid start address in media config block
#define  ERROR_DDI_SPI_INVALID_CLOCK_SPEED_STRUCT_SIZE    (ERROR_DDI_GROUP | ERROR_DDI_SPI_DRIVER_GROUP | 0x3)  

//! 0x80209004
//! Loader didn't give the driver enough memory
#define  ERROR_DDI_SPI_INSUFFICIENT_CONTEXT_MEMORY    (ERROR_DDI_GROUP | ERROR_DDI_SPI_DRIVER_GROUP | 0x4)  

//! 0x80209005
//! Loader didn't give the driver enough memory
#define  ERROR_DDI_SPI_INVALID_CFGBLCK_SECTOR_SIZE    (ERROR_DDI_GROUP | ERROR_DDI_SPI_DRIVER_GROUP | 0x5)  

//! 0x80209006
//! Loader didn't give the driver enough memory
#define  ERROR_DDI_SPI_DRIVER_NOT_INITIALIZED    (ERROR_DDI_GROUP | ERROR_DDI_SPI_DRIVER_GROUP | 0x6)  


//  *****************************
//  ******** SD GROUP *******
//  *****************************

#define ERROR_DDI_SD_DRIVER_GROUP     0x0000A000

//! 0x8020A000
//! Invalid spi_IoCtrl() command
#define  ERROR_DDI_SD_INVALID_IOCTRL_COMMAND       (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x0)  

//! 0x8020A001
//! Invalid boot mode passed to sd_Init()
#define  ERROR_DDI_SD_INVALID_BOOT_MODE            (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x1)  

//! 0x8020A002
//! ?
#define  ERROR_DDI_SD_INVALID_CFGBLK_START_ADDR    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x2)  

//! 0x8020A003
//! ?
#define  ERROR_DDI_SD_INVALID_CLOCK_SPEED_STRUCT_SIZE    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x3)  

//! 0x8020A004
//! ?
#define  ERROR_DDI_SD_INSUFFICIENT_CONTEXT_MEMORY    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x4)  

//! 0x8020A005
//! ?
#define  ERROR_DDI_SD_INVALID_CFGBLCK_SECTOR_SIZE    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x5)  

//! 0x8020A006
//! Loader didn't give the driver enough memory
#define  ERROR_DDI_SD_DRIVER_NOT_INITIALIZED    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x6)  

//! 0x8020A007
//! ?
#define  ERROR_DDI_SD_CONFIG_BLOCK_NOT_FOUND    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x7)  

//! 0x8020A008
//! ?
#define  ERROR_DDI_SD_DETECT_DEVICE_FAIL    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x8)  

//! 0x8020A009
//! ?
#define  ERROR_DDI_SD_BOOT_IMAGE_NOT_FOUND    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x9)  

//! 0x8020A00A
//! ?
#define  ERROR_DDI_SD_UNABLE_TO_SELECT_DEVICE    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0xA)  

//! 0x8020A00B
//! ?
#define  ERROR_DDI_SD_UNABLE_TO_DESELECT_DEVICE    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0xB)  

//! 0x8020A00C
//! ?
#define  ERROR_DDI_SD_IDENTIFY_DEVICE_FAIL    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0xC)  

//! 0x8020A00D
//! ?
#define  ERROR_DDI_SD_GENERAL_TIMEOUT    (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0xD)  

//! 0x8020A00E
//! ?
#define  ERROR_DDI_SD_GENERAL_FAILURE   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0xE)  

//! 0x8020A00F
//! ?
#define  ERROR_DDI_SD_2_X_INCORRECT_CHECK_PATTERN   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0xF)  

//! 0x8020A010
//! ?
#define  ERROR_DDI_SD_2_X_INCOMPATIBLE_VOLTAGE_RANGE   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x10)  

//! 0x8020A011
//! ?
#define  ERROR_DDI_SD_DEVICE_NOT_SUPPORTED   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x11)  

//! 0x8020A012
//! ?
#define  ERROR_DDI_SD_DETECTION_TIME_OUT   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x12)  

//! 0x8020A013
//! ?
#define  ERROR_DDI_SD_SD_SEND_OP_COND_TIMEOUT   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x13)  

//! 0x8020A014
//! ?
#define  ERROR_DDI_SD_MMC_DEVICE_NOT_SUPPORTED   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x14)  

//! 0x8020A015
//! ?
#define  ERROR_DDI_SD_MMC_DETECTION_TIME_OUT   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x15)  

//! 0x8020A016
//! ?
#define  ERROR_DDI_SD_MMC_UNSUPPORTED_EXT_CSD_REV   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x16)  

//! 0x8020A017
//! ?
#define  ERROR_DDI_SD_SD_DEVICE_NOT_SUPPORTED   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x17)  

//! 0x8020A018
//! ?
#define  ERROR_DDI_SD_SD_UNSUPPORTED_SCR   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x18)  

//! 0x8020A019
//! ?
#define  ERROR_DDI_SD_SD_UNSUPPORTED_SPEC   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x19)  

//! 0x8020A01A
//! ?
#define  ERROR_DDI_SD_MMC_INVALID_BUS_WIDTH   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x1A)  

//! 0x8020A01B
//! ?
#define  ERROR_DDI_SD_SD_INVALID_BUS_WIDTH   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x1B)  

//! 0x8020A01C
//! ?
#define  ERROR_DDI_SD_BUS_WIDTH_SELECTION_FAILURE   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x1C)  

//! 0x8020A01D
//! ?
#define  ERROR_DDI_SD_MBR_NOT_FOUND   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x1D)  

//! 0x8020A01E
//! ?
#define  ERROR_DDI_SD_INVALID_MBR   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x1E)  

//! 0x8020A01F
//! ?
#define  ERROR_DDI_SD_INVALID_BLOCK_SIZE   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x1F)  

//! 0x8020A020
//! ?
#define  ERROR_DDI_SD_INVALID_VOLTAGE_WINDOW   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x20)  

//! 0x8020A021
//! ?
#define  ERROR_DDI_SD_IMPROPER_BOOT_IMAGE_ALIGNMENT   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x21)  

//! 0x8020A022
//! ?
#define  ERROR_DDI_SD_INVALID_MBLOCK_READ_REENTRY   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x22)  

//! 0x8020A023
//! ?
#define  ERROR_DDI_SD_MULTI_READ_TIMEOUT   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x23)  

//! 0x8020A024
//! ?
#define  ERROR_DDI_SD_FAILED_TO_READ_CSD   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x24)  

//! 0x8020A025
//! ?
#define  ERROR_DDI_SD_INVALID_PERSISTENT_BUS_WIDTH   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x25)  

//! 0x8020A026
//! ?
#define  ERROR_DDI_SD_SET_BUS_WIDTH_FAILURE   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x26)  

//! 0x8020A027
//! ?
#define  ERROR_DDI_SD_FAILED_SWITCH   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x27)  

//! 0x8020A028
//! ?
#define  ERROR_DDI_SD_CMD2_FAILED   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x28)  

//! 0x8020A029
//! ?
#define  ERROR_DDI_SD_CMD55_FAILED   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x29)  

//! 0x8020A02A
//! ?
#define  ERROR_DDI_SD_PERSISTENT_READ_FAILURE   (ERROR_DDI_GROUP | ERROR_DDI_SD_DRIVER_GROUP | 0x2A)  




//  *****************************
//  ******** ATA GROUP *******
//  *****************************
#define ERROR_ROM_ATA_DRIVER_GROUP      0x00003000

//  ********************************************
//  ******** COMMON TO ALL DRIVERS GROUP *******
//  ********************************************
#define ERROR_ROM_COMMON_DRIVER_GROUP      0x0000B000

//! 0x8020B001
//! Not a valid MBR
#define ERROR_ROM_COMMON_DRIVER_INVALID_MBR  (ERROR_DDI_GROUP | ERROR_ROM_COMMON_DRIVER_GROUP | 0x1) 

//! 0x8020B002
//! Not a valid FW Config Block
#define ERROR_ROM_COMMON_DRIVER_INVALID_CONFIGBLOCK  (ERROR_DDI_GROUP | ERROR_ROM_COMMON_DRIVER_GROUP | 0x2) 

#endif // _RETURN_CODES_H
////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
