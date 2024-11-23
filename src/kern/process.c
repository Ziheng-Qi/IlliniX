// process.c - user process
//

#ifdef PROCESS_TRACE
#define TRACE
#endif

#ifdef PROCESS_DEBUG
#define DEBUG
#endif

#include "process.h"
#include "memory.h"
#include "elf.h"
#include "thread.h"

// COMPILE-TIME PARAMETERS
//

// NPROC is the maximum number of processes

#ifndef NPROC
#define NPROC 16
#endif

// INTERNAL FUNCTION DECLARATIONS
//

// INTERNAL GLOBAL VARIABLES
//

#define MAIN_PID 0

// The main user process struct

static struct process main_proc;

// A table of pointers to all user processes in the system

struct process * proctab[NPROC] = {
    [MAIN_PID] = &main_proc
};

// EXPORTED GLOBAL VARIABLES
//

char procmgr_initialized = 0;

// EXPORTED FUNCTION DEFINITIONS
//

void procmgr_init(void){
    main_proc.id = MAIN_PID; // main process always have pid 0
    main_proc.tid = running_thread(); // main thread always have tid 0
    main_proc.mtag = active_memory_space();
    thread_set_process(main_proc.tid, &main_proc);
    for (int i = 0; i < PROCESS_IOMAX; i++){
        main_proc.iotab[i] = NULL;
    }
    procmgr_initialized = 1;
}



int process_exec(struct io_intf *exeio){
    // 1. Any virtual memory mappings belonging to other user processes should be unmapped
    memory_unmap_and_free_user();
    // 2. A fresh 2nd level (root) page table should be created and initialized with the default mappings for a user process
    // 3. The executable should be loaded from the IO interface provided as an argument into the mapped pages
    uintptr_t entry;
    int result = elf_load(exeio, (void (**)(void)) & entry);
    if (result < 0){
        return result;
    }

    // //4. The thread associated with the process needs to be started in user-mode.
    // An assembly function in thrasm.s would be useful here
    thread_jump_to_user(USER_STACK_VMA, entry);

    // ((void (*)())entry)();  equivalent to this but in user mode
}


//  Cleans up after a finished process by reclaiming the resources of the process. Anything that was
//      associated with the process at initial execution should be released. This covers:
//  • Process memory space
//  • Open I/O interfaces
//  • Associated kernel thread
void process_exit(void){

     
    // reclaim memory space
    if(running_thread() == main_proc.tid){
        memory_space_reclaim();
    }

    // close all io interfaces
    struct process *proc = current_process();
    struct io_intf **iotab = proc->iotab;
    for(int i = 0; i < PROCESS_IOMAX; i++){
        if(iotab[i] != NULL){
            struct io_intf* io = iotab[i];
            io->ops->close(io);
        }
    }

    // exit current thread
    thread_exit();
}

// struct process *current_process(void) is written in process.h

// int current_pid(void) is written in process.h 