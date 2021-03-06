/*
 * Dexter Industries BrickPi3 LEDs driver
 *
 * Copyright (C) 2017 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * DOC: userspace
 *
 * The BrickPi3 has one user-controllable LED. It is turned on by default when
 * the ``brickpi3`` module is loaded. It uses the mainline kernel `LEDs class`_
 * subsystem. You can find it in sysfs at ``/sys/class/leds/brickpi3:amber:ev3dev``.
 *
 * .. _LEDs class: http://lxr.free-electrons.com/source/Documentation/leds/leds-class.txt?v=4.9
 */

#include <linux/device.h>
#include <linux/leds.h>
#include <linux/workqueue.h>

#include "brickpi3.h"

/* setting brightness to -1 returns control to the BrickPi3 firmware */
#define BRICKPI3_LEDS_FIRMWARE_CONTROL	(-1)
#define BRICKPI3_LEDS_MAX_BRIGHTNESS	100

struct brickpi3_leds {
	struct brickpi3 *bp;
	struct led_classdev cdev;
	u8 address;
};

static inline struct brickpi3_leds *to_brickpi3_leds(struct led_classdev *cdev)
{
	return container_of(cdev, struct brickpi3_leds, cdev);
}

static int brickpi3_leds_brightness_set_blocking(struct led_classdev *cdev,
						 enum led_brightness brightness)
{
	struct brickpi3_leds *data = to_brickpi3_leds(cdev);

	return brickpi3_write_u8(data->bp, data->address, BRICKPI3_MSG_SET_LED,
				 brightness);
}
static void brickpi3_leds_release(struct device *dev, void *res)
{
	struct brickpi3_leds *data = res;

	led_classdev_unregister(&data->cdev);
	brickpi3_write_u8(data->bp, data->address, BRICKPI3_MSG_SET_LED,
			  BRICKPI3_LEDS_FIRMWARE_CONTROL);
}

int devm_brickpi3_register_leds(struct device *dev, struct brickpi3 *bp,
				u8 address)
{
	struct brickpi3_leds *data;
	int ret;

	data = devres_alloc(brickpi3_leds_release, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->bp = bp;
	data->address = address;
	data->cdev.name = "brickpi3:amber:ev3dev";
	data->cdev.max_brightness = BRICKPI3_LEDS_MAX_BRIGHTNESS;
	data->cdev.brightness_set_blocking = brickpi3_leds_brightness_set_blocking;
	data->cdev.default_trigger = "default-on";

	ret = led_classdev_register(dev, &data->cdev);
	if (ret < 0) {
		dev_err(dev, "Failed to register LEDs classdev\n");
		devres_free(data);
		return ret;
	}

	devres_add(dev, data);

	return 0;
}
