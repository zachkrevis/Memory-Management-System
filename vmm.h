#ifndef VMM_H
#define VMM_H

#include "interface.h"

// Declare your own data structures and functions here...

struct frame {
    int v_page;
    int offset;
    int read;
    int write;
    void* phys_addr;
};

struct v_page_FIFO {
    int page;
    int type;
    struct v_page_FIFO* next;
};

struct entry
{
    int virt_page;
    void* base_addr;
    int r_bit;
    int m_bit;
};
typedef struct entry entry_t;

void init_phy_memory(int num_frames);
bool is_page_in_phy_memory(void* base_addr);
void third_chance_evict(int* evicted_page, int* write_back);
u_int32_t add_page_to_phy_memory(void* base_addr, bool read, int offset, int virt_page, int* evicted_page, int* write_back);
u_int32_t update_page_in_phy_memory(void* base_addr, bool read, int offset, int* fault_type);

void handler1(int sig, siginfo_t* info, void* ucontext);
void handler2(int sig, siginfo_t* info, void* ucontext);

int virtual_protect(int v_page, void* v_addr, int prot);
unsigned int add_to_mem_FIFO(int v_page, int offset, int type, int* write_back, int* evicted);
int evict_head(int* evicted_type);
int print_v_pages(struct v_page_FIFO* v_pages);
void enqueue(struct v_page_FIFO* newPage);
int evict_from_mem(int v_page);
unsigned int add_to_frames(int v_page, int offset, int type);
void print_mem();
unsigned int update_in_mem_FIFO(int v_page, int offset, int* type, bool read);
bool page_in_frames(int v_page);











#endif
