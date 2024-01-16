#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

static struct proc_dir_entry *my_linked_list_folder;

// Node
struct my_list {
	int data;
	struct list_head list; //Linux Kernel list implementation
};

//Declare and init the head node of the Linked List
LIST_HEAD(head_node);

void list_linkedlist(void)
{
	struct my_list *current_node;

    // Iterate through the Linked List and print data to kernel log
    list_for_each_entry(current_node, &head_node, list) {
		pr_cont("%d - ", current_node->data); // Use pr_cont instead of pr_info to continues a previous log message in the same line.
    }
}

static ssize_t add_to_linkedlist(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
	int value;

	// Used kstrtoint_from_user instead of copy_from_user because I needed to convert char*(buff) to an int*, instead of that I used this function directly.
	if(!kstrtoint_from_user(buff, len, 10, &value)) {
		// Create a new node
		struct my_list *new_node = kmalloc(sizeof(struct my_list), GFP_KERNEL);
		if (!new_node)
			return -ENOMEM;

		// Assgin the data that is received from userspace
		new_node->data = value;
		// Init the list within the struct
		INIT_LIST_HEAD(&new_node->list);
		// Add Node to Linked List
		list_add_tail(&new_node->list, &head_node);
		pr_info("Value added to tail: %d\n", value);
		list_linkedlist();
		return len;
	}

	pr_info("Could not copy value from user-space to kernel-space\n");
	return -EINVAL;
}

static ssize_t remove_from_linkedlist(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
    int value;
	struct my_list *current_node, *tmp;

	if(!kstrtoint_from_user(buff, len, 10, &value)) {
		// We should use list_for_each_entry_safe instead of list_for_each_entry because without safe, it will break if you delete something while iterating the list
		list_for_each_entry_safe(current_node, tmp, &head_node, list) {
			if (current_node->data == value) {
				// Delete value from list
				list_del(&current_node->list);
				// Freed memory which is related to the node where the value is located
				kfree(current_node);
				pr_info("Value removed from Linked List: %d\n", value);
				list_linkedlist();
				return len;
			}
		}
	}

	pr_info("Value %d not found in the Linked List\n", value);
	return -EINVAL;
}

static struct proc_ops add_fops = {
    .proc_write = add_to_linkedlist
};

static struct proc_ops remove_fops = {
    .proc_write = remove_from_linkedlist
};

static int __init ModuleInit(void)
{
	// Create folder under proc, /proc/my_linkedlist
	my_linked_list_folder = proc_mkdir("my_linkedlist", NULL);

	if (!my_linked_list_folder) {
		pr_info("Error occured while creating my_linkedlist folder");
		return -ENOMEM;
	}

	//Can be checked whether it was fail or success but I did not
	proc_create("add", 0666, my_linked_list_folder, &add_fops);
    proc_create("remove", 0666, my_linked_list_folder, &remove_fops);

	pr_info("LinkedList Module inserted");
	return 0;
}

static void __exit ModuleExit(void)
{
	struct my_list *next, *tmp;

	// It will go through the list and free the memory
	list_for_each_entry_safe(tmp, next, &head_node, list) {
		list_del(&tmp->list);
		kfree(tmp);
	}

	// It will remove complete /proc/my_linkedlist
	proc_remove(my_linked_list_folder);
	pr_info("Linked List module removed and memory has been freed");
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("A simple module for Linked List");
