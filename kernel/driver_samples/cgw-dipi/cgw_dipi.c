/*
 * CGW DIPI driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <asm/io.h>

#include "cgw_dipi.h"
#define GPIO_FIXED_DEBOUNCE_TIME 15 //ms
#define MIN_HALF_CYCLE_TIME      80 //ms
#define CNT_MAX_COUNT            999999


struct __dipi_port_data {
	const char *name;
	unsigned int gpio;
	unsigned int irq;
	unsigned int count;
	unsigned int port_type;
	unsigned int state;
	unsigned long last_jiffies;
	bool	en;

	struct delayed_work work;
	unsigned int software_debounce; //ms, debounce = sw + hw
	struct device_attribute en_attr;
	struct device_attribute cnt_attr;
};

static unsigned long pi_half_cycle = MIN_HALF_CYCLE_TIME;
static int gpio_factory_reset, gpio_slq_reset;
static DIPI_POART_ATTR(DI1, S_IRUGO|S_IWUSR|S_IWGRP);
static DIPI_POART_ATTR(PI1, S_IRUGO|S_IWUSR|S_IWGRP);
static DIPI_POART_ATTR(PI2, S_IRUGO|S_IWUSR|S_IWGRP);

static ssize_t PI_min_ontime_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t len)
{
	if(kstrtol(buf, 10, &pi_half_cycle))
		pr_err("get wrong parameter!");
	return len;
}

static ssize_t PI_min_ontime_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%ld", pi_half_cycle);
}
DEVICE_ATTR_RW(PI_min_ontime);

// enable this port and turn on irq
static void en_port(struct __dipi_port_data *data, bool val)
{
	if (val != data->en) {
		data->en = val;
		val ? enable_irq(data->irq) : disable_irq_nosync(data->irq);
		pr_info("Set the port%d to %d\n", data->gpio, data->en);
	}
}

static ssize_t DIPI_EN_attr_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	struct __dipi_port_data *data = en_to_dipi_port(attr);

	if (buf[0] == '1')
		en_port(data, 1);
	else if (buf[0] == '0')
		en_port(data, 0);
	return len;
}

static ssize_t DIPI_EN_attr_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct __dipi_port_data *data = en_to_dipi_port(attr);
	int ret = 0;

	ret = sprintf(buf, "%d", data->en);
	return ret;
}

static ssize_t DIPI_CNT_attr_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	struct __dipi_port_data *data = cnt_to_dipi_port(attr);
	if (buf[0] == '0')
		data->count = 0;
	return len;
}

static ssize_t DIPI_CNT_attr_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = 0;
	struct __dipi_port_data *data = cnt_to_dipi_port(attr);

	ret = sprintf(buf, "%d", data->count);
	return ret;
}

static struct miscdevice cgw_dev = {
	.minor      = MISC_DYNAMIC_MINOR,
	.name       = "cgw_dipi_port",
};

static inline void add_one_count(struct __dipi_port_data *pdata)
{
	pdata->count == CNT_MAX_COUNT ? pdata->count = 0 : pdata->count++;
	sysfs_notify(&cgw_dev.this_device->kobj, NULL, pdata->cnt_attr.attr.name);
}

static void gpio_debounce_func(struct work_struct *work)
{
	struct __dipi_port_data *pdata =
				container_of(work, struct __dipi_port_data, work.work);
	unsigned int val = gpio_get_value(pdata->gpio);
	unsigned int state_keep_time;

	//DI pin, only care the ON cycle
	if (val == ON_CYCLE && pdata->port_type == TYPE_DI)
		add_one_count(pdata);

	//PI pin
	if (pdata->port_type == TYPE_PI) {
		state_keep_time = GPIO_FIXED_DEBOUNCE_TIME + jiffies_to_msecs(jiffies - pdata->last_jiffies);
		if (pdata->last_jiffies) {

			//pr_info("Keep %d state for %d ms..\n", pdata->state & STATE_MASK, state_keep_time);

			// The condition of add one count: on cycle > cycle_time and off cycle > cycle_time
			if (state_keep_time > pi_half_cycle) {
				if (pdata->state & STATE_ON)
					pdata->state |= STATE_ON_FINISH;
				if (pdata->state & STATE_OFF)
					pdata->state |= STATE_OFF_FINISH;
				if ((pdata->state & STATE_ON_FINISH) && (pdata->state & STATE_OFF_FINISH)) {
					add_one_count(pdata);
					pdata->state &= ~STATE_FINISH_MASK;
				}
			}
			pdata->state &= ~STATE_MASK;
			pdata->state |= (val == ON_CYCLE) ? STATE_ON : STATE_OFF;
		}
		pdata->last_jiffies = jiffies;
	}
}

static irqreturn_t dipi_interrupt(int irq, void *data)
{
	struct __dipi_port_data *pdata = data;
	mod_delayed_work(system_wq, &pdata->work,
			msecs_to_jiffies(pdata->software_debounce));

	return IRQ_HANDLED;
}

static int inline gpio_init(struct device_node *np, const char *np_name,
			struct __dipi_port_data *data, int type)
{
	int ret = 0;
	if (!np_name || !data ) {
		pr_err("%s: get wrong paramenter.", __func__);
		return -EINVAL;
	}

	data->gpio = of_get_named_gpio(np, np_name, 0);
	if (data->gpio < 0) {
		pr_err("get wrong %s, err:%d\n", np_name, data->gpio);
		return -EINVAL;
	}

	if  (gpio_request_one(data->gpio, GPIOF_DIR_IN|GPIOF_EXPORT_DIR_FIXED, data->name) < 0) {
		pr_err("config gpio%d failed!\n", data->gpio);
		return -EINVAL;
	}

	data->irq = gpio_to_irq(data->gpio);
	data->port_type = type;
	gpio_set_debounce(data->gpio, 0xff * 31); //we have 7936 HW debounce time
	data->software_debounce = GPIO_FIXED_DEBOUNCE_TIME - (0xff * 31)/1000;
	INIT_DELAYED_WORK(&data->work, gpio_debounce_func);

	ret = request_threaded_irq(data->irq, NULL,
			dipi_interrupt,IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			data->name, data);
	disable_irq_nosync(data->irq);
	return ret;
}

static int __init cgw_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int err;

	pr_info("cgw dipi driver init!\n");

	err = gpio_init(np, "A-gpios", &port_DI1, TYPE_DI);
	err |= gpio_init(np, "B-gpios", &port_PI1, TYPE_PI);
	err |= gpio_init(np, "C-gpios", &port_PI2, TYPE_PI);
	if (err)
		goto exit;

	//for cgw borad, factory reset(input) and SLQ board reset
	gpio_factory_reset = of_get_named_gpio(np, "nFARST-gpios", 0);
	if (gpio_factory_reset  < 0)
		pr_err("get wrong %s, err:%d\n", "nFARST-gpios", gpio_factory_reset);

	if  (gpio_request_one(gpio_factory_reset, GPIOF_DIR_IN|GPIOF_EXPORT_DIR_FIXED, "Factory reset") < 0) {
		pr_err("config gpio%d failed!\n", gpio_factory_reset);
		return -EINVAL;
	}

	gpio_slq_reset = of_get_named_gpio(np, "SLQRST-gpios", 0);
	if (gpio_slq_reset  < 0)
		pr_err("get wrong %s, err:%d\n", "SLQRST-gpios", gpio_slq_reset);

	if  (gpio_request_one(gpio_slq_reset, GPIOF_DIR_OUT|GPIOF_EXPORT_DIR_FIXED, "SLQ reset") < 0) {
		pr_err("config gpio%d failed!\n", gpio_slq_reset);
		return -EINVAL;
	}

	err = misc_register(&cgw_dev);
	if (err) {
		pr_err("cgw misc device register failed!\n");
		goto exit;
	}

	err = device_create_file(cgw_dev.this_device, &port_DI1.en_attr);
	err |= device_create_file(cgw_dev.this_device, &port_DI1.cnt_attr);
	err |= device_create_file(cgw_dev.this_device, &port_PI1.en_attr);
	err |= device_create_file(cgw_dev.this_device, &port_PI1.cnt_attr);
	err |= device_create_file(cgw_dev.this_device, &port_PI2.en_attr);
	err |= device_create_file(cgw_dev.this_device, &port_PI2.cnt_attr);
	err |= device_create_file(cgw_dev.this_device, &dev_attr_PI_min_ontime);
	if (err) {
		pr_err("cgw create sysfs failed!\n");
		goto exit;
	}


#if 1
	{
		unsigned int *map;
		map = ioremap(0x481AE000 + 0xA34, 4);
		pr_info("oe reg = 0x%x\n", *map);
		iounmap(map);

	}
#endif
exit:
	return 0;
}

#ifdef CONFIG_OF_GPIO
static const struct of_device_id of_gpio_dipi_match[] = {
	{ .compatible = "cgw-dipi", },
	{},
};
#endif

static void __exit cgw_exit(void)
{
	&port_DI1.work ? cancel_delayed_work_sync((void *)&port_DI1.work) : pr_err("cancel worka failed!");
	&port_PI1.work ? cancel_delayed_work_sync((void *)&port_PI1.work) : pr_err("cancel workb failed!");
	&port_PI2.work ? cancel_delayed_work_sync((void *)&port_PI2.work) : pr_err("cancel workc failed!");
	port_DI1.gpio ? gpio_free(port_DI1.gpio) : pr_err("free port.di1 failed");
	port_PI1.gpio ? gpio_free(port_PI1.gpio) : pr_err("free port.pi1 failed");
	port_PI2.gpio ? gpio_free(port_PI2.gpio) : pr_err("free port.pi2 failed");
	device_remove_file(cgw_dev.this_device, &port_DI1.en_attr);
	device_remove_file(cgw_dev.this_device, &port_DI1.cnt_attr);
	device_remove_file(cgw_dev.this_device, &port_PI1.en_attr);
	device_remove_file(cgw_dev.this_device, &port_PI1.cnt_attr);
	device_remove_file(cgw_dev.this_device, &port_PI2.en_attr);
	device_remove_file(cgw_dev.this_device, &port_PI2.cnt_attr);
	device_remove_file(cgw_dev.this_device, &dev_attr_PI_min_ontime);
	misc_deregister(&cgw_dev);
}

static struct platform_driver cgw_dipi_driver = {
	.driver = {
		.name = "cgw_dipi",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(of_gpio_dipi_match),
	},
	.remove = __exit_p(cgw_exit),
};

module_platform_driver_probe(cgw_dipi_driver, cgw_probe);
MODULE_DESCRIPTION("CGW DIPI driver");
MODULE_LICENSE("GPL");

