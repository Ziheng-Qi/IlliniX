// process.h - User process
//

#ifndef _PROCESS_H_
#define _PROCESS_H_

#ifndef PROCESS_IOMAX
#define PROCESS_IOMAX 16
#endif

#include "config.h"
#include "io.h"
#include "thread.h"
#include <stdint.h>

// EXPORTED TYPE DEFINITIONS
//

struct process {
    int id; // process id of this process
    int tid; // thread id of associated thread
    uintptr_t mtag; // memory space identifier
    struct io_intf * iotab[PROCESS_IOMAX];
};

// EXPORTED VARIABLES DECLARATIONS
//

extern char procmgr_initialized;
extern struct process * proctab[];

// EXPORTED FUNCTION DECLARATIONS
//

extern void procmgr_init(void);
extern int process_exec(struct io_intf * exeio);

extern void __attribute__ ((noreturn)) process_exit(void);

extern void process_terminate(int pid);

static inline struct process * current_process(void);
static inline int current_pid(void);

// INLINE FUNCTION DEFINITIONS
// 

static inline struct process * current_process(void) {
    return thread_process(running_thread());
}

static inline int current_pid(void) {
    return thread_process(running_thread())->id;
}

#endif // _PROCESS_H_
