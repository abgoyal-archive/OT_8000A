/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 * Copyright 2012 TCL Communication Technology Holdings Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *         Modify History For This Module
 * When           Who             What,Where,Why
 * -----------------------------------------------------------------------------
 * 12/10/31     WQF            Add flash led control for scribe5
 * 12/09/28     WQF            Add leds control for scribe5
 * -----------------------------------------------------------------------------
 */


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <mach/pmic.h>
/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/9/28, Add leds control for scribe5*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
#include <mach/rpc_pmapp.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/

#define LED_MPP(x)		((x) & 0xFF)
#define LED_CURR(x)		((x) >> 16)

struct pmic_mpp_led_data {
	struct led_classdev cdev;
	int curr;
	int mpp;
};

/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2012/11/08, Add flash led brightness control for scribe5*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
static void flash_led_brightness_set(int gpio, int pulse)
{

	int i = 0;
	int inc_pulse;
	static int prev_pulse = 0;

	if (pulse < 0 || pulse > 16)
	{
		pr_err("%s: The bl level is out of range\n", __func__);
		return;
	}

	if (pulse == 0)
	{
		gpio_set_value(gpio, 0);	//turn off the backlight
		msleep(5);
		prev_pulse = 0;
		return;
	}

	if(pulse < prev_pulse)
	{
		inc_pulse = 16 + pulse -prev_pulse;
	} else {
		inc_pulse = pulse - prev_pulse;
	}

	//printk("%s: level= %d, prev_pulse=%d, pulse =%d, inc_pulse = %d\n", __func__, level, prev_pulse, pulse, inc_pulse);
	for (i = 0; i < inc_pulse; i++){
		gpio_set_value(gpio, 0);
		udelay(1);
		gpio_set_value(gpio, 1);
		if (prev_pulse == 0) //The first pulse when turn on the bl must > 30US
		{
			udelay(100);
		} else {
			udelay(1);
		}
	}

	prev_pulse = pulse;
	return;
}
#endif
/*[PLATFORM]-Add-END by TCTNB.WQF*/

static void pm_mpp_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct pmic_mpp_led_data *led;
/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/9/28, Add leds control for scribe5*/
	int ret = 0;
/*[PLATFORM]-Mod-END by TCTNB.WQF*/

	led = container_of(led_cdev, struct pmic_mpp_led_data, cdev);
	if (value < LED_OFF || value > led->cdev.max_brightness) {
		dev_err(led->cdev.dev, "Invalid brightness value");
		return;
	}

/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/9/28, Add leds control for scribe5*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
       if (strcmp(led->cdev.name, "led_green") == 0)
       {
              if (value == LED_OFF)
              {
                     ret = pmapp_disp_backlight_set_brightness(0);
              }
		else if (value == LED_FULL)
              {
                     ret = pmapp_disp_backlight_set_brightness(1);
              }
	       else if (value == LED_HALF)
              {
                     ret = pmapp_disp_backlight_set_brightness(64);
              }
       }
       else if (strcmp(led->cdev.name, "led_red") == 0)
       {
              if (value == LED_OFF)
              {
                     ret = pmapp_disp_backlight_set_brightness(0);
              }
		else if (value == LED_FULL)
              {
                     ret = pmapp_disp_backlight_set_brightness(2);
              }
	       else if (value == LED_HALF)
              {
                     ret = pmapp_disp_backlight_set_brightness(128);
              }
       }
       else if (strcmp(led->cdev.name, "flash_led") == 0)
       {
              if (value == LED_OFF)
              {
                     flash_led_brightness_set(led->mpp, 0);
              }
              else if (value == 1)
              {
                     flash_led_brightness_set(led->mpp, 15);
              }
              else if (value == 2)
              {
                     flash_led_brightness_set(led->mpp, 8);
              }
              else if (value == 3)
              {
                     flash_led_brightness_set(led->mpp, 1);
              }
       }
#else
	ret = pmic_secure_mpp_config_i_sink(led->mpp, led->curr,
			value ? PM_MPP__I_SINK__SWITCH_ENA :
				PM_MPP__I_SINK__SWITCH_DIS);
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/
	if (ret)
		dev_err(led_cdev->dev, "can't set mpp led\n");
}

static int pmic_mpp_led_probe(struct platform_device *pdev)
{
	const struct led_platform_data *pdata = pdev->dev.platform_data;
	struct pmic_mpp_led_data *led, *tmp_led;
	int i, rc;

/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/9/28, Add leds control for scribe5*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	pmapp_disp_backlight_init();
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/

	if (!pdata) {
		dev_err(&pdev->dev, "platform data not supplied\n");
		return -EINVAL;
	}

	led = kcalloc(pdata->num_leds, sizeof(*led), GFP_KERNEL);
	if (!led) {
		dev_err(&pdev->dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, led);

	for (i = 0; i < pdata->num_leds; i++) {
		tmp_led	= &led[i];
		tmp_led->cdev.name = pdata->leds[i].name;
		tmp_led->cdev.brightness_set = pm_mpp_led_set;
		tmp_led->cdev.brightness = LED_OFF;
		tmp_led->cdev.max_brightness = LED_FULL;
		tmp_led->mpp = LED_MPP(pdata->leds[i].flags);
		tmp_led->curr = LED_CURR(pdata->leds[i].flags);

		if (tmp_led->curr < PM_MPP__I_SINK__LEVEL_5mA ||
			tmp_led->curr > PM_MPP__I_SINK__LEVEL_40mA) {
			dev_err(&pdev->dev, "invalid current\n");
			goto unreg_led_cdev;
		}

		rc = led_classdev_register(&pdev->dev, &tmp_led->cdev);
		if (rc) {
			dev_err(&pdev->dev, "failed to register led\n");
			goto unreg_led_cdev;
		}

/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2012/10/31, Add flash led control for scribe5*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
              if (strcmp(tmp_led->cdev.name, "flash_led") == 0)
              {
                     gpio_request(tmp_led->mpp, "flash_led");
                     gpio_direction_output(tmp_led->mpp, 0);
              }
#endif
/*[PLATFORM]-Add-END by TCTNB.WQF*/
	}

	return 0;

unreg_led_cdev:
	while (i)
		led_classdev_unregister(&led[--i].cdev);

	kfree(led);
	return rc;

}

static int __devexit pmic_mpp_led_remove(struct platform_device *pdev)
{
	const struct led_platform_data *pdata = pdev->dev.platform_data;
	struct pmic_mpp_led_data *led = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < pdata->num_leds; i++)
		led_classdev_unregister(&led[i].cdev);

	kfree(led);

	return 0;
}

static struct platform_driver pmic_mpp_led_driver = {
	.probe		= pmic_mpp_led_probe,
	.remove		= __devexit_p(pmic_mpp_led_remove),
	.driver		= {
		.name	= "pmic-mpp-leds",
		.owner	= THIS_MODULE,
	},
};

static int __init pmic_mpp_led_init(void)
{
	return platform_driver_register(&pmic_mpp_led_driver);
}
module_init(pmic_mpp_led_init);

static void __exit pmic_mpp_led_exit(void)
{
	platform_driver_unregister(&pmic_mpp_led_driver);
}
module_exit(pmic_mpp_led_exit);

MODULE_DESCRIPTION("PMIC MPP LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pmic-mpp-leds");
