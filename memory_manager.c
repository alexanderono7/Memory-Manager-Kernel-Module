#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

static int pid = 0;
// Receive arguments to the kernel module
module_param(pid, int, 0644);

// Find the process whose PID matches `pid` and return its task_struct ptr
struct task_struct* find_pid(void){
    struct task_struct* p;
    struct task_struct* result;
    
    for_each_process(p){
        if(p->pid == pid){
            printk("\nFOUND IT! : %d\n",p->pid);
            result = p;
        }
        //printk("don't care: %d",p->pid);
    }
    return result;
}

// Initialize kernel module
int memman_init(void){
    struct task_struct* proc;
    printk("Memory manager launched!\n");
    proc = find_pid();
    return 0;
}

// Exit kernel module.
void memman_exit(void){
    printk("Farewell!!!\n");
    return;
}

module_init(memman_init);
module_exit(memman_exit);
MODULE_LICENSE("GPL");
