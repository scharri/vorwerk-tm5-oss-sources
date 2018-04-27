/** @file  mlanconfig.h
  *
  * @brief This file contains definitions for application
  * 
  * (C) Copyright 2011 Marvell International Ltd.
  * All Rights Reserved
  *
  * MARVELL CONFIDENTIAL
  * Copyright 2008 ~ 2011 Marvell International Ltd All Rights Reserved.
  * The source code contained or described herein and all documents related to
  * the source code ("Material") are owned by Marvell International Ltd or its
  * suppliers or licensors. Title to the Material remains with Marvell International Ltd
  * or its suppliers and licensors. The Material contains trade secrets and
  * proprietary and confidential information of Marvell or its suppliers and
  * licensors. The Material is protected by worldwide copyright and trade secret
  * laws and treaty provisions. No part of the Material may be used, copied,
  * reproduced, modified, published, uploaded, posted, transmitted, distributed,
  * or disclosed in any way without Marvell's prior express written permission.
  *
  * No license under any patent, copyright, trade secret or other intellectual
  * property right is granted to or conferred upon you by disclosure or delivery
  * of the Materials, either expressly, by implication, inducement, estoppel or
  * otherwise. Any license under such intellectual property rights must be
  * express and approved by Marvell in writing.
  *
  */
/************************************************************************
Change log:
     11/26/2008: initial version
************************************************************************/
#ifndef _MLANCONFIG_H_
#define _MLANCONFIG_H_

/** Include header files */
#include    <stdio.h>
#include    <ctype.h>
#include    <unistd.h>
#include    <string.h>
#include    <stdlib.h>
#include    <sys/socket.h>
#include    <sys/ioctl.h>
#include    <errno.h>
#include    <linux/if.h>
#include    <linux/wireless.h>
#include    <sys/types.h>
#include    <linux/if_ether.h>
#include    <time.h>

#if (BYTE_ORDER == LITTLE_ENDIAN)
#undef BIG_ENDIAN_SUPPORT
#endif

/** Type definition: boolean */
typedef enum
{ FALSE, TRUE } boolean;

/**
 * This macro specifies the attribute pack used for structure packing
 */
#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__  __attribute__((packed))
#endif

/** 16 bits byte swap */
#define swap_byte_16(x) \
((t_u16)((((t_u16)(x) & 0x00ffU) << 8) | \
         (((t_u16)(x) & 0xff00U) >> 8)))

/** 32 bits byte swap */
#define swap_byte_32(x) \
((t_u32)((((t_u32)(x) & 0x000000ffUL) << 24) | \
         (((t_u32)(x) & 0x0000ff00UL) <<  8) | \
         (((t_u32)(x) & 0x00ff0000UL) >>  8) | \
         (((t_u32)(x) & 0xff000000UL) >> 24)))

/** Convert to correct endian format */
#ifdef 	BIG_ENDIAN_SUPPORT
/** CPU to little-endian convert for 16-bit */
#define 	cpu_to_le16(x)	swap_byte_16(x)
/** CPU to little-endian convert for 32-bit */
#define		cpu_to_le32(x)  swap_byte_32(x)
/** Little-endian to CPU convert for 16-bit */
#define 	le16_to_cpu(x)	swap_byte_16(x)
/** Little-endian to CPU convert for 32-bit */
#define		le32_to_cpu(x)  swap_byte_32(x)
#else
/** Do nothing */
#define		cpu_to_le16(x)	(x)
/** Do nothing */
#define		cpu_to_le32(x)  (x)
/** Do nothing */
#define 	le16_to_cpu(x)	(x)
/** Do nothing */
#define 	le32_to_cpu(x)	(x)
#endif

/** Character, 1 byte */
typedef char t_s8;
/** Unsigned character, 1 byte */
typedef unsigned char t_u8;

/** Short integer */
typedef signed short t_s16;
/** Unsigned short integer */
typedef unsigned short t_u16;

/** Long integer */
typedef signed long t_s32;
/** Unsigned long integer */
typedef unsigned long t_u32;

/** Long long integer */
typedef signed long long t_s64;
/** Unsigned long long integer */
typedef unsigned long long t_u64;

/** Void pointer (4-bytes) */
typedef void t_void;

/** Success */
#define MLAN_STATUS_SUCCESS         (0)
/** Failure */
#define MLAN_STATUS_FAILURE         (-1)

t_s8 *mlan_config_get_line(FILE * fp, t_s8 * s, t_s32 size, int *line);
int get_priv_ioctl(char *ioctl_name, int *ioctl_val, int *subioctl_val);
int fparse_for_hex(FILE * fp, t_u8 * dst);

/**
 * Hex or Decimal to Integer
 * @param   num string to convert into decimal or hex
 */
#define A2HEXDECIMAL(num)  \
    (strncasecmp("0x", (num), 2)?(unsigned int) strtoll((num),NULL,0):a2hex((num)))

/** Convert character to integer */
#define CHAR2INT(x) (((x) >= 'A') ? ((x) - 'A' + 10) : ((x) - '0'))

/** Convert TLV header from little endian format to CPU format */
#define endian_convert_tlv_header_in(x)            \
    {                                               \
        (x)->tag = le16_to_cpu((x)->tag);       \
        (x)->length = le16_to_cpu((x)->length); \
    }

/** Convert TLV header to little endian format from CPU format */
#define endian_convert_tlv_header_out(x)            \
    {                                               \
        (x)->tag = cpu_to_le16((x)->tag);       \
        (x)->length = cpu_to_le16((x)->length); \
    }
/** Private command ID to pass custom IE list */
#define CUSTOM_IE_CFG          (SIOCDEVPRIVATE + 13)
/* TLV Definitions */
/** TLV header */
#define TLVHEADER       /** Tag */      \
                        t_u16 tag;      \
                        /** Length */   \
                        t_u16 length

/** Maximum IE buffer length */
#define MAX_IE_BUFFER_LEN 256

/** TLV: Management IE list */
#define MRVL_MGMT_IE_LIST_TLV_ID          (PROPRIETARY_TLV_BASE_ID + 0x69)      // 0x0169

/** custom IE */
typedef struct _custom_ie
{
    /** IE Index */
    t_u16 ie_index;
    /** Mgmt Subtype Mask */
    t_u16 mgmt_subtype_mask;
    /** IE Length */
    t_u16 ie_length;
    /** IE buffer */
    t_u8 ie_buffer[0];
} __ATTRIB_PACK__ custom_ie;

/** TLV buffer : custom IE */
typedef struct _tlvbuf_custom_ie
{
    /** Header */
    TLVHEADER;
    /** custom IE data */
    custom_ie ie_data[0];
} __ATTRIB_PACK__ tlvbuf_custom_ie;

extern t_s32 sockfd;  /**< socket */
extern t_s8 dev_name[IFNAMSIZ + 1];   /**< device name */

#endif /* _MLANCONFIG_H_ */
