#include <linux/init.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

static struct usb_device_id hello_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
		USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_KEYBOARD) },
	{ }
};
MODULE_DEVICE_TABLE (usb, hello_id_table);

static int __init hello_init(void)
{
	pr_debug("Hello World init!\n");
	return 0;
}

static void __exit hello_exit(void)
{
	pr_debug("Hello World exit!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Jacob Lee <jacoblee.box.tw@gmail.com> ");
MODULE_DESCRIPTION("Hello World driver");
MODULE_LICENSE("GPL");
