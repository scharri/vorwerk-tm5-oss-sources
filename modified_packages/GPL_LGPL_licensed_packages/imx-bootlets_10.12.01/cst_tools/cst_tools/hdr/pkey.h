#ifndef PKEY_H
#define PKEY_H
/*===========================================================================*/
/**
    @file    pkey.h

    @brief   CST private key and password provider API 

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
Fareed Mohammed  12-Sep-2011                  Initial version
=============================================================================
Portability: 

These definitions are customised for 32 bit cores of either
endianness.

=============================================================================
@endverbatim */

/*===========================================================================
                            INCLUDE FILES
=============================================================================*/

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

/** get_passcode_to_key_file
 *
 * @par Purpose
 *
 * Callback to gte password for the encrypted key file. Default behavior 
 * can be overridden by customers by linking with their own implementation
 * of libpw.a and this function
 *
 * @par Operation 
 *
 * @param[out] buf,  return buffer for the password
 *
 * @param[in] size,  size of the buf in bytes
 *
 * @param[in] rwflag,  not used
 *
 * @param[in] userdata,  not used
 *
 * @retval returns size of password string
 */
int get_passcode_to_key_file(char *buf, int size, int rwflag, void *userdata);

/** get_key_file
 *
 * @par Purpose
 *
 * This API extracts private key filename using cert filename. This API is
 * moved to libpw to allow customers to change its implementation to better
 * suit their needs.
 *
 * @par Operation 
 *
 * @param[in] cert_file,  filename for certificate
 *
 * @param[out] key_file,  filename for private key for the input certificate
 *
 * @retval SUCCESS
 */
int32_t get_key_file(const char* cert_file, char* key_file);

#ifdef __cplusplus
}
#endif

#endif /* PKEY_H */
