#include<linux/init.h>
#include<linux/module.h>
#include<linux/moduleparam.h>

#define array_size 4

int myvalue = 0;
int arr_myvalues[array_size] = {0};
int cb_myvalue = 0;
char *myname = NULL;

/* Begin
 * module_param(name, type, perm)
 * This macro is used to initialize the parameters and creates the sub-directory under '/sys/module'.
 * If the value is changed, we won't receive any notification. If we use module_param_cb() which we used for 'cb_myvalue', we can get a notification.
 */
module_param(myvalue, int, S_IRUSR|S_IWUSR);
module_param(myname, charp, S_IRUSR|S_IWUSR);
module_param_array(arr_myvalues, int, NULL, S_IRUSR|S_IWUSR);
/* End */

/* Begin
 * module_param_cb(name, ops, arg, perm)
 * This macro is used to register the callback. Whenever the parameter changed, this callback function will be called.
 * It takes two callbacks functions 'set' and 'get'. That is called when interact with the parameters.
 * This is done by passing a struct kernel_param_ops to the macro.
 */
int notify_param(const char *val, const struct kernel_param *kp)
{
    int res = param_set_int(val, kp);
    if(res == 0)
    {
        pr_info("Callback function called\n");
        pr_info("New cb_myvalue: %d\n", cb_myvalue);
        return 0;
    }
    return -1;
}

const struct kernel_param_ops my_param_ops =
{
    .set = &notify_param,
    .get = &param_get_int,
};

module_param_cb(cb_myvalue, &my_param_ops, &cb_myvalue, S_IRUGO|S_IWUSR);
/* End */

static int __init ModuleInit(void)
{
    pr_info("myvalue: %d  \n", myvalue);
    pr_info("cb_myvalue: %d  \n", cb_myvalue);
    pr_info("myname: %s \n", myname);
    for (int i = 0; i < array_size; i++)
    {
            pr_info("arr_myvalues[%d]: %d\n", i, arr_myvalues[i]);
    }
    pr_info("Kernel module inserted successfully\n");
    return 0;
}

static void __exit ModuleExit(void)
{
    pr_info("Kernel module removed successfully\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("Linux kernel module which is using parameters.");
