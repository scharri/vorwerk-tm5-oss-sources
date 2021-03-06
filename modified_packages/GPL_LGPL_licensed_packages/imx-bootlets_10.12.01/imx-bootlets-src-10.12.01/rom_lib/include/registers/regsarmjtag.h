/*
 *
 * Filename: regsarmjtag.h
 *
 * Description: PIO Registers for ARMJTAG interface
 *
 * Xml Revision: 1.0
 *
 * Template revision: 20911
 *
 *
 *
 * Copyright (C) Freescale Semiconductor Unpublished
 *
 * Freescale Semiconductor
 * Proprietary & Confidential
 *
 * This source code and the algorithms implemented therein constitute
 * confidential information and may compromise trade secrets of SigmaTel, Inc.
 * or its associates, and any unauthorized use thereof is prohibited.
 *
 *
 *
 * WARNING!  THIS FILE IS AUTOMATICALLY GENERATED FROM XML.
 *                DO NOT MODIFY THIS FILE DIRECTLY.
 *
 *
 *
 * The following naming conventions are followed in this file.
 *      XX_<module>_<regname>_<field>
 *
 * XX specifies the define / macro class
 *      HW pertains to a register
 *      BM indicates a Bit Mask
 *      BF indicates a Bit Field macro
 *
 * <module> is the hardware module name which can be any of the following...
 *      USB20 (Note when there is more than one copy of a given module, the
 *      module name includes a number starting from 0 for the first instance
 *      of that module)
 *
 * <regname> is the specific register within that module
 *
 * <field> is the specific bitfield within that <module>_<register>
 *
 * We also define the following...
 *      hw_<module>_<regname>_t is typedef of anonymous union
 *
 */

#ifndef _ARMJTAG_H
#define _ARMJTAG_H  1

#include "regs.h"

#ifndef REGS_ARMJTAG_BASE
#define REGS_ARMJTAG_BASE (REGS_BASE + 0x3c800)
#endif

/*
 * HW_JTAG_OPERATION - jtag operation register
 */
#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        unsigned CS  : 32;
    } B;
} hw_jtag_operation_t;
#endif

/*
 * constants & macros for entire HW_JTAG_OPERATION register
 */
#define HW_JTAG_OPERATION_ADDR      (0x8003c800)

#ifndef __LANGUAGE_ASM__
#define HW_JTAG_OPERATION           (*(volatile hw_jtag_operation_t *) HW_JTAG_OPERATION_ADDR)
#define HW_JTAG_OPERATION_RD()      (HW_JTAG_OPERATION.U)
#define HW_JTAG_OPERATION_WR(v)     (HW_JTAG_OPERATION.U = (v))
#define HW_JTAG_OPERATION_SET(v)    (HW_JTAG_OPERATION_WR(HW_JTAG_OPERATION_RD() |  (v)))
#define HW_JTAG_OPERATION_CLR(v)    (HW_JTAG_OPERATION_WR(HW_JTAG_OPERATION_RD() & ~(v)))
#define HW_JTAG_OPERATION_TOG(v)    (HW_JTAG_OPERATION_WR(HW_JTAG_OPERATION_RD() ^  (v)))
#endif


/*
 * constants & macros for individual HW_JTAG_OPERATION bitfields
 */
/* --- Register HW_JTAG_OPERATION, field CS */

#define BP_JTAG_OPERATION_CS      0
#define BM_JTAG_OPERATION_CS      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_JTAG_OPERATION_CS(v)   ((reg32_t) v)
#else
#define BF_JTAG_OPERATION_CS(v)   (v)
#endif
#ifndef __LANGUAGE_ASM__
#define BW_JTAG_OPERATION_CS(v)   (HW_JTAG_OPERATION.B.CS = (v))
#endif



/*
 * HW_JTAG_RESULT - jtag operation result register
 */
#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        unsigned CS  : 32;
    } B;
} hw_jtag_result_t;
#endif

/*
 * constants & macros for entire HW_JTAG_RESULT register
 */
#define HW_JTAG_RESULT_ADDR      (0x8003c804)

#ifndef __LANGUAGE_ASM__
#define HW_JTAG_RESULT           (*(volatile hw_jtag_result_t *) HW_JTAG_RESULT_ADDR)
#define HW_JTAG_RESULT_RD()      (HW_JTAG_RESULT.U)
#define HW_JTAG_RESULT_WR(v)     (HW_JTAG_RESULT.U = (v))
#define HW_JTAG_RESULT_SET(v)    (HW_JTAG_RESULT_WR(HW_JTAG_RESULT_RD() |  (v)))
#define HW_JTAG_RESULT_CLR(v)    (HW_JTAG_RESULT_WR(HW_JTAG_RESULT_RD() & ~(v)))
#define HW_JTAG_RESULT_TOG(v)    (HW_JTAG_RESULT_WR(HW_JTAG_RESULT_RD() ^  (v)))
#endif


/*
 * constants & macros for individual HW_JTAG_RESULT bitfields
 */
/* --- Register HW_JTAG_RESULT, field CS */

#define BP_JTAG_RESULT_CS      0
#define BM_JTAG_RESULT_CS      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_JTAG_RESULT_CS(v)   ((reg32_t) v)
#else
#define BF_JTAG_RESULT_CS(v)   (v)
#endif
#ifndef __LANGUAGE_ASM__
#define BW_JTAG_RESULT_CS(v)   (HW_JTAG_RESULT.B.CS = (v))
#endif



/*
 * HW_ARM_RW_ADDRESS - ARM read or write addr register
 */
#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        unsigned CS  : 32;
    } B;
} hw_arm_rw_address_t;
#endif

/*
 * constants & macros for entire HW_ARM_RW_ADDRESS register
 */
#define HW_ARM_RW_ADDRESS_ADDR      (0x8003c808)

#ifndef __LANGUAGE_ASM__
#define HW_ARM_RW_ADDRESS           (*(volatile hw_arm_rw_address_t *) HW_ARM_RW_ADDRESS_ADDR)
#define HW_ARM_RW_ADDRESS_RD()      (HW_ARM_RW_ADDRESS.U)
#define HW_ARM_RW_ADDRESS_WR(v)     (HW_ARM_RW_ADDRESS.U = (v))
#define HW_ARM_RW_ADDRESS_SET(v)    (HW_ARM_RW_ADDRESS_WR(HW_ARM_RW_ADDRESS_RD() |  (v)))
#define HW_ARM_RW_ADDRESS_CLR(v)    (HW_ARM_RW_ADDRESS_WR(HW_ARM_RW_ADDRESS_RD() & ~(v)))
#define HW_ARM_RW_ADDRESS_TOG(v)    (HW_ARM_RW_ADDRESS_WR(HW_ARM_RW_ADDRESS_RD() ^  (v)))
#endif


/*
 * constants & macros for individual HW_ARM_RW_ADDRESS bitfields
 */
/* --- Register HW_ARM_RW_ADDRESS, field CS */

#define BP_ARM_RW_ADDRESS_CS      0
#define BM_ARM_RW_ADDRESS_CS      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_ARM_RW_ADDRESS_CS(v)   ((reg32_t) v)
#else
#define BF_ARM_RW_ADDRESS_CS(v)   (v)
#endif
#ifndef __LANGUAGE_ASM__
#define BW_ARM_RW_ADDRESS_CS(v)   (HW_ARM_RW_ADDRESS.B.CS = (v))
#endif



/*
 * HW_ARM_RW_DATA - arm read or write data register
 */
#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        unsigned CS  : 32;
    } B;
} hw_arm_rw_data_t;
#endif

/*
 * constants & macros for entire HW_ARM_RW_DATA register
 */
#define HW_ARM_RW_DATA_ADDR      (0x8003c80c)

#ifndef __LANGUAGE_ASM__
#define HW_ARM_RW_DATA           (*(volatile hw_arm_rw_data_t *) HW_ARM_RW_DATA_ADDR)
#define HW_ARM_RW_DATA_RD()      (HW_ARM_RW_DATA.U)
#define HW_ARM_RW_DATA_WR(v)     (HW_ARM_RW_DATA.U = (v))
#define HW_ARM_RW_DATA_SET(v)    (HW_ARM_RW_DATA_WR(HW_ARM_RW_DATA_RD() |  (v)))
#define HW_ARM_RW_DATA_CLR(v)    (HW_ARM_RW_DATA_WR(HW_ARM_RW_DATA_RD() & ~(v)))
#define HW_ARM_RW_DATA_TOG(v)    (HW_ARM_RW_DATA_WR(HW_ARM_RW_DATA_RD() ^  (v)))
#endif


/*
 * constants & macros for individual HW_ARM_RW_DATA bitfields
 */
/* --- Register HW_ARM_RW_DATA, field CS */

#define BP_ARM_RW_DATA_CS      0
#define BM_ARM_RW_DATA_CS      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_ARM_RW_DATA_CS(v)   ((reg32_t) v)
#else
#define BF_ARM_RW_DATA_CS(v)   (v)
#endif
#ifndef __LANGUAGE_ASM__
#define BW_ARM_RW_DATA_CS(v)   (HW_ARM_RW_DATA.B.CS = (v))
#endif



/*
 * HW_CHECK_BRPT - check breakpoint register
 */
#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        unsigned CS  : 32;
    } B;
} hw_check_brpt_t;
#endif

/*
 * constants & macros for entire HW_CHECK_BRPT register
 */
#define HW_CHECK_BRPT_ADDR      (0x8003c810)

#ifndef __LANGUAGE_ASM__
#define HW_CHECK_BRPT           (*(volatile hw_check_brpt_t *) HW_CHECK_BRPT_ADDR)
#define HW_CHECK_BRPT_RD()      (HW_CHECK_BRPT.U)
#define HW_CHECK_BRPT_WR(v)     (HW_CHECK_BRPT.U = (v))
#define HW_CHECK_BRPT_SET(v)    (HW_CHECK_BRPT_WR(HW_CHECK_BRPT_RD() |  (v)))
#define HW_CHECK_BRPT_CLR(v)    (HW_CHECK_BRPT_WR(HW_CHECK_BRPT_RD() & ~(v)))
#define HW_CHECK_BRPT_TOG(v)    (HW_CHECK_BRPT_WR(HW_CHECK_BRPT_RD() ^  (v)))
#endif


/*
 * constants & macros for individual HW_CHECK_BRPT bitfields
 */
/* --- Register HW_CHECK_BRPT, field CS */

#define BP_CHECK_BRPT_CS      0
#define BM_CHECK_BRPT_CS      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_CHECK_BRPT_CS(v)   ((reg32_t) v)
#else
#define BF_CHECK_BRPT_CS(v)   (v)
#endif
#ifndef __LANGUAGE_ASM__
#define BW_CHECK_BRPT_CS(v)   (HW_CHECK_BRPT.B.CS = (v))
#endif


#endif /* _ARMJTAG_H */

////////////////////////////////////////////////////////////////////////////////
