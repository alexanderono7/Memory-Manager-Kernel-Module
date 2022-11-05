#include <linux/module.h>
#include <linux/kernel.h>

// Receive arguments to the kernel module
static int pid = 0;
module_param(pid, int, 0644);

// Initialize kernel module
int memman_init(void){
    printk("Memory manager launched!\n");
    return 0;
}

void memman_exit(void){
    printk("Farewell!!!\n");
}
