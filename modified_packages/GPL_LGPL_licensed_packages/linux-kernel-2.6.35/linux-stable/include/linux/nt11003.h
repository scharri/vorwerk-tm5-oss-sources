/*
 * include/linux/nt11003_touch.h
 *
 *
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

#ifndef 	_LINUX_NT11003_TOUCH_H
#define		_LINUX_NT11003_TOUCH_H

#ifdef CONFIG_HAS_EARLYSUSPEND
# include <linux/earlysuspend.h>
#endif
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

#include <mach/pinctrl.h>
#include <mach/mx28_pins.h>

#include <linux/ioctl.h>

#include "nt11004-ioctl.h"

// *************************  Touchscreen general  ***************************** //
#define NT11003_I2C_NAME 	"nt1103-ts"

// Enable / disable FW update
#define UPDATE_FIRMWARE       1
#define FIRMWARE_SIZE     16384
#define FW_CHECKSUM_FAILED   -1
#define FW_CHECKSUM_SAME      1
#define FW_CHECKSUM_DIFFERENT 0

// IO pins
#define TS_RST1  	    	MXS_PIN_TO_GPIO(PINID_ENET0_TXD0)
#define TS_INT1  	    	MXS_PIN_TO_GPIO(PINID_ENET0_TXD1)

// interrupt number
#define INT_PORT                TS_INT1
#define TS_INT                  gpio_to_irq(INT_PORT)

// define I2C address of touch controller
#define FW_ADDRESS         0x01
#define HW_ADDRESS         0x7F

// define default resolution of the touchscreen
#define TOUCH_MAX_HEIGHT 	  800
#define TOUCH_MAX_WIDTH		 1280

// set trigger mode
#define INT_TRIGGER                 0

// polling delay
#define POLL_TIME                  10 // actual query spacing interval:POLL_TIME+6

//#define NT11003_MULTI_TOUCH
#ifdef NT11003_MULTI_TOUCH
#define MAX_FINGER_NUM	2	// our NT11004 supports 2 fingers max
#else
#define MAX_FINGER_NUM	1
#endif

#define IC_VERSION		    4 // NT11004
#define BUFFER_LENGTH		16384

struct nt11003_ts_data {
    uint16_t addr;
    uint8_t bad_data;
    struct i2c_client *client;
    struct input_dev *input_dev;
    int use_reset;		//use RESET flag
    int use_irq;		//use EINT flag
    int read_mode;		//read moudle mode,20110221 by andrew
    struct hrtimer timer;
    struct work_struct  work;
    char phys[32];
    int retry;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
    int (*power)(struct nt11003_ts_data * ts, int on);
    uint16_t abs_x_max;
    uint16_t abs_y_max;
    uint8_t x_num;
    uint8_t y_num;
    uint8_t max_touch_num;
    uint8_t max_button_num;
    uint8_t int_trigger_type;
    uint8_t green_wake_mode;
    uint8_t chipID;
    uint8_t dateCode[11];
    uint8_t fwVersion;
};

static const char *nt11003_ts_name = "Nt11003 Capacitive TouchScreen";
static struct workqueue_struct *nt11003_wq;
struct i2c_client * i2c_connect_client_nt11003 = NULL;
static struct proc_dir_entry *nt11003_proc_entry;
static struct kobject *nt11003_debug_kobj;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void nt11003_ts_early_suspend(struct early_suspend *h);
static void nt11003_ts_late_resume(struct early_suspend *h);
#endif


// *************************  Touchkey support  ***************************** //
#define HAVE_TOUCH_KEY
#ifdef HAVE_TOUCH_KEY
#define READ_COOR_ADDR 0x00
const uint16_t touch_key_array[]= {
    KEY_REWIND,				//CALL
    KEY_FASTFORWARD,			//HOME
    KEY_EQUAL,				//HOME
    KEY_HOME				//MENU
};
#define MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#else
#define READ_COOR_ADDR 0x00
#endif


// *************************  Firmware Update  ******************************* //
#define CONFIG_TOUCHSCREEN_NT11003_IAP
#ifdef CONFIG_TOUCHSCREEN_NT11003_IAP
//#define UPDATE_NEW_PROTOCOL

unsigned int oldcrc32 = 0xFFFFFFFF;
unsigned int crc32_table[256];
unsigned int ulPolynomial = 0x04c11db7;
unsigned char rd_cfg_addr;
unsigned char rd_cfg_len;
unsigned char g_enter_isp = 0;

static int nt11003_update_write(struct file *filp, const char __user *buff, unsigned long len, void *data);
static int nt11003_update_read( char *page, char **start, off_t off, int count, int *eof, void *data );

#define PACK_SIZE 				64	//update file package size
#define MAX_TIMEOUT				60000	//update time out conut
#define MAX_I2C_RETRIES				20	//i2c retry times

//I2C buf address
#define ADDR_CMD				80
#define ADDR_STA				81
#ifdef UPDATE_NEW_PROTOCOL
#define ADDR_DAT				0
#else
#define ADDR_DAT				82
#endif

//moudle state
#define NEW_UPDATE_START			0x01
#define UPDATE_START				0x02
#define SLAVE_READY					0x08
#define UNKNOWN_ERROR				0x00
#define FRAME_ERROR					0x10
#define CHECKSUM_ERROR				0x20
#define TRANSLATE_ERROR				0x40
#define FLASH_ERROR					0X80

//error no
#define ERROR_NO_FILE				2	//ENOENT
#define ERROR_FILE_READ				23	//ENFILE
#define ERROR_FILE_TYPE				21	//EISDIR
#define ERROR_GPIO_REQUEST			4	//EINTR
#define ERROR_I2C_TRANSFER			5	//EIO
#define ERROR_NO_RESPONSE			16	//EBUSY
#define ERROR_TIMEOUT				110	//ETIMEDOUT

//update steps
#define STEP_SET_PATH              1
#define STEP_CHECK_FILE            2
#define STEP_WRITE_SYN             3
#define STEP_WAIT_SYN              4
#define STEP_WRITE_LENGTH          5
#define STEP_WAIT_READY            6
#define STEP_WRITE_DATA            7
#define STEP_READ_STATUS           8
#define FUN_CLR_VAL                9
#define FUN_CMD                    10
#define FUN_WRITE_CONFIG           11

//fun cmd
#define CMD_DISABLE_TP             0
#define CMD_ENABLE_TP              1
#define CMD_READ_VER               2
#define CMD_READ_RAW               3
#define CMD_READ_DIF               4
#define CMD_READ_CFG               5
#define CMD_SYS_REBOOT             101

//read mode
#define MODE_RD_VER                1
#define MODE_RD_RAW                2
#define MODE_RD_DIF                3
#define MODE_RD_CFG                4


#endif


// *************************  others  ******************************* //

struct nt11003_i2c_platform_data {
    uint32_t version;	/* Use this entry for panels with */
    //reservation
};

#endif /* _LINUX_NT11003_TOUCH_H */
