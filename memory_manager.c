#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#define TIMEOUT_NSEC   ( 1000000000L )      //1 second in nano seconds
#define TIMEOUT_SEC    ( 9 )                //10 seconds (?)

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
    if(!pte_read(*ptep) or !pte_mkwrite(*ptep)){
        return ret;
    }
    if(pte_present(*ptep)){
        rss_pages++;
    }else{
        if(!pte_none(*ptep))
            swap_pages++;
    }
    if (pte_young(*ptep)){
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte); //returns 1 if pte accessed
        if(ret) wss_pages++;
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

    pgd = pgd_offset(mm, address); // get pgd from mm and the page address
    if (pgd_none(*pgd) || pgd_bad(*pgd)) {
        // check if pgd is bad or does not exist
        return NULL;
    }else{
        //printk("pgd is good!");
    }

    p4d = p4d_offset(pgd, address); //get p4d from from pgd and the page address
    if (p4d_none(*p4d) || p4d_bad(*p4d)) {
        // check if p4d is bad or does not exist
        return NULL;
    }else{
        //printk("p4d is good!");
    }

    pud = pud_offset(p4d, address); // get pud from from p4d and the page address
    if (pud_none(*pud) || pud_bad(*pud)) {
        // check if pud is bad or does not exist
        //return;
        return NULL;
    }else{
        //printk("pud is good!");
    }

    pmd = pmd_offset(pud, address); // get pmd from from pud and the page address
    if (pmd_none(*pmd) || pmd_bad(*pmd)) {
        // check if pmd is bad or does not exist
        return NULL;
    }else{
        //printk("pmd is good!");
    }

    ptep = pte_offset_map(pmd, address); // get pte from pmd and the page address
    if (!ptep){
        // check if pte does not exist
        return NULL;
    }else{
        //printk("pte is good!");
    }

    pte = *ptep; //is this necessary??
    result = ptep;
    ptep_test_and_clear_young(mm->mmap, address, result);

    return result;
}

// Find the process whose PID matches `pid` and return its task_struct ptr
struct task_struct* find_pid(void){
    struct task_struct* p;
    struct task_struct* result = NULL;
    
    for_each_process(p){
        if(p->pid == pid){
            result = p;
        }
    }
    return result;
}

// Traverse Memory regions (VMAs)?
int traverse_vmas(struct task_struct* task){
    // Do not run this function unless you know the task is valid, otherwise your kernel module will be stuck.
    struct vm_area_struct* foo = task->mm->mmap;
    unsigned long start = foo->vm_start; // get starting addr of memory region (vma)
    unsigned long end = foo->vm_end; // get ending addr of memory region
    unsigned long i; // iterator

    while(foo){
        start = foo->vm_start;
        end = foo->vm_end;
        for(i = start; i <= end; i+=PAGE_SIZE){
            access_page(task->mm, i);
        }
        foo = foo->vm_next; // move to next vma (if it exists)
    }
    return 0;
}

static struct hrtimer etx_hr_timer;

void get_everything(struct task_struct* proc){
    int rss_size=0, swap_size=0, wss_size=0;
    rss_pages=0;
    swap_pages=0;
    wss_pages=0;
    traverse_vmas(proc);
    rss_size  = (rss_pages * PAGE_SIZE)/1000;
    swap_size = (swap_pages * PAGE_SIZE)/1000;
    wss_size  = (wss_pages * PAGE_SIZE)/1000;
    printk("PID %d: RSS=%d KB, SWAP=%d KB, WSS=%d KB", pid, rss_size, swap_size, wss_size);
}

//Timer Callback function. This will be called when timer expires
enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
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
    //traverse_vmas(proc); // clear bits of existing page tables? maybe?
    //get_everything(process);

    ktime = ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC);
    hrtimer_init(&etx_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    etx_hr_timer.function = &timer_callback;
    hrtimer_start( &etx_hr_timer, ktime, HRTIMER_MODE_REL);

    return 0;
}

// Exit kernel module.
void memman_exit(void){
    hrtimer_cancel(&etx_hr_timer);
    printk("Farewell!!!\n");
    return;
}

// Run kernel module
module_init(memman_init);
module_exit(memman_exit);
MODULE_LICENSE("GPL");
