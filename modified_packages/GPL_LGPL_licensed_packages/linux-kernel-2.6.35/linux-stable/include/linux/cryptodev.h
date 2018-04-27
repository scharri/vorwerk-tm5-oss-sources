/*
 * Driver for /dev/crypto device (aka CryptoDev)
 *
 * Copyright (c) 2004 Michal Ludvig <mludvig@logix.net.nz>, SuSE Labs
 *
 * Structures and ioctl command names were taken from
 * OpenBSD to preserve compatibility with their API.
 *
 */

#ifndef _CRYPTODEV_H
#define _CRYPTODEV_H

#ifndef __KERNEL__
#include <inttypes.h>
#endif

#define	CRYPTODEV_MINOR	MISC_DYNAMIC_MINOR


#define CRYPTO_FLAG_HMAC	0x0010
#define CRYPTO_FLAG_MASK	0x00FF

enum {
	CRYPTO_DES_CBC=1,
	CRYPTO_3DES_CBC,
	CRYPTO_BLF_CBC,
	CRYPTO_AES_CBC,
	CRYPTO_CAMELLIA_CBC,
	/* unsupported from here */
	CRYPTO_ARC4,
	CRYPTO_MD5_HMAC,
	CRYPTO_CAST_CBC,
	CRYPTO_SKIPJACK_CBC,
	CRYPTO_SHA1_HMAC=200,
	CRYPTO_RIPEMD160_HMAC,
	CRYPTO_MD5_KPDK,
	CRYPTO_SHA1_KPDK,
	CRYPTO_MD5,
	CRYPTO_SHA1,
	CRYPTO_ALGORITHM_MAX
};

struct crparam;
struct crypt_kop;

/* ioctl parameter to create a session */
struct session_op {
	unsigned int	cipher;		/* e.g. CRYPTO_DES_CBC */
	unsigned int	mac;		/* e.g. CRYPTO_MD5_HMAC */
	size_t		keylen;		/* cipher key */
	char		*key;
	int		mackeylen;	/* mac key */
	char		*mackey;

	/* Return values */
	unsigned int	blocksize;	/* selected algorithm's block size */
	uint32_t	ses;		/* session ID */
};

/* ioctl parameter to request a crypt/decrypt operation against a session  */
struct crypt_op {
	uint32_t	ses;		/* from session_op->ses */
	#define COP_DECRYPT	0
	#define COP_ENCRYPT	1
	uint32_t	op;		/* ie. COP_ENCRYPT */
	uint32_t	flags;		/* unused */

	size_t		len;
	char		*src, *dst;
	char		*mac;
	char		*iv;
};

/* clone original filedescriptor */
#define CRIOGET         _IOWR('c', 101, uint32_t)

/* create crypto session */
#define CIOCGSESSION    _IOWR('c', 102, struct session_op)

/* finish crypto session */
#define CIOCFSESSION    _IOW('c', 103, uint32_t)

/* request encryption/decryptions of a given buffer */
#define CIOCCRYPT       _IOWR('c', 104, struct crypt_op)

/* ioctl()s for asym-crypto. Not yet supported. */
#define CIOCKEY         _IOWR('c', 105, void *)
#define CIOCASYMFEAT    _IOR('c', 106, uint32_t)

#endif /* _CRYPTODEV_H */

/* unused structures */
struct crparam {
        caddr_t         crp_p;
        uint32_t           crp_nbits;
};

#define CRK_MAXPARAM    8

struct crypt_kop {
        uint32_t           crk_op;         /* ie. CRK_MOD_EXP or other */
        uint32_t           crk_status;     /* return status */
        uint16_t         crk_iparams;    /* # of input parameters */ 
        uint16_t         crk_oparams;    /* # of output parameters */
        uint32_t           crk_crid;       /* NB: only used by CIOCKEY2 (rw) */
        struct crparam  crk_param[CRK_MAXPARAM];
};

#define CRK_ALGORITM_MIN        0
#define CRK_MOD_EXP             0
#define CRK_MOD_EXP_CRT         1
#define CRK_DSA_SIGN            2
#define CRK_DSA_VERIFY          3
#define CRK_DH_COMPUTE_KEY      4
#define CRK_ALGORITHM_MAX       4 /* Keep updated - see below */

#define CRF_MOD_EXP             (1 << CRK_MOD_EXP)
#define CRF_MOD_EXP_CRT         (1 << CRK_MOD_EXP_CRT)
#define CRF_DSA_SIGN            (1 << CRK_DSA_SIGN)
#define CRF_DSA_VERIFY          (1 << CRK_DSA_VERIFY)
#define CRF_DH_COMPUTE_KEY      (1 << CRK_DH_COMPUTE_KEY)
