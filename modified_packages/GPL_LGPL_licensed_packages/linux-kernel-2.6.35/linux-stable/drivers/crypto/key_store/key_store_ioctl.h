/******************************************************************************
 *  (c) Vorwerk Elektrowerke GmbH & Co. KG
 *  Rauental 38
 *  42270 Wuppertal
 *  Deutschland
 ******************************************************************************
 *  All contents of this source file are strictly confidential.
 *
 *  Use, reproduction and dissemination of any contents, is not permitted
 *  without express written authority of VORWERK ELEKTROWERKE
 *  (GmbH & Co KG). Usage for other purposes (e.g. development for third
 *  parties) than contractually intends (e.g. inspection for approval) is
 *  strictly prohibited.
 *
 *  All rights, including copyright, rights created by patent grant or
 *  registration, and rights by protection of utility patents, are reserved.
 *  Violations will be prosecuted by civil and criminal law.
 *
 *  Author: Michael Neuwert, Michael.Neuwert@vorwerk.de
 *
 ******************************************************************************/

#ifndef KEY_STORE_IOCTL_H
#define KEY_STORE_IOCTL_H

#include <linux/ioctl.h>

typedef enum {
	CRYPTO_KEY_TYPE_CLOUDAUTH_RSA_PRIVATE,
	CRYPTO_KEY_TYPE_CLOUDAUTH_RSA_PUBLIC,
	CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_128,
	CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_256,
    CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_256_BLACK_BOX,
	CRYPTO_KEY_TYPE_CLOUDCRYPT_RSA_PRIVATE,
	CRYPTO_KEY_TYPE_CLOUDCRYPT_RSA_PUBLIC,
	CRYPTO_KEY_TYPE_INVALID
} crypto_key_type_e;

struct crypto_key_t {
	crypto_key_type_e type;
	void              *keyData;
	int               keySize;
};

#define KEYSTORE_IOCTL_GET_KEY_SIZE _IOWR('c', 0x01, struct crypto_key_t)

#define KEYSTORE_IOCTL_GET_KEY_DATA _IOWR('c', 0x02, struct crypto_key_t)

#endif
