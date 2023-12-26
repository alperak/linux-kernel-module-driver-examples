#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/err.h>

//Declare a static pointer to a task_struct structure representing a thread
static struct task_struct *wait_thread;

//Static declaration of waitqueue
DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);

static struct class *dev_class;
static struct cdev my_cdev;
dev_t dev;

int wait_queue_flag = 0; // 1->Read, 2->Exit
uint32_t read_count = 0;

//Thread function which will be executed by the thread
static int wait_function(void *unused)
{
	while(1) {
		pr_info("Waiting for event...\n");
		wait_event_interruptible(my_wait_queue, wait_queue_flag != 0);
		if (wait_queue_flag == 2) {
			pr_info("Event came from exit function.\n");
			return 0;
		}
		pr_info("Event came from read function: %d\n", ++read_count);
		wait_queue_flag = 0;
	}
	do_exit(0); //This is the only way a process has to end its execution.
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
        pr_info("Release function called.\n");
        return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        pr_info("Read function called.\n");
        wait_queue_flag = 1;
        wake_up_interruptible(&my_wait_queue); //Wakes up only one process from the waitqueue that is in interruptible sleep.
        return 0;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        pr_info("Write function called.\n");
        return len;
}

static int my_open(struct inode *inode, struct file *file)
{
	pr_info("Device file opened.\n");
	return 0;
}

static struct file_operations fops = {
	.owner		= THIS_MODULE,
	.open		= my_open,
	.write		= my_write,
	.read		= my_read,
	.release	= my_release,
};

static int __init ModuleInit(void)
{
	//Dynamically allocating major number
	if ((alloc_chrdev_region(&dev, 0, 1, "my_Dev")) < 0) {
		pr_info("Cant allocate major number.\n");
		return -1;
	}
	pr_info("Major: %d Minor: %d \n", MAJOR(dev), MINOR(dev));

	//Create cdev structure
	cdev_init(&my_cdev, &fops);
	my_cdev.owner = THIS_MODULE;
	my_cdev.ops = &fops;

	//Add character device to the system
	if ((cdev_add(&my_cdev, dev, 1)) < 0) {
		pr_info("Couldnt add device to system.\n");
		goto r_class;
	}

	//Create struct class
	if (IS_ERR(dev_class = class_create(THIS_MODULE, "my_class"))) {
		pr_info("Cant create struct class.\n");
		goto r_class;
	}

	//Create device
	if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "my_device"))) {
		pr_info("Cant create device.\n");
		goto r_device;
	}

	//Create kernel thread with WaitThread name
	wait_thread = kthread_create(wait_function, NULL, "WaitThread");
	if (wait_thread) {
		pr_info("Thread created successfully.\n");
		wake_up_process(wait_thread);
	} else
		pr_info("Thread couldnt be created.\n");

	pr_info("Device driver successfully inserted.\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

static void __exit ModuleExit(void)
{
	wait_queue_flag = 2;
	wake_up_interruptible(&my_wait_queue); //Wakes up only one process from the waitqueue that is in interruptible sleep.
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev, 1);
	pr_info("Device driver has been removed.\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("Basic Linux Driver which is using Waitqueue with static method.");
