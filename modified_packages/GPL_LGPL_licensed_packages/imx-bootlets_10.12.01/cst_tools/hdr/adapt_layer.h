#ifndef ADAPT_LAYER_H
#define ADAPT_LAYER_H
/*===========================================================================*/
/**
    @file    adapt_layer.h

    @brief   CST adaptation layer interface

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
Fareed Mohammed  19-Jan-2011      engr140621  Initial version
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
#define CAL_SUCCESS                 ( 0) /* Operation completed successfully */
#define CAL_FILE_NOT_FOUND          (-1) /* Error when file does not exist   */
#define CAL_INVALID_SIG_DATA_SIZE   (-2) /* Error when sig data size invalid */
#define CAL_FAILED_FILE_CREATE      (-3) /* Error unable to create file      */
#define CAL_MAC_LEN_INCORRECT       (-4) /* Error MAC len is incorrect       */
#define CAL_INVALID_ARGUMENT        (-5) /* Error argument passed is invalid */
#define CAL_CRYPTO_API_ERROR        (-6) /* Error with openssl API           */
#define CAL_INSUFFICIENT_BUFFER_LEN (-7) /* Buffer length is not sufficient  */
#define CAL_DATA_COMPARE_FAILED     (-8) /* Data comparison operation failed */
#define CAL_LAST_ERROR            (-100) /* Max error codes for adapt layer  */

/*===========================================================================
                                ENUMS
=============================================================================*/
typedef enum _SIG_FMT
{
    PKCS1,              /**< RAW PKCS#1 signature format */ 
    CMS                 /**< CMS (PKCS#7) signature format */
} sig_fmt_t;

typedef enum _SIG_ALG
{
    PKCS1_RSA,          /**< PKCS#1 signature using RSA */
    ECDSA               /**< ECDSA signature */
} sig_alg_t;

/** Hash Digetst Algorithm */
typedef enum hash_alg
{
    SHA_1 = 0,          /**< SHA-1 Digest Algorithm */
    SHA_256,            /**< SHA-256 Digest Algorithm */
    SHA_512,            /**< SHA-512 Digest Algorithm */
    INVALID_DIGEST      /**< Invalid Digest Algorithm */
} hash_alg_t;

/*===========================================================================
                    STRUCTURES AND OTHER TYPEDEFS
=============================================================================*/

typedef struct _AEAD {
    uint8_t *uch;
} AEAD_t;

/*===========================================================================
                     GLOBAL VARIABLE DECLARATIONS
=============================================================================*/

/*===========================================================================
                         FUNCTION PROTOTYPES
=============================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

/** Generate Signature Data
 *
 * Generates a signature for the given data file, signer certificate, 
 * hash algorithm and signature format. The signature data is returned 
 * in a buffer provided by caller. 
 *
 * @param[in] in_file path to file with binary data to sign
 *
 * @param[in] cert_file path to signer certificate file
 *
 * @param[in] hash_alg hash algorithm in #hash_alg_t
 *
 * @param[in] sig_fmt signature format in #sig_fmt_t
 *
 * @param[in] sig_alg signature algorithm in #sig_alg_t
 *
 * @param[out] sig_buf buffer to return signature data
 *
 * @param[in,out] sig_buf_bytes input size of sig_buf allocated by caller
 *                              output size of signature data returned by API
 *
 * @post Errors are printed to STDERR
 *
 * @retval #CAL_SUCCESS API completed its task successfully 
 *
 * @retval #CAL_FILE_NOT_FOUND invalid path in one of the arguments
 *
 * @retval #CAL_INVALID_SIG_DATA_SIZE size insufficient to generate sig data
 * 
 * @retval #CAL_INVALID_ARGUMENT one of the input arguments is invalid
 */
int32_t gen_sig_data(const char* in_file,
                     const char* cert_file, 
                     hash_alg_t hash_alg, 
                     sig_fmt_t sig_fmt,
                     sig_alg_t sig_alg, 
                     uint8_t* sig_buf,
                     size_t *sig_buf_bytes);

/** Generate authenticated encrypted data
 *
 * API generates authenticated encrypted data for given plain-text data file 
 *
 * @param[in] in_file path to file with binary data to encrypt
 *
 * @param[in] pass_phrase_file optional, path to file with pass pharse for 
 *            protection of new AES key.  If null the user will be prompted
 *            for a pass phrase.
 *
 * @param[in] mac_bytes length of MAC in bytes, must be an even value between 
 *            4 and 16
 *
 * @param[in] out_file API will create encrypted data file with this name
 *
 * @param[in] aes_key_file The AES wrapped key file will be saved with this 
 *            name
 *
 * @param[out] aead buffer to return aead structure data
 *
 * @param[in] aead_bytes size of aead buffer in bytes
 *
 * @retval #CAL_SUCCESS API completed its task successfully 
 *
 * @retval #CAL_FILE_NOT_FOUND invalid path in one of the arguments
 *
 * @retval #CAL_FAILED_FILE_CREATE the output file cannot be created
 *
 * @retval #CAL_MAC_LEN_INCORRECT the mac_bytes is not correct
 */
int32_t gen_auth_encrypted_data(const char* in_file,
                                size_t mac_bytes,
                                const char* out_file, 
                                const char* aes_key_file,
                                AEAD_t *aead, 
                                size_t aead_bytes);

#ifdef __cplusplus
}
#endif

#endif /* ADAPT_LAYER_H */
