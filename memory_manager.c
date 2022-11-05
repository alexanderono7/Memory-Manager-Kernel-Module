#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

static int pid = 0;
// Receive arguments to the kernel module
module_param(pid, int, 0644);

int find_pid(void){
    struct task_struct* p;
    
    for_each_process(p){
        if(p->pid == pid){
            printk("\nFOUND IT! : %d\n",p->pid);
        }
        printk("don't care: %d",p->pid);
    }
    return 0;
}

// Initialize kernel module
int memman_init(void){
    printk("Memory manager launched!\n");
    find_pid();
    return 0;
}

void memman_exit(void){
    printk("Farewell!!!\n");
    return;
}

module_init(memman_init);
module_exit(memman_exit);
MODULE_LICENSE("GPL");
