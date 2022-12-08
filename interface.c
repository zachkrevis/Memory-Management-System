#include "interface.h"
#include "vmm.h"

// Interface implementation
// Implement APIs here...

void mm_init(enum policy_type policy, void *vm, int vm_size, int num_frames, int page_size)
{
    // Make the virtual memory space inaccessible

    /*
    mm_init 
    givin parameters
    malloc for page_size and num_frames
    for loop to set up pages
    after for loop set mprotect to none
    */

    PAGE_SIZE = page_size;
    NUM_FRAMES = num_frames;
    PHY_MEMORY_SIZE = num_frames;
    START_OF_VM = vm;
    
    if (mprotect(vm, vm_size, PROT_NONE) == -1)
    {
        perror("mprotect() error in mm_init()\n");
        return;
    }

    sigemptyset(&sa.sa_mask);

    if (policy == MM_FIFO)
    {
        sa.sa_sigaction = &handler1;
        USED_FRAMES = 0;
        fifo_frames = (struct frame*)malloc(num_frames * page_size);

        for (int i = 0 ; i < num_frames ; i++) {
            fifo_frames[i].phys_addr = (void*)fifo_frames + page_size*i;
            fifo_frames[i].v_page = -1;
            fifo_frames[i].offset = -1;
            fifo_frames[i].read = 0;
            fifo_frames[i].write = 0;
        }

    }
    else
    {
        sa.sa_sigaction = &handler2;
        phy_memory = (entry_t**)malloc(num_frames*sizeof(entry_t));
        init_phy_memory(num_frames);
        phy_memory_full = false;
        front_of_lst = 0;
        rear_of_lst = -1;
    }

    
    sa.sa_flags = SA_SIGINFO;
    // register handler for SIGSEGV
    if (sigaction(SIGSEGV, &sa, NULL) < 0)
    {
        perror("sigaction() error in mm_init()\n");
        return;
    } 


    /*You are STRONGLY SUGGESTED to first write a small, simple, stand-alone program which
    will generate a SIGSEGV fault, and write a signal handler to catch this fault. Additionally,
    investigate how to determine if the rased fault is from a read/write to a page. Ensure
    that you understand how to write signal handers and leverage mprotect() before proceeding.*/

}









