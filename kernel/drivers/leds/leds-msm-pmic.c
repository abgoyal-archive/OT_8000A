/*
 * leds-msm-pmic.c - MSM PMIC LEDs driver.
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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
 * 12/09/14       Qifeng Wu       Using gpio 122 for keypad backlight for scribe5
 * -----------------------------------------------------------------------------
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>

#include <mach/pmic.h>

/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2012/9/14, Add gpio control for keypad backlight for scribe5*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
#include <linux/module.h>
#include <linux/gpio.h>

#define GPIO_KEYPAD_BACKLIGHT_EN 122
#endif
/*[PLATFORM]-Add-END by TCTNB.WQF*/

#define MAX_KEYPAD_BL_LEVEL	16

static void msm_keypad_bl_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/9/14, Add gpio control for keypad backlight for scribe5*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	if (value == LED_OFF)
	{
		gpio_direction_output(GPIO_KEYPAD_BACKLIGHT_EN, 0);
	} else {
		gpio_direction_output(GPIO_KEYPAD_BACKLIGHT_EN, 1);
	}
#else
	int ret;

	ret = pmic_set_led_intensity(LED_KEYPAD, value / MAX_KEYPAD_BL_LEVEL);
	if (ret)
		dev_err(led_cdev->dev, "can't set keypad backlight\n");
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/
}

static struct led_classdev msm_kp_bl_led = {
	.name			= "keyboard-backlight",
	.brightness_set		= msm_keypad_bl_led_set,
	.brightness		= LED_OFF,
};

static int msm_pmic_led_probe(struct platform_device *pdev)
{
	int rc;

	rc = led_classdev_register(&pdev->dev, &msm_kp_bl_led);
	if (rc) {
		dev_err(&pdev->dev, "unable to register led class driver\n");
		return rc;
	}

/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2012/9/14, Add gpio control for keypad backlight for scribe5*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	rc = gpio_request(GPIO_KEYPAD_BACKLIGHT_EN, "gpio_keypad_backlight_en");
	if (rc < 0) {
		pr_err("failed request GPIO_KEYPAD_BACKLIGHT_EN: %d\n", GPIO_KEYPAD_BACKLIGHT_EN);
		return rc;
	}

	rc = gpio_tlmm_config(GPIO_CFG(
			   GPIO_KEYPAD_BACKLIGHT_EN, 0, GPIO_CFG_OUTPUT,
			   GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("failed GPIO_KEYPAD_BACKLIGHT_EN tlmm config\n");
		return rc;
	}
	rc = gpio_direction_output(GPIO_KEYPAD_BACKLIGHT_EN, 0);
	if (rc < 0) {
		pr_err("failed to enable GPIO_KEYPAD_BACKLIGHT_EN\n");
		gpio_free(GPIO_KEYPAD_BACKLIGHT_EN);
		return rc;
	}
#endif
/*[PLATFORM]-Add-END by TCTNB.WQF*/

	msm_keypad_bl_led_set(&msm_kp_bl_led, LED_OFF);
	return rc;
}

static int __devexit msm_pmic_led_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&msm_kp_bl_led);

	return 0;
}

#ifdef CONFIG_PM
static int msm_pmic_led_suspend(struct platform_device *dev,
		pm_message_t state)
{
	led_classdev_suspend(&msm_kp_bl_led);

	return 0;
}

static int msm_pmic_led_resume(struct platform_device *dev)
{
	led_classdev_resume(&msm_kp_bl_led);

	return 0;
}
#else
#define msm_pmic_led_suspend NULL
#define msm_pmic_led_resume NULL
#endif

static struct platform_driver msm_pmic_led_driver = {
	.probe		= msm_pmic_led_probe,
	.remove		= __devexit_p(msm_pmic_led_remove),
	.suspend	= msm_pmic_led_suspend,
	.resume		= msm_pmic_led_resume,
	.driver		= {
		.name	= "pmic-leds",
		.owner	= THIS_MODULE,
	},
};

static int __init msm_pmic_led_init(void)
{
	return platform_driver_register(&msm_pmic_led_driver);
}
module_init(msm_pmic_led_init);

static void __exit msm_pmic_led_exit(void)
{
	platform_driver_unregister(&msm_pmic_led_driver);
}
module_exit(msm_pmic_led_exit);

MODULE_DESCRIPTION("MSM PMIC LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pmic-leds");
