# thrasm.s - Special functions called from thread.c
#

# struct thread * _thread_swtch(struct thread * resuming_thread)

# Switches from the currently running thread to another thread and returns when
# the current thread is scheduled to run again. Argument /resuming_thread/ is
# the thread to be resumed. Returns a pointer to the previously-scheduled
# thread. This function is called in thread.c. The spelling of swtch is
# historic.

        .text
        .global _thread_swtch
        .type   _thread_swtch, @function
        
        .extern thread_exit

_thread_swtch:

        # We only need to save the ra and s0 - s12 registers. Save them on
        # the stack and then save the stack pointer. Our declaration is:
        # 
        #   struct thread * _thread_swtch(struct thread * resuming_thread);
        #
        # The currently running thread is suspended and resuming_thread is
        # restored to execution. swtch returns when execution is switched back
        # to the calling thread. The return value is the previously executing
        # thread. Interrupts are enabled when swtch returns.
        #
        # tp = pointer to struct thread of current thread (to be suspended)
        # a0 = pointer to struct thread of thread to be resumed
        # 

        sd      s0, 0*8(tp)
        sd      s1, 1*8(tp)
        sd      s2, 2*8(tp)
        sd      s3, 3*8(tp)
        sd      s4, 4*8(tp)
        sd      s5, 5*8(tp)
        sd      s6, 6*8(tp)
        sd      s7, 7*8(tp)
        sd      s8, 8*8(tp)
        sd      s9, 9*8(tp)
        sd      s10, 10*8(tp)
        sd      s11, 11*8(tp)
        sd      ra, 12*8(tp)
        sd      sp, 13*8(tp)

        mv      tp, a0

        ld      sp, 13*8(tp)
        ld      ra, 12*8(tp)
        ld      s11, 11*8(tp)
        ld      s10, 10*8(tp)
        ld      s9, 9*8(tp)
        ld      s8, 8*8(tp)
        ld      s7, 7*8(tp)
        ld      s6, 6*8(tp)
        ld      s5, 5*8(tp)
        ld      s4, 4*8(tp)
        ld      s3, 3*8(tp)
        ld      s2, 2*8(tp)
        ld      s1, 1*8(tp)
        ld      s0, 0*8(tp)



        ret

        .global _thread_setup
        .type   _thread_setup, @function
        
# void _thread_setup (
#      struct thread * thr,             in a0
#      void * sp,                       in a1
#      void (*start)(void * arg),       in a2
#      void * arg)                      in a3
#
# Sets up the initial context for a new thread. The thread will begin execution
# in /start/, receiving /arg/ as the first argument. 

_thread_setup:
        # FIXME your code goes here
        
        # Save the stack pointer in the thread structure.
        # this function will only initial the context, which is the first struct of the thread struct
        
        # a1 is the stack pointer and stored in the thread struct
        
        sd      a1, 13*8(a0)

        la      t0, _thread_start

        # Save the start function and argument in the thread structure.
        sd      t0, 12*8(a0)
        # a2 is the start function and stored in the thread struct, with a3 as the argument
        sd      a2, 0*8(a0)
        sd      a3, 1*8(a0)
        sd      zero, 2*8(a0)
        sd      zero, 3*8(a0)
        sd      zero, 4*8(a0)
        sd      zero, 5*8(a0)
        sd      zero, 6*8(a0)
        sd      zero, 7*8(a0)
        sd      zero, 8*8(a0)
        sd      zero, 9*8(a0)
        sd      zero, 10*8(a0)
        sd      zero, 11*8(a0)


        ret

        .global _thread_start
        .type   _thread_start, @function

# void _thread_start(void (*start)(void * arg), void * arg)

# This function is called by the scheduler to start a new thread. It is called
# with the start function and argument that were passed to _thread_setup. The
# function should return to thread_exit when the thread is done.

_thread_start:
        # passing arguments to the start function
        mv      a0, s1
        mv      a1, s0

        jalr    ra, a1, 0

        call    thread_exit  



        # thread_exit will never return



# Statically allocated stack for the idle thread.

        .section        .data.idle_stack
        .align          16
        
        .equ            IDLE_STACK_SIZE, 1024
        .equ            IDLE_GUARD_SIZE, 0

        .global         _idle_stack
        .type           _idle_stack, @object
        .size           _idle_stack, IDLE_STACK_SIZE

        .global         _idle_guard
        .type           _idle_guard, @object
        .size           _idle_guard, IDLE_GUARD_SIZE

_idle_stack:
        .fill   IDLE_STACK_SIZE, 1, 0xA5

_idle_guard:
        .fill   IDLE_GUARD_SIZE, 1, 0x5A
        .end

