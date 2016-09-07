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
	int read_len;

	read_len = ID_LEN - *ptr;
	if (read_len <= 0)
		return 0;

	if (count < read_len)
		return -EINVAL;

	if (copy_to_user(buf, MY_ID + *ptr, read_len))
		return -EFAULT;
	*ptr += read_len;
	return read_len;
}

static ssize_t hello_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	unsigned char id_buf[ID_LEN];

	if (count != ID_LEN)
		return -EINVAL;
	if (copy_from_user(id_buf, buf, ID_LEN))
		return -EFAULT;
	if (strncmp(id_buf, buf, ID_LEN))
		return -EINVAL;
	return ID_LEN;
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
