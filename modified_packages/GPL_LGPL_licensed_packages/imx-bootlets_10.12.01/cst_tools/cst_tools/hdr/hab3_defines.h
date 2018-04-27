#ifndef HAB3_DEFINES_H
#define HAB3_DEFINES_H
/*===========================================================================*/
/**
    @file    hab3_defines.h

    @brief   CST CSF HAB3 definitions

@verbatim
=============================================================================

              Freescale Semiconductor Confidential Proprietary
        (c) Freescale Semiconductor, Inc. 2011 All rights reserved.

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
Fareed Mohammed  03-Mar-2011                  Initial version
=============================================================================
Portability: 

These definitions are customised for 32 bit cores of either
endianness.

=============================================================================
@endverbatim */

/*===========================================================================
                                MACROS
=============================================================================*/

/* Version offset in header */
#define CSF_HDR_HAB3_VERSION_OFFSET (0)

/* HAB3 definition of version byte present in all known chips */
#define CSF_HAB3_VERSION_BYTE (0xB3)

/* HAB3 definitions for possible values in CSF type field of CSF header */
#define HAB3_SEC_CFG_ENGINEERING    (1)
#define HAB3_SEC_CFG_PRODUCTION     (2)

/* Command codes for public_key_sig_verify  */
#define CSF_HAB3_CMD_PUBLIC_KEY_SIG_VERIFY_H (0x30)
#define CSF_HAB3_CMD_PUBLIC_KEY_SIG_VERIFY_L (0xFF)

/* Offsets in public_key_sig_verify command structure  */
#define CSF_HAB3_CMD_ID_OFFSET (0)
#define CSF_HAB3_CERT_TYPE_OFFSET (2)
#define CSF_HAB3_CERT_DATA_ADDRESS_OFFSET (3)
#define CSF_HAB3_CERT_HASH_OFFSET (7)

/* Command codes for code_sig_verify and unique_code_sig_verify */
#define CSF_HAB3_CODE_SIG_VERIFY_H (0x0C)
#define CSF_HAB3_CODE_SIG_VERIFY_L (0x30)
#define CSF_HAB3_UNIQUE_CODE_SIG_VERIFY_H (0x0C)
#define CSF_HAB3_UNIQUE_CODE_SIG_VERIFY_L (0x0C)

/* Offsets in public_key_sig_verify command structure  */
#define CSF_HAB3_AUTDAT_BLKCNT_OFFSET (4)
#define CSF_HAB3_AUTDAT_SIG_OFFSET (21)

/* Command codes for write_register command */
#define CSF_HAB3_WRTDAT_CMD_ID_H (0x33)
#define CSF_HAB3_WRTDAT_CMD_ID_L (0xCF)

/* Command codes for NOP command */
#define CSF_HAB3_NOP_CMD_ID_H (0xF0)
#define CSF_HAB3_NOP_CMD_ID_L (0xF0)

/* HAB3 hash types */
#define HAB_RSA_SHA1                    (0x0011)
#define HAB_RSA_SHA1_SAHARA             (0x00C1)
#define HAB_RSA_SHA1_RTIC               (0x00D1)
#define HAB_RSA_SHA1_RTIC_KEEP          (0x80D1)
#define HAB_RSA_SHA256                  (0x0012)
#define HAB_RSA_SHA256_SAHARA           (0x00C2)
#define HAB_RSA_SHA256_RTIC             (0x00D2)
#define HAB_RSA_SHA256_RTIC_KEEP        (0x80D2)

/* String "Generic" appears in CSF file for UID argument but here we are 
 * defining a numeric dummy value for use in label_map in cst.c file 
 */
#define HAB3_UID_GENERIC            (0xD0BB7)

/* Max length allowed */
#define HAB3_MAX_UID_LENGTH         (255)

/* Certificate type for HAB3 */
#define HAB_WTLS_CERTIFICATE        (0xff)

#endif //HAB3_DEFINES_H
