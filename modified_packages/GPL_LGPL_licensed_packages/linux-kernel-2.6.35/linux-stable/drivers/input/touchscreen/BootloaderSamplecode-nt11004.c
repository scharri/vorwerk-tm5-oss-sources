#ifndef  __NT1004_BTSC_C__

enum {
	BT_OK,
	BT_ERR,
};
#define  HW_RST      	0
#define  SW_RST			1
#define  FW_DATASIZE      (1024*16)
#define  SECTORSIZE       (128)
#define  FLASHSECTORSIZE  (FW_DATASIZE/128)
#define  FW_CHECKSUM_ADDR     (FW_DATASIZE - 8)



/*******************************************************	
Description:
	Read data from the i2c slave device;
	This operation consisted of 2 i2c_msgs,the first msg used
	to write the operate address,the second msg used to read data.

Parameter:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
return:
	numbers of i2c_msgs to transfer
*********************************************************/
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret=-1;

	msgs[0].flags=!I2C_M_RD;
	msgs[0].addr=client->addr;
	msgs[0].len=1;
	msgs[0].buf=&buf[0];

	msgs[1].flags=I2C_M_RD;
	msgs[1].addr=client->addr;
	msgs[1].len=len-1;
	msgs[1].buf=&buf[1];
	
	ret=i2c_transfer(client->adapter,msgs, 2);
	return ret;
}

/*******************************************************	
Description:
	write data to the i2c slave device.

Parameter:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
return:
	numbers of i2c_msgs to transfer.
*********************************************************/
static int i2c_write_bytes(struct i2c_client *client,uint8_t *data,int len)
{
	struct i2c_msg msg;
	int ret=-1;

	msg.flags=!I2C_M_RD;
	msg.addr=client->addr;
	msg.len=len;
	msg.buf=data;		
	
	ret=i2c_transfer(client->adapter,&msg, 1);
	return ret;
}
/**********************************************************************************/
//
//
//
//
//
/**********************************************************************************/
void nvctp_DelayMs(unsigned long wtime)
{
	msleep(wtime);
}
/**********************************************************************************/
//@brief   novatek capicitance touch panel controller boot loader initial
//
//author   ivers liu
//
//boot loader step 1: initial include mcu reset and enter idle mode.
/**********************************************************************************/
uint8  nvctp_BootloaderInitial(struct i2c_client *client,uint8 uRsttype)
{
	uint8 cbuffer[]={0};
	int ret = BT_OK;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
	
	client->addr = 0x7F,
	if(uRsttype == HW_RST)
	{
		
	}
	else  //(uRsttype == SW_RST)
	{
		cbuffer[0] = 0x00;
		cbuffer[1] = 0xA5;      // reset and idel mode
		i2c_write_bytes(ts->client, cbuffer, 2);
		nvctp_DelayMs(5);    // delay 5ms
		
	}

	/***********************************************/
	// Read status
	i2c_read_bytes(ts->client, cbuffer, 2);

	if(cbuffer[1] != 0xAA)
	{
		ret = BT_ERR;
	}

	return ret;
}
/**********************************************************************************/
//@brief  novatek capicitance touch panel controller flash erease function
//
//author  ivers liu
//
//boot loader step 2: flash erase row mode / all erase command 0x30
/**********************************************************************************/
uint8 nvctp_flashereaserowmode(struct i2c_client *client)
{
	uint8 cbuffer[] = {0};
	int ret = BT_OK;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);

	cbuffer[0] = 0x00;
	cbuffer[1] = 0x30;
	
	i2c_write_bytes(ts->client, cbuffer, 2);
    nvctp_DelayMs(40);
	/*****Read Status******/
	i2c_read_bytes(ts->client, cbuffer, 2);

	if(cbuffer[1] != 0xAA)
	{
		ret = BT_ERR;
	}

	return ret;
}

/**********************************************************************************/
//@brief  novatek capicitance touch panel controller flash erease function
//
//author  ivers liu
//
//boot loader step 2: flash erase sector mode / erase command 0x33
/**********************************************************************************/
uint8 nvctp_flashereasesector(struct i2c_client *client,uint8 csector)
{
	uint8 cbuffer[] = {0};
	int ret = BT_OK;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);

	cbuffer[0] = 0x00;
	cbuffer[1] = 0x33;
	cbuffer[2] = 0x00 + (csector << 4);
	i2c_write_bytes(ts->client, cbuffer, 3);
    nvctp_DelayMs(40);
	/*****Read Status******/
	i2c_read_bytes(ts->client, cbuffer, 2);

	if(cbuffer[1] != 0xAA)
	{
		ret = BT_ERR;
	}

	return ret;
}
/**********************************************************************************/
//@brief  novatek capicitance touch panel controller write flash function
//
//author  ivers liu
//
//boot loader step 3: write data to flash
/**********************************************************************************/
uint8 nvctp_writedatatoflash(struct i2c_client *client, uint8 *fw_BinaryData, uint16 binarydatalen)
{
	int ret = BT_OK;
	int8 i,j;
	int8 cbuffer[14] = {0};
	int8 writechecksum[16]={0x00};
	int8 readchecksum[16] ={0x00};
	int8 sector, sectortotal;
	int16 flash_addr;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);

    sectortotal = (binarydatalen%SECTORSIZE)? (binarydatalen / SECTORSIZE + 1) : (binarydatalen / SECTORSIZE);
	writeflashstart:
					cbuffer[0] = 0x00;
					sector = 0;
					flash_addr = 0;

					for(sector = 0; sector < sectortotal; sector++)
					{
						flash_addr = 128*sector;
						for (i = 0; i < 16; i++)
						{
							cbuffer[1] = 0x55; //write command
							cbuffer[2] = (uint8)(flash_addr >> 8);
                    		cbuffer[3] = (uint8)flash_addr;
							cbuffer[4] = 8;

							cbuffer[6] 	= fw_BinaryData[flash_addr + 0];
							cbuffer[7] 	= fw_BinaryData[flash_addr + 1];
							cbuffer[8] 	= fw_BinaryData[flash_addr + 2];
							cbuffer[9] 	= fw_BinaryData[flash_addr + 3];
							cbuffer[10] = fw_BinaryData[flash_addr + 4];
							cbuffer[11] = fw_BinaryData[flash_addr + 5];
							cbuffer[12] = fw_BinaryData[flash_addr + 6];
							cbuffer[13] = fw_BinaryData[flash_addr + 7];
							 writechecksum[i] = ~(cbuffer[2]+cbuffer[3]+cbuffer[4]+cbuffer[6]+cbuffer[7]+\
		    	             cbuffer[8]+cbuffer[9]+cbuffer[10]+cbuffer[11]+cbuffer[12]+cbuffer[13]) + 1;
							cbuffer[5] = writechecksum[i];

							i2c_write_bytes(ts->client, cbuffer, 14);
							if(i == 15) nvctp_DelayMs(6);
							flash_addr += 8;
						}
						/******Generate checksum***************/
						flash_addr = 128*sector;  // flash address break sector header..
						for(j = 0; j < 16; j++)
						{
							cbuffer[0] = 0x00;
							cbuffer[1] = 0x99;
							cbuffer[2] = (unsigned char)(flash_addr >> 8);
	      	        		cbuffer[3] = (unsigned char)flash_addr & 0xFF;
			        		cbuffer[4] = 8;
							i2c_write_bytes(ts->client, cbuffer,5);
							nvctp_DelayMs(2);
							i2c_write_bytes(ts->client, cbuffer, 14);

							readchecksum[j] = cbuffer[5];

							if(writechecksum[j] != readchecksum[j])
							{
								ret = BT_ERR;
								goto writeflashstart;
							}
							flash_addr += 8;
						}
					}
}
/**********************************************************************************/
//@brief  novatek capicitance touch panel controller boot loader function
//
//author  ivers liu
//
//boot loader step 0: write data to flash
/**********************************************************************************/
uint8 nvctp_Bootloader(struct i2c_client *client,uint8 *nvctp_binaryfile, uint16 binarydatalen)
{
    int ret = BT_OK;
	uint8 i; 
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
		//step 1: boot loader initial
		if(nvctp_BootloaderInitial(ts->client, SW_RST) == BT_OK)
		{
			
		}
		else
		{
			ret = BT_ERR;
			return ret;
		}

		//step 2: erase flash 
		for(i = 0; i < 4; i++)
		{
			if(nvctp_flashereasesector(ts->client, i) == BT_ERR)
			{
				ret = BT_ERR;
				return ret;
			}
		}
		//step 3: write data
		if(nvctp_writedatatoflash(ts->client, nvctp_binaryfile, binarydatalen) == BT_ERR)
		{
			 ret = BT_ERR;
		}
		
		return ret;
}
#endif
