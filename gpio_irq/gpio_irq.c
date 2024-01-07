#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

// Contain the pin number of the interrupt controller to which GPIO 23 is mapped to
int irq_number;

// Interrupt service routine(ISR) is called, when interrupt is triggered
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	pr_info("Interrupt was triggered with BUTTON and ISR was called!\n");
	return IRQ_HANDLED;
}

static int __init ModuleInit(void)
{
	// Request access to GPIO 23 interface
	if (gpio_request(23, "rpi-gpio-23")) {
		pr_info("Error - Cant allocate GPIO 23\n");
		return -1;
	}

	// Set GPIO 23 as input
	if (gpio_direction_input(23)) {
		pr_info("Error - Cant set GPIO 23 to input\n");
		gpio_free(23);
		return -1;
	}

	// To map a specific GPIO number(23) to an IRQ number
	irq_number = gpio_to_irq(23);

	if (request_irq(irq_number, gpio_irq_handler, IRQF_TRIGGER_RISING, "my_gpio_irq", NULL) != 0) {
		pr_info("Error - Cant request interrupt number: %d\n", irq_number);
		gpio_free(23);
		return -1;
	}

	pr_info("GPIO IRQ module loaded!");
	pr_info("GPIO 23 is mapped to IRQ number: %d\n", irq_number);
	return 0;
}

static void __exit ModuleExit(void)
{
	printk("GPIO IRQ module unloading... ");
	free_irq(irq_number, NULL);
	gpio_free(23);
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("A simple module for a GPIO interrupt using a breadboard and button.");
