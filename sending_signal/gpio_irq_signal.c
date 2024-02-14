#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/sched/signal.h>
#include <linux/ioctl.h>

/* Contain the pin number of the interrupt controller to which GPIO 23 is mapped to */
int irq_number;

/* Global variables and defines for userspace app registration */
#define REGISTER_USERAPP _IO('R', 'g')
static struct task_struct *task = NULL;

/* define for Signal sending */
#define SIGNR 60

dev_t dev;
static struct class *dev_class;
static struct cdev my_cdev;

/* Contain the pin number of the interrupt controller to which GPIO 23 is mapped to */
int irq_number;

/* Interrupt service routine(ISR) is called, when interrupt is triggered */
static irqreturn_t gpio_irq_signal_handler(int irq, void *dev_id)
{
    struct siginfo info;
    printk("Interrupt was triggered with BUTTON and ISR was called!\n");

    if (task != NULL) {
        memset(&info, 0, sizeof(info));
        info.si_signo = SIGNR;
        info.si_code = SI_QUEUE;

        /* Send the signal */
        if (send_sig_info(SIGNR, (struct kernel_siginfo *)&info, task) < 0)
            printk("error occured while sending signal\n");
    }
    return IRQ_HANDLED;
}

static int my_release(struct inode *inode, struct file *file)
{
    struct task_struct *ref_task = get_current();
    /* Delete the task */
    if(ref_task == task)
        task = NULL;
    return 0;
}

static long int my_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	if(cmd == REGISTER_USERAPP) {
		task = get_current();
		printk("Userspace app with PID %d is registered\n", task->pid);
	}
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.release = my_release,
	.unlocked_ioctl = my_ioctl,
};

static int __init ModuleInit(void)
{
	/* Dynamically allocating major number */
	if ((alloc_chrdev_region(&dev, 0, 1, "my_Dev")) < 0) {
		pr_err("Cant allocate major number\n");
		return -1;
	}
	pr_info("Major: %d Minor: %d \n", MAJOR(dev), MINOR(dev));

	/* Creating cdev structure */
	cdev_init(&my_cdev, &fops);

	/* Adding character device */
	if ((cdev_add(&my_cdev, dev, 1)) < 0) {
		pr_err("Cant add character device\n");
		goto r_class;
	}

	/* Creating struct class */
	if (IS_ERR(dev_class = class_create(THIS_MODULE, "my_class"))) {
		pr_err("Cant create the struct class\n");
		goto r_class;
	}

	/* Creating device to /dev */
	if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "gpio_irq_signal"))) {
		pr_err("Cant create the device\n");
		goto r_device;
	}

	/* Request access to GPIO 23 interface */
	if (gpio_request(23, "rpi-gpio-23")) {
		pr_info("Error - Cant allocate GPIO 23\n");
		return -1;
	}

	/* Set GPIO 23 as input */
	if (gpio_direction_input(23)) {
		pr_info("Error - Cant set GPIO 23 to input\n");
		gpio_free(23);
		return -1;
	}

	/* To map a specific GPIO number(23) to an IRQ number */
	irq_number = gpio_to_irq(23);

	/* Request an interrupt resource (IRQ) */
	if (request_irq(irq_number, gpio_irq_signal_handler, IRQF_TRIGGER_RISING, "my_gpio_irq_signal", NULL) != 0) {
		pr_info("Error - Cant request interrupt number: %d\n", irq_number);
		gpio_free(23);
		return -1;
	}

	pr_info("GPIO IRQ Signal module loaded!");
	pr_info("GPIO 23 is mapped to IRQ number: %d\n", irq_number);
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

static void __exit ModuleExit(void)
{
	printk("GPIO IRQ Signal module unloading... ");
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev, 1);
	free_irq(irq_number, NULL);
	gpio_free(23);
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("A simple module which sends a signal to an userspace application and using a breadboard and button.");
