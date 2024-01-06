#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/string.h>

char my_array[255] = "sysfs test";

static struct kobject *example_kobj;

// Will be called when we read sysfs file
static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	pr_info("Sysfs read.\n");
	return sprintf(buf, "You have read from /sys/kernel/%s/%s -> %s\n", kobj->name, attr->attr.name, my_array);
}

// Will be called when write to sysfs file
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	pr_info("Sysfs write - Wrote '%s' to /sys/kernel/%s/%s\n", buf, kobj->name, attr->attr.name);
	strncpy(my_array, buf, sizeof(my_array) - 1);
	my_array[sizeof(my_array) - 1] = '\0';
	return count;
}

// Create attribute with _ATTR macro
struct kobj_attribute my_attr = __ATTR(my_value, 0660, sysfs_show, sysfs_store);

static int __init my_init(void)
{
    // Create my_sysfs folder
    example_kobj = kobject_create_and_add("my_sysfs", kernel_kobj);
    if (!example_kobj) {
        pr_info("Failed to create /sys/kernel/my_sysfs\n");
        return -ENOMEM;
    }

    // Create my_value file in my_sysfs folder
    if (sysfs_create_file(example_kobj, &my_attr.attr)) {
        pr_info("Failed to create /sys/kernel/my_sysfs/my_value\n");
        kobject_put(example_kobj); // Will be dynamically freed when it is no longer being used
        return -ENOMEM;
    }

    pr_info("Created /sys/kernel/my_sysfs/my_value successfully.\n");
    return 0;
}

static void __exit my_exit(void)
{
    sysfs_remove_file(example_kobj, &my_attr.attr); // When we done with the sysfs file, we should delete it
    kobject_put(example_kobj);
    pr_info("Removed successfully.\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("Basic sysfs example");
