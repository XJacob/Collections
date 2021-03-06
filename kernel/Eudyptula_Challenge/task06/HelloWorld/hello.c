#include <linux/init.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#define MY_ID "aa1d2514c1a6"
#define ID_LEN sizeof(MY_ID)

static ssize_t hello_read (struct file *filp, char __user *buf, size_t count, loff_t *ptr)
{
	return simple_read_from_buffer(buf, count, ptr, MY_ID, ID_LEN);
}

static ssize_t hello_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	unsigned char id_buf[ID_LEN];
	ssize_t rc;

	if (count != ID_LEN)
		return -EINVAL;
	rc = simple_write_to_buffer(id_buf, ID_LEN, ppos, buf, count);
	if (rc < 0)
		return rc;
	if (strncmp(id_buf, buf, ID_LEN))
		return -EINVAL;
	return rc;
}

static const struct file_operations hello_fps = {
	.owner		= THIS_MODULE,
	.read		= hello_read,
	.write		= hello_write,
};

static struct miscdevice hello_dev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "eudyptula",
	.fops		= &hello_fps,
};

static int __init hello_init(void)
{
	int retval;
	pr_debug("Hello World init!\n");

	retval = misc_register(&hello_dev);
	return retval;
}

static void __exit hello_exit(void)
{
	pr_debug("Hello World exit!\n");
	misc_deregister(&hello_dev);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Jacob Lee <jacoblee.box.tw@gmail.com> ");
MODULE_DESCRIPTION("Hello World driver");
MODULE_LICENSE("GPL");
