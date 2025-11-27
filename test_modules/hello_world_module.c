#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hello world! - module");
MODULE_AUTHOR("Me Lol");

static int __init hello_init(void) {
    printk(KERN_ALERT "Initialized hello module!\n");
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_ALERT "Exited hello module!\n");
}

module_init(hello_init);
module_exit(hello_exit);
