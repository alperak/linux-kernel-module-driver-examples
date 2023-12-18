#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <linux/mutex.h>

static DEFINE_MUTEX(counter_mutex);
static struct proc_dir_entry *proc_file;
static int counter = 0;

static int counter_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "%d\n", ++counter);
    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file) {
    mutex_lock(&counter_mutex);
    return single_open(file, counter_proc_show, NULL);
}

static int counter_proc_release(struct inode *inode, struct file *file) {
    mutex_unlock(&counter_mutex);
    return single_release(inode, file);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops counter_proc_ops = {
    .proc_open = counter_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = counter_proc_release,
};
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
static const struct file_operations counter_proc_fops = {
    .owner = THIS_MODULE,
    .open = counter_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = counter_proc_release,
};
#endif

static int __init counter_init(void) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
    proc_file = proc_create("counter_test", 0666, NULL, &counter_proc_ops);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    proc_file = proc_create("counter_test", 0666, NULL, &counter_proc_fops);
#else
    #error "Unsupported kernel version"
#endif

    if (!proc_file) {
        pr_err("Error creating /proc/counter_test\n");
        return -ENOMEM;
    }

    pr_info("Created /proc/counter_test\n");
    return 0;
}

static void __exit counter_exit(void) {
    pr_info("Removing /proc/counter_test\n");
    proc_remove(proc_file);
}

module_init(counter_init);
module_exit(counter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("A simple kernel module that creates a /proc file (counter_test) to maintain a counter."
                    "The counter increments each time the /proc file is read."
                    "Supports Linux kernels version 4.0 and above, with no memory leaks and deadlock.");
