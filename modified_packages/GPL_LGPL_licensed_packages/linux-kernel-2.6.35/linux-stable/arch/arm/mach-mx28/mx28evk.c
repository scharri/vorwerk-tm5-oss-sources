/*
 * Copyright (C) 2009-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH
#include <linux/atmel_maxtouch.h>
#endif

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/hardware.h>
#include <mach/device.h>
#include <mach/pinctrl.h>

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>

#include <linux/input.h>
#ifdef CONFIG_INPUT_ALPMXS_ROTARY_ENCODER
# include <linux/alpmxs_rotary_encoder.h>
#endif
#ifdef CONFIG_INPUT_MXS_ROTARY_ENCODER
# include <linux/mxs_rotary_encoder.h>
#endif

#include "device.h"
#include "mx28evk.h"

#include <mach/mx28_pins.h>


#ifdef CONFIG_TOUCHSCREEN_NOVATEK
#include <linux/novatek_ts.h>
#endif

//#ifdef CONFIG_TOUCHSCREEN_NT11003
//#include <linux/nt11003.h>
//#endif



#define GPIO_TOUCH_CHANGE  MXS_PIN_TO_GPIO(PINID_ENET0_TXD1)
#define GPIO_TOUCH_ADS     MXS_PIN_TO_GPIO(PINID_GPMI_CE1N)


#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH
static u8 read_chg()
{
	return gpio_get_value(GPIO_TOUCH_CHANGE);
}

static u8 valid_interrupt()
{
	return !read_chg();
}

struct mxt_platform_data Atmel_mxt_info = {
	/* Maximum number of simultaneous touches to report. */
	.numtouch = 1,
	// TODO: no need for any hw-specific things at init/exit?
	.init_platform_hw = NULL,
	.exit_platform_hw = NULL,
	//	.max_x = 480,
	//	.max_y = 272,
	.max_x = 480,
	.max_y = 372,
	.valid_interrupt = &valid_interrupt,
	.read_chg = &read_chg,
};
#endif 

#if defined (CONFIG_TOUCHSCREEN_NOVATEK)
struct novatek_platform_data novatek_ts_info = {
  .platform_sleep   = NULL,
  .platform_wakeup  = NULL,
  .init_platform_hw = NULL,
  .exit_platform_hw = NULL,
};
#endif



#if defined(CONFIG_TOUCHSCREEN_QT602240) 
/* TSP */
static struct qt602240_platform_data qt602240_platform_data = {
	.x_line		= 17,
	.y_line		= 11,
	.x_size		= 800,
	.y_size		= 480,
	.blen		= 0x21,
	.threshold	= 0x28,
	.voltage	= 2800000,              /* 2.8V */
	.orient		= QT602240_DIAGONAL,
};



static struct i2c_board_info __initdata mxs_i2c_device[] = 
{
        { I2C_BOARD_INFO("pcf8563", 0x51),                                             },
	{ I2C_BOARD_INFO("qt602240_ts",  0x4B), .platform_data=&qt602240_platform_data },
	{ I2C_BOARD_INFO("sgtl5000-i2c", 0xa), .flags = I2C_M_TEN                      },
//	{ I2C_BOARD_INFO("maXTouch",     0x4B), .platform_data=&atmel_maxtouch_data    },

};
#else


static struct i2c_board_info __initdata mxs_i2c_device[] = 
{
  //{ I2C_BOARD_INFO("pcf8563", 0x51),                                             },
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH
#warning "Init Atmel MaxTouch"
        { I2C_BOARD_INFO("maXTouch",0x4B),.platform_data = &Atmel_mxt_info,.irq=0,     },
#endif
#if defined (CONFIG_TOUCHSCREEN_NOVATEK)
	{ I2C_BOARD_INFO(NOVATEK_I2C_NAME,0x01),.platform_data = &novatek_ts_info,.irq=0,     },
#endif
#if defined(CONFIG_TOUCHSCREEN_NT11003)
	{ I2C_BOARD_INFO("nt1103-ts",0x01),.platform_data = NULL,.irq=0,     },
#endif
	{ I2C_BOARD_INFO("sgtl5000-i2c", 0xa), .flags = I2C_M_TEN                      },
};

#endif

static void __init i2c_device_init(void)
{
	i2c_register_board_info(0, mxs_i2c_device, ARRAY_SIZE(mxs_i2c_device));
}

#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
static struct flash_platform_data mx28_spi_flash_data = {
	.name = "m25p80",
	.type = "w25x80",
};
#endif

static struct spi_board_info spi_board_info[] __initdata = {
#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
	{
		/* the modalias must be the same as spi device driver name */
		.modalias = "m25p80", /* Name of spi_driver for this device */
		.max_speed_hz = 20000000,     /* max spi clock (SCK) speed in HZ */
		.bus_num = 1, /* Framework bus number */
		.chip_select = 0, /* Framework chip select. */
		.platform_data = &mx28_spi_flash_data,
	},
#endif
};

static void spi_device_init(void)
{
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
}

static void __init fixup_board(struct machine_desc *desc, struct tag *tags,
			       char **cmdline, struct meminfo *mi)
{
	mx28_set_input_clk(24000000, 24000000, 32000, 50000000);
}

#if defined(CONFIG_LEDS_MXS) || defined(CONFIG_LEDS_MXS_MODULE)
#warning "LEDs ACTIVE!!!!"
static struct mxs_pwm_led  mx28evk_led_pwm[2] = {
	[0] = {
		.name = "led-pwm0",
		.pwm = 0,
		},
	[1] = {
		.name = "led-pwm1",
		.pwm = 1,
		},
};

struct mxs_pwm_leds_plat_data mx28evk_led_data = {
	.num = ARRAY_SIZE(mx28evk_led_pwm),
	.leds = mx28evk_led_pwm,
};

static struct resource mx28evk_led_res = {
	.flags = IORESOURCE_MEM,
	.start = PWM_PHYS_ADDR,
	.end   = PWM_PHYS_ADDR + 0x3FFF,
};

static void __init mx28evk_init_leds(void)
{
	struct platform_device *pdev;

	pdev = mxs_get_device("mxs-leds", 0);
	if (pdev == NULL || IS_ERR(pdev))
		return;

	pdev->resource = &mx28evk_led_res;
	pdev->num_resources = 1;
	pdev->dev.platform_data = &mx28evk_led_data;
	mxs_add_device(pdev, 3);
}
#else
static void __init mx28evk_init_leds(void)
{
	;
}
#endif

#if defined(CONFIG_TOUCHSCREEN_QT602240) 

static void __init qt602240_ts_init(void)
{
	int gpio_chg=0, gpio_ads=0;

        // Set Addr_SEL = LOW -> 0x4A
        //              = HI  -> 0x4B
        gpio_ads = gpio_request(GPIO_TOUCH_ADS, "TOUCH_ADS");
        gpio_direction_output(  GPIO_TOUCH_ADS, 1);
        gpio_set_value(         GPIO_TOUCH_ADS, 1);

	gpio_chg = gpio_request(GPIO_TOUCH_CHANGE , "TSP_INT");
        gpio_direction_input(   GPIO_TOUCH_CHANGE);

        mxs_i2c_device[1].irq = gpio_to_irq(GPIO_TOUCH_CHANGE);
	
#if 0
	gpio = S5PV210_GPJ1(3);		/* XMSMADDR_11 */
	gpio_request(gpio, "TSP_LDO_ON");
	gpio_direction_output(gpio, 1);
	gpio_export(gpio, 0);
	gpio = S5PV210_GPJ0(5);		/* XMSMADDR_5 */
	gpio_request(gpio, "TSP_INT");
	s5p_register_gpio_interrupt(gpio);
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	i2c2_devs[0].irq = gpio_to_irq(gpio);
#endif

}
#endif

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH
static void __init MaxTouch_ts_init(void)
{
	int gpio_chg=0, gpio_ads=0;

        // Set Addr_SEL = LOW -> 0x4A
        //              = HI  -> 0x4B
        gpio_ads = gpio_request(GPIO_TOUCH_ADS, "TOUCH_ADS");
        gpio_direction_output(  GPIO_TOUCH_ADS, 1);
        gpio_set_value(         GPIO_TOUCH_ADS, 1);

	gpio_chg = gpio_request(GPIO_TOUCH_CHANGE , "TSP_INT");
        gpio_direction_input(   GPIO_TOUCH_CHANGE);

        mxs_i2c_device[1].irq = gpio_to_irq(GPIO_TOUCH_CHANGE);
}
#endif

#if defined (CONFIG_TOUCHSCREEN_NOVATEK) || defined(CONFIG_TOUCHSCREEN_NT11003)
static void __init NovaTek_ts_init(void)
{
	int gpio_chg=0, gpio_ads=0;

        // Set Addr_SEL = LOW -> 0x4A
        //              = HI  -> 0x4B
        gpio_ads = gpio_request(GPIO_TOUCH_ADS, "TOUCH_ADS");
        gpio_direction_output(  GPIO_TOUCH_ADS, 1);
        gpio_set_value(         GPIO_TOUCH_ADS, 1);

	gpio_chg = gpio_request(GPIO_TOUCH_CHANGE , "TSP_INT");
        gpio_direction_input(   GPIO_TOUCH_CHANGE);
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH
        mxs_i2c_device[2].irq = gpio_to_irq(GPIO_TOUCH_CHANGE);
#else
        mxs_i2c_device[1].irq = gpio_to_irq(GPIO_TOUCH_CHANGE);
#endif
}
#endif


#ifdef CONFIG_INPUT_ALPMXS_ROTARY_ENCODER
static struct alpmxs_rotary_encoder_platform_data one_knob_rot_info = {
  .steps         = 24,
  .axis          = REL_X,
  .gpio_a1       = MXS_PIN_TO_GPIO(PINID_PWM0),
  .gpio_b1       = MXS_PIN_TO_GPIO(PINID_PWM1),
  .gpio_a2       = MXS_PIN_TO_GPIO(PINID_SSP2_SS0),
  .gpio_b2       = MXS_PIN_TO_GPIO(PINID_SSP2_MISO),
  .gpio_push     = MXS_PIN_TO_GPIO(PINID_ENET0_RXD1),
  .inverted_a    = 0,
  .inverted_b    = 0,
  .inverted_push = 0,
  .relative_axis = true,
  .rollover      = false,
  .base          = IO_ADDRESS(TIMROT_PHYS_ADDR),
};

static struct platform_device one_knob_rot_dev ={
  .name          = "alpmxs-rotary-encoder",
  .id            = 0,
  .dev           = {
     .platform_data = &one_knob_rot_info,
  }
};
#endif
#ifdef CONFIG_INPUT_MXS_ROTARY_ENCODER
static struct mxs_rotary_encoder_platform_data one_knob_rot_info = {
  .steps         = 24,
  .axis          = REL_X,
  .gpio_a        = MXS_PIN_TO_GPIO(PINID_PWM0),
  .gpio_b        = MXS_PIN_TO_GPIO(PINID_PWM1),
  .gpio_push     = MXS_PIN_TO_GPIO(PINID_ENET0_RXD1),
  .pin_idx_a     = 0x1,//Code for PWM0
  .pin_idx_b     = 0x2,//Code for PWM1
  .inverted_a    = 0,
  .inverted_b    = 0,
  .relative_axis = true,
  .rollover      = false,
  .base          = IO_ADDRESS(TIMROT_PHYS_ADDR),
};

static struct platform_device one_knob_rot_dev ={
  .name          = "mxs_rotary-encoder",
  .id            = 0,
  .dev           = {
     .platform_data = &one_knob_rot_info,
  }
};
#endif

static void __init mx28evk_device_init(void)
{
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH
	/* Add mx28evk special code */
        MaxTouch_ts_init();
#endif
#if defined (CONFIG_TOUCHSCREEN_NOVATEK) || defined(CONFIG_TOUCHSCREEN_NT11003)
        NovaTek_ts_init();
#endif
#if defined(CONFIG_TOUCHSCREEN_QT602240) 
#warning "CONFIG_TOUCHSCREEN_QT602240"
        qt602240_ts_init(); 
#endif
	i2c_device_init();
	spi_device_init();
	mx28evk_init_leds();
	
	//#if defined(CONFIG_TOUCHSCREEN_QT602240) 
	//#warning "CONFIG_TOUCHSCREEN_QT602240"
	//        qt602240_ts_init(); 
	//#endif

}



static void __init mx28evk_init_machine(void)
{
	mx28_pinctrl_init();
	/* Init iram allocate */
#ifdef CONFIG_VECTORS_PHY_ADDR
	/* reserve the first page for irq vector table*/
	iram_init(MX28_OCRAM_PHBASE + PAGE_SIZE, MX28_OCRAM_SIZE - PAGE_SIZE);
#else
	iram_init(MX28_OCRAM_PHBASE, MX28_OCRAM_SIZE);
#endif

	mx28_gpio_init();
	mx28evk_pins_init();
	mx28_device_init();
	mx28evk_device_init();
#if defined(CONFIG_INPUT_ALPMXS_ROTARY_ENCODER) || defined(CONFIG_INPUT_MXS_ROTARY_ENCODER)
	mxs_add_device(&one_knob_rot_dev,3);
#endif
}

MACHINE_START(MX28EVK, "Freescale MX28EVK board")
	.phys_io	= 0x80000000,
	.io_pg_offst	= ((0xf0000000) >> 18) & 0xfffc,
	.boot_params	= 0x40000100,
	.fixup		= fixup_board,
	.map_io		= mx28_map_io,
	.init_irq	= mx28_irq_init,
	.init_machine	= mx28evk_init_machine,
	.timer		= &mx28_timer.timer,
MACHINE_END
