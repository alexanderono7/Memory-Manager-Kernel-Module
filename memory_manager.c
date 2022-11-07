// Alexander Ono
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

//#define TIMEOUT_NSEC   ( 1000000000L )      //1 second in nano seconds
//#define TIMEOUT_SEC    ( 9 )                //9 seconds
#define TIMEOUT_NSEC   ( 10e9 )      //0 second in nano seconds
#define TIMEOUT_SEC    ( 0 )                //10 seconds

static int pid = 0;
static unsigned int rss_pages = 0;
static unsigned int swap_pages = 0;
static unsigned int wss_pages = 0;
struct task_struct* process;
static int counter = 0;

/* Test and clear the accessed bit of a given pte entry. vma is the pointer
to the memory region, addr is the address of the page, and ptep is a pointer
to a pte. It returns 1 if the pte was accessed, or 0 if not accessed. */

/* The ptep_test_and_clear_young() is architecture dependent and is not
exported to be used in a kernel module. You will need to add its
implementation as follows to your kernel module. */
int ptep_test_and_clear_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep) {
    int ret = 0;
    if (pte_young(*ptep)){
      ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte); //returns 1 if pte accessed
    }

    return ret;
}

// Receive arguments to the kernel module
module_param(pid, int, 0644);

void experimental(struct mm_struct* mm, unsigned long address){
    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pud_t *pud;
    pte_t *ptep, pte;

    pgd = pgd_offset(mm, address); // get pgd from mm and the page address
    if (pgd_none(*pgd)) return;

    p4d = p4d_offset(pgd, address); //get p4d from from pgd and the page address
    if (p4d_none(*p4d)) return;

    pud = pud_offset(p4d, address); // get pud from from p4d and the page address
    if (pud_none(*pud)) return;

    pmd = pmd_offset(pud, address); // get pmd from from pud and the page address
    if (pmd_none(*pmd)) return;

    ptep = pte_offset_map(pmd, address); // get pte from pmd and the page address
    //if (!ptep) return;
    if(!pte_present(*ptep)){
        swap_pages++;
    }
}

// "Page table walk" - given mm and addr, walk to its page table entry (quit midway if it isn't valid).
pte_t* access_page(struct mm_struct* mm, unsigned long address){
    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pud_t *pud;
    pte_t *ptep, pte;
    pte_t *result = NULL;

    pgd = pgd_offset(mm, address); // get pgd from mm and the page address
    if (pgd_none(*pgd) || pgd_bad(*pgd)) return NULL;

    p4d = p4d_offset(pgd, address); //get p4d from from pgd and the page address
    if (p4d_none(*p4d) || p4d_bad(*p4d)) return NULL;

    pud = pud_offset(p4d, address); // get pud from from p4d and the page address
    if (pud_none(*pud) || pud_bad(*pud)) return NULL;

    pmd = pmd_offset(pud, address); // get pmd from from pud and the page address
    if (pmd_none(*pmd) || pmd_bad(*pmd)) return NULL;

    ptep = pte_offset_map(pmd, address); // get pte from pmd and the page address
    //wss_pages += ptep_test_and_clear_young(mm->mmap, address, ptep);
    if (pte_young(*ptep)) wss_pages += test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
    if (!ptep) return NULL;

    pte = *ptep;
    experimental(mm, address);
    if(!pte_none(*ptep)){
        if(pte_present(*ptep)){ 
            rss_pages++;
        }else{
            //swap_pages++;
        }
    }
    return result;
}

// Find the process whose PID matches `pid` and return its task_struct ptr
struct task_struct* find_pid(void){
    struct task_struct* p;
    struct task_struct* result = NULL;

    // Iterate through task list
    for_each_process(p){
        if(p->pid == pid){
            result = p;
        }
    }
    return result;
}

// Traverse Memory regions (VMAs).
void traverse_vmas(struct task_struct* task){
    // Do not run this function unless you know the task is valid, otherwise your kernel module will be stuck.
    struct vm_area_struct* foo = task->mm->mmap;
    unsigned long start, end, i; // starting addr of memory region, ending addr, iterator value

    // Iterate through VMAs Linked List as long as next VMA exists.
    while(1){
        start = foo->vm_start;
        end = foo->vm_end;

        // Iterate through the memory region of a VMA
        for(i = start; i < end; i+=PAGE_SIZE){
            access_page(task->mm, i);
        }
        if(foo->vm_next == NULL) return;
        foo = foo->vm_next; // move to next vma (if it exists)
    }
    return;
}

// Get Kibibytes of RSS, SWAP, and WSS and print them to kernel ring buffer.
void get_everything(struct task_struct* proc){
    int rss_size=0, swap_size=0, wss_size=0;
    rss_pages=0; // number of pages in RSS
    swap_pages=0;
    wss_pages=0;

    traverse_vmas(proc); 

    rss_size  = (rss_pages * PAGE_SIZE)/1024; // size of RSS in Kibibytes
    swap_size = (swap_pages * PAGE_SIZE)/1024;
    wss_size  = (wss_pages * PAGE_SIZE)/1024;
    printk("PID %d: RSS=%d KB, SWAP=%d KB, WSS=%d KB", pid, rss_size, swap_size, wss_size);
}

//Timer Callback function. This will be called when timer expires. Executes every 10 seconds until rmmod is called.
static struct hrtimer etx_hr_timer;
ktime_t ktime;
enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    /* vvv do your timer stuff here vvv */
    ktime = ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC);
    if(counter) get_everything(process);
    hrtimer_forward_now(timer,ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC));
    counter++;
    return HRTIMER_RESTART;
}

// Initialize kernel module
int memman_init(void){
    struct task_struct* proc;

    // Retrieve task_struct of PID (if it exists - otherwise quit).
    proc = find_pid();
    if(!proc){
        printk("Couldn't find process w/ PID %d. Exiting.", pid);
        return 0;
    }
    process = proc;

    // Begin timer loop
    //ktime = ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC); // keep this here
    hrtimer_init(&etx_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    etx_hr_timer.function = &timer_callback;
    hrtimer_start(&etx_hr_timer, ktime, HRTIMER_MODE_REL);

    return 0;
}

// Exit kernel module.
void memman_exit(void){
    hrtimer_cancel(&etx_hr_timer); // stop timer
    printk("Exiting.");
    return;
}

// Run kernel module
module_init(memman_init);
module_exit(memman_exit);
MODULE_LICENSE("GPL");
