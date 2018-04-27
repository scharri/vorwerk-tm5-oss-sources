#ifndef HAB_SUPER_ROOT_H
#define HAB_SUPER_ROOT_H
/*===========================================================================

     File:  hab_super_root.h

     General Description:  This file contains the definition for the HAB 
                           Super Root public key.  This file is required
                           only for HAB version 3.

=============================================================================*/

/*===========================================================================
                              CONSTANTS
=============================================================================*/

#define TRUE 1
#define HAB_MAX_EXPONENT_BYTES 4

/*===========================================================================
                    STRUCTURES AND OTHER TYPEDEFS
=============================================================================*/

/* Structure containing RSA Super Root Key data required for HAB version 3 */
typedef struct
{
    /* RSA public exponent */
    unsigned char rsa_exponent[HAB_MAX_EXPONENT_BYTES]; 
    unsigned char *rsa_modulus;              /* RSA modulus pointer */
    unsigned short int exponent_bytes;       /* Exponent size in bytes */
    unsigned short int modulus_bytes;        /* Modulus size in bytes */
    unsigned char init_flag;                 /* Indicates if key initialized */
} hab_rsa_public_key;

/*===========================================================================
                     GLOBAL VARIABLE DECLARATIONS
=============================================================================*/
extern const hab_rsa_public_key hab_super_root_key;

#endif /* HAB_SUPER_ROOT_H */
