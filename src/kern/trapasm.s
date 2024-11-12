# trap.s
#

        # struct trap_frame {
        #     uint64_t x[32]; // x[0] used to save tp when in U mode
        #     uint64_t sstatus;
        #     uint64_t sepc;
        # };

        .macro  save_gprs_except_t6_and_sp
        # Saves all general purpose registers except sp and t6 to trap frame to
        # which sp points. (Save original sp and t6 before using this macro.)
        sd      x1, 1*8(sp)     # x1 is ra
        sd      x3, 3*8(sp)     # x3 is gp
        sd      x4, 4*8(sp)     # x4 is tp
        sd      x5, 5*8(sp)     # x5 is t0
        sd      x6, 6*8(sp)     # x6 is t1
        sd      x7, 7*8(sp)     # x7 is t2
        sd      x8, 8*8(sp)     # x8 is s0/fp
        sd      x9, 9*8(sp)     # x9 is s1
        sd      x10, 10*8(sp)   # x10 is a0
        sd      x11, 11*8(sp)   # x11 is a1
        sd      x12, 12*8(sp)   # x12 is a2
        sd      x13, 13*8(sp)   # x13 is a3
        sd      x14, 14*8(sp)   # x14 is a4
        sd      x15, 15*8(sp)   # x15 is a5
        sd      x16, 16*8(sp)   # x16 is a6
        sd      x17, 17*8(sp)   # x17 is a7
        sd      x18, 18*8(sp)   # x18 is s2
        sd      x19, 19*8(sp)   # x19 is s3
        sd      x20, 20*8(sp)   # x20 is s4
        sd      x21, 21*8(sp)   # x21 is s5
        sd      x22, 22*8(sp)   # x22 is s6
        sd      x23, 23*8(sp)   # x23 is s7
        sd      x24, 24*8(sp)   # x24 is s8
        sd      x25, 25*8(sp)   # x25 is s9
        sd      x26, 26*8(sp)   # x26 is s10
        sd      x27, 27*8(sp)   # x27 is s11
        sd      x28, 28*8(sp)   # x28 is t3
        sd      x29, 29*8(sp)   # x29 is t4
        sd      x30, 30*8(sp)   # x30 is t5
        .endm

        .macro  save_sstatus_and_sepc
        # Saves sstatus and sepc to trap frame to which sp points. Uses t6 as a
        # temporary. This macro must be used after the original t6 and sp have
        # been saved to the trap frame.
        csrr    t6, sstatus
        sd      t6, 32*8(sp)
        csrr    t6, sepc
        sd      t6, 33*8(sp)
        .endm

        .macro  restore_sstatus_and_sepc
        # Restores sstatus and sepc from trap frame to which sp points. We use
        # t6 as a temporary, so must be used after this macro, not before.

        ld      t6, 33*8(sp)
        csrw    sepc, t6
        ld      t6, 32*8(sp)
        csrw    sstatus, t6
        .endm

        .macro  restore_gprs_except_t6_and_sp

        ld      x30, 30*8(sp)   # x30 is t5
        ld      x29, 29*8(sp)   # x29 is t4
        ld      x28, 28*8(sp)   # x28 is t3
        ld      x27, 27*8(sp)   # x27 is s11
        ld      x26, 26*8(sp)   # x26 is s10
        ld      x25, 25*8(sp)   # x25 is s9
        ld      x24, 24*8(sp)   # x24 is s8
        ld      x23, 23*8(sp)   # x23 is s7
        ld      x22, 22*8(sp)   # x22 is s6
        ld      x21, 21*8(sp)   # x21 is s5
        ld      x20, 20*8(sp)   # x20 is s4
        ld      x19, 19*8(sp)   # x19 is s3
        ld      x18, 18*8(sp)   # x18 is s2
        ld      x17, 17*8(sp)   # x17 is a7
        ld      x16, 16*8(sp)   # x16 is a6
        ld      x15, 15*8(sp)   # x15 is a5
        ld      x14, 14*8(sp)   # x14 is a4
        ld      x13, 13*8(sp)   # x13 is a3
        ld      x12, 12*8(sp)   # x12 is a2
        ld      x11, 11*8(sp)   # x11 is a1
        ld      x10, 10*8(sp)   # x10 is a0
        ld      x9, 9*8(sp)     # x9 is s1
        ld      x8, 8*8(sp)     # x8 is s0/fp
        ld      x7, 7*8(sp)     # x7 is t2
        ld      x6, 6*8(sp)     # x6 is t1
        ld      x5, 5*8(sp)     # x5 is t0
        ld      x4, 4*8(sp)     # x4 is tp
        ld      x3, 3*8(sp)     # x3 is gp
        ld      x1, 1*8(sp)     # x1 is ra

        .endm

        .text
        .global _trap_entry_from_smode
        .type   _trap_entry_from_smode, @function
        .balign 4 # Trap entry must be 4-byte aligned

_trap_entry_from_smode:

        # Save t6 and original sp to trap frame, then save rest

        addi    sp, sp, -34*8   # allocate space for trap frame
        sd      t6, 31*8(sp)    # save t6 (x31) in trap frame
        addi    t6, sp, 34*8    # save original sp
        sd      t6, 2*8(sp)     # 

        save_gprs_except_t6_and_sp
        save_sstatus_and_sepc

        call    trap_smode_cont

        # S mode handlers return here because the call instruction above places
        # this address in /ra/ before we jump to exception or trap handler.

        restore_sstatus_and_sepc
        restore_gprs_except_t6_and_sp
        
        ld      t6, 31*8(sp)
        ld      sp, 2*8(sp)

        sret

        # Execution of trap entry continues here. Jump to handlers.

trap_smode_cont:
        csrr    a0, scause      # a0 contains "exception code"
        mv      a1, sp          # a1 contains trap frame pointer
        
        bgez    a0, smode_excp_handler  # in excp.c
        
        slli    a0, a0, 1               # clear msb
        srli    a0, a0, 1               #

        j       intr_handler            # in intr.c


        .global _trap_entry_from_umode
        .type   _trap_entry_from_umode, @function
        .balign 4 # Trap entry must be 4-byte aligned

_trap_entry_from_umode:

        # When we're in S mode, sscratch points to the kernel thread's
        # thread_stack_anchor struct, which contains the thread pointer. The
        # address of the thread_stack_anchor also serves as our initial kernel
        # stack pointer. We start by allocating a trap frame and saving t6
        # there, so we can use it as a temporary register.


        # TODO: FIXME your code here

        # We're now in S mode, so update our trap handler address to
        # _trap_entry_from_smode.

        # TODO: FIXME your code here

        # U mode handlers return here because the call instruction above places
        # this address in /ra/ before we jump to exception or trap handler.
        # We're returning to U mode, so restore _smode_trap_entry_from_umode as
        # trap handler.

        # TODO: FIXME your code here

        # Execution of trap entry continues here. Jump to handlers.

trap_umode_cont:
        
        # TODO: FIXME your code here

        .global _mmode_trap_entry
        .type   _mmode_trap_entry, @function
        .balign 4 # Trap entry must be 4-byte aligned for mtvec CSR

_mmode_trap_entry:

        # All traps are redirected to S mode, so if we end up here, something
        # went very wrong.

        la      a0, trap_mmode_cont
        call    panic

        .section        .rodata, "a", @progbits
trap_mmode_cont:
        .asciz          "Unexpected M-mode trap"

        .end