#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/err.h>

#define WR_VALUE _IOW('a', 'a', int32_t*)
#define RD_VALUE _IOR('a', 'b', int32_t*)

int32_t value = 0;

dev_t dev;
static struct class *dev_class;
static struct cdev my_cdev;

//This function will be called when we write or read IOCTL on the device file
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case WR_VALUE:
		if (copy_from_user(&value, (int32_t*)arg, sizeof(value)))
				pr_err("Data Write: Error!\n");
		pr_info("Value = %d\n", value);
		break;
	case RD_VALUE:
		if (copy_to_user((int32_t*)arg, &value, sizeof(value)))
				pr_err("Data Read : Error!\n");
		break;
	default:
		break;
	}

	return 0;
}

static struct file_operations fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = my_ioctl,
};

static int __init ModuleInit(void)
{
	//Dynamically allocating major number
	if ((alloc_chrdev_region(&dev, 0, 1, "my_Dev")) < 0) {
		pr_err("Cant allocate major number\n");
		return -1;
	}
	pr_info("Major: %d Minor: %d \n", MAJOR(dev), MINOR(dev));

	//Creating cdev structure
	cdev_init(&my_cdev, &fops);

	//Adding character device
	if ((cdev_add(&my_cdev, dev, 1)) < 0) {
		pr_err("Cant add character device\n");
		goto r_class;
	}

	//Creating struct class
	if (IS_ERR(dev_class = class_create(THIS_MODULE, "my_class"))) {
		pr_err("Cant create the struct class\n");
		goto r_class;
	}

	//Creating device to /dev
	if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "my_device"))) {
		pr_err("Cant create the device\n");
		goto r_device;
	}
	pr_info("Device driver inserted successfully\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

static void __exit ModuleExit(void)
{
	device_destroy(dev_class,dev);
	class_destroy(dev_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev, 1);
	pr_info("Device driver removed successfully\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("Simple Linux device driver which is using IOCTL.");
