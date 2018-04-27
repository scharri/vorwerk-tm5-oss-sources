#ifndef HAB_ROM_H
#define HAB_ROM_H
/*===========================================================================*/
/**
    @file    hab_rom.h

    @brief   HAB ROM interface


@verbatim
=============================================================================

              Freescale Semiconductor Confidential Proprietary
    (c) Freescale Semiconductor, Inc. 2007, 2008, 2009. All rights reserved.

Presence of a copyright notice is not an acknowledgement of
publication. This software file listing contains information of
Freescale Semiconductor, Inc. that is of a confidential and
proprietary nature and any viewing or use of this file is prohibited
without specific written permission from Freescale Semiconductor, Inc.

=============================================================================
Revision History:

               Modification Date   Tracking
Author           (dd-mmm-yyyy)      Number    Description of Changes
---------------  -------------    ----------  -----------------------
David Hartley    11-Dec-2007      ENGR55511   Initial version
David Hartley    29-Jul-2008      ENGR82581   Add CMS, X.509v3, SHA-512
                                              and ECDSA over P-521
David Hartley    07-Nov-2008      ENGR92335   Implementation updates
Ram Korukonda    04-Mar-2009      ENGR106041  Updated hab_fab_test description
Ram Korukonda    15-Apr-2009      ENGR111389  Updated the preconfigure 
                                              function declarations.
David Hartley    02-Sep-2009      ENGR114783  Add support for export control
Rod Ziolkowski                    ENGR116107  Add support for SCCv2
                                  ENGR116112  Add support for CSU
David Hartley    09-Nov-2009      ENGR117771  Run all fab tests at FAIL stage

=============================================================================
Portability: 

These definitions are customised for 32 bit cores of either endianness.

=============================================================================
@endverbatim */

/*===========================================================================
                            INCLUDE FILES
=============================================================================*/

#include "hab_types.h"          /* For types, constants, macros */

/** @cond rom */

/*===========================================================================
                              CONSTANTS
=============================================================================*/

/*===========================================================================
                                MACROS
=============================================================================*/

/*===========================================================================
                                ENUMS
=============================================================================*/

/*===========================================================================
                    STRUCTURES AND OTHER TYPEDEFS
=============================================================================*/

/*===========================================================================
                     GLOBAL VARIABLE DECLARATIONS
=============================================================================*/

/*===========================================================================
                         FUNCTION PROTOTYPES
=============================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

/** @rom Pre-configuration for SCCv2
 * @ingroup pre_cfg
 *
 * @par Purpose
 *
 * This is the pre-configuration variant intended for ICs with SCCv2 only.
 *
 * @par Operation
 *
 * With SCC enabled, the initialization steps performed are: 
 *
 * - Wait for secure RAM zeroization to complete
 * - Allocate partitions indicated in #hab_hal_ict.preconfigure_data
 * - Set permissions for allocated partitions to "host read/write/execute" and
 * "trusted host read/write" and "user/supervisor" access
 * - Configure allocated partitions not to zeroize on alert.
 * - Initialise the audit log
 *
 * @remark Once the partitions are allocated and configured, they may be used
 * for ROM stack.  The stack pointer may be set accordingly prior to entering
 * this function.
 *
 * @warning Care must be taken allocating partitions on multi-core ICs to
 * ensure that stacks do not overlap.
 *
 * @warning This function should not be called directly, but instead via the
 * IC configuration table.
 *
 * @see hab_hal_ict.preconfigure() for generic details.
 */
extern void hab_eng_scc_preconfigure(void);

/** @rom Pre-configuration for CSU/SCCv2
 * @ingroup pre_cfg
 *
 * @par Purpose
 *
 * This is the pre-configuration variant intended for ICs with both CSU and
 * SCCv2.
 *
 * @par Operation
 *
 * With SCC enabled, the initialization steps described for
 * hab_eng_scc_preconfigure() are performed first.
 *
 * With CSU enabled, further initialization steps performed are:
 *
 * - Unmask all CSU alarm inputs 
 * - Grant all peripheral bus masters supervisor access status
 * - If the IC is not configured as #HAB_CFG_CLOSED,
 * disable the reset controller and SRTC alarm routes for all alarm inputs.
 *
 * @remark Unlike hab_eng_scc_preconfigure() the stack pointer must be set
 * accordingly prior to entering this function.
 *
 * @remark No CSU register lock bits are set.
 *
 * @warning This function should not be called directly, but instead via the
 * IC configuration table.
 *
 * @see hab_hal_ict.preconfigure() for generic details.
 */
extern void hab_eng_csu_scc_preconfigure(void);

/** @rom Pre-configuration for no SHW
 * @ingroup pre_cfg
 *
 * @par Purpose
 *
 * This is the pre-configuration variant intended for ICs with no SHW.
 *
 * @par Operation
 *
 * The initialisation steps performed are: 
 *
 * - Initialise the audit log
 *
 * @remark This variant requires no preconfiguration data.
 *
 * @warning This function should not be called directly, but instead via the
 * IC configuration table.
 *
 * @see hab_hal_ict.preconfigure() for generic details.
 */
extern void hab_eng_sw_preconfigure(void);

/** @rom Fabrication tests of security HW plugins.
 * @ingroup fab_test
 *
 * @par Purpose
 *
 * This function exercises the @ref shw plugins in fabrication test sequences.
 * It is intended for use by the boot ROM in fabrication test boot mode.
 * 
 * @par Operation 
 *
 * If a state machine engine is present and enabled, this function performs
 * the following operations:
 *
 * - Wait for the system to enter Check state
 * - Run the fabrication test sequence (Check) of each @ref shw engine
 * - Move the state machine to Trusted state
 * - Run the fabrication test sequence (Trusted) of each @ref shw engine
 * - Move the state machine to Soft Fail state
 * - Run the fabrication test sequence (Fail) of each @ref shw engine
 *
 * If any failure occurs, an audit event is logged and all remaining
 * operations are abandoned, except at the Soft Fail stage, where the audit
 * event is logged but the remaining engine fabrication test sequences are
 * still run.
 *
 * @remark if fab test config is either #HAB_HAL_TEST_CFG_SECURE or 
 * #HAB_HAL_TEST_CFG_KEYGEN, all the above mentioned operations will 
 * be performed.
 *
 * @remark if fab test config is #HAB_HAL_TEST_CFG_NONSEC, the @ref shw 
 * fabrication test sequences (Check and Trusted) are not run, but the 
 * state machine is still moved to Soft Fail state and the @ref shw 
 * fabrication test sequences (Fail) are still run.
 * 
 * @remark if no state machine engine is present and enabled, this function
 * behaves as described for #HAB_HAL_TEST_CFG_NONSEC above.
 * 
 * @remark This function should not be enclosed within a hab_rvt.entry()
 * ... hab_rvt.exit() pair.
 *
 * @remark The fabrication test sequence (Trusted) for the final @ref shw
 * engine may itself move the state machine to Soft Fail state, but this is
 * not regarded as a failure of the fabrication test.
 *
 * @post If a failure or warning occurs, an audit event is logged.  The status
 * and reason logged are described in the relevant @ref shw plugin
 * documentation.
 *
 * @post If #hab_hal_ict.fab_test_point is provided, an @ref evt record is
 * written to successive word locations, starting with the given address.
 *
 * @retval None.
 */
extern void
hab_fab_test(void);

#ifdef __cplusplus
}
#endif

/** @endcond */

#endif /* HAB_ROM_H */
