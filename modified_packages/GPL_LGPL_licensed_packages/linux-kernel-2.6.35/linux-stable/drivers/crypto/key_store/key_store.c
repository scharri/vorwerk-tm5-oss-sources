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

#include <linux/version.h>
#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "key_store.h"

// Include all the keys

#ifdef USE_FCR_CLOUD_KEYS
#include "cloudauth_public_key_fcr.h"
#include "cloudcrypt_aes_cbc_keys_fcr.h"
#include "cloudcrypt_public_key_fcr.h"
#else
#include "cloudauth_public_key.h"
#include "cloudcrypt_public_key.h"
#endif
#include "cloudcrypt_aes_cbc_keys.h"

MODULE_AUTHOR("Michael Neuwert <michael.neuwert@vorwerk.de>");
MODULE_DESCRIPTION("Key Store Driver");
MODULE_LICENSE("GPL");

static DEFINE_MUTEX(ioctl_mutex);

static int ks_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ks_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long ks_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct crypto_key_t key_info;
    __u32 key_length = 0;
    void *key_ptr = NULL;
	unsigned long bytes_not_copied = 0;
    long ret = 0;

	printk(KERN_INFO PFX "ioctl %d called with arg: %lu\n", cmd, arg);
	
	mutex_lock(&ioctl_mutex);

	switch(cmd)
	{
	case KEYSTORE_IOCTL_GET_KEY_SIZE:
	{	
		// Copy the structure from user space to get the key type
		if (copy_from_user(&key_info, (struct crypto_key_t *)arg, sizeof(struct crypto_key_t)) )
        {
			ret = -EACCES;
			goto bailout;
        }
		// Determine the key size and copy the structure back
		key_info.keySize = _get_key_size(key_info.type);
		if (copy_to_user((struct crypto_key_t *)arg, &key_info, sizeof(struct crypto_key_t)) )
        {
            ret = -EACCES;
			goto bailout;
        }
		break;
	}
	case KEYSTORE_IOCTL_GET_KEY_DATA:
	{
		// Copy the structure from user space to get the key type
		if (copy_from_user(&key_info, (struct crypto_key_t *)arg, sizeof(struct crypto_key_t)) )
        {
			ret = -EACCES;
			goto bailout;
        }

		// Get the static key data for the given key type
		key_length = _get_key_size(key_info.type);
		key_ptr = _get_key_ptr(key_info.type);
		
		// Check if the key is valid and if the requested key size is correct
		if((key_ptr != NULL && key_length > 0) && 
			(key_info.keyData != NULL) && (key_info.keySize == key_length) )
		{
			// Copy the key to the userspace
			bytes_not_copied = copy_to_user(key_info.keyData, key_ptr, key_length);
			if (bytes_not_copied != 0)
        	{
				printk(KERN_ERR PFX "Copy of %d bytes of key data failed. Could not copy %lu bytes\n", key_length, bytes_not_copied);
            	ret = -EACCES;
				goto bailout;
        	}
		}
		else
		{
			ret = -EINVAL;
			goto bailout;
		}
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}

bailout:
	mutex_unlock(&ioctl_mutex);
	return ret;
}

__u32 _get_key_size(crypto_key_type_e type)
{
	switch(type)
	{
	case CRYPTO_KEY_TYPE_CLOUDAUTH_RSA_PRIVATE:
		return /*cloudauth_private_key_der_len*/ 0;
	case CRYPTO_KEY_TYPE_CLOUDAUTH_RSA_PUBLIC:
#ifdef USE_FCR_CLOUD_KEYS
		return cloudauth_public_key_fcr_der_len;
#else
		return cloudauth_public_key_der_len;
#endif
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_128:
		return 16;
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_256:
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_256_BLACK_BOX:
		return 32;
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_RSA_PRIVATE:
		return /*cloudcrypt_private_key_der_len*/ 0;
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_RSA_PUBLIC:
#ifdef USE_FCR_CLOUD_KEYS
		return cloudcrypt_public_key_fcr_der_len;
#else
		return cloudcrypt_public_key_der_len;
#endif

	default:
		break;
	}
	return 0;
}
void *_get_key_ptr(crypto_key_type_e type)
{
	switch(type)
	{
/*
	case CRYPTO_KEY_TYPE_CLOUDAUTH_RSA_PRIVATE:
		return cloudauth_private_key_der;
*/
	case CRYPTO_KEY_TYPE_CLOUDAUTH_RSA_PUBLIC:
#ifdef USE_FCR_CLOUD_KEYS
		return cloudauth_public_key_fcr_der;
#else
		return cloudauth_public_key_der;
#endif
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_128:
#ifdef USE_FCR_CLOUD_KEYS
		return key128_fcr;
#else
		return key128;
#endif
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_256:
#ifdef USE_FCR_CLOUD_KEYS
		return key256_fcr;
#else
		return key256;
#endif
        case CRYPTO_KEY_TYPE_CLOUDCRYPT_AES_CBC_256_BLACK_BOX:
                return key256_bb_events;
/*
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_RSA_PRIVATE:
		return cloudcrypt_private_key_der;
*/
	case CRYPTO_KEY_TYPE_CLOUDCRYPT_RSA_PUBLIC:
#ifdef USE_FCR_CLOUD_KEYS
		return cloudcrypt_public_key_fcr_der;
#else
		return cloudcrypt_public_key_der;
#endif
	default:
		break;
	}
	return NULL;
}

static const struct file_operations ks_fops = {
	.owner = THIS_MODULE,
	.open = ks_open,
	.release = ks_release,
	.unlocked_ioctl = ks_ioctl
};

static struct miscdevice keystoredev = {
	.minor = /* MISC_DYNAMIC_MINOR */ 0,
	.name = "ks",
	.fops = &ks_fops,
};

static int  __init ks_init(void)
{
	int rc;

	rc = misc_register(&keystoredev);
	if (unlikely(rc)) {
		printk(KERN_ERR PFX "registration of /dev/ks failed\n");
		return rc;
	}

	printk(KERN_INFO PFX "driver loaded\n");

	return 0;
}

static void __exit ks_exit(void)
{
	misc_deregister(&keystoredev);
	printk(KERN_INFO PFX "driver unloaded.\n");
	return;
}

module_init(ks_init);
module_exit(ks_exit);
