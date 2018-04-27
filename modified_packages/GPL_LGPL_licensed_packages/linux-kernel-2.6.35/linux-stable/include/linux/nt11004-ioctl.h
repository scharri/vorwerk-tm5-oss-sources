/*
 * include/linux/nt11004-ioctl.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef 	_LINUX_NT11004_TOUCH_IOCTL_H
#define		_LINUX_NT11004_TOUCH_IOCTL_H

#include <linux/ioctl.h>

typedef struct _fw_query_arg_t
{  
    unsigned char  fwVersion;
    int            fwChecksum;
    char           fwPath[256];	
} fw_query_arg_t;

#define IOCTL_MAJOR           100
#define IOCTL_MAGIC           0x56

#define IOCTL_GET_FW_VERSION  _IOR(IOCTL_MAGIC,  0x01, fw_query_arg_t)
#define IOCTL_UPDATE_FW       _IOW(IOCTL_MAGIC,  0x02, fw_query_arg_t)
#define IOCTL_GET_CHECKSUM    _IOR(IOCTL_MAGIC,  0x03, fw_query_arg_t)

#define IOCTL_DEVICE_NAME "nt1104-ioctl"

#endif 
