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

/**
 * @brief Initializes the process manager.
 *
 * This function sets up the process table and initializes the main process.
 * It ensures that all process slots except the main process are set to NULL.
 * The main process is assigned a process ID (pid) of MAIN_PID and a thread ID (tid)
 * corresponding to the currently running thread. The main process's memory tag is
 * set to the active memory space, and the thread is associated with the main process.
 * Additionally, all I/O table entries for the main process are set to NULL.
 * Finally, the process manager is marked as initialized.
 */
void procmgr_init(void){
    // initialize process table
    for(int i = 0; i < NPROC; i++){
        if(i != MAIN_PID){
            proctab[i] = NULL;
        }
    }

    main_proc.id = MAIN_PID; // main process always have pid 0
    main_proc.tid = running_thread(); // main thread always have tid 0
    main_proc.mtag = active_memory_space();
    thread_set_process(main_proc.tid, &main_proc);
    // just in case
    for (int i = 0; i < PROCESS_IOMAX; i++){
        main_proc.iotab[i] = NULL;
    }
    procmgr_initialized = 1;
}

/**
 * @brief Executes a process from the given IO interface.
 *
 * This function performs the following steps to execute a process:
 * 1. Unmaps any virtual memory mappings belonging to other user processes.
 * 2. Creates and initializes a fresh 2nd level (root) page table with the default mappings for a user process.
 * 3. Loads the executable from the provided IO interface into the mapped pages.
 * 4. Starts the thread associated with the process in user-mode.
 *
 * @param exeio Pointer to the IO interface from which the executable is loaded.
 * @return int Returns 0 on success, or a negative error code on failure.
 */
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
/**
 * @brief Terminates the current process and performs necessary cleanup.
 *
 * This function is responsible for terminating the current process by performing
 * the following steps:
 * 1. Reclaims memory space if the running thread is not the main process.
 * 2. Closes all I/O interfaces associated with the current process.
 * 3. Exits the current thread.
 *
 * @note This function should be called when a process needs to be terminated
 *       to ensure proper resource cleanup.
 */
void process_exit(void){

    // reclaim memory space
    if(running_thread() != main_proc.tid){
        memory_space_reclaim();
    }

    // close all io interfaces
    struct process *proc = current_process();
    struct io_intf **iotab = proc->iotab;
    for(int i = 0; i < PROCESS_IOMAX; i++){
        if (iotab[i] != NULL)
        {
            ioclose(iotab[i]);
        }
    }

    // exit current thread
    thread_exit();
}

// struct process *current_process(void) is written in process.h

// int current_pid(void) is written in process.h 

// return 0 in child thread, return tid of child if in parent thread.
/**
 * @brief Forks the current process to create a new child process.
 *
 * This function creates a new child process by finding an unused process ID (PID)
 * from the process table (`proctab`). It assigns the new PID to the child process
 * and sets up the necessary process structures. The function distinguishes between
 * the parent and child processes and returns the appropriate thread ID (TID).
 *
 * @return the TID of the child process.
 * - If called by the child process, it will write 0 to the child's trap frame's a0 instead of directly returning, because the latter will make it write to the parent trap frame
 */
int process_fork(const struct trap_frame * parent_tfr){
    // Find an unused PID for child
    int child_pid = 0;
    for(;child_pid < NPROC; child_pid++){
        if(proctab[child_pid] == NULL){ // this is an unused pid, child pid is now this
            break;
        }
    }

    // create new process struct
    proctab[child_pid] = kmalloc(sizeof(struct process));
    proctab[child_pid]->id = child_pid;
    proctab[child_pid]->mtag = memory_space_clone(0);


    // copies the io_intf pointers from parent's iotab to child's iotab 
    // and increment the reference count
    struct io_intf** child_iotab = proctab[child_pid]->iotab;
    for (int i = 0; i < PROCESS_IOMAX; i++)
    {
        child_iotab[i] = current_process()->iotab[i];
        if (child_iotab[i] != NULL)
        {
            ioref(child_iotab[i]);
        }
    }

    // now every thing with the new process is initiliazed except the thread
    int child_tid = thread_fork_to_user(proctab[child_pid], parent_tfr);
    proctab[child_pid]->tid = child_tid;

    // this return value will only save to parent's trap frame, so just child_tid
    return child_tid;
}