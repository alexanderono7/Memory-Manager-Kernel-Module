#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/hrtimer.h>

static int pid = 0;
static int rss_pages = 0;
static int swap_pages = 0;
static int wss_pages = 0;
struct task_struct* process;

int ptep_test_and_clear_young (struct vm_area_struct *vma, unsigned long addr, pte_t *ptep);
/* Test and clear the accessed bit of a given pte entry. vma is the pointer
to the memory region, addr is the address of the page, and ptep is a pointer
to a pte. It returns 1 if the pte was accessed, or 0 if not accessed. */
/* The ptep_test_and_clear_young() is architecture dependent and is not
exported to be used in a kernel module. You will need to add its
implementation as follows to your kernel module. */
int ptep_test_and_clear_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep) {
    int ret = 0;
    if (pte_young(*ptep)){
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
        wss_pages++;
    }
    return ret;
}

// Receive arguments to the kernel module
module_param(pid, int, 0644);

// experimental function - access a page? idk.
pte_t* access_page(struct mm_struct* mm, unsigned long address){
    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pud_t *pud;
    pte_t *ptep, pte;
    pte_t *result = NULL;

    swap_pages++;

    pgd = pgd_offset(mm, address); // get pgd from mm and the page address
    if (pgd_none(*pgd) || pgd_bad(*pgd)) {
        // check if pgd is bad or does not exist
        //printk("pgd is bad or does not exist");
        return NULL;
    }else{
        //printk("pgd is good!");
    }

    p4d = p4d_offset(pgd, address); //get p4d from from pgd and the page address
    if (p4d_none(*p4d) || p4d_bad(*p4d)) {
        // check if p4d is bad or does not exist
        //printk("p4d is bad or does not exist");
        return NULL;
    }else{
        //printk("p4d is good!");
    }

    pud = pud_offset(p4d, address); // get pud from from p4d and the page address
    if (pud_none(*pud) || pud_bad(*pud)) {
        // check if pud is bad or does not exist
        //printk("pud is bad or does not exist");
        //return;
        return NULL;
    }else{
        //printk("pud is good!");
    }

    pmd = pmd_offset(pud, address); // get pmd from from pud and the page address
    if (pmd_none(*pmd) || pmd_bad(*pmd)) {
        // check if pmd is bad or does not exist
        //printk("pmd is bad or does not exist");
        return NULL;
    }else{
        //printk("pmd is good!");
    }

    ptep = pte_offset_map(pmd, address); // get pte from pmd and the page address
    if (!ptep){
        // check if pte does not exist
        //printk("pte is bad or does not exist");
        return NULL;
    }else{
        //printk("pte is good!");
    }

    pte = *ptep; //is this necessary??
    result = ptep;
    rss_pages++;
    swap_pages--;
    ptep_test_and_clear_young(mm->mmap, address, result);

    return result;
}

// Print out all tasks which have a valid mm and mmap (used for testing purposes)
void probe(void){
    struct task_struct* p;
    for_each_process(p){
        if(p->mm){
            if(p->mm->mmap){
                printk("\nvalid: process # %d", p->pid);
                if(p->mm->mmap->vm_start){
                    unsigned long start = p->mm->mmap->vm_start;
                    unsigned long end = p->mm->mmap->vm_end;
                    unsigned long i;
                    struct vm_area_struct* foo;
                    pte_t *ptep;
                    int res;

                    printk("starting addr: %p", (void*)(p->mm->mmap->vm_start));
                    printk("ending addr: %p", (void*)(p->mm->mmap->vm_end));
                    printk("diff: %lu", end-start);
                    printk("# pages?: %lu", (end-start)/PAGE_SIZE);
                    printk("remainder?: %lu", (end-start)%PAGE_SIZE);

                    if(p->mm->mmap->vm_next){
                        printk("has a next vma?: yes!");
                    }else{
                        printk("has a next vma?: no.");
                    }
                    //access_page(p->mm, p->mm->mmap->vm_start); // access_page on vm_start
                    foo = p->mm->mmap;
                    for(i = start; i <= end; i+=PAGE_SIZE){
                        access_page(p->mm, i);
                        //if(i==end) printk("completed.");
                    }
                    while(foo->vm_next){
                        start = foo->vm_start;
                        end = foo->vm_end;
                        for(i = start; i <= end; i+=PAGE_SIZE){
                            ptep = access_page(p->mm, i);
                            if(ptep){
                                res = ptep_test_and_clear_young(foo, i, ptep);
                                printk("ptep test value: %d", res);
                            }
                        }
                        foo = foo->vm_next;
                    }
                }
            }
        }
        return; // I only want to do one process for testing the page walk.
    }
    return;
}

// Find the process whose PID matches `pid` and return its task_struct ptr
struct task_struct* find_pid(void){
    struct task_struct* p;
    struct task_struct* result = NULL;
    
    for_each_process(p){
        if(p->pid == pid){
            //printk("\nFOUND IT! : %d\n",p->pid);
            result = p;
        }
    }
    return result;
}

// Traverse Memory regions (VMAs)?
int traverse_vmas(struct task_struct* task){
    if(task->mm){
        if(task->mm->mmap){
            if(task->mm->mmap->vm_start){
                unsigned long start = task->mm->mmap->vm_start; // get starting addr of memory region (vma)
                unsigned long end = task->mm->mmap->vm_end; // get ending addr of memory region
                unsigned long i; // iterator
                struct vm_area_struct* foo;

                foo = task->mm->mmap;
                while(foo){
                    start = foo->vm_start;
                    end = foo->vm_end;
                    for(i = start; i <= end; i+=PAGE_SIZE){
                        access_page(task->mm, i);
                    }
                    foo = foo->vm_next; // move to next vma (if it exists)
                }
            }
        }
    }
    return 0;
}

#define TIMEOUT_NSEC   ( 1000000000L )      //1 second in nano seconds
#define TIMEOUT_SEC    ( 3 )                //4 seconds
static struct hrtimer etx_hr_timer;


void get_everything(struct task_struct* proc){
    int rss_size=0, swap_size=0, wss_size=0;
    traverse_vmas(proc);
    rss_size  = rss_pages * PAGE_SIZE;
    swap_size = swap_pages * PAGE_SIZE;
    wss_size  = wss_pages * PAGE_SIZE;
    printk("PID %d: RSS=%d KB, SWAP=%d KB, WSS=%d KB", pid, rss_size, swap_size, wss_size);
}

//Timer Callback function. This will be called when timer expires
enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    //pr_info("Timer Callback function Called [%d]\n",count++);
    /* vvv do your timer stuff here vvv */
    get_everything(process);
    hrtimer_forward_now(timer,ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC));
    return HRTIMER_RESTART;
}

// Initialize kernel module
int memman_init(void){
    struct task_struct* proc;
    ktime_t ktime;
    //probe();

    proc = find_pid();
    if(!proc){
        printk("Couldn't find process w/ PID %d. Exiting.", pid);
        return 0;
    }
    process = proc;
    traverse_vmas(proc); // clear bits of existing page tables? maybe?


    ktime = ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC);
    hrtimer_init(&etx_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    etx_hr_timer.function = &timer_callback;
    hrtimer_start( &etx_hr_timer, ktime, HRTIMER_MODE_REL);


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
