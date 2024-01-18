#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>

static struct task_struct *my_kthread1;
static struct task_struct *my_kthread2;
// We Will use these int values to pass the thread function as thread numbers
static int t1 = 1, t2 = 2;
// Let's use dynamic method instead of static because we used stati method in procfs example
static struct mutex lock;
int my_global_variable = 0;

int kthread_function(void *thread_nr)
{
	while (!kthread_should_stop()) {
		pr_info("Thread %d is executed\n", *((int *)thread_nr));
		mutex_lock(&lock);
		pr_info("Thread %d is in critical section and increased my_global_variable to: %d\n", *((int *)thread_nr), ++my_global_variable);
		msleep(1000);
		mutex_unlock(&lock);
		pr_info("Thread %d left from critical section and finished execution\n", *((int *)thread_nr));
	}

	return 0;
}

static int __init ModuleInit(void)
{
	// Initialize mutex
	mutex_init(&lock);

	/*
	* We will use kthread_create instead of kthread_run, so we will have to wake up the thread ourselves
	* In addition, we pass the t1(thread number) to thread function
	*/
	my_kthread1 = kthread_create(kthread_function, &t1, "mykthread1");

	if (!IS_ERR(my_kthread1)) {
		//Let's wake up the thread and it will start to run
		wake_up_process(my_kthread1);
		pr_info("Thread 1 created and is running now\n");
	}
	else {
		pr_info("Thread 1 could not be created\n");
		return -1;
	}

	/*
	* This time we used kthread_run instead of kthread_create, so it will create a new thread and tells it to run
	* Also, we pass the t2(thread number) to thread function
	*/
	my_kthread2 = kthread_run(kthread_function, &t2, "mykthread2");

	if (!IS_ERR(my_kthread2))
		pr_info("Thread 2 created and is running now\n");
	else {
		pr_info("Thread 2 could not be created\n");
		return -1;
	}

	pr_info("Thread 1 and Thread 2 are running now\n");
	return 0;
}

static void __exit ModuleExit(void)
{
	kthread_stop(my_kthread1);
	kthread_stop(my_kthread2);
	pr_info("Thread 1 and Thread 2 has been stopped and module removed\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("A simple module for mutex usage with threads");
