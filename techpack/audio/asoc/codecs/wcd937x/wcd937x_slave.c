/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/component.h>
#include <soc/soundwire.h>
#include <linux/delay.h>
#define SWR_MAX_RETRY 5

#ifdef CONFIG_VENDOR_EDIT
/* Wang.kun@MM.AudioDriver.Machine.2156142, 2019/07/18, add for sound card bind */
#include <linux/delay.h>

#define WCD937X_NUM_RETRY 5
#endif /* CONFIG_VENDOR_EDIT */
struct wcd937x_slave_priv {
	struct swr_device *swr_slave;
};

static int wcd937x_slave_bind(struct device *dev,
				struct device *master, void *data)
{
	int ret = 0;
	struct wcd937x_slave_priv *wcd937x_slave = NULL;
	uint8_t devnum = 0;
	struct swr_device *pdev = to_swr_device(dev);
	#ifndef CONFIG_VENDOR_EDIT
	/* Wang.kun@MM.AudioDriver.Machine.2156142, 2019/07/18, add for sound card bind */
	int retry = SWR_MAX_RETRY;
	#else
	int retry = WCD937X_NUM_RETRY;
	#endif /* CONFIG_VENDOR_EDIT */

	if (pdev == NULL) {
		dev_err(dev, "%s: pdev is NULL\n", __func__);
		return -EINVAL;
	}

	wcd937x_slave = devm_kzalloc(&pdev->dev,
				sizeof(struct wcd937x_slave_priv), GFP_KERNEL);
	if (!wcd937x_slave)
		return -ENOMEM;

	swr_set_dev_data(pdev, wcd937x_slave);

	wcd937x_slave->swr_slave = pdev;

	do {
		/* Add delay for soundwire enumeration */
		usleep_range(100, 110);
		ret = swr_get_logical_dev_num(pdev, pdev->addr, &devnum);
	} while (ret && --retry);

	if (ret) {
		dev_dbg(&pdev->dev,
				"%s get devnum %d for dev addr %lx failed\n",
				__func__, devnum, pdev->addr);
		swr_remove_device(pdev);
		return ret;
	}
	pdev->dev_num = devnum;

	return ret;
}

static void wcd937x_slave_unbind(struct device *dev,
				struct device *master, void *data)
{
	struct wcd937x_slave_priv *wcd937x_slave = NULL;
	struct swr_device *pdev = to_swr_device(dev);

	if (pdev == NULL) {
		dev_err(dev, "%s: pdev is NULL\n", __func__);
		return;
	}

	wcd937x_slave = swr_get_dev_data(pdev);
	if (!wcd937x_slave) {
		dev_err(&pdev->dev, "%s: wcd937x_slave is NULL\n", __func__);
		return;
	}

	swr_set_dev_data(pdev, NULL);
}

static const struct swr_device_id wcd937x_swr_id[] = {
	{"wcd937x-slave", 0},
	{}
};

static const struct of_device_id wcd937x_swr_dt_match[] = {
	{
		.compatible = "qcom,wcd937x-slave",
	},
	{}
};

static const struct component_ops wcd937x_slave_comp_ops = {
	.bind   = wcd937x_slave_bind,
	.unbind = wcd937x_slave_unbind,
};

static int wcd937x_swr_up(struct swr_device *pdev)
{
	return 0;
}

static int wcd937x_swr_down(struct swr_device *pdev)
{
	return 0;
}

static int wcd937x_swr_reset(struct swr_device *pdev)
{
	return 0;
}

static int wcd937x_swr_probe(struct swr_device *pdev)
{
	return component_add(&pdev->dev, &wcd937x_slave_comp_ops);
}

static int wcd937x_swr_remove(struct swr_device *pdev)
{
	component_del(&pdev->dev, &wcd937x_slave_comp_ops);
	return 0;
}

static struct swr_driver wcd937x_slave_driver = {
	.driver = {
		.name = "wcd937x-slave",
		.owner = THIS_MODULE,
		.of_match_table = wcd937x_swr_dt_match,
	},
	.probe = wcd937x_swr_probe,
	.remove = wcd937x_swr_remove,
	.id_table = wcd937x_swr_id,
	.device_up = wcd937x_swr_up,
	.device_down = wcd937x_swr_down,
	.reset_device = wcd937x_swr_reset,
};

static int __init wcd937x_slave_init(void)
{
	return swr_driver_register(&wcd937x_slave_driver);
}

static void __exit wcd937x_slave_exit(void)
{
	swr_driver_unregister(&wcd937x_slave_driver);
}

module_init(wcd937x_slave_init);
module_exit(wcd937x_slave_exit);

MODULE_DESCRIPTION("WCD937X Swr Slave driver");
MODULE_LICENSE("GPL v2");
