#include "interface.h"
#include "vmm.h"

// Interface implementation
// Implement APIs here...

void mm_init(enum policy_type policy, void *vm, int vm_size, int num_frames, int page_size)
{
    // Make the virtual memory space inaccessible
    if (mprotect(vm, vm_size, PROT_NONE) == -1)
    {
        perror("mprotect() error - mm_init()");
        return;
    }

    memset(&sa, '\0', sizeof(sa));
    sa.sa_sigaction = &handler;
    sa.sa_flags = SA_SIGINFO;
    // register handler for SIGSEGV
    if (sigaction(SIGSEGV, &sa, NULL) < 0)
    {
        perror("sigaction error - mm_init()");
        return;
    } 


    /*You are STRONGLY SUGGESTED to first write a small, simple, stand-alone program which
    will generate a SIGSEGV fault, and write a signal handler to catch this fault. Additionally,
    investigate how to determine if the rased fault is from a read/write to a page. Ensure
    that you understand how to write signal handers and leverage mprotect() before proceeding.*/

    /*void handler(int sig)
{
    write(1, "success", strlen("success")); // printf is not recommended here, but should work as well
    exit(0);
}

int main()
{
    struct sigaction sa;
    int *a; // a will contain some garbage value
    int b = *a; // trigger segmentation fault; transfer control to handler*/

}









