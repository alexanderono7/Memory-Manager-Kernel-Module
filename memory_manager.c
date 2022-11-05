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

// Traverse Memory regions (VMAs)?
int traverse_vmas(struct task_struct* task){
    struct vm_area_struct* vma;
    int x;
    x = PAGE_SIZE;
    printk("%d",x);
    vma = task->mm->mmap;
    //printk("%lu", vma->vm_start);
    //printk("\n\n\n\n");
    //printk("%lu", vma->vm_next->vm_start);
    //printk("%d", vma->vm_end);
    return 0;
}

// Initialize kernel module
int memman_init(void){
    struct task_struct* proc;
    printk("Memory manager launched!\n");
    proc = find_pid();
    traverse_vmas(proc);
    return 0;
}

// Exit kernel module.
void memman_exit(void){
    printk("Farewell!!!\n");
    return;
}

// Run kernel module
module_init(memman_init);
module_exit(memman_exit);
MODULE_LICENSE("GPL");
