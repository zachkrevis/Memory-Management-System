#include "vmm.h"

// Memory Manager implementation
// Implement all other functions here...

void handler1(int sig, siginfo_t* info, void* ucontext)
{
    if (sig == SIGSEGV)
    {
        ucontext_t* ctx = (ucontext_t*)ucontext;
        long long err = ctx->uc_mcontext.gregs[REG_ERR];
        void* addr = info->si_addr;

        void* v_addr = (void*)(addr-(addr-START_OF_VM)%PAGE_SIZE);
        int v_page=(addr-START_OF_VM-(addr-START_OF_VM)%PAGE_SIZE)/PAGE_SIZE;
        int offset = (addr-START_OF_VM)%PAGE_SIZE;

        unsigned int phys_addr = 0;
        int write_back = 0;
        int evicted = -1;
        int type = 0;

        if (err & 0x2)
        {

            if (!page_in_frames(v_page)) {
                type = 1;
                phys_addr = add_to_mem_FIFO(v_page, offset, type, &write_back, &evicted);
            } else {
                phys_addr = update_in_mem_FIFO(v_page, offset, &type, false);
            }

            if (mprotect(v_addr, PAGE_SIZE, PROT_WRITE | PROT_READ) == -1)
            {
                perror("mprotect() error in handler1()");
                raise(SIGKILL);
                return;
            }
        }
        else
        {

            if (!page_in_frames(v_page)) {
                type = 0;
                phys_addr = add_to_mem_FIFO(v_page, offset, type, &write_back, &evicted);
            } else {

                type = 3;
                phys_addr = update_in_mem_FIFO(v_page, offset, &type, true);
            }

            if (mprotect(v_addr, PAGE_SIZE, PROT_READ) == -1)
            {
                perror("mprotect() error in handler1()");
                raise(SIGKILL);
                return;
            }

        }
        mm_logger(v_page, type, evicted, write_back, (unsigned int)phys_addr + offset);
    }
    return;
}


int virtual_protect(int v_page, void* v_addr, int prot) {
    return mprotect((void*)v_addr+(PAGE_SIZE*v_page), PAGE_SIZE, prot);
}

unsigned int add_to_mem_FIFO(int v_page, int offset, int type, int* write_back, int* evicted) {

    if (NUM_FRAMES <= USED_FRAMES) {
        *evicted = evict_head(write_back);
    }

    struct v_page_FIFO* newPage = (struct v_page_FIFO*)malloc(sizeof(struct v_page_FIFO));
    newPage->page = v_page;
    newPage->type = type;
    newPage->next = NULL;

    enqueue(newPage);

    unsigned int phys_addr = add_to_frames(v_page, offset, type);
    return phys_addr;
}

unsigned int update_in_mem_FIFO(int v_page, int offset, int* type, bool read) {
    for (int i = 0 ; i < NUM_FRAMES ; i++) {
        if (fifo_frames[i].v_page == v_page) {
            fifo_frames[i].offset = offset;

            struct v_page_FIFO* temp = fifo_queue;
            if (!read) {
                if (fifo_frames[i].write == 0 & fifo_frames[i].read == 1) {
                    fifo_frames[i].write = 1;
                    fifo_frames[i].read = 1;
                    *type = 2;
                }
                else if (fifo_frames[i].write == 1 & fifo_frames[i].read == 1) {
                    fifo_frames[i].write = 1;
                    fifo_frames[i].read = 1;
                    *type = 4;
                }
            }
            while (temp != NULL) {
                if (temp->page == v_page) {
                    temp->type = *type;
                    break;
                }
                temp = temp->next;
            }
            return (long long)fifo_frames[i].phys_addr - (long long)fifo_frames;
        }
    }
    return -1;
}


int evict_head(int* evicted_type) {
    if (fifo_queue == NULL)
    {
        return -1;
    }
    struct v_page_FIFO* v_page = fifo_queue;

    fifo_queue = fifo_queue->next;
    USED_FRAMES--;

    int page = v_page->page;
    if (v_page->type != 0) {
        *evicted_type = 1;
    } else {
        *evicted_type = 0;
    }

    evict_from_mem(page);
    free(v_page);

    if (virtual_protect(page, START_OF_VM, PROT_NONE) == -1) {
        perror("mprotect() error in handler1()");
        raise(SIGKILL);
        return -1;
    }

    return page;
}

int print_v_pages(struct v_page_FIFO* v_pages) {
    struct v_page_FIFO* temp = v_pages;
    printf("printing v pages\n");
    while (temp != NULL) {
        printf("%d-%d ", temp->page, temp->type);
        temp = temp->next;
    }
    printf("\n");
}

void enqueue(struct v_page_FIFO* newPage) {
    if (USED_FRAMES == 0) {
        fifo_queue = newPage;
    }
    else {
        struct v_page_FIFO* temp = fifo_queue;
        while (temp->next != NULL) {
            temp = temp->next;
        }

        temp->next = newPage;
    }
    USED_FRAMES++;
}

int evict_from_mem(int v_page) {
    for (int i = 0 ; i < NUM_FRAMES ; i++) {
        if (fifo_frames[i].v_page == v_page) {
            fifo_frames[i].v_page = -1;
            fifo_frames[i].offset = -1;
            fifo_frames[i].read = 0;
            fifo_frames[i].write = 0;
            break;
        }
    }
}

u_int32_t add_to_frames(int v_page, int offset, int type) {
    u_int32_t frame;
    for (int i = 0 ; i < NUM_FRAMES ; i++) {
        if (fifo_frames[i].v_page == -1) {
            fifo_frames[i].v_page = v_page;
            fifo_frames[i].offset = offset;
            fifo_frames[i].read = 1;
            fifo_frames[i].write = type;
            frame = (long long)fifo_frames[i].phys_addr - (long long)fifo_frames;
            break;
        }
    }
    return frame;
}

void print_mem() {
    for (int i = 0 ; i < NUM_FRAMES ; i++) {
        printf("%p - %d %d %d %d\n\n", fifo_frames[i].phys_addr - (void *)fifo_frames, fifo_frames[i].v_page, fifo_frames[i].offset, fifo_frames[i].read, fifo_frames[i].write);
    }
}

bool page_in_frames(int v_page) {
    for (int i = 0 ; i < NUM_FRAMES ; i++) {
        if (fifo_frames[i].v_page == v_page) {
            return true;
        }
    }
    return false;
}



void handler2(int sig, siginfo_t* info, void* ucontext)
{
    // printf("transfer control to handler\n");
    int virt_page = -1;
    int fault_type = -1;
    int evicted_page = -1;
    int write_back = 0;
    u_int32_t phy_addr;

    if (sig == SIGSEGV)
    {
        ucontext_t* ctx = (ucontext_t*)ucontext;
        long long err = ctx->uc_mcontext.gregs[REG_ERR];
        void* addr = info->si_addr;

        // get offset of virtual memory
        int offset = ((long long)addr - (long long)START_OF_VM)%PAGE_SIZE;
        // get base address for virtual page that was accessed 
        void* base_addr = (void*)(long long)addr - offset;
        virt_page = ((long long)base_addr -(long long)START_OF_VM)/PAGE_SIZE;

        /*
        (1, 0) PROT_READ
        (1, 1) PROT_READ | PROT_WRITE
        (0, 0) PROT_NONE
        (0, 1) PROT_NONE in order to change r_bit when referenced
        */
        // Check if page is already in phy_memory
        // Add page to next available space in phy_memory (include base_addr)
        // The phy_addr can be calculated by index*page_size + offset*4
        // Give the approapriate mprotect()
        // Evict page if needed.
        if (err & 0x2)
        {
            // printf("its a sigsegv from write access at address %p\n", addr);
            // Write sigsegv if m_bit == 0 or not phy_memory...
            if (!is_page_in_phy_memory(base_addr))
            {
                fault_type = 1;
                phy_addr = add_page_to_phy_memory(base_addr, false, offset, virt_page, &evicted_page, &write_back);
            }
            else
            {
                phy_addr = update_page_in_phy_memory(base_addr, false, offset, &fault_type);
            }
        }
        else
        {
            // printf("its a sigsegv from read access at address %p\n", addr);
            // Read sigsegv if r_bit == 0 or not in phy_memory...
            if (!is_page_in_phy_memory(base_addr))
            {
                fault_type = 0;
                phy_addr = add_page_to_phy_memory(base_addr, true, offset, virt_page, &evicted_page, &write_back);
            }
            else
            {
                fault_type = 3;
                phy_addr = update_page_in_phy_memory(base_addr, true, offset, &fault_type);
            }
        }
    }
    mm_logger(virt_page, fault_type, evicted_page, write_back, phy_addr);
    return;
}

void init_phy_memory(int num_frames)
{
    for (int i = 0; i < num_frames; i++)
    {
        entry_t* entry = (entry_t*)malloc(sizeof(entry_t));
        entry->virt_page = -1;
        entry->base_addr = NULL;
        entry->r_bit = 0;
        entry->m_bit = 0;

        phy_memory[i] = entry;
    }
    return;
}

bool is_page_in_phy_memory(void* base_addr)
{
    for (int i = 0; i < PHY_MEMORY_SIZE; i++)
    {
        if (phy_memory[i]->base_addr != NULL)
        {
            if (phy_memory[i]->base_addr == base_addr)
            {
                return true;
            }
        }
    }
    return false;
}

void third_chance_evict(int* evicted_page, int* write_back)
{
    while(true)
    {
        entry_t* entry = phy_memory[front_of_lst];
        if ((entry->r_bit == 0 && entry->m_bit == 0) || (entry->r_bit == -1 && entry->m_bit == 1))
        {
            mprotect(entry->base_addr, PAGE_SIZE, PROT_NONE);
            *evicted_page = entry->virt_page;
            if (entry->m_bit == 1)
            {
                *write_back = 1;
            }
            entry->base_addr = NULL;
            front_of_lst = (front_of_lst + 1)%NUM_FRAMES;
            return;
        }
        if (entry->r_bit == 1)
        {
            entry->r_bit = 0;
            mprotect(entry->base_addr, PAGE_SIZE, PROT_NONE);
        }
        else
        {
            // printf("hellooooo %d\n", entry->r_bit);
            entry->r_bit = -1;
            // printf("byeeeeeee %d\n", entry->r_bit);

        }
        front_of_lst = (front_of_lst + 1)%NUM_FRAMES;
        rear_of_lst = (rear_of_lst + 1)%NUM_FRAMES;
    }
}

u_int32_t add_page_to_phy_memory(void* base_addr, bool read, int offset, int virt_page, int* evicted_page, int* write_back)
{
    int phy_addr;
    entry_t* entry = (entry_t*)malloc(sizeof(entry_t));
    entry->virt_page = virt_page;
    entry->base_addr = base_addr;

    if (phy_memory_full)
    {
        third_chance_evict(evicted_page, write_back);
        // please return if there is a write_back....

    }
    if (read)
    {
        entry->r_bit = 1;
        entry->m_bit = 0;
        mprotect(base_addr, PAGE_SIZE, PROT_READ);
    }
    else
    {
        entry->r_bit = 1;
        entry->m_bit = 1;
        mprotect(base_addr, PAGE_SIZE, PROT_READ | PROT_WRITE);
    }
    // add page
    rear_of_lst = (rear_of_lst + 1)%NUM_FRAMES;
    phy_memory[rear_of_lst] = entry;
    phy_addr = rear_of_lst*PAGE_SIZE + offset;  // calculate physical address to return

    // check and update if phy_memory is full
    if ((rear_of_lst + 1)%NUM_FRAMES == front_of_lst)
    {
        phy_memory_full = true;
    }

    return phy_addr;
}

u_int32_t update_page_in_phy_memory(void* base_addr, bool read, int offset, int* fault_type)
{
    for (int i = 0; i < PHY_MEMORY_SIZE; i++)
    {
        if (phy_memory[i]->base_addr != NULL)
        {
            if (phy_memory[i]->base_addr == base_addr)
            {
                if (!read)
                {
                    // Could be (0, 0) or (1, 0) or (0, 1) or (-1, 1)
                    int r = phy_memory[i]->r_bit;
                    int m = phy_memory[i]->m_bit;
                    if ((r == 0 && m == 0)||(r == 1 && m == 0))
                    {
                        *fault_type = 2;
                    }
                    else
                    {
                        *fault_type = 4;
                    }
                    phy_memory[i]->m_bit = 1;
                    mprotect(base_addr, PAGE_SIZE, PROT_READ | PROT_WRITE);
                }
                else
                {
                    // Could be (0,0) or (0, 1) or (-1, 1)
                    if (phy_memory[i]->m_bit == 1)
                    {
                        mprotect(base_addr, PAGE_SIZE, PROT_READ | PROT_WRITE);
                    }
                    else
                    {
                        mprotect(base_addr, PAGE_SIZE, PROT_READ);
                    }
                }
                phy_memory[i]->r_bit = 1;
                return i*PAGE_SIZE + offset;
            }
        }
    }
    printf("Updating page in physical memory failed...\n");
    return 0;
}












