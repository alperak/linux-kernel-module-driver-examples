#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define MY_GPIO 23
#define GPIO_ON 1
#define GPIO_OFF 0

static struct timer_list my_timer;
static int led_state = GPIO_ON;

void timer_callback(struct timer_list * data)
{
	led_state = !led_state;
	gpio_set_value(MY_GPIO, led_state);
	if (led_state == GPIO_ON) {
		pr_info("LED is ON\n");
	} else {
		pr_info("LED is OFF\n");
	}
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
}

static int __init ModuleInit(void)
{
	/* Request access to GPIO 23 interface */
	if (gpio_request(MY_GPIO, "rpi-gpio-23")) {
		pr_info("Error - Cant allocate GPIO 23\n");
		return -1;
	}

	/* Set GPIO 23 as output */
	if (gpio_direction_output(MY_GPIO, 0)) {
		pr_info("Error - Cant set GPIO 23 to output\n");
		gpio_free(23);
		return -1;
	}

	/* Turn LED on */
	gpio_set_value(MY_GPIO, GPIO_ON);

	/* Initialize timer */
	timer_setup(&my_timer, timer_callback, 0);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));

	pr_info("Kernel timer module inserted successfully\n");
	return 0;
}

static void __exit ModuleExit(void)
{
	printk("Kernel timer module unloading...");
	del_timer(&my_timer);
	gpio_free(MY_GPIO);
	gpio_set_value(MY_GPIO, GPIO_OFF);
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("A simple module which is using kernel timer with LED");
