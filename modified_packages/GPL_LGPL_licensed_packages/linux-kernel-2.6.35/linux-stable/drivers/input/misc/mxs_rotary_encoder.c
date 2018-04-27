/*
 * mxs_rotary_encoder.c
 *
 * (c) 2012 Volker Greive <volker.greive@gmx.de>
 * (c) 2009 Daniel Mack <daniel@caiaq.de>
 *
 * A generic driver for rotary encoders connected to i.MX rotary register.
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
#include <linux/mxs_rotary_encoder.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include <mach/regs-timrot.h>
#include <linux/io.h>

#define DRV_NAME "mxs_rotary-encoder"

//set the key codes to be sent to input interface for push & long push
#define PUSH_KEY       KEY_STOP
#define LONGPUSH_KEY   KEY_POWER
//delay after which to detect a long push (in integer seconds)
#define LONGPUSH_DELAY 2


struct mxs_rotary_encoder {
  struct input_dev *input;
  struct mxs_rotary_encoder_platform_data *pdata;

  unsigned int axis;
  unsigned int pos;

  unsigned int irq_rot_a;
  unsigned int irq_rot_b;

  unsigned int irq_push;

  struct timer_list long_push_timer;

  unsigned int timestamp;

#ifdef CONFIG_INPUT_MXS_ROTARY_ENCODER_1ST_CLICK_HACK
#warning "Code uses evil CONFIG_INPUT_MXS_ROTARY_ENCODER_1ST_CLICK_HACK!!!\n"
  bool a_saw_low;
  bool b_saw_low;
#endif
};

void mxs_rotary_push_check_long(unsigned long ptr)
{
   struct mxs_rotary_encoder *encoder = (struct mxs_rotary_encoder*)ptr;
   struct mxs_rotary_encoder_platform_data *pdata = encoder->pdata;
   if( ! gpio_get_value(pdata->gpio_push) ) {
     input_report_key(encoder->input,LONGPUSH_KEY,1);
     input_report_key(encoder->input,LONGPUSH_KEY,0);
     input_sync(encoder->input);
   }
}

static irqreturn_t mxs_rotary_push_irq(int irq, void *dev_id)
{
   struct mxs_rotary_encoder *encoder = dev_id;
   struct mxs_rotary_encoder_platform_data *pdata = encoder->pdata;
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


static irqreturn_t mxs_rotary_encoder_rel_irq(int irq, void *dev_id)
{
  struct mxs_rotary_encoder *encoder = dev_id;
  struct mxs_rotary_encoder_platform_data *pdata = encoder->pdata;
  int ticks = 0;
  disable_irq_nosync(irq);
  ticks = (signed short)BF_TIMROT_ROTCOUNT_UPDOWN(__raw_readl(pdata->base + HW_TIMROT_ROTCOUNT));
#ifdef CONFIG_INPUT_MXS_ROTARY_ENCODER_1ST_CLICK_HACK
  if( ! (encoder->a_saw_low && encoder->b_saw_low ) ) {
    if(irq == encoder->irq_rot_a ) {
      if( encoder->b_saw_low ) {
	ticks--;
      }
      encoder->a_saw_low = true;
    } else if( irq == encoder->irq_rot_b ) {
      if( encoder->a_saw_low ) {
	ticks++;
      }
      encoder->b_saw_low = true;
    }
  }
#endif
  //printk("Hit handler ticks %i, irq %i!\n",ticks,irq);
  if( ticks ) {
    unsigned int diff, now;

    //   disable_irq_nosync(irq);

    now = __raw_readl(pdata->base + HW_TIMROT_RUNNING_COUNTn(0)); //FIXME: might want to use own timer for that instead of global clock...
    diff = encoder->timestamp - now;
    diff = ((diff & (UINT_MAX^(UINT_MAX>>8))) ? (UINT_MAX^0xff) : (diff << 8)) | (ticks & 0xff);
    encoder->timestamp = now;
 
    input_report_rel(encoder->input, pdata->axis, diff);

    //  enable_irq(irq);
  }
  enable_irq(irq);
  return IRQ_HANDLED;
}

static irqreturn_t mxs_rotary_encoder_abs_irq(int irq, void *dev_id)
{
  struct mxs_rotary_encoder *encoder = dev_id;
  struct mxs_rotary_encoder_platform_data *pdata = encoder->pdata;
  int new_pos;
  new_pos = (signed short)BF_TIMROT_ROTCOUNT_UPDOWN(__raw_readl(pdata->base + HW_TIMROT_ROTCOUNT));
  if( pdata->rollover ) {
    new_pos %= pdata->steps;
  }
  if( new_pos != encoder->pos ) {
    unsigned int diff, now;

    disable_irq_nosync(irq);

    now = __raw_readl(pdata->base + HW_TIMROT_RUNNING_COUNTn(0)); //FIXME: might want to use own timer for that instead of global clock...
    diff = encoder->timestamp - now;
    //use the full 16 bits for rotary value here, as it is absolute; what happens when ROTCOUNT register overflows!?
    diff = ((diff & (UINT_MAX^(UINT_MAX>>16))) ? (UINT_MAX^0xffff) : (diff << 16)) | (new_pos & 0xffff);
    encoder->timestamp = now;

    input_report_abs(encoder->input, pdata->axis, diff);

    enable_irq(irq);
  }
  return IRQ_HANDLED;
}

static int __devinit mxs_rotary_encoder_probe(struct platform_device *pdev)
{
  struct mxs_rotary_encoder_platform_data *pdata = pdev->dev.platform_data;
  struct mxs_rotary_encoder *encoder;
  struct input_dev *input;
  int err;
  
  if (!pdata) {
    dev_err(&pdev->dev, "missing platform data\n");
    return -ENOENT;
  }
  
  /* set up rotary control register */
  __raw_writel((BM_TIMROT_ROTCTRL_ROTARY_PRESENT                               |
		(pdata->relative_axis ?  BM_TIMROT_ROTCTRL_RELATIVE  : 0)      |
		BF_TIMROT_ROTCTRL_OVERSAMPLE(BV_TIMROT_ROTCTRL_OVERSAMPLE__8X) |
		(pdata->inverted_b    ? BM_TIMROT_ROTCTRL_POLARITY_B : 0)      |
		(pdata->inverted_a    ? BM_TIMROT_ROTCTRL_POLARITY_A : 0)      |
		BF_TIMROT_ROTCTRL_SELECT_B(pdata->pin_idx_b)                   |
		BF_TIMROT_ROTCTRL_SELECT_A(pdata->pin_idx_a)                   )
	       ,pdata->base + HW_TIMROT_ROTCTRL_SET);

  encoder = kzalloc(sizeof(struct mxs_rotary_encoder), GFP_KERNEL);
  input = input_allocate_device();
  if (!encoder || !input) {
    dev_err(&pdev->dev, "failed to allocate memory for device\n");
    err = -ENOMEM;
    goto exit_free_mem;
  }

  encoder->input = input;
  encoder->pdata = pdata;
  /* irqs for the rotary function: just trigger the readout of the
     counter register, no state machine is implemented based on them! */
  encoder->irq_rot_a = gpio_to_irq(pdata->gpio_a);
  encoder->irq_rot_b = gpio_to_irq(pdata->gpio_b);

  /* the push button irq */
  encoder->irq_push  = gpio_to_irq(pdata->gpio_push);

  /* kernel timer to detect a long push */
  init_timer(&(encoder->long_push_timer));
  encoder->long_push_timer.function = mxs_rotary_push_check_long;
  encoder->long_push_timer.data = (unsigned long)encoder;
  
  encoder->timestamp = 0;

#ifdef CONFIG_INPUT_MXS_ROTARY_ENCODER_1ST_CLICK_HACK
  encoder->a_saw_low = false;
  encoder->b_saw_low = false;
#endif

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
  err = request_irq(encoder->irq_rot_a, pdata->relative_axis ? &mxs_rotary_encoder_rel_irq :  &mxs_rotary_encoder_abs_irq,
		    (pdata->inverted_a ? IORESOURCE_IRQ_LOWEDGE :
		                         IORESOURCE_IRQ_HIGHEDGE   ) | IRQF_SAMPLE_RANDOM,
		    DRV_NAME, encoder);
  if (err) {
    dev_err(&pdev->dev, "unable to request IRQ %d\n",
	    encoder->irq_rot_a);
    goto exit_unregister_input;
  }

  err = request_irq(encoder->irq_rot_b, pdata->relative_axis ? &mxs_rotary_encoder_rel_irq :  &mxs_rotary_encoder_abs_irq,
		    (pdata->inverted_b ? IORESOURCE_IRQ_LOWEDGE :
		                         IORESOURCE_IRQ_HIGHEDGE   ) | IRQF_SAMPLE_RANDOM,
		    DRV_NAME, encoder);
  if (err) {
    dev_err(&pdev->dev, "unable to request IRQ %d\n",
	    encoder->irq_rot_b);
    goto exit_free_irq_rot_a;
  }

  err = request_irq(encoder->irq_push, &mxs_rotary_push_irq,
		    IORESOURCE_IRQ_LOWEDGE | IRQF_SAMPLE_RANDOM,
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
 exit_free_irq_rot_a:
  free_irq(encoder->irq_rot_a, encoder);
 exit_unregister_input:
  input_unregister_device(input);
  input = NULL; /* so we don't try to free it */
 exit_free_mem:
  input_free_device(input);
  kfree(encoder);
  return err;
}

static int __devexit mxs_rotary_encoder_remove(struct platform_device *pdev)
{
  struct mxs_rotary_encoder *encoder = platform_get_drvdata(pdev);

  free_irq(encoder->irq_rot_a, encoder);
  free_irq(encoder->irq_rot_b, encoder);
  free_irq(encoder->irq_push, encoder);
  input_unregister_device(encoder->input);
  platform_set_drvdata(pdev, NULL);
  kfree(encoder);

  return 0;
}

static struct platform_driver mxs_rotary_encoder_driver = {
  .probe		= mxs_rotary_encoder_probe,
  .remove		= __devexit_p(mxs_rotary_encoder_remove),
  .driver		= {
    .name	= DRV_NAME,
    .owner	= THIS_MODULE,
  }
};

static int __init mxs_rotary_encoder_init(void)
{
  return platform_driver_register(&mxs_rotary_encoder_driver);
}

static void __exit mxs_rotary_encoder_exit(void)
{
  platform_driver_unregister(&mxs_rotary_encoder_driver);
}

module_init(mxs_rotary_encoder_init);
module_exit(mxs_rotary_encoder_exit);

MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DESCRIPTION("MXS rotary encoder driver");
MODULE_AUTHOR("Volker Greive <volker.greive@gmx.de>");
MODULE_LICENSE("GPL v2");

