#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

static int pid = 0;
// Receive arguments to the kernel module
module_param(pid, int, 0644);

// Print out all tasks which have a valid mm and mmap (used for testing purposes)
void probe(void){
    struct task_struct* p;
    for_each_process(p){
        if(p->mm){
            if(p->mm->mmap){
                printk("valid: process # %d", p->pid);
                if(p->mm->mmap->vm_start){
                    unsigned long start = p->mm->mmap->vm_start;
                    unsigned long end = p->mm->mmap->vm_end;
                    printk("starting addr: %p", (void*)(p->mm->mmap->vm_start));
                    printk("ending addr: %p", (void*)(p->mm->mmap->vm_end));
                    printk("diff: %lu", end-start);
                    printk("# pages?: %lu", (end-start)/PAGE_SIZE);
                    printk("remainder?: %lu", (end-start)%PAGE_SIZE);
                }
            }
        }
    }
    return;
}

// Find the process whose PID matches `pid` and return its task_struct ptr
struct task_struct* find_pid(void){
    struct task_struct* p;
    struct task_struct* result = NULL;
    
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
    //printk("%d",PAGE_SIZE); // this works btw, printing the page size
    //vma = task->mm->mmap;
    if(task->mm){
        if(task->mm->mmap){
            printk("%lu", vma->vm_next->vm_start);
        }
    }else{
        //printk("");
    }
    //printk("%d", &vma->vm_start);
    //printk("\n\n\n\n");
    //printk("%d", vma->vm_end);
    return 0;
}

// Initialize kernel module
int memman_init(void){
    struct task_struct* proc;
    printk("Memory manager launched!\n");
    probe();
    proc = find_pid();
    if(!proc){
        printk("Couldn't find process w/ PID %d", pid);
        return 0;
    }
    //traverse_vmas(proc);
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
