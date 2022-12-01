#include "vmm.h"

// Memory Manager implementation
// Implement all other functions here...

void handler1(int sig, siginfo_t* info, void* ucontext)
{
    printf("transfer control to handler\n");
    if (sig == SIGSEGV)
    {
        ucontext_t* ctx = (ucontext_t*)ucontext;
        long long err = ctx->uc_mcontext.gregs[REG_ERR];
        void* addr = info->si_addr;

        if (err & 0x2)
        {
            printf("its a sigsegv from write access at address %p\n", addr);
        }
        else
        {
            printf("its a sigsegv from read access at address %p\n", addr);
        }
    }
    // logger(fault, asdklfj;alsdjkf, )
    return;
}

void handler2(int sig, siginfo_t* info, void* ucontext)
{
    printf("transfer control to handler\n");
    if (sig == SIGSEGV)
    {
        ucontext_t* ctx = (ucontext_t*)ucontext;
        long long err = ctx->uc_mcontext.gregs[REG_ERR];
        void* addr = info->si_addr;

        if (err & 0x2)
        {
            printf("its a sigsegv from write access at address %p\n", addr);
        }
        else
        {
            printf("its a sigsegv from read access at address %p\n", addr);
        }
    }
    // logger
    return;
}














