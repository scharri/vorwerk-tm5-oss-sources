/*
 * alpmxs_rotary_encoder.c
 *
 * (c) 2012 Volker Greive <volker.greive@gmx.de>
 * (c) 2009 Daniel Mack <daniel@caiaq.de>
 *
 * state machine code inspired by code from Tim Ruetz
 *
 * A driver for alps rotary encoders connected to two i.MX28 GPIO lines.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/alpmxs_rotary_encoder.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include <mach/regs-timrot.h>
#include <linux/io.h>

#define DRV_NAME "alpmxs-rotary-encoder"


//set the key codes to be sent to input interface for push & long push
#define PUSH_KEY       KEY_STOP
#define LONGPUSH_KEY   KEY_POWER
//delay after which to detect a long push (in integer seconds)
#define LONGPUSH_DELAY 2


struct alpmxs_rotary_encoder {
	struct input_dev *input;
	struct alpmxs_rotary_encoder_platform_data *pdata;

	unsigned int axis;
	unsigned int pos;

	unsigned int irq_a1;
	unsigned int irq_b1;
	unsigned int irq_a2;
	unsigned int irq_b2;

        unsigned int irq_push;

        struct timer_list long_push_timer;

        unsigned int timestamp;
        unsigned char prev_state;
};


static void alpmxs_rotary_push_check_long(unsigned long ptr)
{
   struct alpmxs_rotary_encoder *encoder = (struct alpmxs_rotary_encoder*)ptr;
   struct alpmxs_rotary_encoder_platform_data *pdata = encoder->pdata;
   if( ! gpio_get_value(pdata->gpio_push) ) {
     input_report_key(encoder->input,LONGPUSH_KEY,1);
     input_report_key(encoder->input,LONGPUSH_KEY,0);
     input_sync(encoder->input);
   }
}

static irqreturn_t alpmxs_rotary_push_irq(int irq, void *dev_id)
{
   struct alpmxs_rotary_encoder *encoder = dev_id;
   struct alpmxs_rotary_encoder_platform_data *pdata = encoder->pdata;
   if( ! gpio_get_value(pdata->gpio_push) ) {
     del_timer_sync(&(encoder->long_push_timer));
     encoder->long_push_timer.expires = jiffies + (HZ*LONGPUSH_DELAY);
     add_timer(&(encoder->long_push_timer));
     input_report_key(encoder->input,PUSH_KEY,1);
     input_report_key(encoder->input,PUSH_KEY,0);
     input_sync(encoder->input);
   }
   return IRQ_HANDLED;
}

static irqreturn_t alpmxs_rotary_encoder_rel_irq(int irq, void *dev_id)
{
	struct alpmxs_rotary_encoder *encoder = dev_id;
	struct alpmxs_rotary_encoder_platform_data *pdata = encoder->pdata;

	int state = ((!!gpio_get_value(pdata->gpio_b1))<<1) | (!!gpio_get_value(pdata->gpio_a1));

	unsigned char clicks = 0;

	const unsigned int now = __raw_readl(pdata->base + HW_TIMROT_RUNNING_COUNTn(0));
	unsigned int diff = encoder->timestamp - now;
	diff = ((diff & (UINT_MAX^(UINT_MAX>>8))) ? (UINT_MAX^0xff) : (diff << 8));

	switch(encoder->prev_state) {
	case 0:
	  switch(state) {
	  case 1:
	    clicks = 0x01;
	    break;
	  case 2:
	    clicks = 0xff;
	    break;
	  default:
	    printk(KERN_ERR "%s: hit default, (encoder->prev_state %i, state %i, irq %i)!\n",__func__,encoder->prev_state,state,irq);
	    break;
	  }
	  break;
	case 1:
	  switch(state) {
	  case 0:
	    clicks = 0xff;
	    break;
	  case 3:
	    clicks = 0x01;
	    break;
	  default:
	    printk(KERN_ERR "%s: hit default, (encoder->prev_state %i, state %i, irq %i)!\n",__func__,encoder->prev_state,state,irq);
	    break;
	  }
	  break;
	case 2:
	  switch(state) {
	  case 0:
	    clicks = 0x01;
	    break;
	  case 3:
	    clicks = 0xff;
	    break;
	  default:
	    printk(KERN_ERR "%s: hit default, (encoder->prev_state %i, state %i, irq %i)!\n",__func__,encoder->prev_state,state,irq);
	    break;
	  }
	  break;
	case 3:
	  switch(state) {
	  case 1:
	    clicks = 0xff;
	    break;
	  case 2:
	    clicks = 0x01;
	    break;
	  default:
	    printk(KERN_ERR "%s: hit default, (encoder->prev_state %i, state %i, irq %i)!\n",__func__,encoder->prev_state,state,irq);
	    break;
	  }
	  break;
	default:
	  printk(KERN_ERR "%s: hit default, (encoder->prev_state %i, state %i, irq %i)!\n",__func__,encoder->prev_state,state,irq);
	  break;
	}

	if( clicks ) {
	  input_report_rel(encoder->input, pdata->axis, diff | clicks);
	  input_sync(encoder->input);
	}

	encoder->timestamp  = now; //FIXME: move that into if-statement above?
	encoder->prev_state = state;

	return IRQ_HANDLED;
}


static int __devinit alpmxs_rotary_encoder_probe(struct platform_device *pdev)
{
	struct alpmxs_rotary_encoder_platform_data *pdata = pdev->dev.platform_data;
	struct alpmxs_rotary_encoder *encoder;
	struct input_dev *input;
	int err;

	printk(KERN_ERR "Probing ALPMXS driver");

	if (!pdata) {
		dev_err(&pdev->dev, "missing platform data\n");
		return -ENOENT;
	}

	encoder = kzalloc(sizeof(struct alpmxs_rotary_encoder), GFP_KERNEL);
	input = input_allocate_device();
	if (!encoder || !input) {
		dev_err(&pdev->dev, "failed to allocate memory for device\n");
		err = -ENOMEM;
		goto exit_free_mem;
	}

	encoder->prev_state = ((!!gpio_get_value(pdata->gpio_b1))<<1) | (!!gpio_get_value(pdata->gpio_a1));

	encoder->input = input;
	encoder->pdata = pdata;
	encoder->irq_a1 = gpio_to_irq(pdata->gpio_a1);
	encoder->irq_b1 = gpio_to_irq(pdata->gpio_b1);
	encoder->irq_a2 = gpio_to_irq(pdata->gpio_a2);
	encoder->irq_b2 = gpio_to_irq(pdata->gpio_b2);



	/* the push button irq */
	encoder->irq_push  = gpio_to_irq(pdata->gpio_push);
	
	/* kernel timer to detect a long push */
	init_timer(&(encoder->long_push_timer));
	encoder->long_push_timer.function = alpmxs_rotary_push_check_long;
	encoder->long_push_timer.data = (unsigned long)encoder;
  
	encoder->timestamp = 0;


	/* create and register the input driver */
	input->name = pdev->name;
	input->id.bustype = BUS_HOST;
	input->dev.parent = &pdev->dev;

	if (pdata->relative_axis) {
		input->evbit[0] = BIT_MASK(EV_REL);
		input->relbit[0] = BIT_MASK(pdata->axis);
	} else {
		input->evbit[0] = BIT_MASK(EV_ABS);
		input_set_abs_params(encoder->input,
				     pdata->axis, 0, pdata->steps, 0, 1);
	}
	input->evbit[0] |= BIT_MASK(EV_KEY);
	set_bit(PUSH_KEY,     input->keybit);
	set_bit(LONGPUSH_KEY, input->keybit);

	err = input_register_device(input);
	if (err) {
		dev_err(&pdev->dev, "failed to register input device\n");
		goto exit_free_mem;
	}

	/* request the IRQs */
	err = request_irq(encoder->irq_a1, &alpmxs_rotary_encoder_rel_irq,
			  (pdata->inverted_a ? IORESOURCE_IRQ_HIGHEDGE : IORESOURCE_IRQ_LOWEDGE)
			  | IRQF_SAMPLE_RANDOM,
			  DRV_NAME, encoder);
	if (err) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n",
			encoder->irq_a1);
		goto exit_free_irq_a1;
	}

	err = request_irq(encoder->irq_b1, &alpmxs_rotary_encoder_rel_irq,
			  (pdata->inverted_b ? IORESOURCE_IRQ_HIGHEDGE : IORESOURCE_IRQ_LOWEDGE)
			  | IRQF_SAMPLE_RANDOM,
			  DRV_NAME, encoder);
	if (err) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n",
			encoder->irq_b1);
		goto exit_free_irq_b1;
	}

	err = request_irq(encoder->irq_a2, &alpmxs_rotary_encoder_rel_irq,
			  (pdata->inverted_a ? IORESOURCE_IRQ_LOWEDGE : IORESOURCE_IRQ_HIGHEDGE)
			  | IRQF_SAMPLE_RANDOM,
			  DRV_NAME, encoder);
	if (err) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n",
			encoder->irq_a2);
		goto exit_free_irq_a2;
	}

	err = request_irq(encoder->irq_b2, &alpmxs_rotary_encoder_rel_irq,
			  (pdata->inverted_b ? IORESOURCE_IRQ_LOWEDGE : IORESOURCE_IRQ_HIGHEDGE)
			  | IRQF_SAMPLE_RANDOM,
			  DRV_NAME, encoder);
	if (err) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n",
			encoder->irq_b2);
		goto exit_free_irq_b2;
	}


	err = request_irq(encoder->irq_push, &alpmxs_rotary_push_irq,
			  (pdata->inverted_push ? IORESOURCE_IRQ_HIGHEDGE : IORESOURCE_IRQ_LOWEDGE)
			  | IRQF_SAMPLE_RANDOM,
			  DRV_NAME, encoder);
	if (err) {
	  dev_err(&pdev->dev, "unable to request IRQ %d\n",
		  encoder->irq_push);
	  goto exit_free_irq_push;
	}




	platform_set_drvdata(pdev, encoder);

	return 0;
exit_free_irq_push:
	free_irq(encoder->irq_push, encoder);
exit_free_irq_b2:
	free_irq(encoder->irq_b2, encoder);
exit_free_irq_a2:
	free_irq(encoder->irq_a2, encoder);
exit_free_irq_b1:
	free_irq(encoder->irq_b1, encoder);
exit_free_irq_a1:
	free_irq(encoder->irq_a1, encoder);

//exit_unregister_input:
	input_unregister_device(input);
	input = NULL; /* so we don't try to free it */
exit_free_mem:
	input_free_device(input);
	kfree(encoder);
	return err;
}

static int __devexit alpmxs_rotary_encoder_remove(struct platform_device *pdev)
{
	struct alpmxs_rotary_encoder *encoder = platform_get_drvdata(pdev);
	struct alpmxs_rotary_encoder_platform_data *pdata = pdev->dev.platform_data;

	free_irq(encoder->irq_a1, encoder);
	free_irq(encoder->irq_b1, encoder);
	free_irq(encoder->irq_a2, encoder);
	free_irq(encoder->irq_b2, encoder);
	free_irq(encoder->irq_push, encoder);
	gpio_free(pdata->gpio_a1);
	gpio_free(pdata->gpio_b1);
	gpio_free(pdata->gpio_a2);
	gpio_free(pdata->gpio_b2);
	gpio_free(pdata->gpio_push);
	input_unregister_device(encoder->input);
	platform_set_drvdata(pdev, NULL);
	kfree(encoder);

	return 0;
}

static struct platform_driver alpmxs_rotary_encoder_driver = {
	.probe		= alpmxs_rotary_encoder_probe,
	.remove		= __devexit_p(alpmxs_rotary_encoder_remove),
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	}
};

static int __init alpmxs_rotary_encoder_init(void)
{
	return platform_driver_register(&alpmxs_rotary_encoder_driver);
}

static void __exit alpmxs_rotary_encoder_exit(void)
{
	platform_driver_unregister(&alpmxs_rotary_encoder_driver);
}

module_init(alpmxs_rotary_encoder_init);
module_exit(alpmxs_rotary_encoder_exit);

MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DESCRIPTION("GPIO rotary encoder driver");
MODULE_AUTHOR("Volker Greive <volker.greive@gmx.de>");
MODULE_LICENSE("GPL v2");

