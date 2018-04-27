/*
 *  Atmel maXTouch Touchscreen Controller Driver
 *
 *
 *  Copyright (C) 2010 Atmel Corporation
 *  Copyright (C) 2010 Ulf Samuelsson (ulf@atmel.com)
 *  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *
 * Driver for Atmel maXTouch family of touch controllers.
 *
 */

#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>
#include <linux/firmware.h>

#include <linux/atmel_maxtouch.h>


#define DRIVER_VERSION "0.91a"

static int debug = 0;//DEBUG_TRACE;
/* Defaul comms mode = 0 */
static int comms = 0;
module_param(debug, int, 0644);
module_param(comms, int, 0644);

MODULE_PARM_DESC(debug, "Activate debugging output");
MODULE_PARM_DESC(comms, "Select communications mode");

/* Device Info descriptor */
/* Parsed from maXTouch "Id information" inside device */
struct mxt_device_info {
	u8   family_id;
	u8   variant_id;
	u8   major;
	u8   minor;
	u8   build;
	u8   num_objs;
	u8   x_size;
	u8   y_size;
	char family_name[16];	 /* Family name */
	char variant_name[16];    /* Variant name */
	u16  num_nodes;           /* Number of sensor nodes */
};

/* object descriptor table, parsed from maXTouch "object table" */
struct mxt_object {
	u16 chip_addr;
	u8  type;
	u8  size;
	u8  instances;
	u8  num_report_ids;
};


/* Mapping from report id to object type and instance */
struct report_id_map {
	u8  object;
	u8  instance;
/*
 * This is the first report ID belonging to object. It enables us to
 * find out easily the touch number: each touch has different report
 * ID (which are assigned to touches in increasing order). By
 * subtracting the first report ID from current, we get the touch
 * number.
 */
	u8  first_rid;
};


/* Driver datastructure */
struct mxt_data {
	struct i2c_client    *client;
	struct input_dev     *input;
	char                 phys_name[32];
	int                  irq;

	u16                  last_read_addr;
	bool                 new_msgs;
	u8                   *last_message;

	int                  valid_irq_counter;
	int                  invalid_irq_counter;
	int                  irq_counter;
	int                  message_counter;
	int                  read_fail_counter;


	int                  bytes_to_read;

	struct delayed_work  dwork;
	u8                   xpos_format;
	u8                   ypos_format;

	u8                   numtouch;

	struct mxt_device_info	device_info;

	u32		     info_block_crc;
	u32                  configuration_crc;
	u16                  report_id_count;
	struct report_id_map *rid_map;
	struct mxt_object    *object_table;

	u16                  msg_proc_addr;
	u8                   message_size;

	u16                  max_x_val;
	u16                  max_y_val;

	void                 (*init_hw)(void);
	void                 (*exit_hw)(void);
	u8                   (*valid_interrupt)(void);
	u8                   (*read_chg)(void);

	/* debugfs variables */
	struct dentry        *debug_dir;
	int                  current_debug_datap;

	struct mutex         debug_mutex;
	u16                  *debug_data;

	/* Character device variables */
	struct cdev          cdev;
	struct cdev          cdev_messages;  /* 2nd Char dev for messages */
	dev_t                dev_num;
	struct class         *mxt_class;


	u16                  address_pointer;
	bool                 valid_ap;

	/* Message buffer & pointers */
	char                 *messages;
	int                  msg_buffer_startp, msg_buffer_endp;

	/* Put only non-touch messages to buffer if this is set */
	char                 nontouch_msg_only;
	struct mutex         msg_mutex;
};

#define I2C_RETRY_COUNT 5
#define I2C_PAYLOAD_SIZE 254

/* Returns the start address of object in mXT memory. */
#define	MXT_BASE_ADDR(object_type, mxt)					\
	get_object_address(object_type, 0, mxt->object_table,           \
			   mxt->device_info.num_objs)

/* Maps a report ID to an object type (object type number). */
#define	REPORT_ID_TO_OBJECT(rid, mxt)			\
	(((rid) == 0xff) ? 0 : mxt->rid_map[rid].object)

/* Maps a report ID to an object type (string). */
#define	REPORT_ID_TO_OBJECT_NAME(rid, mxt)			\
	object_type_name[REPORT_ID_TO_OBJECT(rid, mxt)]

/* Returns non-zero if given object is a touch object */
#define IS_TOUCH_OBJECT(object) \
	((object == MXT_TOUCH_MULTITOUCHSCREEN_T9) || \
	 (object == MXT_TOUCH_KEYARRAY_T15) ||	\
	 (object == MXT_TOUCH_PROXIMITY_T23) || \
	 (object == MXT_TOUCH_SINGLETOUCHSCREEN_T10) || \
	 (object == MXT_TOUCH_XSLIDER_T11) || \
	 (object == MXT_TOUCH_YSLIDER_T12) || \
	 (object == MXT_TOUCH_XWHEEL_T13) || \
	 (object == MXT_TOUCH_YWHEEL_T14) || \
	 (object == MXT_TOUCH_KEYSET_T31) || \
	 (object == MXT_TOUCH_XSLIDERSET_T32) ? 1 : 0)

#define mxt_debug(level, ...) \
	do { \
		if (debug >= (level)) \
			pr_debug(__VA_ARGS__); \
	} while (0)

struct RepeatData
{
    struct timer_list timer;
    int intervalMs;
	int stored_x;
	int stored_y;
	int stored_size;
    struct input_dev* input;
    int inUse;
};

static void repeatTimer_cb(unsigned long data);

static struct RepeatData gs_repeatData =
    {
        TIMER_INITIALIZER(repeatTimer_cb, 0, 0),
        100,
        0,
        0,
        0,
        0,
        0
    };


static void timerSetup( struct timer_list* timer, int msecs )
{
	if( msecs_to_jiffies( msecs ) == MAX_JIFFY_OFFSET ) 
    {
		printk( KERN_ERR "atmel_maxtouch: timerSetup: timeout = %d too large. Reset to 100msec\n",msecs );
		msecs = 100;
	}

	timer->expires = msecs_to_jiffies( msecs ) + 1 + jiffies;
	add_timer( timer );
}


static void timerRemove( struct timer_list* timer )
{
    ( void ) del_timer_sync( timer );
}


static void repeatTimer_cb(unsigned long data)
{
	static int old_x=0, old_y=0;

	input_report_abs(gs_repeatData.input, ABS_PRESSURE, 1);

	if (gs_repeatData.stored_x != old_x) 
    {
		input_report_abs(gs_repeatData.input, ABS_X, gs_repeatData.stored_x);
		old_x = gs_repeatData.stored_x;
	} 
    else 
    {
		old_x = old_x + 3;
		input_report_abs(gs_repeatData.input, ABS_X, old_x);
	}

	if (gs_repeatData.stored_y != old_y) 
    {
		input_report_abs(gs_repeatData.input, ABS_Y, gs_repeatData.stored_y);
		old_y = gs_repeatData.stored_y;
	} 
    else 
    {
		old_y = old_y + 3;
		input_report_abs(gs_repeatData.input, ABS_Y, old_y);
	}

	input_sync(gs_repeatData.input);

    //retrigger self
    if( gs_repeatData.inUse )
    {
        timerSetup( &( gs_repeatData.timer ), gs_repeatData.intervalMs );
    }
}


static void repeatStart( struct input_dev* input )
{
    if( ! gs_repeatData.inUse )
    {
        gs_repeatData.inUse = 1;
        gs_repeatData.input = input;
        timerSetup( &( gs_repeatData.timer ), gs_repeatData.intervalMs );
    }
    else
    {
        //printk( KERN_ERR "atmel_maxtouch: repeatStart: inUse == 1 !!!\n" );
    }
}


static void repeatStop( void )
{
    if( gs_repeatData.inUse )
    {
        gs_repeatData.inUse = 0;
        timerRemove( &( gs_repeatData.timer ) );
    }
}




/*
 * Check whether we have multi-touch enabled kernel; if not, report just the
 * first touch (on mXT224, the maximum is 10 simultaneous touches).
 * Because just the 1st one is reported, it might seem that the screen is not
 * responding to touch if the first touch is removed while the screen is being
 * touched by another finger, so beware.
 *
 */

#ifdef ABS_MT_TRACKING_ID
static inline void report_mt(int touch_number, int size, int x, int y,
			     struct mxt_data *mxt) {
	input_report_abs(mxt->input, ABS_MT_TRACKING_ID, touch_number);
	input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR, size);
	input_report_abs(mxt->input, ABS_MT_POSITION_X, x);
	input_report_abs(mxt->input, ABS_MT_POSITION_Y, y);
	input_mt_sync(mxt->input);
}
#else
static inline void report_mt(int touch_number, int size, int x, int y,
			     struct mxt_data *mxt) {
	if (touch_number == 0) {
		input_report_abs(mxt->input, ABS_TOOL_WIDTH, size);
		input_report_abs(mxt->input, ABS_X, x);
		input_report_abs(mxt->input, ABS_Y, y);
	}
}
#endif


static inline void report_gesture(int data, struct mxt_data *mxt)
{
	input_event(mxt->input, EV_MSC, MSC_GESTURE, data);
}


static const u8	*object_type_name[] = {
	[0]  = "Reserved",
	[5]  = "GEN_MESSAGEPROCESSOR_T5",
	[6]  = "GEN_COMMANDPROCESSOR_T6",
	[7]  = "GEN_POWERCONFIG_T7",
	[8]  = "GEN_ACQUIRECONFIG_T8",
	[9]  = "TOUCH_MULTITOUCHSCREEN_T9",
	[15] = "TOUCH_KEYARRAY_T15",
	[18] = "SPT_COMMSCONFIG_T18",
	[19] = "SPT_GPIOPWM_T19",
	[20] = "PROCI_GRIPFACESUPPRESSION_T20",
	[22] = "PROCG_NOISESUPPRESSION_T22",
	[23] = "TOUCH_PROXIMITY_T23",
	[24] = "PROCI_ONETOUCHGESTUREPROCESSOR_T24",
	[25] = "SPT_SELFTEST_T25",
	[27] = "PROCI_TWOTOUCHGESTUREPROCESSOR_T27",
	[28] = "SPT_CTECONFIG_T28",
	[37] = "DEBUG_DIAGNOSTICS_T37",
	[38] = "SPT_USER_DATA_T38",
	[40] = "PROCI_GRIPSUPPRESSION_T40",
	[41] = "PROCI_PALMSUPPRESSION_T41",
	[42] = "PROCI_FACESUPPRESSION_T42",
	[43] = "SPT_DIGITIZER_T43",
	[44] = "SPT_MESSAGECOUNT_T44",
	[46] = "MXT_SPT_CTECONFIG_T46",
	[47] = "MXT_PROCI_STYLUS_T47",
	[48] = "MXT_PROCG_NOISESUPPRESSION_T48",
	[55] = "MXT_PROCI_ADAPTIVETHRESHOLD_T55",
};


static u16 get_object_address(uint8_t object_type,
			      uint8_t instance,
			      struct mxt_object *object_table,
				  int max_objs);

static int mxt_write_ap(struct mxt_data *mxt, u16 ap);

static int mxt_read_block_wo_addr(struct i2c_client *client,
				  u16 length,
				  u8 *value);

ssize_t debug_data_read(struct mxt_data *mxt, char *buf, size_t count,
			loff_t *ppos, u8 debug_command)
{
	int i;
	u16 *data;
	u16 diagnostics_reg;
	int offset = 0;
	int size;
	int read_size;
	int error;
	char *buf_start;
	u16 debug_data_addr;
	u16 page_address;
	u8 page;
	u8 debug_command_reg;

	data = mxt->debug_data;
	if (data == NULL)
		return -EIO;

	/* If first read after open, read all data to buffer. */
	if (mxt->current_debug_datap == 0) {
		diagnostics_reg =
			MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) +
			MXT_ADR_T6_DIAGNOSTIC;
		if (count > (mxt->device_info.num_nodes * 2))
			count = mxt->device_info.num_nodes;
		debug_data_addr =
			MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTIC_T37, mxt) +
			MXT_ADR_T37_DATA;
		page_address = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTIC_T37, mxt) +
			       MXT_ADR_T37_PAGE;
		error = mxt_read_block(mxt->client, page_address, 1, &page);
		if (error < 0)
			return error;
		mxt_debug(DEBUG_TRACE, "debug data page = %d\n", page);
		while (page != 0) {
			error = mxt_write_byte(mxt->client,
					diagnostics_reg,
					MXT_CMD_T6_PAGE_DOWN);
			if (error < 0)
				return error;
			/* Wait for command to be handled; when it has, the
			   register will be cleared. */
			debug_command_reg = 1;
			while (debug_command_reg != 0) {
				error = mxt_read_block(mxt->client,
						diagnostics_reg, 1,
						&debug_command_reg);
				if (error < 0)
					return error;
				mxt_debug(DEBUG_TRACE,
					"Waiting for debug diag command "
					"to propagate...\n");

			}
			error = mxt_read_block(mxt->client, page_address, 1,
					       &page);
			if (error < 0)
				return error;
			mxt_debug(DEBUG_TRACE, "debug data page = %d\n", page);
		}

		/*
		 * Lock mutex to prevent writing some unwanted data to debug
		 * command register. User can still write through the char
		 * device interface though. TODO: fix?
		 */

		mutex_lock(&mxt->debug_mutex);
		/* Configure Debug Diagnostics object to show deltas/refs */
		error = mxt_write_byte(mxt->client, diagnostics_reg,
				debug_command);

		/* Wait for command to be handled; when it has, the
		 * register will be cleared. */
		debug_command_reg = 1;
		while (debug_command_reg != 0) {
			error = mxt_read_block(mxt->client,
					diagnostics_reg, 1,
					&debug_command_reg);
			if (error < 0)
				return error;
			mxt_debug(DEBUG_TRACE, "Waiting for debug diag command "
				"to propagate...\n");
		}

		if (error < 0) {
			printk(KERN_WARNING
				"Error writing to maXTouch device!\n");
			return error;
		}

		size = mxt->device_info.num_nodes * sizeof(u16);

		while (size > 0) {
			read_size = size > 128 ? 128 : size;
			mxt_debug(DEBUG_TRACE,
				"Debug data read loop, reading %d bytes...\n",
				read_size);
			error = mxt_read_block(mxt->client,
					       debug_data_addr,
					       read_size,
					       (u8 *) &data[offset]);
			if (error < 0) {
				printk(KERN_WARNING
					"Error reading debug data\n");
				goto error;
			}
			offset += read_size/2;
			size -= read_size;

			/* Select next page */
			error = mxt_write_byte(mxt->client, diagnostics_reg,
					MXT_CMD_T6_PAGE_UP);
			if (error < 0) {
				printk(KERN_WARNING
					"Error writing to maXTouch device!\n");
				goto error;
			}
		}
		mutex_unlock(&mxt->debug_mutex);
	}

	buf_start = buf;
	i = mxt->current_debug_datap;

	while (((buf - buf_start) < (count - 6)) &&
		(i < mxt->device_info.num_nodes)) {

		mxt->current_debug_datap++;
		if (debug_command == MXT_CMD_T6_REFERENCES_MODE)
			buf += sprintf(buf, "%d: %5d\n", i,
				       (u16) le16_to_cpu(data[i]));
		else if (debug_command == MXT_CMD_T6_DELTAS_MODE)
			buf += sprintf(buf, "%d: %5d\n", i,
				       (s16) le16_to_cpu(data[i]));
		i++;
	}

	return buf - buf_start;
error:
	mutex_unlock(&mxt->debug_mutex);
	return error;
}

ssize_t deltas_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return debug_data_read(file->private_data, buf, count, ppos,
			       MXT_CMD_T6_DELTAS_MODE);
}

ssize_t refs_read(struct file *file, char *buf, size_t count,
			loff_t *ppos)
{
	return debug_data_read(file->private_data, buf, count, ppos,
			       MXT_CMD_T6_REFERENCES_MODE);
}

int debug_data_open(struct inode *inode, struct file *file)
{
	struct mxt_data *mxt;
	int i;
	mxt = inode->i_private;
	if (mxt == NULL)
		return -EIO;
	mxt->current_debug_datap = 0;
	mxt->debug_data = kmalloc(mxt->device_info.num_nodes * sizeof(u16),
				  GFP_KERNEL);
	if (mxt->debug_data == NULL)
		return -ENOMEM;
	for (i = 0; i < mxt->device_info.num_nodes; i++)
		mxt->debug_data[i] = 7777;
	file->private_data = mxt;
	return 0;
}

int debug_data_release(struct inode *inode, struct file *file)
{
	struct mxt_data *mxt;
	mxt = file->private_data;
	kfree(mxt->debug_data);
	return 0;
}

static const struct file_operations delta_fops = {
	.owner = THIS_MODULE,
	.open = debug_data_open,
	.release = debug_data_release,
	.read = deltas_read,
};

static const struct file_operations refs_fops = {
	.owner = THIS_MODULE,
	.open = debug_data_open,
	.release = debug_data_release,
	.read = refs_read,
};


int mxt_memory_open(struct inode *inode, struct file *file)
{
	struct mxt_data *mxt;
	mxt = container_of(inode->i_cdev, struct mxt_data, cdev);
	if (mxt == NULL)
		return -EIO;
	file->private_data = mxt;
	return 0;
}

int mxt_message_open(struct inode *inode, struct file *file)
{
	struct mxt_data *mxt;
	mxt = container_of(inode->i_cdev, struct mxt_data, cdev_messages);
	if (mxt == NULL)
		return -EIO;
	file->private_data = mxt;
	return 0;
}

ssize_t mxt_memory_read(struct file *file, char *buf, size_t count,
			loff_t *ppos)
{
	int i;
	struct mxt_data *mxt;

	mxt = file->private_data;
	if (mxt->valid_ap) {
		mxt_debug(DEBUG_TRACE, "Reading %d bytes from current ap\n",
			(int) count);
		i = mxt_read_block_wo_addr(mxt->client, count, (u8 *) buf);
	} else {
		mxt_debug(DEBUG_TRACE, "Address pointer changed since set;"
			"writing AP (%d) before reading %d bytes",
			mxt->address_pointer, (int) count);
		i = mxt_read_block(mxt->client, mxt->address_pointer, count,
				buf);
	}

	return i;
}

ssize_t mxt_memory_write(struct file *file, const char *buf, size_t count,
			 loff_t *ppos)
{
	int i;
	int whole_blocks;
	int last_block_size;
	struct mxt_data *mxt;
	u16 address;

	mxt = file->private_data;
	address = mxt->address_pointer;

	mxt_debug(DEBUG_TRACE, "mxt_memory_write entered\n");
	whole_blocks = count / I2C_PAYLOAD_SIZE;
	last_block_size = count % I2C_PAYLOAD_SIZE;

	for (i = 0; i < whole_blocks; i++) {
		mxt_debug(DEBUG_TRACE, "About to write to %d...",
			address);
		mxt_write_block(mxt->client, address, I2C_PAYLOAD_SIZE,
				(u8 *) buf);
		address += I2C_PAYLOAD_SIZE;
		buf += I2C_PAYLOAD_SIZE;
	}

	mxt_write_block(mxt->client, address, last_block_size, (u8 *) buf);

	return count;
}

static int program_object_data(struct i2c_client *client, struct mxt_data *mxt)
{
	const struct firmware *fw;
	int ret = 0;
	const u8 *object_data;
	u8 object_number, object_offset, count, i;
	size_t size,data_read=0;
	
	ret = request_firmware(&fw, "maxTouch_objects.bin", &client->dev);
	if (ret) {
		dev_info(&client->dev,"No object data found using defaults!\n");
		return 0;
	}
	
	object_data = fw->data;
	size = fw->size;
	if (object_data[data_read] != mxt->device_info.family_id) {
		dev_info(&client->dev,"Object data not valid for this product!\n");
		return 0;
	}
	data_read++;
	while (data_read != size) {
		object_number = object_data[data_read];
		data_read++;
		object_offset = object_data[data_read];
		data_read++;
		count = object_data[data_read];
		data_read++;
		printk(KERN_INFO "Writing to object %d at offset %d %d bytes: ",object_number,object_offset,count);
		for (i=0;i<count;i++) {
			printk(KERN_INFO "0x%x ",object_data[data_read]);
			ret = mxt_write_byte(mxt->client,
			      MXT_BASE_ADDR(object_number, mxt) +
			      object_offset+i,
			      object_data[data_read]);
			data_read++;
		}
		printk(KERN_INFO "\n");
		
	}
	release_firmware(fw);

	return 0;
}

static int mxt_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	int retval =0,i=0,j=0;
	u8 value=0;
	struct mxt_data *mxt;
	mxt = file->private_data;

	switch (cmd) {
	case MXT_SET_ADDRESS_IOCTL:
		retval = mxt_write_ap(mxt, (u16) arg);
		if (retval >= 0) {
			mxt->address_pointer = (u16) arg;
			mxt->valid_ap = 1;
		}
		break;
	case MXT_RESET_IOCTL:
		retval = mxt_write_byte(mxt->client,
			      MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) +
			      MXT_ADR_T6_RESET,
			      1);
		break;
	case MXT_CALIBRATE_IOCTL:
		retval = mxt_write_byte(mxt->client,
			      MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) +
			      MXT_ADR_T6_CALIBRATE,
			      1);

		break;
	case MXT_BACKUP_IOCTL:
		retval = mxt_write_byte(mxt->client,
			      MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) +
			      MXT_ADR_T6_BACKUPNV,
			      MXT_CMD_T6_BACKUP);
		break;
	case MXT_NONTOUCH_MSG_IOCTL:
		mxt->nontouch_msg_only = 1;
		break;
	case MXT_ALL_MSG_IOCTL:
		mxt->nontouch_msg_only = 0;
		break;
	case MXT_SET_THRESHOLD_IOCTL:
		retval = mxt_write_byte(mxt->client,
			      MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt) +
			      MXT_MSG_T9_TCHVECTOR,
			      (u8) arg);
		break;
	case MXT_SET_REPEAT:
				gs_repeatData.intervalMs = (int) arg;
		break;
	case MXT_LOAD_SETTINGS:
		program_object_data(mxt->client,mxt);
		break;
	case MXT_PRINT_OBJECTS:
		for (i = 0;i<mxt->device_info.num_objs;i++) {
			printk(KERN_INFO "Object %s is from type %d and has the following register values:!\n",object_type_name[mxt->object_table[i].type],mxt->object_table[i].type);
			for (j=0;j<mxt->object_table[i].size;j++){
				mxt_read_block(mxt->client,MXT_BASE_ADDR(mxt->object_table[i].type, mxt) + j,1,&value);
				printk(KERN_INFO "Register 0x%X = 0x%X\n",j,value);
			}
		}
		break;
	case MXT_SET_CONFBYTE_IOCTL: {
	  u8  obj = (arg >> 24) & 0xff;
	  u8  dat = (arg >> 16) & 0xff;
	  u16 ofs = arg & 0xffff;
	  for (i = 0;i<mxt->device_info.num_objs;i++) {
	    if( mxt->object_table[i].type == obj ) {
	      if( mxt->object_table[i].size > ofs ) {
		printk(KERN_INFO "Setting config byte at offset 0x%04X of object %s to 0x%02X!\n",ofs,object_type_name[mxt->object_table[i].type],dat);
		return mxt_write_byte(mxt->client,MXT_BASE_ADDR(obj, mxt) + ofs, (u8) dat);
	      } else {
		printk(KERN_WARNING "Object %s is of size 0x%04X; unable to write offset 0x%04X!\n",object_type_name[mxt->object_table[i].type],mxt->object_table[i].size,ofs);
		return -EIO;
	      }
	    }
	  }
	  printk(KERN_WARNING "Config object %s not found!\n",object_type_name[mxt->object_table[i].type]);
	  return -EIO;
	  break;
	}
	default:
		return -EIO;
	}

	return retval;
}

/*
 * Copies messages from buffer to user space.
 *
 * NOTE: if less than (mxt->message_size * 5 + 1) bytes requested,
 * this will return 0!
 *
 */
ssize_t mxt_message_read(struct file *file, char *buf, size_t count,
			 loff_t *ppos)
{
	int i;
	struct mxt_data *mxt;
	char *buf_start;

	mxt = file->private_data;
	if (mxt == NULL)
		return -EIO;
	buf_start = buf;

	mutex_lock(&mxt->msg_mutex);
	/* Copy messages until buffer empty, or 'count' bytes written */
	while ((mxt->msg_buffer_startp != mxt->msg_buffer_endp) &&
		((buf - buf_start) < (count - (5 * mxt->message_size) - 1))) {

		for (i = 0; i < mxt->message_size; i++) {
			buf += sprintf(buf, "[%2X] ",
				*(mxt->messages + mxt->msg_buffer_endp *
					mxt->message_size + i));
		}
		buf += sprintf(buf, "\n");
		if (mxt->msg_buffer_endp < MXT_MESSAGE_BUFFER_SIZE)
			mxt->msg_buffer_endp++;
		else
			mxt->msg_buffer_endp = 0;
	}
	mutex_unlock(&mxt->msg_mutex);
	return buf - buf_start;
}

static const struct file_operations mxt_message_fops = {
	.owner = THIS_MODULE,
	.open = mxt_message_open,
	.read = mxt_message_read,
};

static const struct file_operations mxt_memory_fops = {
	.owner = THIS_MODULE,
	.open = mxt_memory_open,
	.read = mxt_memory_read,
	.write = mxt_memory_write,
	.ioctl = mxt_ioctl,
};


/* Writes the address pointer (to set up following reads). */

int mxt_write_ap(struct mxt_data *mxt, u16 ap)
{
	struct i2c_client *client;
	__le16	le_ap = cpu_to_le16(ap);
	client = mxt->client;
	if (mxt != NULL)
		mxt->last_read_addr = -1;
	if (i2c_master_send(client, (u8 *) &le_ap, 2) == 2) {
		mxt_debug(DEBUG_TRACE, "Address pointer set to %d\n", ap);
		return 0;
	} else {
		mxt_debug(DEBUG_INFO, "Error writing address pointer!\n");
		return -EIO;
	}
}



/* Calculates the 24-bit CRC sum. */
static u32 CRC_24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 result;
	u32 data_word;

	data_word = ((((u16) byte2) << 8u) | byte1);
	result = ((crc << 1u) ^ data_word);
	if (result & 0x1000000)
		result ^= crcpoly;
	return result;
}

/* Returns object address in mXT chip, or zero if object is not found */
static u16 get_object_address(uint8_t object_type,
			      uint8_t instance,
			      struct mxt_object *object_table,
			      int max_objs)
{
	uint8_t object_table_index = 0;
	uint8_t address_found = 0;
	uint16_t address = 0;
	struct mxt_object *obj;

	while ((object_table_index < max_objs) && !address_found) {
		obj = &object_table[object_table_index];
		if (obj->type == object_type) {
			address_found = 1;
			/* Are there enough instances defined in the FW? */
			if (obj->instances >= instance) {
				address = obj->chip_addr +
					  (obj->size + 1) * instance;
			} else {
				return 0;
			}
		}
		object_table_index++;
	}
	return address;
}


/*
 * Reads a block of bytes from given address from mXT chip. If we are
 * reading from message window, and previous read was from message window,
 * there's no need to write the address pointer: the mXT chip will
 * automatically set the address pointer back to message window start.
 */

static int mxt_read_block(struct i2c_client *client,
		   u16 addr,
		   u16 length,
		   u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16	le_addr;
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	if (mxt != NULL) {
		if ((mxt->last_read_addr == addr) &&
			(addr == mxt->msg_proc_addr)) {
			if  (i2c_master_recv(client, value, length) == length)
				return length;
			else
				return -EIO;
		} else {
			mxt->last_read_addr = addr;
		}
	}

	mxt_debug(DEBUG_TRACE, "Writing address pointer & reading %d bytes "
		"in on i2c transaction...\n", length);

	le_addr = cpu_to_le16(addr);
	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;
	int i2cret;
	if  ((i2cret = i2c_transfer(adapter, msg, 2)) == 2)
	  return length;
	else {
	  printk(KERN_ERR "Error during i2c_tranfer, returned %i!",i2cret);
	  return -EIO;
	}
}

/* Reads a block of bytes from current address from mXT chip. */

static int mxt_read_block_wo_addr(struct i2c_client *client,
			   u16 length,
			   u8 *value)
{


	if  (i2c_master_recv(client, value, length) == length) {
		mxt_debug(DEBUG_TRACE, "I2C block read ok\n");
		return length;
	} else {
		mxt_debug(DEBUG_INFO, "I2C block read failed\n");
		return -EIO;
	}

}


/* Writes one byte to given address in mXT chip. */

static int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value)
{
	struct {
		__le16 le_addr;
		u8 data;

	} i2c_byte_transfer;

	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;
	i2c_byte_transfer.le_addr = cpu_to_le16(addr);
	i2c_byte_transfer.data = value;
	if  (i2c_master_send(client, (u8 *) &i2c_byte_transfer, 3) == 3)
		return 0;
	else
		return -EIO;
}


/* Writes a block of bytes (max 256) to given address in mXT chip. */
static int mxt_write_block(struct i2c_client *client,
		    u16 addr,
		    u16 length,
		    u8 *value)
{
	int i;
	struct {
		__le16	le_addr;
		u8	data[256];

	} i2c_block_transfer;

	struct mxt_data *mxt;

	mxt_debug(DEBUG_TRACE, "Writing %d bytes to %d...", length, addr);
	if (length > 256)
		return -EINVAL;
	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;
	for (i = 0; i < length; i++)
		i2c_block_transfer.data[i] = *value++;
	i2c_block_transfer.le_addr = cpu_to_le16(addr);
	i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);
	if (i == (length + 2))
		return length;
	else
		return -EIO;
}

/* Calculates the CRC value for mXT infoblock. */
int calculate_infoblock_crc(u32 *crc_result, u8 *data, int crc_area_size)
{
	u32 crc = 0;
	int i;

	for (i = 0; i < (crc_area_size - 1); i = i + 2)
		crc = CRC_24(crc, *(data + i), *(data + i + 1));
	/* If uneven size, pad with zero */
	if (crc_area_size & 0x0001)
		crc = CRC_24(crc, *(data + i), 0);
	/* Return only 24 bits of CRC. */
	*crc_result = (crc & 0x00FFFFFF);

	return 0;
}

/* Processes a touchscreen message. */
void process_T9_message(u8 *message, struct mxt_data *mxt, int last_touch)
{

	struct	input_dev *input;
	u8  status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8  touch_size = 255;
	u8  touch_number;
	u8  amplitude;
	u8  report_id;

	static int stored_size[MXT_MAX_NUM_TOUCHES];
	static int stored_x[MXT_MAX_NUM_TOUCHES];
	static int stored_y[MXT_MAX_NUM_TOUCHES];
	int i;
	int active_touches = 0;
	/*
	 * If the 'last_touch' flag is set, we have received all the touch
	 * messages there are available in this cycle, so send the events for
	 * touches that are active.
	 */
	if (last_touch) {
		/*
		 * For compatibility with single-touch systems, send also
		 * ABS_X & ABS_Y & BTN_TOUCH events.
		 */
		gs_repeatData.stored_size = stored_size[0];
		if (stored_size[0]) {
			input_report_key(mxt->input, BTN_TOUCH, 1);
			input_report_abs(mxt->input, ABS_PRESSURE, 1);
			input_report_abs(mxt->input, ABS_X, stored_x[0]);
			input_report_abs(mxt->input, ABS_Y, stored_y[0]);
			//input_sync(mxt->input);
			gs_repeatData.stored_x = stored_x[0];
			gs_repeatData.stored_y = stored_y[0];
			repeatStart( mxt->input );
		} else {
			input_report_key(mxt->input, BTN_TOUCH, 0);
			input_report_abs(mxt->input, ABS_PRESSURE, 0);
			input_sync(mxt->input);
			repeatStop();
		}
#ifdef ABS_MT_TRACKING_ID
		for (i = 0; i < mxt->numtouch; i++) {
			if (stored_size[i]) {
				active_touches++;
				input_report_abs(mxt->input,
						ABS_MT_TRACKING_ID,
						i);
				input_report_abs(mxt->input,
						ABS_MT_TOUCH_MAJOR,
						stored_size[i]);
				input_report_abs(mxt->input,
						ABS_MT_POSITION_X,
						stored_x[i]);
				input_report_abs(mxt->input,
						ABS_MT_POSITION_Y,
						stored_y[i]);
				input_mt_sync(mxt->input);
			}
		}
		if (active_touches == 0)
			input_mt_sync(mxt->input);
#endif
		input_sync(mxt->input);
	} else {
		input = mxt->input;
		status = message[MXT_MSG_T9_STATUS];
		report_id = message[0];

		if (status & MXT_MSGB_T9_SUPPRESS) {
			/* Touch has been suppressed by grip/face */
			/* detection                              */
			mxt_debug(DEBUG_TRACE, "SUPRESS");
		} else {
			/* Put together the 10-/12-bit coordinate values. */
			xpos = message[MXT_MSG_T9_XPOSMSB] * 16 +
				((message[MXT_MSG_T9_XYPOSLSB] >> 4) & 0xF);
			ypos = message[MXT_MSG_T9_YPOSMSB] * 16 +
				((message[MXT_MSG_T9_XYPOSLSB] >> 0) & 0xF);

			if (mxt->max_x_val < 1024)
				xpos >>= 2;
			if (mxt->max_y_val < 1024)
				ypos >>= 2;

			touch_number = message[MXT_MSG_REPORTID] -
				mxt->rid_map[report_id].first_rid;

			stored_x[touch_number] = xpos;
			stored_y[touch_number] = ypos;

			if (status & MXT_MSGB_T9_DETECT) {
				/*
				 * mXT224 reports the number of touched nodes,
				 * so the exact value for touch ellipse major
				 * axis length in nodes would be
				 * 2*sqrt(touch_size/pi)
				 * (assuming round touch shape), which would
				 * then need to be scaled using information
				 * about how many sensor lines we do have. So
				 * this is very much simplified,
				 * but sufficient for most if not all apps?
				 */
				touch_size = message[MXT_MSG_T9_TCHAREA];
				touch_size = touch_size >> 2;
				if (!touch_size)
					touch_size = 1;
				stored_size[touch_number] = touch_size;
				if (status & MXT_MSGB_T9_AMP)
					/* Amplitude of touch has changed */
					amplitude =
					    message[MXT_MSG_T9_TCHAMPLITUDE];
			}

			if (status & MXT_MSGB_T9_RELEASE) {
				/* The previously reported touch has been
				 * removed. */
				stored_size[touch_number] = 0;
			}
		}

		if (status & MXT_MSGB_T9_SUPPRESS) {
			mxt_debug(DEBUG_TRACE, "SUPRESS");
		} else {
			if (status & MXT_MSGB_T9_DETECT) {
				mxt_debug(DEBUG_TRACE, "DETECT:%s%s%s%s",
					((status & MXT_MSGB_T9_PRESS) ?
						" PRESS" : ""),
					((status & MXT_MSGB_T9_MOVE) ?
						" MOVE" : ""),
					((status & MXT_MSGB_T9_AMP) ?
						" AMP" : ""),
					((status & MXT_MSGB_T9_VECTOR) ?
						" VECT" : ""));
			} else if (status & MXT_MSGB_T9_RELEASE) {
				mxt_debug(DEBUG_TRACE, "RELEASE");
			}
		}
		mxt_debug(DEBUG_TRACE, "X=%d, Y=%d, TOUCHSIZE=%d",
			xpos, ypos, touch_size);
	}
	return;
}


int process_message(u8 *message, u8 object, struct mxt_data *mxt)
{
	struct i2c_client *client;
	u8  status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8  event;
	u8  direction;
	u16 distance;
	u8  length;
	u8  report_id;
	static u8 error_cond;

	client = mxt->client;
	length = mxt->message_size;
	report_id = message[0];

	if ((mxt->nontouch_msg_only == 0) ||
	    (!IS_TOUCH_OBJECT(object))) {
		mutex_lock(&mxt->msg_mutex);
		/* Copy the message to buffer */
		if (mxt->msg_buffer_startp < MXT_MESSAGE_BUFFER_SIZE)
			mxt->msg_buffer_startp++;
		else
			mxt->msg_buffer_startp = 0;
		if (mxt->msg_buffer_startp == mxt->msg_buffer_endp) {
			mxt_debug(DEBUG_TRACE,
				  "Message buf full, discarding last entry.\n");
			if (mxt->msg_buffer_endp < MXT_MESSAGE_BUFFER_SIZE)
				mxt->msg_buffer_endp++;
			else
				mxt->msg_buffer_endp = 0;
		}
		memcpy((mxt->messages + mxt->msg_buffer_startp * length),
		       message,
		       length);
		mutex_unlock(&mxt->msg_mutex);
	}

	switch (object) {
	case MXT_GEN_COMMANDPROCESSOR_T6:
		status = message[1];

		if (status & MXT_MSGB_T6_COMSERR) {
			if (!(error_cond & MXT_MSGB_T6_COMSERR)) {
				dev_err(&client->dev,
					"maXTouch checksum error\n");
				error_cond |= MXT_MSGB_T6_COMSERR;
			}
		}
		if (status & MXT_MSGB_T6_CFGERR) {
			/*
			 * Configuration error. A proper configuration
			 * needs to be written to chip and backed up.
			 */
			if (!(error_cond & MXT_MSGB_T6_CFGERR)) {
				dev_err(&client->dev,
					"maXTouch configuration error\n");
				error_cond |= MXT_MSGB_T6_CFGERR;
			}
		}
		if (status & MXT_MSGB_T6_CAL) {
			/* Calibration in action, no need to react */
			dev_info(&client->dev,
				"maXTouch calibration in progress\n");
		}
		if (status & MXT_MSGB_T6_SIGERR) {
			/*
			 * Signal acquisition error, something is seriously
			 * wrong, not much we can in the driver to correct
			 * this
			 */
			if (!(error_cond & MXT_MSGB_T6_SIGERR)) {
				dev_err(&client->dev,
					"maXTouch acquisition error\n");
				error_cond |= MXT_MSGB_T6_SIGERR;
			}
		}
		if (status & MXT_MSGB_T6_OFL) {
			/*
			 * Cycle overflow, the acquisition interval is too
			 * short.
			 */
			dev_err(&client->dev,
				"maXTouch cycle overflow\n");
		}
		if (status & MXT_MSGB_T6_RESET) {
			/* Chip has reseted, no need to react. */
			dev_info(&client->dev,
				"maXTouch chip reset\n");
		}
		if (status == 0) {
			/* Chip status back to normal. */
			dev_info(&client->dev,
				"maXTouch status normal\n");
			error_cond = 0;
		}
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		process_T9_message(message, mxt, 0);
		break;

	case MXT_SPT_GPIOPWM_T19:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving GPIO message\n");
		break;

	case MXT_PROCI_GRIPFACESUPPRESSION_T20:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving face suppression msg\n");
		break;

	case MXT_PROCG_NOISESUPPRESSION_T22:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving noise suppression msg\n");
		status = message[MXT_MSG_T22_STATUS];
		if (status & MXT_MSGB_T22_FHCHG) {
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
					"maXTouch: Freq changed\n");
		}
		if (status & MXT_MSGB_T22_GCAFERR) {
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
					"maXTouch: High noise "
					"level\n");
		}
		if (status & MXT_MSGB_T22_FHERR) {
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
					"maXTouch: Freq changed - "
					"Noise level too high\n");
		}
		break;

	case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
		printk(KERN_ERR "T24 message received!\n");
		break;
		
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving one-touch gesture msg\n");

		event = message[MXT_MSG_T24_STATUS] & 0x0F;
		xpos = message[MXT_MSG_T24_XPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 4) & 0x0F);
		ypos = message[MXT_MSG_T24_YPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 0) & 0x0F);
		if (mxt->max_x_val < 1024)
			xpos >>= 2;
		if (mxt->max_y_val < 1024)
			ypos >>= 2;
		direction = message[MXT_MSG_T24_DIR];
		distance = message[MXT_MSG_T24_DIST] +
			   (message[MXT_MSG_T24_DIST + 1] << 16);

		report_gesture((event << 24) | (direction << 16) | distance,
			mxt);
		report_gesture((xpos << 16) | ypos, mxt);

		break;

	case MXT_SPT_SELFTEST_T25:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving Self-Test msg\n");

		if (message[MXT_MSG_T25_STATUS] == MXT_MSGR_T25_OK) {
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
					"maXTouch: Self-Test OK\n");

		} else  {
			dev_err(&client->dev,
				"maXTouch: Self-Test Failed [%02x]:"
				"{%02x,%02x,%02x,%02x,%02x}\n",
				message[MXT_MSG_T25_STATUS],
				message[MXT_MSG_T25_STATUS + 0],
				message[MXT_MSG_T25_STATUS + 1],
				message[MXT_MSG_T25_STATUS + 2],
				message[MXT_MSG_T25_STATUS + 3],
				message[MXT_MSG_T25_STATUS + 4]
				);
		}
		break;

	case MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving 2-touch gesture message\n");

		event = message[MXT_MSG_T27_STATUS] & 0xF0;
		xpos = message[MXT_MSG_T27_XPOSMSB] * 16 +
			((message[MXT_MSG_T27_XYPOSLSB] >> 4) & 0x0F);
		ypos = message[MXT_MSG_T27_YPOSMSB] * 16 +
			((message[MXT_MSG_T27_XYPOSLSB] >> 0) & 0x0F);
		if (mxt->max_x_val < 1024)
			xpos >>= 2;
		if (mxt->max_y_val < 1024)
			ypos >>= 2;

		direction = message[MXT_MSG_T27_ANGLE];
		distance = message[MXT_MSG_T27_SEPARATION] +
			   (message[MXT_MSG_T27_SEPARATION + 1] << 16);

		report_gesture((event << 24) | (direction << 16) | distance,
			mxt);
		report_gesture((xpos << 16) | ypos, mxt);


		break;

	case MXT_SPT_CTECONFIG_T28:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving CTE message...\n");
		status = message[MXT_MSG_T28_STATUS];
		if (status & MXT_MSGB_T28_CHKERR)
			dev_err(&client->dev,
				"maXTouch: Power-Up CRC failure\n");

		break;
	default:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"maXTouch: Unknown message!\n");

		break;
	}

	return 0;
}


/*
 * Processes messages when the interrupt line (CHG) is asserted. Keeps
 * reading messages until a message with report ID 0xFF is received,
 * which indicates that there is no more new messages.
 *
 */

static void mxt_worker(struct work_struct *work)
{
	struct	mxt_data *mxt;
	struct	i2c_client *client;

	u8	*message;
	u16	message_length;
	u16	message_addr;
	u8	report_id;
	u8	object;
	int	error;
	int	i;
	char    *message_string;
	char    *message_start;
   message = NULL;
	mxt = container_of(work, struct mxt_data, dwork.work);
	client = mxt->client;
	message_addr = mxt->msg_proc_addr;
	message_length = mxt->message_size;
   if (message_length < 256) {
		message = kmalloc(message_length, GFP_KERNEL);
		if (message == NULL) {
			dev_err(&client->dev, "Error allocating memory\n");
			return;
		}
	} else {
		dev_err(&client->dev,
			"Message length larger than 256 bytes not supported\n");
		return;
	}
   mxt_debug(DEBUG_TRACE, "maXTouch worker active:\n");
   
   do {
		/* Read next message, reread on failure. */
		/* This driver currently does not request messages with CRC
		 * byte, so we can read one byte less than message_length. */
		mxt->message_counter++;
		for (i = 1; i < I2C_RETRY_COUNT; i++) {
	      error = mxt_read_block(client,
					       message_addr,
					       message_length - 1,
					       message);

			if (error >= 0)
				break;
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"Failure reading maxTouch device\n");
		}
		if (error < 0) {
			kfree(message);
			return;
		}

		if (mxt->address_pointer != message_addr)
			mxt->valid_ap = 0;
		report_id = message[0];
      
		if (0) {
			mxt_debug(DEBUG_RAW, "%s message [msg count: %08x]:",
				  REPORT_ID_TO_OBJECT_NAME(report_id, mxt),
				  mxt->message_counter
			);
			/* 5 characters per one byte */
			message_string = kmalloc(message_length * 5,
						 GFP_KERNEL);
			if (message_string == NULL) {
				dev_err(&client->dev,
					"Error allocating memory\n");
				kfree(message);
				return;
			}
			message_start = message_string;
			for (i = 0; i < message_length; i++) {
				message_string +=
					sprintf(message_string,
						"0x%02X ", message[i]);
			}
			mxt_debug(DEBUG_RAW, "%s", message_start);
			kfree(message_start);
		}
		
      if ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)) {
		   memcpy(mxt->last_message, message, message_length);
			mxt->new_msgs = 1;
			smp_wmb();
			/* Get type of object and process the message */
			object = mxt->rid_map[report_id].object;
		   process_message(message, object, mxt);
		}
		process_T9_message(NULL, mxt, 1);
		
      mxt_debug(DEBUG_TRACE, "chgline: %d\n", mxt->read_chg());
	} while (comms ? (mxt->read_chg() == 0) :
		 ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)));

    
	/* All messages processed, send the events) */
	process_T9_message(NULL, mxt, 1);

	kfree(message);
	enable_irq(mxt->irq);
	/* Make sure we just didn't miss a interrupt. */
	//if (mxt->read_chg() == 0)
	//	schedule_delayed_work(&mxt->dwork, 0);
}


/*
 * The maXTouch device will signal the host about a new message by asserting
 * the CHG line. This ISR schedules a worker routine to read the message when
 * that happens.
 */

static irqreturn_t mxt_irq_handler(int irq, void *_mxt)
{
	struct mxt_data *mxt = _mxt;

	mxt->irq_counter++;
	//	mxt_debug(DEBUG_TRACE, "Interrupt number %d detected\n",mxt->irq_counter);
	if (mxt->valid_interrupt()) {
	        //mxt_debug(DEBUG_TRACE, "VALID Interrupt number %d detected",mxt->irq_counter);
		disable_irq_nosync(irq);
		/* Send the signal only if falling edge generated the irq. */
	        //mxt_debug(DEBUG_TRACE, "VALID Interrupt number %d detected",mxt->irq_counter);
		cancel_delayed_work(&mxt->dwork);
		schedule_delayed_work(&mxt->dwork, 0);
		mxt->valid_irq_counter++;
		//	        mxt_debug(DEBUG_TRACE, "VALID Interrupt number %d detected\n",mxt->valid_irq_counter);
	} else {
		mxt->invalid_irq_counter++;
		//	        mxt_debug(DEBUG_TRACE, "INVALID Interrupt number %d detected\n",mxt->invalid_irq_counter);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}



/******************************************************************************/
/* Initialization of driver                                                   */
/******************************************************************************/

static int __devinit mxt_identify(struct i2c_client *client,
				  struct mxt_data *mxt,
				  u8 *id_block_data)
{
	u8 buf[MXT_ID_BLOCK_SIZE];
	int error;
	int identified;

	identified = 0;

	/* Read Device info to check if chip is valid */
	error = mxt_read_block(client, MXT_ADDR_INFO_BLOCK, MXT_ID_BLOCK_SIZE,
			       (u8 *) buf);

	if (error < 0) {
		mxt->read_fail_counter++;
		dev_err(&client->dev, "Failure accessing maXTouch device: error %i\n",error);
		return -EIO;
	}

	memcpy(id_block_data, buf, MXT_ID_BLOCK_SIZE);

	mxt->device_info.family_id  = buf[0];
	mxt->device_info.variant_id = buf[1];
	mxt->device_info.major	    = ((buf[2] >> 4) & 0x0F);
	mxt->device_info.minor      = (buf[2] & 0x0F);
	mxt->device_info.build	    = buf[3];
	mxt->device_info.x_size	    = buf[4];
	mxt->device_info.y_size	    = buf[5];
	mxt->device_info.num_objs   = buf[6];
	mxt->device_info.num_nodes  = mxt->device_info.x_size *
				      mxt->device_info.y_size;

	/*
	 * Check Family & Variant Info; warn if not recognized but
	 * still continue.
	 */

	/* MXT224 */
	if (mxt->device_info.family_id == MXT224_FAMILYID) {
		strcpy(mxt->device_info.family_name, "mXT224");

		if (mxt->device_info.variant_id == MXT224_CAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Calibrated");
		} else if (mxt->device_info.variant_id ==
			MXT224_UNCAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Uncalibrated");
		} else {
			dev_err(&client->dev,
				"Warning: maXTouch Variant ID [%d] not "
				"supported\n",
				mxt->device_info.variant_id);
			strcpy(mxt->device_info.variant_name, "UNKNOWN");
			/* identified = -ENXIO; */
		}
	/* MXT 224E */
	} else if (mxt->device_info.family_id == 0x81) {
		strcpy(mxt->device_info.family_name, "mXT224E");

		if (mxt->device_info.variant_id == MXT224_CAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Calibrated");
		} else if (mxt->device_info.variant_id ==
			MXT224_UNCAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Uncalibrated");
		} else {
			dev_err(&client->dev,
				"Warning: maXTouch Variant ID [%d] not "
				"supported\n",
				mxt->device_info.variant_id);
			strcpy(mxt->device_info.variant_name, "UNKNOWN");
			/* identified = -ENXIO; */
		}
	/* MXT1386 */
	} else if (mxt->device_info.family_id == MXT1386_FAMILYID) {
		strcpy(mxt->device_info.family_name, "mXT1386");

		if (mxt->device_info.variant_id == MXT1386_CAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Calibrated");
		} else {
			dev_err(&client->dev,
				"Warning: maXTouch Variant ID [%d] not "
				"supported\n",
				mxt->device_info.variant_id);
			strcpy(mxt->device_info.variant_name, "UNKNOWN");
			/* identified = -ENXIO; */
		}
	/* Unknown family ID! */
	} else {
		dev_err(&client->dev,
			"Warning: maXTouch Family ID [%d] not supported\n",
			mxt->device_info.family_id);
		strcpy(mxt->device_info.family_name, "UNKNOWN");
		strcpy(mxt->device_info.variant_name, "UNKNOWN");
		/* identified = -ENXIO; */
	}

	dev_info(
		&client->dev,
		"Atmel maXTouch (Family %s (%X), Variant %s (%X)) Firmware "
		"version [%d.%d] Build %d\n",
		mxt->device_info.family_name,
		mxt->device_info.family_id,
		mxt->device_info.variant_name,
		mxt->device_info.variant_id,
		mxt->device_info.major,
		mxt->device_info.minor,
		mxt->device_info.build
	);
	dev_info(
		&client->dev,
		"Atmel maXTouch Configuration "
		"[X: %d] x [Y: %d]\n",
		mxt->device_info.x_size,
		mxt->device_info.y_size
	);
	return identified;
}

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH_DEFAULT_DRIVER_SETTINGS
typedef struct {
  u8 type;
  u8 idx; //unsused
  u8 size;
  u8 data[256];
} obj_entry_t;

static const obj_entry_t init_vals_ver_20[] = {
  /* UNUSED (0) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (1) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (2) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (3) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (4) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (5) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* QT602240_GEN_COMMAND(6) */
  { 0x06, 0x00, 0x06, 
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },

  /* QT602240_GEN_POWER(7) */
  { 0x07, 0x00, 0x3,				       
    { 0xFF, 0xFF, 0x32 } },
  
  /* QT602240_GEN_ACQUIRE(8) */
  { 0x08, 0x00, 0x08,
    { 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10 } },

  /* QT602240_TOUCH_MULTI(9) */
  { 0x09, 0x00, 0x1E,
    { 0x83, 0x00, 0x00, 0x0C, 0x0a, 0x00, 0x00, 0x18, 0x06, 0x02,
      0x32, 0x32, 0x01, 0x22, 0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },

  /* UNUSED (10) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (11) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (12) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (13) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (14) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* QT602240_TOUCH_KEYARRAY(15) */
  { 0x0F, 0x00, 0x0B,
    { 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x41, 0x1e, 0x02, 0x00,
      0x00 } },

  /* UNUSED (16) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (17) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (18) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* QT602240_SPT_GPIOPWM(19) */
  { 0x13, 0x00, 0x0C,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00 } },

  /* QT602240_PROCI_GRIPFACE(20) */
  { 0x14, 0x00, 0x0C,
    { 0x00, 0x64, 0x64, 0x64, 0x64, 0x00, 0x00, 0x1e, 0x14, 0x04,
      0x0f, 0x05 } },

  /* UNUSED (21) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* QT602240_PROCG_NOISE(22) */
  /** { 0x16, 0x00, 0x15,
    { 0x05, 0x00, 0x00, 0x19, 0x00, 0xe6, 0xff, 0x04, 0x04, 0x00,
    0x01, 0x0a, 0x0f, 0x14, 0x19, 0x1e, 0x04, 0x36,0x20 } },**/
  { 0x16, 0x00, 0x11,
    { 0x05, 0x00, 0x00, 0x19, 0x00, 0xe6, 0xff, 0x04, 0x08, 0x00,
      0x01, 0x0a, 0x0f, 0x14, 0x19, 0x1e, 0x04 } },

  /* QT602240_TOUCH_PROXIMITY(23) */
  { 0x17, 0x00, 0x0D,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00 } },

  /* QT602240_PROCI_ONETOUCH(24) */
  { 0x18, 0x00, 0x13,
    { 0x00, 0x0a, 0xff, 0x03, 0x00, 0x64, 0x64, 0x01, 0x0a, 0x14,
      0x28, 0x4b, 0x00, 0x02, 0x00, 0x64, 0x00, 0x19, 0x00 } },

  /* QT602240_SPT_SELFTEST(25) */
  { 0x19, 0x00, 0x0E,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00 } },

  /* UNUSED (26) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* QT602240_PROCI_TWOTOUCH(27) */
  { 0x1B, 0x00, 0x07,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },


  /* QT602240_SPT_CTECONFIG(28) */
  { 0x1C, 0x00, 0x06,
    { 0x00, 0x00, 0x03, 0x20, 0x20, 0x00 } },

  /* UNUSED (29) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (30) */
  { 0x00, 0x00, 0x00, { 0x00 } },

  /* UNUSED (31) */
  { 0x00, 0x00, 0x00, { 0x00 } }
};
#endif



/*
 * Reads the object table from maXTouch chip to get object data like
 * address, size, report id. For Info Block CRC calculation, already read
 * id data is passed to this function too (Info Block consists of the ID
 * block and object table).
 *
 */
static int __devinit mxt_read_object_table(struct i2c_client *client,
					   struct mxt_data *mxt,
					   u8 *raw_id_data)
{
	u16	report_id_count;
	u8	buf[MXT_OBJECT_TABLE_ELEMENT_SIZE];
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH_DEFAULT_DRIVER_SETTINGS
	u8      databuf[256];
#endif
	u8      *raw_ib_data;
	u8	object_type;
	u16	object_address;
	u16	object_size;
	u8	object_instances;
	u8	object_report_ids;
	u16	object_info_address;
	u32	crc;
	u32     calculated_crc;
	int	i;
	int	error;

	u8	object_instance;
	u8	object_report_id;
	u8	report_id;
	int     first_report_id;
	int     ib_pointer;
	struct mxt_object *object_table;

	mxt_debug(DEBUG_TRACE, "maXTouch driver reading configuration\n");

	object_table = kzalloc(sizeof(struct mxt_object) *
			       mxt->device_info.num_objs,
			       GFP_KERNEL);
	if (object_table == NULL) {
		printk(KERN_WARNING "maXTouch: Memory allocation failed!\n");
		error = -ENOMEM;
		goto err_object_table_alloc;
	}

	raw_ib_data = kmalloc(MXT_OBJECT_TABLE_ELEMENT_SIZE *
			mxt->device_info.num_objs + MXT_ID_BLOCK_SIZE,
			GFP_KERNEL);
	if (raw_ib_data == NULL) {
		printk(KERN_WARNING "maXTouch: Memory allocation failed!\n");
		error = -ENOMEM;
		goto err_ib_alloc;
	}

	/* Copy the ID data for CRC calculation. */
	memcpy(raw_ib_data, raw_id_data, MXT_ID_BLOCK_SIZE);
	ib_pointer = MXT_ID_BLOCK_SIZE;

	mxt->object_table = object_table;

	mxt_debug(DEBUG_TRACE, "maXTouch driver Memory allocated\n");

	object_info_address = MXT_ADDR_OBJECT_TABLE;

	report_id_count = 0;
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		mxt_debug(DEBUG_TRACE, "Reading maXTouch at [0x%04x]: ",
			  object_info_address);

		error = mxt_read_block(client, object_info_address,
				       MXT_OBJECT_TABLE_ELEMENT_SIZE, buf);

		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"maXTouch Object %d could not be read\n", i);
			error = -EIO;
			goto err_object_read;
		}

		memcpy(raw_ib_data + ib_pointer, buf,
		       MXT_OBJECT_TABLE_ELEMENT_SIZE);
		ib_pointer += MXT_OBJECT_TABLE_ELEMENT_SIZE;

		object_type       =  buf[0];
		object_address    = (buf[2] << 8) + buf[1];
		object_size       =  buf[3] + 1;
		object_instances  =  buf[4] + 1;
		object_report_ids =  buf[5];
		mxt_debug(DEBUG_TRACE, "Type=%03d, Address=0x%04x, "
			  "Size=0x%02x, %d instances, %d report id's\n",
			  object_type,
			  object_address,
			  object_size,
			  object_instances,
			  object_report_ids
		);


#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH_DEFAULT_DRIVER_SETTINGS
		if( (object_type < 32) && (init_vals_ver_20[object_type].type == object_type) ) {
		  u16 write_size = init_vals_ver_20[object_type].size;
		  if( write_size != object_size) {
		    printk(KERN_WARNING "maXTouch: Object No. %i preset size (0x%04X) differs from actual size (0x%04X)!\n", object_type, write_size, object_size);
		    write_size = write_size > object_size ? object_size : write_size;
		  }
		  error = mxt_write_block(client, object_address, write_size ,
					  (u8*)init_vals_ver_20[object_type].data);
		  if (error < 0) {
		    //mxt->read_fail_counter++;
		    dev_err(&client->dev,
			    "maXTouch Object %d could not be written\n", object_type);
		    error = -EIO;
		    goto err_object_read;
		  }
		} else {
		  mxt_debug(DEBUG_TRACE, "Ignoring object type %i, no init entry found!\n",object_type);
		}

		error = mxt_read_block(client,object_address,object_size,databuf);
		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"maXTouch Object %d could not be read\n", i);
			error = -EIO;
			goto err_object_read;
		}

		int jj;
		mxt_debug(DEBUG_TRACE, "Data=");
		for( jj=0; jj<object_size; jj++ ) {
		  mxt_debug(DEBUG_TRACE,"0x%02X ",databuf[jj]);
		}
		mxt_debug(DEBUG_TRACE,"\n");
#endif


		/* TODO: check whether object is known and supported? */
		/* Save frequently needed info. */
		if (object_type == MXT_GEN_MESSAGEPROCESSOR_T5) {
			mxt->msg_proc_addr = object_address;
			mxt->message_size = object_size;
		}

		object_table[i].type            = object_type;
		object_table[i].chip_addr       = object_address;
		object_table[i].size            = object_size;
		object_table[i].instances       = object_instances;
		object_table[i].num_report_ids  = object_report_ids;
		report_id_count += object_instances * object_report_ids;

		object_info_address += MXT_OBJECT_TABLE_ELEMENT_SIZE;
	}

	mxt->rid_map =
		kzalloc(sizeof(struct report_id_map) * (report_id_count + 1),
			/* allocate for report_id 0, even if not used */
			GFP_KERNEL);
	if (mxt->rid_map == NULL) {
		printk(KERN_WARNING "maXTouch: Can't allocate memory!\n");
		error = -ENOMEM;
		goto err_rid_map_alloc;
	}

	mxt->messages = kzalloc(mxt->message_size * MXT_MESSAGE_BUFFER_SIZE,
				GFP_KERNEL);
	if (mxt->messages == NULL) {
		printk(KERN_WARNING "maXTouch: Can't allocate memory!\n");
		error = -ENOMEM;
		goto err_msg_alloc;
	}

	mxt->last_message = kzalloc(mxt->message_size, GFP_KERNEL);
	if (mxt->last_message == NULL) {
		printk(KERN_WARNING "maXTouch: Can't allocate memory!\n");
		error = -ENOMEM;
		goto err_msg_alloc;
	}

	mxt->report_id_count = report_id_count;
	if (report_id_count > 254) {	/* 0 & 255 are reserved */
			dev_err(&client->dev,
				"Too many maXTouch report id's [%d]\n",
				report_id_count);
			error = -ENXIO;
			goto err_max_rid;
	}

	/* Create a mapping from report id to object type */
	report_id = 1; /* Start from 1, 0 is reserved. */

	/* Create table associating report id's with objects & instances */
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		for (object_instance = 0;
		     object_instance < object_table[i].instances;
		     object_instance++){
			first_report_id = report_id;
			for (object_report_id = 0;
			     object_report_id < object_table[i].num_report_ids;
			     object_report_id++) {
				mxt->rid_map[report_id].object =
					object_table[i].type;
				mxt->rid_map[report_id].instance =
					object_instance;
				mxt->rid_map[report_id].first_rid =
					first_report_id;
				report_id++;
			}
		}
	}

	/* Read 3 byte CRC */
	error = mxt_read_block(client, object_info_address, 3, buf);
	if (error < 0) {
		mxt->read_fail_counter++;
		dev_err(&client->dev, "Error reading CRC\n");
	}

	crc = (buf[2] << 16) | (buf[1] << 8) | buf[0];

	if (calculate_infoblock_crc(&calculated_crc, raw_ib_data,
				    ib_pointer)) {
		printk(KERN_WARNING "Error while calculating CRC!\n");
		calculated_crc = 0;
	}
	kfree(raw_ib_data);

	mxt_debug(DEBUG_TRACE, "\nReported info block CRC = 0x%6X\n", crc);
	mxt_debug(DEBUG_TRACE, "Calculated info block CRC = 0x%6X\n\n",
		       calculated_crc);

	if (crc == calculated_crc) {
		mxt->info_block_crc = crc;
	} else {
		mxt->info_block_crc = 0;
		printk(KERN_ALERT "maXTouch: Info block CRC invalid!\n");
	}

	if (debug >= DEBUG_VERBOSE) {

		dev_info(&client->dev, "maXTouch: %d Objects\n",
				mxt->device_info.num_objs);

		for (i = 0; i < mxt->device_info.num_objs; i++) {
			dev_info(&client->dev, "Type:\t\t\t[%d]: %s\n",
				 object_table[i].type,
				 object_type_name[object_table[i].type]);
			dev_info(&client->dev, "\tAddress:\t0x%04X\n",
				object_table[i].chip_addr);
			dev_info(&client->dev, "\tSize:\t\t%d Bytes\n",
				 object_table[i].size);
			dev_info(&client->dev, "\tInstances:\t%d\n",
				 object_table[i].instances);
			dev_info(&client->dev, "\tReport Id's:\t%d\n",
				 object_table[i].num_report_ids);
		}
	}

	return 0;


err_max_rid:
	kfree(mxt->last_message);
err_msg_alloc:
	kfree(mxt->rid_map);
err_rid_map_alloc:
err_object_read:
	kfree(raw_ib_data);
err_ib_alloc:
	kfree(object_table);
err_object_table_alloc:
	return error;
}


static int __devinit mxt_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct mxt_data          *mxt;
	struct mxt_platform_data *pdata;
	struct input_dev         *input;
	u8 *id_data;
	int error;

	mxt_debug(DEBUG_INFO, "mXT224: mxt_probe\n");

	if (client == NULL) {
		pr_debug("maXTouch: client == NULL\n");
		return	-EINVAL;
	} else if (client->adapter == NULL) {
		pr_debug("maXTouch: client->adapter == NULL\n");
		return	-EINVAL;
	} else if (&client->dev == NULL) {
		pr_debug("maXTouch: client->dev == NULL\n");
		return	-EINVAL;
	} else if (&client->adapter->dev == NULL) {
		pr_debug("maXTouch: client->adapter->dev == NULL\n");
		return	-EINVAL;
	} else if (id == NULL) {
		pr_debug("maXTouch: id == NULL\n");
		return	-EINVAL;
	}

	mxt_debug(DEBUG_INFO, "maXTouch driver v. %s\n", DRIVER_VERSION);
	mxt_debug(DEBUG_INFO, "\t \"%s\"\n", client->name);
	mxt_debug(DEBUG_INFO, "\taddr:\t0x%04x\n", client->addr);
	mxt_debug(DEBUG_INFO, "\tirq:\t%d\n", client->irq);
	mxt_debug(DEBUG_INFO, "\tflags:\t0x%04x\n", client->flags);
	mxt_debug(DEBUG_INFO, "\tadapter:\"%s\"\n", client->adapter->name);
	mxt_debug(DEBUG_INFO, "\tdevice:\t\"%s\"\n", client->dev.init_name);

	mxt_debug(DEBUG_TRACE, "maXTouch driver functionality OK\n");

	/* Allocate structure - we need it to identify device */
	mxt = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	if (mxt == NULL) {
		dev_err(&client->dev, "insufficient memory\n");
		error = -ENOMEM;
		goto err_mxt_alloc;
	}

	id_data = kmalloc(MXT_ID_BLOCK_SIZE, GFP_KERNEL);
	if (id_data == NULL) {
		dev_err(&client->dev, "insufficient memory\n");
		error = -ENOMEM;
		goto err_id_alloc;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&client->dev, "error allocating input device\n");
		error = -ENOMEM;
		goto err_input_dev_alloc;
	}

	/* Initialize Platform data */

	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		dev_err(&client->dev, "platform data is required!\n");
		error = -EINVAL;
		goto err_pdata;
	}
	if (debug >= DEBUG_TRACE)
		printk(KERN_INFO "Platform OK: pdata = 0x%08x\n",
		       (unsigned int) pdata);

	mxt->read_fail_counter = 0;
	mxt->message_counter   = 0;
	mxt->max_x_val         = pdata->max_x;
	mxt->max_y_val         = pdata->max_y;
	mxt->numtouch          = pdata->numtouch;

    mxt_debug(DEBUG_INFO, "\tpdata->max_x:\t\"%d\"\n", pdata->max_x);
    mxt_debug(DEBUG_INFO, "\tpdata->max_y:\t\"%d\"\n", pdata->max_y);
    mxt_debug(DEBUG_INFO, "\tpdata->numtouch:\t\"%d\"\n", pdata->numtouch);

	/* Get data that is defined in board specific code. */
	mxt->init_hw = pdata->init_platform_hw;
	mxt->exit_hw = pdata->exit_platform_hw;
	mxt->read_chg = pdata->read_chg;

	if (pdata->valid_interrupt != NULL)
		mxt->valid_interrupt = pdata->valid_interrupt;
	else
		mxt->valid_interrupt = mxt_valid_interrupt_dummy;

	if (mxt->init_hw != NULL)
		mxt->init_hw();

	if (debug >= DEBUG_TRACE)
		printk(KERN_INFO "maXTouch driver identifying chip\n");

	if (mxt_identify(client, mxt, id_data) < 0) {
		dev_err(&client->dev, "Chip could not be identified\n");
		error = -ENODEV;
		goto err_identify;
	}
	/* Chip is valid and active. */
	if (debug >= DEBUG_TRACE)
		printk(KERN_INFO "maXTouch driver allocating input device\n");

	mxt->client = client;
	mxt->input  = input;

	INIT_DELAYED_WORK(&mxt->dwork, mxt_worker);
	mutex_init(&mxt->debug_mutex);
	mutex_init(&mxt->msg_mutex);
	mxt_debug(DEBUG_TRACE, "maXTouch driver creating device name\n");

	snprintf(
		mxt->phys_name,
		sizeof(mxt->phys_name),
		"%s/input0",
		dev_name(&client->dev)
	);
	input->name = "Atmel maXTouch Touchscreen controller";
	input->phys = mxt->phys_name;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	mxt_debug(DEBUG_INFO, "maXTouch name: \"%s\"\n", input->name);
	mxt_debug(DEBUG_INFO, "maXTouch phys: \"%s\"\n", input->phys);
	mxt_debug(DEBUG_INFO, "maXTouch driver setting abs parameters\n");

	set_bit(BTN_TOUCH, input->keybit);

	/* Single touch */
	input_set_abs_params(input, ABS_X, 0, mxt->max_x_val, 0, 0);
	input_set_abs_params(input, ABS_Y, 0, mxt->max_y_val, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE, 0, MXT_MAX_REPORTED_PRESSURE,
			     0, 0);
	input_set_abs_params(input, ABS_TOOL_WIDTH, 0, MXT_MAX_REPORTED_WIDTH,
			     0, 0);
#ifdef ABS_MT_TRACKING_ID
	/* Multitouch */
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, mxt->max_x_val, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, mxt->max_y_val, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, MXT_MAX_TOUCH_SIZE,
			     0, 0);
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, MXT_MAX_NUM_TOUCHES,
			     0, 0);
#endif
	__set_bit(EV_ABS, input->evbit);
	__set_bit(EV_SYN, input->evbit);
	__set_bit(EV_KEY, input->evbit);
	__set_bit(EV_MSC, input->evbit);
	input->mscbit[0] = BIT_MASK(MSC_GESTURE);

	mxt_debug(DEBUG_TRACE, "maXTouch driver setting client data\n");
	i2c_set_clientdata(client, mxt);
	mxt_debug(DEBUG_TRACE, "maXTouch driver setting drv data\n");
	input_set_drvdata(input, mxt);
	mxt_debug(DEBUG_TRACE, "maXTouch driver input register device\n");
	error = input_register_device(mxt->input);
	if (error < 0) {
		dev_err(&client->dev,
			"Failed to register input device\n");
		goto err_register_device;
	}

	error = mxt_read_object_table(client, mxt, id_data);
	if (error < 0)
		goto err_read_ot;


	/* Create debugfs entries. */
	mxt->debug_dir = debugfs_create_dir("maXTouch", NULL);
	if (mxt->debug_dir == -ENODEV) {
		/* debugfs is not enabled. */
		printk(KERN_WARNING "debugfs not enabled in kernel\n");
	} else if (mxt->debug_dir == NULL) {
		printk(KERN_WARNING "error creating debugfs dir\n");
	} else {
		mxt_debug(DEBUG_TRACE, "created \"maXTouch\" debugfs dir\n");
		debugfs_create_file("deltas", S_IRUSR, mxt->debug_dir, mxt,
				    &delta_fops);
		debugfs_create_file("refs", S_IRUSR, mxt->debug_dir, mxt,
				    &refs_fops);
	}

	/* Create character device nodes for reading & writing registers */
	mxt->mxt_class = class_create(THIS_MODULE, "maXTouch_memory");
	if (IS_ERR(mxt->mxt_class)) {
		printk(KERN_WARNING "class create failed! exiting...");
		goto err_read_ot;
	}
	/* 2 numbers; one for memory and one for messages */
	error = alloc_chrdev_region(&mxt->dev_num, 0, 2,
				    "maXTouch_memory");
	mxt_debug(DEBUG_VERBOSE,
		"device number %d allocated!\n", MAJOR(mxt->dev_num));
	if (error)
		printk(KERN_WARNING "Error registering device\n");

	cdev_init(&mxt->cdev, &mxt_memory_fops);
	cdev_init(&mxt->cdev_messages, &mxt_message_fops);

	mxt_debug(DEBUG_VERBOSE, "cdev initialized\n");
	mxt->cdev.owner = THIS_MODULE;
	mxt->cdev_messages.owner = THIS_MODULE;

	error = cdev_add(&mxt->cdev, mxt->dev_num, 1);
	if (error)
		printk(KERN_WARNING "Bad cdev\n");

	error = cdev_add(&mxt->cdev_messages, mxt->dev_num + 1, 1);
	if (error)
		printk(KERN_WARNING "Bad cdev\n");

	mxt_debug(DEBUG_VERBOSE, "cdev added\n");

	device_create(mxt->mxt_class, NULL, MKDEV(MAJOR(mxt->dev_num), 0), NULL,
		"maXTouch");

	device_create(mxt->mxt_class, NULL, MKDEV(MAJOR(mxt->dev_num), 1), NULL,
		"maXTouch_messages");

	mxt->msg_buffer_startp = 0;
	mxt->msg_buffer_endp = 0;

	/* Allocate the interrupt */
	mxt_debug(DEBUG_TRACE, "maXTouch driver allocating interrupt...\n");
	printk(KERN_ERR "maXTouch driver allocating interrupt..%d.\n",client->irq);
	mxt->irq = client->irq;
	mxt->valid_irq_counter = 0;
	mxt->invalid_irq_counter = 0;
	mxt->irq_counter = 0;
	
	if (mxt->irq) {
		/* Try to request IRQ with falling edge first. This is
		 * not always supported. If it fails, try with any edge. */
		error = request_irq(mxt->irq,
				    mxt_irq_handler,
				    IRQF_TRIGGER_LOW,
				    client->dev.driver->name,
				    mxt);
		if (error < 0) {
			printk(KERN_ERR "Try any edge!\n");
			/* TODO: why only 0 works on STK1000? */
			error = request_irq(mxt->irq,
					    mxt_irq_handler,
					    0,
					    client->dev.driver->name,
					    mxt);
		}

		if (error < 0) {
			dev_err(&client->dev,
				"failed to allocate irq %d\n", mxt->irq);
			goto err_irq;
		}
	}

	if (debug > DEBUG_INFO)
		dev_info(&client->dev, "touchscreen, irq %d\n", mxt->irq);
	kfree(id_data);

	gs_repeatData.intervalMs = 30;

	return 0;


err_irq:
	kfree(mxt->rid_map);
	kfree(mxt->object_table);
	kfree(mxt->last_message);
err_read_ot:
err_register_device:
err_identify:
err_pdata:
	input_free_device(input);
err_input_dev_alloc:
	kfree(id_data);
err_id_alloc:
	if (mxt->exit_hw != NULL)
		mxt->exit_hw();
	kfree(mxt);
err_mxt_alloc:
	return error;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	/* Remove debug dir entries */
	debugfs_remove_recursive(mxt->debug_dir);

	if (mxt != NULL) {
		if (mxt->exit_hw != NULL)
			mxt->exit_hw();
		if (mxt->irq)
			free_irq(mxt->irq, mxt);
		unregister_chrdev_region(mxt->dev_num, 2);
		device_destroy(mxt->mxt_class, MKDEV(MAJOR(mxt->dev_num), 0));
		device_destroy(mxt->mxt_class, MKDEV(MAJOR(mxt->dev_num), 1));
		cdev_del(&mxt->cdev);
		cdev_del(&mxt->cdev_messages);
		cancel_delayed_work_sync(&mxt->dwork);
		input_unregister_device(mxt->input);
		class_destroy(mxt->mxt_class);
		debugfs_remove(mxt->debug_dir);

		kfree(mxt->rid_map);
		kfree(mxt->object_table);
		kfree(mxt->last_message);
	}
	kfree(mxt);

	i2c_set_clientdata(client, NULL);
	if (debug >= DEBUG_TRACE)
		dev_info(&client->dev, "Touchscreen unregistered\n");

	return 0;
}

#if defined(CONFIG_PM)
/*
 * Allow the touchscreen to resume the system from sleep.
 * Alternatively we could put the maXTouch to deep sleep mode and use
 * something else to resume.
 *
 * TODO: set ACQINT to some high value, and restore to original at resume?
 */

static int mxt_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);
	if (device_may_wakeup(&client->dev))
		enable_irq_wake(mxt->irq);
	return 0;
}

static int mxt_resume(struct i2c_client *client)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);
	if (device_may_wakeup(&client->dev))
		disable_irq_wake(mxt->irq);
    cancel_delayed_work(&mxt->dwork);
	schedule_delayed_work(&mxt->dwork, 0);
	return 0;
}
#else
#define mxt_suspend NULL
#define mxt_resume NULL
#endif

static const struct i2c_device_id mxt_idtable[] = {
	{"maXTouch", 0,},
	{ }
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "maXTouch",
		.owner  = THIS_MODULE,
	},

	.id_table	= mxt_idtable,
	.probe		= mxt_probe,
	.remove		= __devexit_p(mxt_remove),
	.suspend	= mxt_suspend,
	.resume		= mxt_resume,

};

static int __init mxt_init(void)
{
	int err;
	err = i2c_add_driver(&mxt_driver);
	if (err)
		printk(KERN_WARNING "Adding maXTouch driver failed "
		       "(errno = %d)\n", err);
	else
		mxt_debug(DEBUG_TRACE, "Successfully added driver %s\n",
			  mxt_driver.driver.name);
	return err;
}

static void __exit mxt_cleanup(void)
{
	i2c_del_driver(&mxt_driver);
}


module_init(mxt_init);
module_exit(mxt_cleanup);

MODULE_AUTHOR("Iiro Valkonen");
MODULE_DESCRIPTION("Driver for Atmel maXTouch Touchscreen Controller");
MODULE_LICENSE("GPL");
