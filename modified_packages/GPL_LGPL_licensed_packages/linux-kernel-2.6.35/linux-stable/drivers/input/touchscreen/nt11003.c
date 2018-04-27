/* drivers/input/touchscreen/nt11003_touch.c
*
* Copyright (C) 2010 - 2011 Novatek, Inc.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
# include <linux/earlysuspend.h>
#endif
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>

#include <linux/nt11003.h>

#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/cdev.h>

static struct nt11003_ts_data *ts;


// *************************  I2C general  ***************************** //

/**
 * i2c_addr_read_bytes - read data from the I2C slave device
 * @client: I2C device
 * @address: I2C device address
 * @buf[0]: I2C command byte
 * @buf[1]~buf[len-1]: read data buffer
 * @len: operate length
 *
 * Read data from the I2C slave device.
 * This operation consists of two i2c_msgs, the first msg used
 * to write the operate address, the second msg used to read data.
 *
 * Returns the number of i2c_msgs transfered.
 */
static int i2c_addr_read_bytes (struct i2c_client *client, uint8_t address, uint8_t *buf, uint8_t len)
{
    struct i2c_msg msgs[2];
    int ret = -1;
    int retries = 0;

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr  = address;
    msgs[0].len   = 1;
    msgs[0].buf   = &buf[0];

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr  = address;
    msgs[1].len   = len-1;
    msgs[1].buf   = &buf[1];

    while (retries < 5)
    {
        ret = i2c_transfer (client->adapter, msgs, 2);
        if (ret == 2)
            break;
        retries++;
    }

    return ret;
}

/**
 * i2c_addr_write_bytes - write data to the I2C slave device
 * @client: I2C device
 * @address: I2C device address
 * @buf[0]: I2C command byte
 * @buf[1]~buf[len-1]: write data buffer
 * @len: operate length
 *
 * Returns the number of i2c_msgs transfered.
 */
static int i2c_addr_write_bytes (struct i2c_client *client, uint8_t address, uint8_t *buf, uint8_t len)
{
    struct i2c_msg msg;
    int ret = -1;
    int retries = 0;

    msg.flags = !I2C_M_RD;
    msg.addr  = address;
    msg.len   = len;
    msg.buf   = buf;

    while (retries < 5)
    {
        ret = i2c_transfer (client->adapter, &msg, 1);
        if (ret == 1)
            break;
        retries++;
    }

    return ret;
}

static int i2c_write_bytes (struct i2c_client *client, uint8_t *buf, uint8_t len)
{
    return i2c_addr_write_bytes (client, client->addr, buf, len);
}

static int i2c_read_bytes (struct i2c_client *client, uint8_t *buf, uint8_t len)
{
    return i2c_addr_read_bytes (client, client->addr, buf, len);
}


// *************************  Helper functions  ***************************** //

static void get_fw_version (void)
{
    uint8_t rd_buf[16] = {0x78, };
    i2c_addr_read_bytes (ts->client, FW_ADDRESS, rd_buf, 3);
    dev_info (&ts->client->dev, "FW version = %d\n", rd_buf[2]);
    ts->fwVersion = rd_buf[2];

    msleep (10);
}

static void get_date_code (void)
{
    int ret;
    uint32_t i;
    uint8_t rd_buf[17] = {0xA0, };

    ret = i2c_read_bytes (ts->client, rd_buf, 17);
    if (ret != 2)
    {
        dev_err (&ts->client->dev, "Read date code failed! (%d)\n", ret);
        for (i = 0; i < 11; i++)
        {
            ts->dateCode[i] = 0;
        }
        return;
    }

    // display date code read
    dev_info (&ts->client->dev, "Date code read: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", \
              rd_buf[1], rd_buf[2], rd_buf[3], rd_buf[4], rd_buf[5], rd_buf[6], rd_buf[7], rd_buf[8], \
              rd_buf[9], rd_buf[10], rd_buf[11], rd_buf[12], rd_buf[13], rd_buf[14], rd_buf[15], rd_buf[16]);

    // check date code signature
    if ((rd_buf[1] == 'U') &&
            (rd_buf[2] == 'R') &&
            (rd_buf[3] == 'T') &&
            (rd_buf[4] == 'T') &&
            (rd_buf[5] == 'P'))
    {
        for (i = 0; i < 11; i++)
        {
            ts->dateCode[i] = rd_buf[6 + i];
        }
    }
    else
    {
        dev_err (&ts->client->dev, "Date code signature check failed! (%d)\n", ret);
        for (i = 0; i < 11; i++)
        {
            ts->dateCode[i] = 0;
        }
    }

    msleep (10);
}

void hw_reset (void)
{
    // perform hardware reset
    gpio_set_value (TS_RST1, 1);
    msleep (10);
    gpio_set_value (TS_RST1, 0);
    msleep (10);
    gpio_set_value (TS_RST1, 1);
    msleep (100);
}

void sw_reset (void)
{
    // perform soft reset
    uint8_t rd_buf[8] = {0x00, 0x5A, };
    i2c_addr_write_bytes (ts->client, HW_ADDRESS, rd_buf, 2);

    msleep(50);
}

// ****************** Firmware Update Functionality ***************************** //

#if UPDATE_FIRMWARE

int Update_Firmware(char *fw_file_path);
int get_fw_checksum(void);

static struct file* file_open(const char* path, int flags, int rights)
{
    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

static void file_close(struct file* file)
{
    filp_close(file, NULL);
}

static int file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_read(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

// ******************* IOCTL functions *****************************

static int fw_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
{
    fw_query_arg_t q;
    int retVal = 0;
    struct i2c_client *client = ts->client;

    dev_info(&client->dev, "IOCTL called cmd = %d", cmd);

    switch(cmd)
    {
    case IOCTL_GET_FW_VERSION:
    {
        get_fw_version();
        q.fwVersion = ts->fwVersion;
        if (copy_to_user((fw_query_arg_t *)arg, &q, sizeof(fw_query_arg_t)) )
        {
            return -EACCES;
        }
        break;
    }
    case IOCTL_UPDATE_FW:
    {
        if (copy_from_user(&q, (fw_query_arg_t *)arg, sizeof(fw_query_arg_t)) )
        {
            return -EACCES;
        }
        retVal= Update_Firmware((char*)q.fwPath);
        break;
    }
    case IOCTL_GET_CHECKSUM:
    {
        q.fwChecksum = get_fw_checksum();
        if (copy_to_user((fw_query_arg_t *)arg, &q, sizeof(fw_query_arg_t)) )
        {
            return -EACCES;
        }
        break;
    }
    default:
        return -EINVAL;
    }

    if(retVal < 0)
      return -EINVAL;

    return 0;
}

static struct file_operations ioctl_file_ops =
{
    .owner = THIS_MODULE,
    .ioctl = fw_ioctl
};

static struct cdev *driver_object;
static struct class *ioctl_class;
static struct device *ioctl_dev;

static int init_fw_ioctl(void)
{
    if(register_chrdev_region(MKDEV(IOCTL_MAJOR,0), 1, IOCTL_DEVICE_NAME) < 0)
    {
        printk("NT11004 IOCTL: register_chrdev_region() failed");
        return -EIO;
    }

    driver_object = cdev_alloc();
    if( driver_object == NULL )
    {
        goto free_device_number;
    }

    driver_object->owner = THIS_MODULE;
    driver_object->ops = &ioctl_file_ops;

    if( cdev_add(driver_object, MKDEV(IOCTL_MAJOR,0), 1) )
    {
        goto free_cdev;
    }

    /* Create entry in sysfs. */
    ioctl_class = class_create( THIS_MODULE, "NT11004-IOCTL");
    ioctl_dev = device_create( ioctl_class,
                               NULL,
                               MKDEV(IOCTL_MAJOR,0),
                               NULL,
                               "%s",
                               IOCTL_DEVICE_NAME );

    printk("NT11004 IOCTL: successfully initialized");
    return 0;

free_cdev:
    kobject_put( &driver_object->kobj );
free_device_number:
    unregister_chrdev_region(MKDEV(IOCTL_MAJOR,0), 1 );
    printk("NT11004 IOCTL: initialization failed");
    return -EIO;
}

static void exit_fw_ioctl( void )
{
    /* Remove sysfs entry */
    device_destroy( ioctl_class, MKDEV(IOCTL_MAJOR,0));
    class_destroy( ioctl_class );

    /* Deregister the driver */
    cdev_del( driver_object );
    unregister_chrdev_region(MKDEV(IOCTL_MAJOR,0), 1 );

    return;
}

int get_fw_checksum(void) 
{
    uint8_t I2C_Buf[8];
    struct i2c_client *client = ts->client;
    int retry_count, checksum;

    checksum = -1;

    // Step 1: Soft reset and wait 50ms
    sw_reset();

    // Step 2: Change I2C buffer address to 0x0CD7
    //         and write 3 bytes at offset 0 to clear the buffer
    I2C_Buf[0] = 0xFF;
    I2C_Buf[1] = 0x0C;
    I2C_Buf[2] = 0xD7;
    i2c_addr_write_bytes(ts->client, FW_ADDRESS, I2C_Buf, 3);

    I2C_Buf[0] = 0x00;
    I2C_Buf[1] = 0x00;
    I2C_Buf[2] = 0x00;
    I2C_Buf[3] = 0x00;
    i2c_addr_write_bytes(ts->client, FW_ADDRESS, I2C_Buf, 4);

    // Step 3: Send short test command and wait 50ms
    I2C_Buf[0] = 0xFF;
    I2C_Buf[1] = 0x0E;
    I2C_Buf[2] = 0xF9;
    i2c_addr_write_bytes(ts->client, FW_ADDRESS, I2C_Buf, 3);

    I2C_Buf[0] = 0x00;
    I2C_Buf[1] = 0xE1;
    i2c_addr_write_bytes(ts->client, FW_ADDRESS, I2C_Buf, 2);

    msleep(50);

    // Step 4: Wait for NT11004 to report 'Data Ready'
    // (read one byte from 0x0CD7 until 0xAA is read)
    do
    {
        I2C_Buf[0] = 0xFF;
        I2C_Buf[1] = 0x0C;
        I2C_Buf[2] = 0xD7;
        i2c_addr_write_bytes(ts->client, FW_ADDRESS, I2C_Buf, 3);

        I2C_Buf[0] = 0x00;
        i2c_addr_read_bytes(ts->client, FW_ADDRESS, I2C_Buf, 2);

        retry_count++;
        msleep(20);

    } while((retry_count < MAX_I2C_RETRIES) && (I2C_Buf[1] != 0xAA));

    if(I2C_Buf[1] == 0xAA)
    {
        // Step 5: Read result (2 bytes from 0x0CD8)
        I2C_Buf[0] = 0xFF;
        I2C_Buf[1] = 0x0C;
        I2C_Buf[2] = 0xD8;
        i2c_addr_write_bytes(ts->client, FW_ADDRESS, I2C_Buf, 3);

        I2C_Buf[0] = 0x00;
        i2c_addr_read_bytes(ts->client, FW_ADDRESS, I2C_Buf, 3);

        checksum = (I2C_Buf[1] << 8) + I2C_Buf[2];

        dev_info(&client->dev, "FW Update : burned image checksum: %d\n", checksum);
    }

    // Step 6: Soft reset and wait 50ms
    sw_reset();

    return checksum;
}

int Check_CheckSum(uint8_t *image)
{
    int i, retVal = FW_CHECKSUM_FAILED;
    unsigned short current_checksum = 0, update_checksum = 0;

    if(image != NULL)
    {
        // Calculate the check-sum for built in SW image
        for(i = 0; i < FIRMWARE_SIZE; i++)
        {
            update_checksum += (image[i]);
        }
    }

    current_checksum = get_fw_checksum();

    if(current_checksum > 0)
    {
        if(current_checksum == update_checksum)
            retVal = FW_CHECKSUM_SAME;
        else
            retVal = FW_CHECKSUM_DIFFERENT;
    }

    return retVal;
}

int Update_Firmware(char *fw_file_path)
{
    uint8_t I2C_Buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    uint8_t i = 0;
    uint8_t j = 0;
    unsigned int Flash_Address = 0;
    unsigned int Row_Address = 0;
    uint8_t CheckSum[16]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	// 128/8 = 16 times ;
    struct i2c_client *client = ts->client;
    int ret, retry_count, retVal = -1;

    struct file *fw_file = NULL;
    uint8_t *firmware_image = NULL;

    // No update is required -> return
    if(fw_file_path == NULL)
        return -1;

    dev_info(&client->dev, "NT11004 FW Update : Preparing for update");

    // Try to open the firmware file
    fw_file = file_open(fw_file_path, O_RDONLY, 0);
    if(fw_file == NULL)
    {
        dev_info(&client->dev, "NT11004 FW Update : Can't open NT1104 firmware file: %s\n", fw_file_path);
        return -1;
    }

    // Read the firmware file into memory
    firmware_image = kzalloc(FIRMWARE_SIZE, GFP_KERNEL);
    if(!firmware_image)
    {
        dev_info(&client->dev, "NT11004 FW Update : Couldn't allocate memory to load firmware image\n");
        goto cleanup;
    }

    ret = file_read(fw_file, 0 /* offset 0*/, (unsigned char*)firmware_image, FIRMWARE_SIZE);
    if(ret != FIRMWARE_SIZE)
    {
        dev_info(&client->dev, "NT11004 FW Update : Can't read firmare image. vfs_read() returned error code: %d\n", ret);
        goto cleanup;
    }

    // Compare the checksums ob the image from the file against image burned into flash
    ret = Check_CheckSum(firmware_image);
    if(ret == FW_CHECKSUM_SAME)
    {
        dev_info(&client->dev, "NT11004 FW Update : Flash image has the same checksum. Update not required.\n");
        goto cleanup;
    }
    else if(ret == FW_CHECKSUM_FAILED)
    {
        dev_info(&client->dev, "NT11004 FW Update : Updating due to checksum failure\n");
    }
    else if(ret == FW_CHECKSUM_DIFFERENT)
    {
        dev_info(&client->dev, "NT11004 FW Update : Updating since checksum is different\n");
    }

    // Initiate the firmware update
    retry_count = 20;
    do {
        //-------------------------------
        // Step1 --> initial BootLoader
        // Note. HW_Address -> 0x00 -> 0x00 ;
        //-------------------------------
        I2C_Buf[0] = 0x00;
        I2C_Buf[1] = 0xA5;
        i2c_addr_write_bytes(ts->client, HW_ADDRESS, I2C_Buf, 2);	// Write a 0x00 0xA5 NT1100x

        //msleep(2);	// 2ms delay
        msleep(5);

        //Step 1 : Initiate Flash Block
        I2C_Buf[0] = 0x00;
        I2C_Buf[1] = 0x00;
        i2c_addr_write_bytes(ts->client, HW_ADDRESS, I2C_Buf, 2);	// Write a 0x00 0x00 to NT1100x

        //msleep(20);	// 5ms delay
        msleep(40);

        // Read NT1100x status
        I2C_Buf[0] = 0x00;
        i2c_addr_read_bytes(ts->client, HW_ADDRESS, I2C_Buf, 2);

        retry_count--;
    } while (retry_count > 0 && I2C_Buf[1] != 0xAA);

    // if 0xAA is returned, then going next step
    if (I2C_Buf[1] != 0xAA)
    {
        dev_info(&client->dev, "NT11004 FW Update : init get status(0x%2X) error", I2C_Buf[1]);
        goto cleanup;
    }
    dev_info(&client->dev, "NT11004 FW Update : init get status(0x%2X) success", I2C_Buf[1]);

    //---------------------------------------------------------
    // Step 2 : Erase
    //---------------------------------------------------------
    I2C_Buf[0]=0xFF;
    I2C_Buf[1]=0xF0;
    I2C_Buf[2]=0xAC;
    i2c_addr_write_bytes(ts->client, 0x01, I2C_Buf, 3);
    msleep(20);

    I2C_Buf[0]=0x00;
    I2C_Buf[1]=0x21;
    i2c_addr_write_bytes(ts->client, 0x01, I2C_Buf, 2);
    msleep(20);

    I2C_Buf[0]=0x00;
    I2C_Buf[1]=0x99;
    I2C_Buf[2]=0x00;
    I2C_Buf[3]=0x0E;
    I2C_Buf[4]=0x01;
    i2c_addr_write_bytes(ts->client, HW_ADDRESS, I2C_Buf, 5);
    msleep(20);

    I2C_Buf[0]=0x00;
    I2C_Buf[1]=0x81;
    i2c_addr_write_bytes(ts->client, 0x01, I2C_Buf, 2);
    msleep(20);

    I2C_Buf[0]=0x00;
    I2C_Buf[1]=0x99;
    I2C_Buf[2]=0x00;
    I2C_Buf[3]=0x0F;
    I2C_Buf[4]=0x01;
    i2c_addr_write_bytes(ts->client, HW_ADDRESS, I2C_Buf, 5);
    msleep(20);

    I2C_Buf[0]=0x00;
    I2C_Buf[1]=0x01;
    i2c_addr_write_bytes(ts->client, 0x01, I2C_Buf, 2);
    msleep(20);

    I2C_Buf[0]=0xFF;
    I2C_Buf[1]=0x00;
    I2C_Buf[2]=0x00;
    i2c_addr_write_bytes(ts->client, 0x01, I2C_Buf, 3);
    msleep(20);

    for(i=0; i < (FIRMWARE_SIZE / 4096); i++)
    {
        Row_Address=(i*16);
        I2C_Buf[0]=0x00;
        I2C_Buf[1]=0x33; // Flash sector erase
        I2C_Buf[2]=Row_Address;
        i2c_addr_write_bytes(ts->client, HW_ADDRESS, I2C_Buf, 3);
        msleep(80);

        I2C_Buf[0]=0x00;

        while(1)
        {
            i2c_addr_read_bytes(ts->client, HW_ADDRESS, I2C_Buf, 2);
            if(I2C_Buf[1]==0xAA)
                break;
            msleep(1);
        }
    }
    dev_info(&client->dev, "NT11004 FW Update : erase(0x%2X) success", I2C_Buf[1]);

    Flash_Address = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //----------------------------------------
    // Step3. Host write 128 bytes to IC
    //----------------------------------------
    dev_info(&client->dev, "NT11004 FW Update : write begin, please wait ...");

    for (j = 0 ; (j < FIRMWARE_SIZE / 128) ; j++)
    {
        Flash_Address = j * 128 ;

        for (i = 0 ; i < 16 ; i++)
        {
            I2C_Buf[0] = 0x00;
            I2C_Buf[1] = 0x55;	                          //Flash write command
            I2C_Buf[2] = (uint8_t)(Flash_Address  >> 8);  //Flash address [15:8]
            I2C_Buf[3] = (uint8_t)(Flash_Address & 0xFF); //Flash address [7:0]
            I2C_Buf[4] = 0x08;	                          //How many prepare to write to NT1100
            I2C_Buf[6] = firmware_image[Flash_Address + 0];  //Binary data 1
            I2C_Buf[7] = firmware_image[Flash_Address + 1];  //Binary data 2
            I2C_Buf[8] = firmware_image[Flash_Address + 2];  //Binary data 3
            I2C_Buf[9] = firmware_image[Flash_Address + 3];  //Binary data 4
            I2C_Buf[10] = firmware_image[Flash_Address + 4]; //Binary data 5
            I2C_Buf[11] = firmware_image[Flash_Address + 5]; //Binary data 6
            I2C_Buf[12] = firmware_image[Flash_Address + 6]; //Binary data 7
            I2C_Buf[13] = firmware_image[Flash_Address + 7]; //Binary data 8

            CheckSum[i] = ~(I2C_Buf[2] + I2C_Buf[3] + I2C_Buf[4] + I2C_Buf[6] + I2C_Buf[7] +
                            I2C_Buf[8] + I2C_Buf[9] + I2C_Buf[10] + I2C_Buf[11] + I2C_Buf[12] +
                            I2C_Buf[13]) + 1;

            I2C_Buf[5] = CheckSum[i];	                       // Load check sum to I2C Buffer
            i2c_addr_write_bytes(ts->client, HW_ADDRESS, I2C_Buf, 14);	//Host write I2C_Buf[0??2] to NT1100x.

            msleep(1);	// Delay 1 ms

            // Read NT1100x status
            I2C_Buf[0] = 0x00;
            i2c_addr_read_bytes(ts->client, HW_ADDRESS, I2C_Buf, 2);

            // Proceed with the next step if I2C success code is returned
            if (I2C_Buf[1] != 0xAA)
            {
                dev_info(&client->dev, "NT11004 FW Update : write(j=%d, i=%d, 0x%2X) error", j, i, I2C_Buf[1]);
                goto cleanup;
            }
            Flash_Address += 8 ; // Increase Flash Address. 8 bytes for 1 time
        }

        msleep(15);	// Each Row program --> Need 15ms delay time
    }

    //----------------------------------------
    // Step4. Verify
    //----------------------------------------
    printk(KERN_INFO "NT11004 FW Update : Verification in progress, please wait ...");
    ret = Check_CheckSum(firmware_image);

    if(ret == FW_CHECKSUM_SAME)
        dev_info(&client->dev, "NT11004 FW Update : Verification PASSED!");
    else if(ret == FW_CHECKSUM_DIFFERENT)
        dev_info(&client->dev, "NT11004 FW Update : Verification FAILED!");
    else if(ret == FW_CHECKSUM_FAILED)
        dev_info(&client->dev, "NT11004 FW Update : Verification couldn't be performed!");

    retVal = 0;
cleanup:
    dev_info(&client->dev, "NT11004 FW Update : Finished");
    file_close(fw_file);
    kfree(firmware_image);
    
    return retVal;
}

#endif // UPDATE_FIRMWARE

// *************************  Touchscreen general  ***************************** //

static int nt11003_init_panel (void)
{
    int ret;
    uint8_t rd_buf[16] = {0x78, };  // I2C register = 0x78
    // read byte 1 = ~(FW version)
    // read byte 2 = FW version

    // read byte 3 = X screen aspect ratio
    // read byte 4 = Y screen aspect ratio

    // read byte 5 = X resolution max (high)
    // read byte 6 = X resolution max (low)
    // read byte 7 = Y resolution max (high)
    // read byte 8 = Y resolution max (low)

    // read byte 10 = max fingers number
    // read byte 11 = max buttons number
    // read byte 12 = interrupt trigger type
    // read byte 13 = some flags? (green mode)

    // read byte 15 = chip ID
    ret = i2c_read_bytes (ts->client, rd_buf, 16);
    if (ret != 2)
    {
        dev_err (&ts->client->dev, "Reading config failed, using default values\n");
        ts->abs_x_max = TOUCH_MAX_HEIGHT;
        ts->abs_y_max = TOUCH_MAX_WIDTH;
        ts->max_touch_num = MAX_FINGER_NUM;
        ts->max_button_num = MAX_KEY_NUM;
        ts->int_trigger_type = INT_TRIGGER;
        ts->chipID = IC_VERSION;
        return 0;
    }

    dev_info (&ts->client->dev, "Configuration read: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", \
              rd_buf[1], rd_buf[2], rd_buf[3], rd_buf[4], rd_buf[5], rd_buf[6], rd_buf[7], rd_buf[8], \
              rd_buf[9], rd_buf[10], rd_buf[11], rd_buf[12], rd_buf[13], rd_buf[14], rd_buf[15]);

    ts->fwVersion = rd_buf[2];
    ts->x_num = rd_buf[3];
    ts->y_num = rd_buf[4];
    ts->abs_x_max = (rd_buf[5] << 8) + rd_buf[6];
    ts->abs_y_max = (rd_buf[7] << 8) + rd_buf[8];
    ts->max_touch_num = rd_buf[10];
    ts->max_button_num = rd_buf[11];
    ts->int_trigger_type = rd_buf[12];
    ts->chipID = rd_buf[15];

    dev_info (&ts->client->dev, "ts->x_num = %d\n", ts->x_num);
    dev_info (&ts->client->dev, "ts->y_num = %d\n", ts->y_num);
    dev_info (&ts->client->dev, "ts->abs_x_max = %d\n", ts->abs_x_max);
    dev_info (&ts->client->dev, "ts->abs_y_max = %d\n", ts->abs_y_max);

    dev_info (&ts->client->dev, "ts->max_touch_num = %d\n", ts->max_touch_num);
    dev_info (&ts->client->dev, "ts->max_button_num = %d\n", ts->max_button_num);
    dev_info (&ts->client->dev, "ts->int_trigger_type = %d\n", ts->int_trigger_type);

    dev_info (&ts->client->dev, "ts->fwVersion = V%d\n", ts->fwVersion);
    dev_info (&ts->client->dev, "ts->chipID = %d\n", ts->chipID);

    // some sanity checks

    if ((!ts->abs_x_max) || (!ts->abs_y_max) || (!ts->max_touch_num))
    {
        dev_err (&ts->client->dev, "Invalid resolution and/or multi-touch number, using default values\n");
        ts->abs_x_max = TOUCH_MAX_HEIGHT;
        ts->abs_y_max = TOUCH_MAX_WIDTH;
        ts->max_touch_num = MAX_FINGER_NUM;
    }

    if (ts->chipID != IC_VERSION)
    {
        dev_err (&ts->client->dev, "Driver is for chipID = %d, but read out chipID = %d\n", IC_VERSION, ts->chipID);
    }

    // wake-up mode from green mode
    if ((rd_buf[13] & 0x0f) == 0x05)
    {
        dev_info (&ts->client->dev, "Touchscreen works in INT wake-up green mode\n");
        ts->green_wake_mode = 1;
    }
    else
    {
        dev_info (&ts->client->dev, "Touchscreen works in IIC wake-up green mode\n");
        ts->green_wake_mode = 0;
    }

    msleep (10);

    return 0;
}

#ifdef NT11003_MULTI_TOUCH
/*******************************************************
Description:
	Read nt11003 touchscreen version function.

Parameter:
	ts:	i2c client private struct.

return:
	Executive outcomes.0---succeed.
*******************************************************/

/*******************************************************
Description:
	Novatek touchscreen work function.

Parameter:
	ts:	i2c client private struct.

return:
	Executive outcomes.0---succeed.
*******************************************************/
static void nt11003_ts_work_func(struct work_struct *work)
{
    int ret=-1;
    int tmp = 0;
    // Support 10 points maximum
    uint8_t  point_data[61]= {0}; // [(1-READ_COOR_ADDR)+1+2+5*MAX_FINGER_NUM+1]={ 0 };
    //read address(1byte)+key index(1byte)+point mask(2bytes)+5bytes*MAX_FINGER_NUM+coor checksum(1byte)
    uint8_t  check_sum = 0;
    uint16_t  finger_current = 0;
    uint16_t  finger_bit = 0;
    unsigned int  count = 0, point_count = 0;
    unsigned int position = 0;
    uint8_t track_id[MAX_FINGER_NUM] = {0};
    unsigned int input_x = 0;
    unsigned int input_y = 0;
    unsigned int input_w = 0;
    unsigned char index = 0;
    unsigned char touch_num = 0,touch_num_make=0;

    struct nt11003_ts_data *ts = container_of(work, struct nt11003_ts_data, work);
    struct i2c_client *client = ts->client;
    // ????
    //point_data[0] = READ_COOR_ADDR;		//read coor address
    ret=i2c_read_bytes(ts->client, point_data,  sizeof(point_data)/sizeof(point_data[0]));

#if 0
    dev_info(&client->dev, "\n\n\nTouch work func started, point_data dump follows: ============================\n");
    for(index=0,touch_num=0; touch_num < 3; index+=16,touch_num++ ) {
        printk(KERN_ERR "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",point_data[index],point_data[index+1],point_data[index+2],point_data[index+3],point_data[index+4],point_data[index+5],point_data[index+6],point_data[index+7],point_data[index+8],point_data[index+9],point_data[index+10],point_data[index+11],point_data[index+12],point_data[index+13],point_data[index+14],point_data[index+15]);
    }
    printk(KERN_ERR "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",point_data[48],point_data[49],point_data[50],point_data[51],point_data[52],point_data[53],point_data[54],point_data[55],point_data[56],point_data[57],point_data[58],point_data[59],point_data[60]);
    printk(KERN_ERR "\n\n");
#endif


    //***
#ifdef NT11003_MULTI_TOUCH
    touch_num = ts->max_touch_num;
    touch_num_make = ts->max_touch_num;
#else
    touch_num = 1;
    touch_num_make = 1;
#endif

    for(index = 0; index < touch_num; index++)
    {
        position = 1 + 6*index;
        if((point_data[position]&0x3)== 0x03)
            touch_num_make--;
#if 0
        dev_info(&client->dev, "point_data[%d]=0x%02X\n", index, point_data[position]);
        dev_info(&client->dev, "touch_num%d, touch_num_make%d\n", touch_num, touch_num_make);
#endif
    }

    if(touch_num_make > 0)
    {
        for(index=0; index<touch_num; index++)
        {
            position = 1 + 6*index;
            input_x = (unsigned int) (point_data[position+1]<<4) + (unsigned int)( point_data[position+3]>>4);
            input_y = (unsigned int)(point_data[position+2]<<4) + (unsigned int) (point_data[position+3]&0x0f);
            input_w =(unsigned int) (point_data[position+4])+127;
            //input_x = input_x *SCREEN_MAX_HEIGHT/(TOUCH_MAX_HEIGHT);
            //input_y = input_y *SCREEN_MAX_WIDTH/(TOUCH_MAX_WIDTH);
#if 0
            dev_info(&client->dev, "input_x = %d,input_y = %d, input_w = %d\n", input_x, input_y, input_w);
#endif
            if((input_x > ts->abs_x_max)||(input_y > ts->abs_y_max))continue;
#ifdef NT11003_MULTI_TOUCH
#warning "Compiling *MULTI* touch support!\n"
            input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
            input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
            input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
            input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, input_w);
            input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, track_id[index]);
            input_mt_sync(ts->input_dev);
#else
#warning "Compiling *SINGLE* touch support!\n"
            input_report_key(ts->input_dev, BTN_TOUCH, 1);
            input_report_abs(ts->input_dev, ABS_PRESSURE, 1);
            input_report_abs(ts->input_dev, ABS_X, input_x*480/ts->abs_x_max);
            input_report_abs(ts->input_dev, ABS_Y, input_y*272/ts->abs_y_max);
            ////input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
            ////input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, input_w);
            ////input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, track_id[index]);
            ////input_mt_sync(ts->input_dev);
            //break; // only report the first event
#endif
        }
    }
    else
    {
#ifdef NT11003_MULTI_TOUCH
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
        dev_info(&client->dev, "relase event \n");
        input_mt_sync(ts->input_dev);
#else
        //dev_info(&client->dev, "Single Touch relase event \n");
        input_report_key(ts->input_dev, BTN_TOUCH, 0);
        input_report_abs(ts->input_dev, ABS_PRESSURE, 0);
#endif
    }
#define TESNELDA_WORKAROUND
#ifdef HAVE_TOUCH_KEY
    //dev_info(&client->dev, "HAVE KEY DOWN!0x%x\n",point_data[0xe]);
    for(count = 0; count < MAX_KEY_NUM; count++)
    {
#ifdef TESNELDA_WORKAROUND
        static int key_state[4] = { 0, 0, 0, 0 };
        int new_state = !!(point_data[0xe]&(0x01<<count));
        if( new_state != key_state[count] ) {
            input_report_key(ts->input_dev, BTN_TOUCH, new_state);
            input_report_abs(ts->input_dev, ABS_PRESSURE, new_state);
            input_report_abs(ts->input_dev, ABS_X, (480-60)-(count<<7) );
            input_report_abs(ts->input_dev, ABS_Y, 322);
            key_state[count] = new_state;
        }

#else
        input_report_key(ts->input_dev, touch_key_array[count], !!(point_data[0xe]&(0x01<<count)));
#endif
    }
#endif
    input_sync(ts->input_dev);

#if defined(INT_PORT)
    if(ts->int_trigger_type> 1)
    {
        msleep(POLL_TIME);
        //goto COORDINATE_POLL;
    }
#endif
    goto END_WORK_FUNC;

NO_ACTION:

#ifdef HAVE_TOUCH_KEY
    //dev_info(&client->dev, KERN_INFO"HAVE KEY DOWN!0x%x\n",point_data[1]);
    for(count = 0; count < MAX_KEY_NUM; count++)
    {
#ifdef TESNELDA_WORKAROUND
        printk(KERN_ERR "NEVER ENTER THIS SHIT!!!!!");
#else
        input_report_key(ts->input_dev, touch_key_array[count], !!(point_data[0xe]&(0x01<<count)));
#endif
    }
    input_sync(ts->input_dev);
#endif
END_WORK_FUNC:
XFER_ERROR:
    if(ts->use_irq)
        enable_irq(ts->client->irq);

}

#else
# warning "Using minimal single touch work func!\n"
/*******************************************************
Description:
	Read nt11003 touchscreen version function.

Parameter:
	ts:	i2c client private struct.

return:
	Executive outcomes.0---succeed.
*******************************************************/

/*******************************************************
Description:
	Novatek touchscreen work function.

Parameter:
	ts:	i2c client private struct.

return:
	Executive outcomes.0---succeed.
*******************************************************/
static void nt11003_ts_work_func(struct work_struct *work)
{
    int ret=-1;
    uint8_t  point_data[61]= {0};
    unsigned int  count = 0;
    const unsigned int position = 1;
    unsigned int input_x = 0;
    unsigned int input_y = 0;
    int touch_up = 0;
# define TESNELDA_WORKAROUND
# ifdef TESNELDA_WORKAROUND
    int key_down = 0;
# endif

    struct nt11003_ts_data *ts = container_of(work, struct nt11003_ts_data, work);
    struct i2c_client *client = ts->client;

    static int init_int = 1;

    ret=i2c_read_bytes(ts->client, point_data,  sizeof(point_data)/sizeof(point_data[0]));

    if( init_int ) {
        init_int = 0;
        goto bailout;
    }

    if( (point_data[position]&0x3) != 0x03 ) {
        input_x = (unsigned int) (point_data[position+1]<<4) +
                  (unsigned int)( point_data[position+3]>>4);
        input_y = (unsigned int)(point_data[position+2]<<4) +
                  (unsigned int) (point_data[position+3]&0x0f);
        input_report_key(ts->input_dev, BTN_TOUCH, 1);
        input_report_abs(ts->input_dev, ABS_PRESSURE, 1);
        input_report_abs(ts->input_dev, ABS_X, input_x*480/ts->abs_x_max);
        input_report_abs(ts->input_dev, ABS_Y, input_y*272/ts->abs_y_max);
    } else {
        touch_up = 1;
    }


# ifdef HAVE_TOUCH_KEY
    for(count = 0; count < MAX_KEY_NUM; count++)
    {
#  ifdef TESNELDA_WORKAROUND
        static int coord_jitter = 0;
        static int key_state[4] = { 0, 0, 0, 0 };
        static int tsusers = 0;
        int new_state = !!(point_data[0xe]&(0x01<<count));
        if( ts->input_dev->users != tsusers ) {
            int i;
            printk(KERN_INFO "Touch user count changed (is %i, was %i), resetting touch state.\n",ts->input_dev->users,tsusers);
            coord_jitter = 0;
            for( i = 0; i < 4; i++ ) {
                key_state[i] = 0;
            }
            tsusers = ts->input_dev->users;
        }
        if( new_state ) {
            key_down = 1;
        }
        if( new_state != key_state[count] ) {
            if( new_state ) {
                touch_up = 0;
                input_report_key(ts->input_dev, BTN_TOUCH, new_state);
                input_report_abs(ts->input_dev, ABS_PRESSURE, new_state);
                input_report_abs(ts->input_dev, ABS_X, (480-60)-(count<<7)+coord_jitter );
                input_report_abs(ts->input_dev, ABS_Y, 322+coord_jitter);
                coord_jitter = !coord_jitter;
            } else {
                touch_up = 1;
            }
            key_state[count] = new_state;
        }
#  else
input_report_key(ts->input_dev, touch_key_array[count], !!(point_data[0xe]&(0x01<<count)));
#  endif //TESNELDA_WORKAROUND
    }
# ifdef TESNELDA_WORKAROUND
    if( key_down ) {
        touch_up = 0;
    }
# endif

# endif // HAVE_TOUCH_KEY

    if( touch_up ) {
        input_report_key(ts->input_dev, BTN_TOUCH, 0);
        input_report_abs(ts->input_dev, ABS_PRESSURE, 0);
    }

    input_sync(ts->input_dev);

bailout:

# if defined(INT_PORT)
    if( ts->int_trigger_type > 1 ) {
        msleep(POLL_TIME);
    }
# endif

    if(ts->use_irq) {
        enable_irq(ts->client->irq);
    }
}
# undef TESNELDA_WORKAROUND
#endif // NT11003_MULTI_TOUCH


/*******************************************************
Description:
	Timer interrupt service routine.

Parameter:
	timer:	timer struct pointer.

return:
	Timer work mode. HRTIMER_NORESTART---not restart mode
*******************************************************/
static enum hrtimer_restart nt11003_ts_timer_func(struct hrtimer *timer)
{
    struct nt11003_ts_data *ts = container_of(timer, struct nt11003_ts_data, timer);
    queue_work(nt11003_wq, &ts->work);
    hrtimer_start(&ts->timer, ktime_set(0, (POLL_TIME+6)*1000000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
Description:
	External interrupt service routine.

Parameter:
	irq:	interrupt number.
	dev_id: private data pointer.

return:
	irq execute status.
*******************************************************/
static irqreturn_t nt11003_ts_irq_handler(int irq, void *dev_id)
{
    struct nt11003_ts_data *ts = dev_id;
    disable_irq_nosync(ts->client->irq);
    queue_work(nt11003_wq, &ts->work);

    return IRQ_HANDLED;
}

/*******************************************************
Description:
	Novatek touchscreen power manage function.

Parameter:
	on:	power status.0---suspend;1---resume.

return:
	Executive outcomes.-1---i2c transfer error;0---succeed.
*******************************************************/
static int nt11003_ts_power(struct nt11003_ts_data * ts, int on)
{
    int ret = -1;
    unsigned char i2c_control_buf[2] = {255,  0};		//suspend cmd, (I2C buffer = 255, data = 0)
    int retry = 0;
    struct i2c_client *client = ts->client;
    if(on != 0 && on !=1)
    {
        dev_info(&client->dev, KERN_DEBUG "%s: Cant't support this command.", nt11003_ts_name);
        return -EINVAL;
    }

    if(ts != NULL && !ts->use_irq)
        return -2;

    if(on == 0)		//suspend
    {
        if(ts->green_wake_mode)
        {
            disable_irq(ts->client->irq);
            gpio_direction_output(INT_PORT, 1);
            msleep(5);
            //s3c_gpio_cfgpin(INT_PORT, INT_CFG);
            enable_irq(ts->client->irq);
        }
        while(retry<5)
        {
            ret = i2c_write_bytes(ts->client, i2c_control_buf, 2);
            if(ret == 1)
            {
                dev_info(&client->dev, "Send suspend cmd\n");
                break;
            }
            dev_info(&client->dev, "Send cmd failed!\n");
            retry++;
            msleep(10);
        }
        if(ret > 0)
            ret = 0;
    }
    else if(on == 1)		//resume
    {
        dev_info(&client->dev, KERN_INFO"Int resume\n");
        gpio_direction_output(INT_PORT, 0);
        msleep(20);
        if(ts->use_irq)
            gpio_direction_input(INT_PORT);
        //s3c_gpio_cfgpin(INT_PORT, INT_CFG);	//Set IO port as interrupt port
        else
            gpio_direction_input(INT_PORT);

        hw_reset();
        msleep(400);

        ret = 0;
    }
    return ret;
}

/*******************************************************
Description:
	Novatek debug sysfs cat version function.

Parameter:
	standard sysfs show param.

return:
	Executive outcomes. 0---failed.
*******************************************************/

/*******************************************************
Description:
	Novatek debug sysfs cat resolution function.

Parameter:
	standard sysfs show param.

return:
	Executive outcomes. 0---failed.
*******************************************************/
static ssize_t nt11003_debug_resolution_show(struct device *dev,
                struct device_attribute *attr, char *buf)
    {
        struct nt11003_ts_data *ts;
        ts = i2c_get_clientdata(i2c_connect_client_nt11003);
        dev_info(&ts->client->dev,"ABS_X_MAX = %d,ABS_Y_MAX = %d\n",ts->abs_x_max,ts->abs_y_max);
        sprintf(buf,"ABS_X_MAX = %d,ABS_Y_MAX = %d\n",ts->abs_x_max,ts->abs_y_max);

        return strlen(buf);
    }
/*******************************************************
Description:
	Novatek debug sysfs cat version function.

Parameter:
	standard sysfs show param.

return:
	Executive outcomes. 0---failed.
*******************************************************/
static ssize_t nt11003_debug_diffdata_show(struct device *dev,
                struct device_attribute *attr, char *buf)
    {
        //char diff_data[300];
        unsigned char diff_data[2241] = {00,};
        int ret = -1;
        char diff_data_cmd[2] = {80, 202};
        int i;
        int short_tmp;
        struct nt11003_ts_data *ts;

        disable_irq(TS_INT);

        ts = i2c_get_clientdata(i2c_connect_client_nt11003);
        //memset(diff_data, 0, sizeof(diff_data));
        if(ts->green_wake_mode)
        {
            //disable_irq(client->irq);
            gpio_direction_output(INT_PORT, 0);
            msleep(5);
            //zy--s3c_gpio_cfgpin(INT_PORT, INT_CFG);
            //enable_irq(client->irq);
        }
        ret = i2c_write_bytes(ts->client, diff_data_cmd, 2);
        if(ret != 1)
        {
            dev_info(&ts->client->dev, "Write diff data cmd failed!\n");
            enable_irq(TS_INT);
            return 0;
        }

        while(gpio_get_value(INT_PORT));
        ret = i2c_read_bytes(ts->client, diff_data, sizeof(diff_data));
        if(ret != 2)
        {
            dev_info(&ts->client->dev, "Read diff data failed!\n");
            enable_irq(TS_INT);
            return 0;
        }
        for(i=1; i<sizeof(diff_data); i+=2)
        {
            short_tmp = diff_data[i] + (diff_data[i+1]<<8);
            if(short_tmp&0x8000)
                short_tmp -= 65535;
            if(short_tmp == 512)continue;
            sprintf(buf+strlen(buf)," %d",short_tmp);
            //dev_info(&client->dev, " %d\n", short_tmp);
        }

        diff_data_cmd[1] = 0;
        ret = i2c_write_bytes(ts->client, diff_data_cmd, 2);
        if(ret != 1)
        {
            dev_info(&ts->client->dev, "Write diff data cmd failed!\n");
            enable_irq(TS_INT);
            return 0;
        }
        enable_irq(TS_INT);
        /*for (i=0; i<1024; i++)
        {
        	sprintf(buf+strlen(buf)," %d",i);
        }*/

        return strlen(buf);
    }


/*******************************************************
Description:
	Novatek debug sysfs echo calibration function.

Parameter:
	standard sysfs store param.

return:
	Executive outcomes..
*******************************************************/
static ssize_t nt11003_debug_calibration_store(struct device *dev,
                struct device_attribute *attr, const char *buf, ssize_t count)
    {
        int ret = -1;
        char cal_cmd_buf[] = {110,1};
        struct nt11003_ts_data *ts;

        ts = i2c_get_clientdata(i2c_connect_client_nt11003);
        dev_info(&ts->client->dev,"Begin calibration......\n");
        if((*buf == 10)||(*buf == 49))
        {
            if(ts->green_wake_mode)
            {
                // ????
                disable_irq(ts->client->irq);
                gpio_direction_output(INT_PORT, 0);
                msleep(5);
                //zy--s3c_gpio_cfgpin(INT_PORT, INT_CFG);
                enable_irq(ts->client->irq);
            }
            ret = i2c_write_bytes(ts->client,cal_cmd_buf,2);
            if(ret!=1)
            {
                dev_info(&ts->client->dev,"Calibration failed!\n");
                return count;
            }
            else
            {
                dev_info(&ts->client->dev,"Calibration succeed!\n");
            }
        }
        return count;
    }


/*******************************************************
Description:
	Novatek touchscreen probe function.

Parameter:
	client:	i2c device struct.
	id:device id.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int nt11003_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
    {
        int ret = 0;
        int retry=0;
        ////struct nt11003_ts_data *ts;
        char *version_info = NULL;
        //zy---char test_data = 1;
        uint8_t test_data[7] = {0x00,};
        const char irq_table[4] = {IRQ_TYPE_EDGE_RISING,
                                   IRQ_TYPE_EDGE_FALLING,
                                   IRQ_TYPE_LEVEL_LOW,
                                   IRQ_TYPE_LEVEL_HIGH
                                  };

        printk(KERN_INFO "Touchscreen driver probe called.\n");

        ////struct nt11003_i2c_rmi_platform_data *pdata;
        dev_info(&client->dev, "Install touch driver.\n");

        i2c_connect_client_nt11003 = client;

#warning "HW reset compiled in!"        
        gpio_direction_output(TS_RST1, 1);
        hw_reset();//zy+++++++++++debug

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
        {
            dev_info(&client->dev,  "Must have I2C_FUNC_I2C.\n");
            ret = -ENODEV;
            goto err_check_functionality_failed;
        }

        ts = kzalloc(sizeof(*ts), GFP_KERNEL);
        if (ts == NULL) {
            ret = -ENOMEM;
            goto err_alloc_data_failed;
        }

        // Test the I2C communication with touch controller
        for(retry=0; retry<30; retry++)
        {
            disable_irq(client->irq);
            //zy--gpio_direction_output(INT_PORT, 0);
            msleep(5);
            //zy--s3c_gpio_cfgpin(INT_PORT, INT_CFG);
            enable_irq(client->irq);
            ret =i2c_read_bytes(client, test_data, 5);
            dev_info(&client->dev, "test_data[1]=%d,test_data[2]=%d,test_data[3]=%d,test_data[4]=%d,test_data[5]=%d\n",test_data[1],test_data[2],test_data[3],test_data[4],test_data[5]);
            if (ret > 0)
                break;
            dev_info(&client->dev, "nt11003 i2c test failed!\n");
        }
        
        /*
        if(ret <= 0)
        {
            dev_info(&client->dev,  "I2C communication ERROR! nt11003 touchscreen driver become invalid\n");
            goto err_i2c_failed;
        }
        */
        if(ret > 0)
        {
            INIT_WORK(&ts->work, nt11003_ts_work_func);
            ts->client = client;
            i2c_set_clientdata(client, ts);

            ts->input_dev = input_allocate_device();
            if (ts->input_dev == NULL) 
            {
                ret = -ENOMEM;
                dev_info(&client->dev, "Failed to allocate input device\n");
                goto err_input_dev_alloc_failed;
            }

           for(retry=0; retry<3; retry++)
           {
               ret=nt11003_init_panel();
               msleep(2);
               if(ret != 0)
                   continue;
               else
                   break;
           }
        
           if(ret != 0) {
              ts->bad_data=1;
              goto err_init_godix_ts;
           }

           // print date code
           get_date_code();

           ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
           ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
           ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE); 						// absolute coor (x,y)
#ifdef HAVE_TOUCH_KEY
           for(retry = 0; retry < MAX_KEY_NUM; retry++)
           {
              input_set_capability(ts->input_dev,EV_KEY,touch_key_array[retry]);
           }
#endif

           input_set_abs_params(ts->input_dev, ABS_X, 0, 480, 0, 0);
           input_set_abs_params(ts->input_dev, ABS_Y, 0, 272, 0, 0);
           input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);

#ifdef NT11003_MULTI_TOUCH
           input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
           input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
           input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max, 0, 0);

           input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);
           input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, ts->max_touch_num, 0, 0);
#endif

           // ????
           sprintf(ts->phys, "input/event0");
           ts->input_dev->name = nt11003_ts_name;
           ts->input_dev->phys = ts->phys;
           ts->input_dev->id.bustype = BUS_I2C;
           ts->input_dev->id.vendor = 0xDEAD;
           ts->input_dev->id.product = 0xBEEF;
           ts->input_dev->id.version = 10427;	//screen firmware version

           ret = input_register_device(ts->input_dev);
           if (ret) {
               dev_info(&client->dev, "Probe: Unable to register %s input device\n", ts->input_dev->name);
               goto err_input_register_device_failed;
           }
           ts->bad_data = 0;
        }

        get_fw_version();

#ifdef UPDATE_FIRMWARE
        ret = init_fw_ioctl();
#endif

#ifdef INT_PORT
        gpio_direction_input(INT_PORT);
        msleep(10);

        client->irq=TS_INT;
        if (client->irq)
        {
            //zy--ret = gpio_request(INT_PORT, "TS_INT");
            //gpio_direction_input(INT_PORT);//zy++
            if (ret < 0)
            {
                dev_info(&client->dev, "Failed to request GPIO:%d, ERRNO:%d\n",(int)INT_PORT,ret);
                goto err_gpio_request_failed;
            }
            //zy--s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_UP);
            //zy--s3c_gpio_cfgpin(INT_PORT, INT_CFG);
            //dev_info(&client->dev, "ts->int_trigger_type=%d\n",ts->int_trigger_type);
            ret  = request_irq(client->irq, nt11003_ts_irq_handler ,  irq_table[ts->int_trigger_type],
                               client->name, ts);
            if (ret != 0) {
                dev_info(&client->dev, "Cannot allocate ts INT!ERRNO:%d\n", ret);
                gpio_direction_input(INT_PORT);
                gpio_free(INT_PORT);
                goto err_gpio_request_failed;
            }
            else
            {
                // ????
                disable_irq(client->irq);
                ts->use_irq = 1;
                dev_info(&client->dev, "Reques EIRQ %d succesd on GPIO:%d\n",TS_INT,INT_PORT);
            }
            //#warning "HW reset compiled in!"
            //hw_reset();//zy+++++++++++debug
        }    //End of "if (client->irq)"
#endif

err_gpio_request_failed:


        if (!ts->use_irq)
        {
            hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
            ts->timer.function = nt11003_ts_timer_func;
            hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
        }

        if(ts->use_irq)
            enable_irq(client->irq);
#if 0
#if defined(INT_PORT)
        if(ts->use_irq)
            ts->power = nt11003_ts_power;
#endif

//zy--
        ret = nt11003_read_version(ts, &version_info);
        if(ret <= 0)
        {
            dev_info(&client->dev, "Read version data failed!\n");
        }
        else
        {
            dev_info(&client->dev, "Novatek TouchScreen Version:%s\n", (version_info+1));
        }
        vfree(version_info);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
        ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        ts->early_suspend.suspend = nt11003_ts_early_suspend;
        ts->early_suspend.resume = nt11003_ts_late_resume;
        register_early_suspend(&ts->early_suspend);
#endif
#if 0
#ifdef CONFIG_TOUCHSCREEN_NT11003_IAP
        nt11003_proc_entry = create_proc_entry("nt11003-update", 0666, NULL);
        if(nt11003_proc_entry == NULL)
        {
            dev_info(&client->dev, "Couldn't create proc entry!\n");
            ret = -ENOMEM;
            goto err_create_proc_entry;
        }
        else
        {
            dev_info(&client->dev, "Create proc entry success!\n");
            nt11003_proc_entry->write_proc = nt11003_update_write;
            nt11003_proc_entry->read_proc = nt11003_update_read;
            //zy--nt11003_proc_entry->owner =THIS_MODULE;
        }
#endif
        nt11003_debug_sysfs_init();
#endif
        dev_info(&client->dev, "Start %s in %s mode\n",
                 ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");
        ////dev_info(&client->dev,  "Driver Modify Date:2011-06-27\n");
        return 0;

err_init_godix_ts:
        if(ts->use_irq)
        {
            ts->use_irq = 0;
            free_irq(client->irq,ts);
#ifdef INT_PORT
            gpio_direction_input(INT_PORT);
            gpio_free(INT_PORT);
#endif
        }
        else
            hrtimer_cancel(&ts->timer);

err_input_register_device_failed:
        input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
        i2c_set_clientdata(client, NULL);
err_i2c_failed:
        kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
err_create_proc_entry:
        return ret;
    }


/*******************************************************
Description:
	Novatek touchscreen driver release function.

Parameter:
	client:	i2c device struct.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int nt11003_ts_remove(struct i2c_client *client)
{
    struct nt11003_ts_data *ts = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif
#ifdef CONFIG_TOUCHSCREEN_NT11003_IAP
    remove_proc_entry("nt11003-update", NULL);
#endif
    //nt11003_debug_sysfs_deinit();
    if (ts && ts->use_irq)
    {
#ifdef INT_PORT
        gpio_direction_input(INT_PORT);
        gpio_free(INT_PORT);
#endif
        free_irq(client->irq, ts);
    }
    else if(ts)
        hrtimer_cancel(&ts->timer);

    dev_notice(&client->dev,"The driver is removing...\n");
    i2c_set_clientdata(client, NULL);
    input_unregister_device(ts->input_dev);
    kfree(ts);
    return 0;
}

static int nt11003_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret;
    struct nt11003_ts_data *ts = i2c_get_clientdata(client);

    if (ts->use_irq)
        disable_irq(client->irq);
    else
        hrtimer_cancel(&ts->timer);
    //ret = cancel_work_sync(&ts->work);
    //if(ret && ts->use_irq)
    //enable_irq(client->irq);
    if (ts->power) {
        ret = ts->power(ts, 0);
        if (ret < 0)
            dev_info(&client->dev, KERN_ERR "nt11003_ts_resume power off failed\n");
    }
    return 0;
}

static int nt11003_ts_resume(struct i2c_client *client)
{
    int ret;
    struct nt11003_ts_data *ts = i2c_get_clientdata(client);

    if (ts->power) {
        ret = ts->power(ts, 1);
        if (ret < 0)
            dev_info(&client->dev, KERN_ERR "nt11003_ts_resume power on failed\n");
    }

    if (ts->use_irq)
        enable_irq(client->irq);
    else
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void nt11003_ts_early_suspend(struct early_suspend *h)
{
    struct nt11003_ts_data *ts;
    ts = container_of(h, struct nt11003_ts_data, early_suspend);
    nt11003_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void nt11003_ts_late_resume(struct early_suspend *h)
{
    struct nt11003_ts_data *ts;
    ts = container_of(h, struct nt11003_ts_data, early_suspend);
    nt11003_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id nt11003_ts_id[] = {
    { NT11003_I2C_NAME, 0x00 },
    { }
};

static struct i2c_driver nt11003_ts_driver = {
    .probe		= nt11003_ts_probe,
    .remove		= nt11003_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend	= nt11003_ts_suspend,
    .resume		= nt11003_ts_resume,
#endif
    .id_table	= nt11003_ts_id,
    .driver = {
        .name	= NT11003_I2C_NAME,
        .owner = THIS_MODULE,
    },
};

/*******************************************************
Description:
	Driver Install function.
return:
	Executive Outcomes. 0---succeed.
********************************************************/
static int __devinit nt11003_ts_init(void)
{
    int ret;

    printk(KERN_INFO "Touchscreen driver init called.\n");

    nt11003_wq = create_workqueue("nt11003_wq");		//create a work queue and worker thread
    if (!nt11003_wq) {
        printk(KERN_ALERT "create_workqueue() failed\n");
        return -ENOMEM;

    }
    ret = i2c_add_driver(&nt11003_ts_driver);

    return ret;
}

/*******************************************************
Description:
	Driver uninstall function.
return:
	Executive Outcomes. 0---succeed.
********************************************************/

static void __exit nt11003_ts_exit(void)
{
    printk(KERN_ALERT "Touchscreen driver of guitar exited.\n");
    i2c_del_driver(&nt11003_ts_driver);
    if (nt11003_wq)
        destroy_workqueue(nt11003_wq);		//release our work queue

    exit_fw_ioctl();
}

late_initcall(nt11003_ts_init);
//module_init(nt11003_ts_init);
module_exit(nt11003_ts_exit);

MODULE_DESCRIPTION("Novatek Touchscreen Driver");
MODULE_LICENSE("GPL");
