/*
 * Freescale MXS PWM LED driver
 *
 * Author: Drew Benedetti <drewb@embeddedalley.com>
 *
 * Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/clk.h>

#include <mach/hardware.h>
#include <mach/system.h>
#include <mach/device.h>
#include <mach/regs-pwm.h>


/*
 * PWM enables are the lowest bits of HW_PWM_CTRL register
 */
#define BM_PWM_CTRL_PWM_ENABLE	((1<<(CONFIG_MXS_PWM_CHANNELS)) - 1)
#define BF_PWM_CTRL_PWM_ENABLE(n) ((1<<(n)) & BM_PWM_CTRL_PWM_ENABLE)

#define BF_PWM_PERIODn_SETTINGS					\
		(BF_PWM_PERIODn_CDIV(5) | /* divide by 64 */ 	\
		BF_PWM_PERIODn_INACTIVE_STATE(3) | /* low */ 	\
		BF_PWM_PERIODn_ACTIVE_STATE(2) | /* high */ 	\
		BF_PWM_PERIODn_PERIOD(LED_FULL)) /* 255 cycles */

struct mxs_pwm_leds {
	struct clk *pwm_clk;
	unsigned int base;
	unsigned int led_num;
	struct mxs_pwm_led *leds;
};

static struct mxs_pwm_leds leds;

static void mxs_pwm_led_brightness_set(struct led_classdev *pled,
					    enum led_brightness value)
{
	struct mxs_pwm_led *pwm_led;
	//	printk("Setting PWM LED %s brightness to %i, leds are on %08X", pled->name, value,leds.base);
	pwm_led = container_of(pled, struct mxs_pwm_led, dev);

	if (pwm_led->pwm < CONFIG_MXS_PWM_CHANNELS) {
		__raw_writel(BF_PWM_CTRL_PWM_ENABLE(pwm_led->pwm),
			     leds.base + HW_PWM_CTRL_CLR);
		__raw_writel(BF_PWM_ACTIVEn_INACTIVE(LED_FULL) |
				BF_PWM_ACTIVEn_ACTIVE(value),
			     leds.base + HW_PWM_ACTIVEn(pwm_led->pwm));
		__raw_writel(BF_PWM_PERIODn_SETTINGS,
			     leds.base + HW_PWM_PERIODn(pwm_led->pwm));
		__raw_writel(BF_PWM_CTRL_PWM_ENABLE(pwm_led->pwm),
			     leds.base + HW_PWM_CTRL_SET);
	}
}

static int mxs_pwm_on(int pwm)
{
	int reg;

	if (pwm >= CONFIG_MXS_PWM_CHANNELS)
		return -EINVAL;

	__raw_writel(BF_PWM_ACTIVEn_INACTIVE(255) | BF_PWM_ACTIVEn_ACTIVE(0),
		     leds.base + HW_PWM_ACTIVEn(pwm));

	reg = (BF_PWM_PERIODn_CDIV(0) /* divide by 1 */
		| BF_PWM_PERIODn_INACTIVE_STATE(3) /* low (inv) */
		| BF_PWM_PERIODn_ACTIVE_STATE(2) /* high (inv) */
		| BF_PWM_PERIODn_PERIOD(255)); /* period + 1 cycles */

	__raw_writel(reg,
		     leds.base + HW_PWM_PERIODn(pwm));
	__raw_writel(BF_PWM_CTRL_PWM_ENABLE(pwm),
		     leds.base + HW_PWM_CTRL_SET);
	return 0;
}
EXPORT_SYMBOL_GPL(mxs_pwm_on);

static int mxs_pwm_off(int pwm)
{
	if (pwm >= CONFIG_MXS_PWM_CHANNELS)
		return -EINVAL;

	__raw_writel(BF_PWM_CTRL_PWM_ENABLE(pwm),
		     leds.base + HW_PWM_CTRL_CLR);
	return 0;
}
EXPORT_SYMBOL_GPL(mxs_pwm_off);

static int mxs_pwm_config(int pwm, int period, int duty_cycle)
{
	u32 reg;

	if (pwm >= CONFIG_MXS_PWM_CHANNELS || duty_cycle > period)
		return -EINVAL;

	__raw_writel(BF_PWM_ACTIVEn_INACTIVE(LED_FULL) |
			BF_PWM_ACTIVEn_ACTIVE(duty_cycle),
		     leds.base + HW_PWM_ACTIVEn(pwm));

	reg = (BF_PWM_PERIODn_CDIV(0) /* divide by 1 */
		| BF_PWM_PERIODn_INACTIVE_STATE(3) /* low (inv) */
		| BF_PWM_PERIODn_ACTIVE_STATE(2) /* high (inv) */
		| BF_PWM_PERIODn_PERIOD(period - 1)); /* period + 1 cycles */

	__raw_writel(reg,
		     leds.base + HW_PWM_PERIODn(pwm));
	return 0;
}
EXPORT_SYMBOL_GPL(mxs_pwm_config);

#if 0
static int mxs_pwm_set_duty_cycle(int pwm, int duty_cycle)
{
	if (pwm >= CONFIG_MXS_PWM_CHANNELS)
		return -EINVAL;

	__raw_writel(BF_PWM_ACTIVEn_INACTIVE(LED_FULL) |
			BF_PWM_ACTIVEn_ACTIVE(duty_cycle),
		     leds.base + HW_PWM_ACTIVEn(pwm));
	__raw_writel(BF_PWM_CTRL_PWM_ENABLE(pwm),
		     leds.base + HW_PWM_CTRL_SET);
	return 0;
}
EXPORT_SYMBOL_GPL(mxs_pwm_set_duty_cycle);
#endif

static int __devinit mxs_pwm_led_probe(struct platform_device *pdev)
{
	struct mxs_pwm_leds_plat_data *plat_data;
	struct resource *res;
	struct led_classdev *led;
	unsigned int pwmn;
	int leds_in_use = 0, rc = 0;
	int i;

	plat_data = (struct mxs_pwm_leds_plat_data *)pdev->dev.platform_data;
	if (plat_data == NULL)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL)
		return -ENODEV;
	leds.base = (unsigned int)IO_ADDRESS(res->start);

	mxs_reset_block((void __iomem *)leds.base, 1);

	leds.led_num = plat_data->num;
	if (leds.led_num <= 0 || leds.led_num > CONFIG_MXS_PWM_CHANNELS)
		return -EFAULT;
	leds.leds = plat_data->leds;
	if (leds.leds == NULL)
		return -EFAULT;

	leds.pwm_clk = clk_get(&pdev->dev, "pwm");
	if (IS_ERR(leds.pwm_clk)) {
		rc = PTR_ERR(leds.pwm_clk);
		return rc;
	}

	clk_enable(leds.pwm_clk);

	for (i = 0; i < leds.led_num; i++) {
		pwmn = leds.leds[i].pwm;
		if (! strncmp("led-", leds.leds[i].name, 4) == 0) {
			dev_info(&pdev->dev,
				"[led-pwm%d]: skipping %s.\n",
				i, leds.leds[i].name);
			continue;
		}
		if (pwmn >= CONFIG_MXS_PWM_CHANNELS) {
			dev_err(&pdev->dev,
				"[led-pwm%d]:PWM %d doesn't exist\n",
				i, pwmn);
			continue;
		}
		led = &(leds.leds[i].dev);
		led->name = leds.leds[i].name;
// ===================== VG20110616: Evil hack to change "wrong" led init (pwm is actually connected to display)
#if defined(CONFIG_ARCH_MX28) && defined(CONFIG_FB_MXS_LCD_LMS430)
		led->brightness = pwmn == 1 ? LED_FULL : LED_HALF;
#else
		led->brightness = LED_HALF;
#endif
// ===================== EO Hack
		led->flags = 0;
		led->brightness_set = mxs_pwm_led_brightness_set;
		led->default_trigger = 0;

		rc = led_classdev_register(&pdev->dev, led);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"Unable to register LED device %d (err=%d)\n",
				i, rc);
			continue;
		}

		leds_in_use++;

		/* Set default brightness */
// ===================== VG20110616: Evil hack to change "wrong" led init (pwm is actually connected to display)
#if defined(CONFIG_ARCH_MX28) && defined(CONFIG_FB_MXS_LCD_LMS430)
		mxs_pwm_led_brightness_set(led, pwmn == 1 ? LED_FULL : LED_HALF);
#else
		mxs_pwm_led_brightness_set(led, LED_HALF);
#endif
// ===================== EO Hack
	}

	if (leds_in_use == 0) {
		dev_info(&pdev->dev, "No PWM LEDs available\n");
		clk_disable(leds.pwm_clk);
		clk_put(leds.pwm_clk);
		return -ENODEV;
	}
	return 0;
}

static int __devexit mxs_pwm_led_remove(struct platform_device *pdev)
{
	int i;
	unsigned int pwm;
	for (i = 0; i < leds.led_num; i++) {
		pwm = leds.leds[i].pwm;
		__raw_writel(BF_PWM_CTRL_PWM_ENABLE(pwm),
			     leds.base + HW_PWM_CTRL_CLR);
		__raw_writel(BF_PWM_ACTIVEn_INACTIVE(0) |
				BF_PWM_ACTIVEn_ACTIVE(0),
			     leds.base + HW_PWM_ACTIVEn(pwm));
		__raw_writel(BF_PWM_PERIODn_SETTINGS,
			     leds.base + HW_PWM_PERIODn(pwm));
		led_classdev_unregister(&leds.leds[i].dev);
	}

	clk_disable(leds.pwm_clk);
	clk_put(leds.pwm_clk);

	return 0;
}

#ifdef CONFIG_PM
static int mxs_led_suspend(struct platform_device *dev, pm_message_t state)
{
	int i;

	for (i = 0; i < leds.led_num; i++)
		led_classdev_suspend(&leds.leds[i].dev);
	return 0;
}

static int mxs_led_resume(struct platform_device *dev)
{
	int i;

	for (i = 0; i < leds.led_num; i++)
		led_classdev_resume(&leds.leds[i].dev);
	return 0;
}
#else
#define mxs_led_suspend NULL
#define mxs_led_resume NULL
#endif


static struct platform_driver mxs_pwm_led_driver = {
	.probe   = mxs_pwm_led_probe,
	.remove  = __devexit_p(mxs_pwm_led_remove),
	.suspend = mxs_led_suspend,
	.resume = mxs_led_resume,
	.driver  = {
		.name = "mxs-leds",
	},
};

static int __init mxs_pwm_led_init(void)
{
	return platform_driver_register(&mxs_pwm_led_driver);
}

static void __exit mxs_pwm_led_exit(void)
{
	platform_driver_unregister(&mxs_pwm_led_driver);
}

module_init(mxs_pwm_led_init);
module_exit(mxs_pwm_led_exit);

MODULE_AUTHOR("Drew Benedetti <drewb@embeddedalley.com>");
MODULE_DESCRIPTION("mxs PWM LED driver");
MODULE_LICENSE("GPL");
