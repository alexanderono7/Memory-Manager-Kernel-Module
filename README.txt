# Memory Manager Kernel Module

To support multiprogramming, modern OSes present virtual memory to processes so that each process
sees a contiguous logical memory address space which may not be contiguous in physical memory and
may not be completely in memory. The OS maintains the mapping between a process’ virtual memory
and the computer’s physical memory through the process’ page table. Every page table entry stores the
mapping information of a virtual page, indicating whether the page is present in memory and, if yes, the
physical frame number it is mapped to. Additional data structures track the temporarily swapped pages
from memory to disk.


In this project, we implemented new kernel functions to reveal the “magic” that the kernel does to
virtualize memories, using the same virtual machine environment you prepared in the previous projects.
This project helps one understand how a real-world OS like Linux performs memory management and
master the skills to implement it in kernel space


In this project, we implemented a kernel module to walk the page tables of a given process and find
out how many of the process’ pages are present in the physical memory (resident set size--RSS), how
many are swapped out to disk (swap size--SWAP), and how many pages are in the process working set
(working set size--WSS).


11/22/2022


Yisely Barraza
Nathan Cluff
Varun Ravi Kumar
Alexander Ono
