#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/semaphore.h>

// Receive module parameters
static int buffSize = 0;
module_param(buffSize, int, 0644);
static int prod = 0; // should only be 0 or 1
module_param(prod, int, 0644);
static int cons = 0;
module_param(cons, int, 0644);
static int uuid = 0;
module_param(uuid, int, 0644);

// Forward declarations
static int producer(void *arg);
static int consumer(void *arg);
struct task_struct* buffer[10000];
static struct task_struct* pthread;
static struct task_struct* cthread;

//Semaphore stuff
struct semaphore empty; // define a semaphore named "empty"
sema_init(&empty, 5); // init the semaphore as 5

// ===MODULE INITIALIZATION===
int pc_init(void){
    // prod = # of producers (can only be 0 or 1)
    // cons = # of consumers (any num > 0)
    if(prod){
        pthread = kthread_run(producer, NULL, "producerthread"); //create and run producer kernel thread
    }
    cthread = kthread_run(consumer, NULL, "consumerthread");
    return 0;
}

// module exit
void pc_exit(void){
    printk("Goodbye!!!\n");
}

// Function to run in the producer thread
static int producer(void *arg){ 
    struct task_struct* p;
    int tid = -1; //task id variable
    int counter = 0;

    // Loop through task list
    for_each_process(p) {
        tid = p->cred->uid.val;
        if(tid == uuid){
            buffer[counter] = p;
            // Fix this later vvv
            printk("[Producer-1] Produced Item#-%d at buffer index: %d for PID:%d",99,counter,p->pid);
            counter++;
        }
     }

    //printk("The number of processes running is: %d",proc_counter);
    //printk("The PID of the first value in the buffer is: %d", buffer[proc_counter-1]->pid);
    return 0;
}

static int consumer(void *arg){
    struct task_struct* task;
    int i;
    for(i=0; i<buffSize; i++){
        task = buffer[i];
        if(task == NULL){
            printk("Consumer found EMPTY process @ index %d", i);
        }else{
            int hh = 0;
            int mm = 0;
            int ss = 0;
            int curtime;
            curtime = ktime_get_ns(); //get current time
            int tt = curtime - task->start_time; //tt = total time of that specific task. sometimes this appears as negative?

            hh = tt / (3600000000000);
            tt = tt % (3600000000000);

            mm = tt / (60000000000);
            tt = tt % (60000000000);

            ss = tt / (1000000000);
            tt = tt % (1000000000);

            // fix: threadname, item# vvv
            printk("[%s] Consumed Item#-%d on buffer index:%d PID:%d Elapsed Time-%d:%d:%d", "cthread_name",-1,i,task->pid,hh,mm,ss); 
            //printk("[%s] Consumed Item#-%d on buffer index:%d PID:%d Elapsed Time-%d ...", "cthread_name",-1,i,task->pid,task_time); 
            buffer[i] = NULL;
        }
    }
    return 0;
}

module_init(pc_init);
module_exit(pc_exit);
MODULE_LICENSE("GPL");
