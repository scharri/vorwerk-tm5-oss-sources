////////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    persistent_bit.h
//! \brief   Persistent Bit definitions
//! \version 0.1
//! \date    04/2006
//!
////////////////////////////////////////////////////////////////////////////////

#include "registers/regsrtc.h"
#include "rom_types.h"
#include "return_codes.h"


/* ----------------------------------------------- */
/*            HW_RTC_PERSISTENT0 - do not use      */
/* ----------------------------------------------- */

/* ----------------------------------------------- */
/*            HW_RTC_PERSISTENT1 - Use for ROM     */
/* ----------------------------------------------- */

#define BP_PERSIST1_FORCE_RECOVERY              0
#define BM_PERSIST1_FORCE_RECOVERY              0x00000001

#define BP_PERSIST1_ROM_REDUNDANT_BOOT          1
#define BM_PERSIST1_ROM_REDUNDANT_BOOT          0x00000002

#define BP_PERSIST1_NAND_SDK_BLOCK_REWRITE      2
#define BM_PERSIST1_NAND_SDK_BLOCK_REWRITE      0x00000004

#define BP_PERSIST1_SD_SPEED_ENABLE             3
#define BM_PERSIST1_SD_SPEED_ENABLE             0x00000008

#define BP_PERSIST1_SD_INIT_SEQ_1_DISABLE       4
#define BM_PERSIST1_SD_INIT_SEQ_1_DISABLE       0x00000010

#define BP_PERSIST1_SD_CMD0_DISABLE             5
#define BM_PERSIST1_SD_CMD0_DISABLE             0x00000020

#define BP_PERSIST1_SD_INIT_SEQ_2_ENABLE        6
#define BM_PERSIST1_SD_INIT_SEQ_2_ENABLE        0x00000040

#define HW_RTC_ROM_PERSISTENT_BITS1         ((volatile reg32_t *)HW_RTC_PERSISTENT1_ADDR)
#define HW_RTC_ROM_PERSISTENT_BITS1_SET     ((volatile reg32_t *)HW_RTC_PERSISTENT1_SET_ADDR)
#define HW_RTC_ROM_PERSISTENT_BITS1_CLR     ((volatile reg32_t *)HW_RTC_PERSISTENT1_CLR_ADDR)
#define HW_RTC_ROM_PERSISTENT_BITS1_TOG     ((volatile reg32_t *)HW_RTC_PERSISTENT1_TOG_ADDR)

/* ----------------------------------------------- */
/*            HW_RTC_PERSISTENT2                   */
/* ----------------------------------------------- */

/* ----------------------------------------------- */
/*            HW_RTC_PERSISTENT3                   */
/* ----------------------------------------------- */

/* ----------------------------------------------- */
/*            HW_RTC_PERSISTENT4                   */
/* ----------------------------------------------- */

/* ----------------------------------------------- */
/*            HW_RTC_PERSISTENT5                   */
/* ----------------------------------------------- */

// Use this mask to reduce the number of clocks to wait for the Persistent bits
// to update.  Since the copy starts at 0, then word 1, then word 2, we can
// save time by returning as soon as the word 1 has been copied.  We're reserving
// word1 for ROM, but if we need to start using word2, increase this to 0x7.
#define HW_RTC_ROM_USE_MASK                     0x00000003

////////////////////////////////////////////////////////////////////////////////
//! Prototypes
////////////////////////////////////////////////////////////////////////////////
#ifndef __LANGUAGE_ASM__

// Ensure the RTC is out of reset.
#define initialize_RTC()    (HW_RTC_CTRL_CLR(BM_RTC_CTRL_SFTRST | BM_RTC_CTRL_CLKGATE))

RtStatus_t load_persistent_registers(volatile reg32_t * pu32PersistentWord, 
                                     uint32_t * pu32Result);
RtStatus_t write_persistent_word(volatile reg32_t * pu32PersistentWord, 
                                 uint32_t u32Data);
#endif

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

