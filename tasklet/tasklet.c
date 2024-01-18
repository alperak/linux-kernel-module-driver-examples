#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

void my_tasklet_handler(unsigned long data);
/*
 * Init the Tasklet as static method
 * DECLARE_TASKLET had a third parameter called data. It has been removed.
 * The old form of function pointer seems to use the macro DECLARE_TASKLET_OLD
 */
DECLARE_TASKLET_OLD(my_tasklet, my_tasklet_handler);

void my_tasklet_handler(unsigned long data)
{
	pr_info("Tasklet start\n");
	mdelay(4000);
	pr_info("Tasklet end\n");
}

static int __init ModuleInit(void)
{
	//Scheduling task to Tasklet
	tasklet_schedule(&my_tasklet);
	pr_info("Tasklet module has been initialized");
	return 0;
}

static void __exit ModuleExit(void)
{
	tasklet_kill(&my_tasklet);
	pr_info("Tasklet module has been removed");
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("A simple module for Tasklet");
