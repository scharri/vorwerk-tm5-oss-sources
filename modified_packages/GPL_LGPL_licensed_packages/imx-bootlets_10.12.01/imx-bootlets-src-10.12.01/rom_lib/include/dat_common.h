#ifndef DAT_COMMON_H
#define DAT_COMMON_H
/*===========================================================================*/
/**
    @file    dat_common.h

    @brief

@verbatim
=============================================================================

              Freescale Semiconductor Confidential Proprietary
        (c) Freescale Semiconductor, Inc. 2007, 2008. All rights reserved.

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


=============================================================================
Portability:

These definitions are customised for 32 bit cores of either
endianness.

=============================================================================
@endverbatim */

/*===========================================================================
                                 INCLUDE FILES
=============================================================================*/
#include "hab.h"
#include "hab_cmd.h"
/*===========================================================================
                                   CONSTANTS
=============================================================================*/

/*===========================================================================
                                     MACROS
=============================================================================*/

#define HAB_EVT_HDR(bytes)                      \
    HDR(HAB_TAG_EVT, (bytes), HAB_VERSION)

#define LEN2(a) \
    (((a)[0] << 8) + (a)[1])

#define HDR_LEN(hdr)                            \
    LEN2(((hab_hdr_t*)&(hdr))->len)

#define HAB_EVT_INFO(sts, rsn, ctx, eng)                                \
    (uint8_t)(sts), (uint8_t)(rsn), (uint8_t)(ctx), (uint8_t)(eng)

/**DCD write masks*/
#define DCD_INPUT_32_BIT    0x98765432
#define DCD_INPUT_16_BIT    0xfedc
#define DCD_INPUT_8_BIT     0xba

#endif


