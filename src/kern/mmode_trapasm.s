### PLEASE READ:
# This should replace the corresponding lines in your current trapasm.s
#
# That is, copy the text below, place your cursor at .global _mmode_trap_entry and select until the end of file,
# and paste.
#
# You must redefine _mmode_trap_entry to have a new definition.
#
# Or, another way of saying it is that we are replacing the old mmode trap entry (which originally just panicked)
# to provide some key functionality in addition to the normal panic function. The purpose and motivation for this
# is discussed in the comments below.
#
#

        .global _mmode_trap_entry
        .type   _mmode_trap_entry, @function
        .balign 4 # Trap entry must be 4-byte aligned for mtvec CSR

# RISC-V does not provide a built-in S mode timer, only an M mode timer. The
# rationale is that the M mode environment will provide a virtualized timer to S
# mode guests. That's fancy and modern, but we're trying to create a "bare
# metal" OS. So the solution we have here is this:
#
#   1. S mode has full RW access to the memory-mapped timer registers.
#   2. When S mode wants to enable timer interrupts, in addition to setting the
#      STIE bit, it also executes an ecall instruction, which we handle here,
#      setting MTIE and clearing STIP.
#   3. When a M mode timer interrupt occurs, we set STIP and clear MTIE. S mode
#      then needs to re-arm timer interrupts using (2).
#

_mmode_trap_entry:
        # Stash t0 away in mscratch

        csrw    mscratch, t0

        csrr    t0, mcause
        bgez    t0, mmode_excp_handler

        # If it's not a timer interrupt, panic

        addi    t0, t0, -7      # subtract 7
        slli    t0, t0, 1       # clear msb
        bnez    t0, unexpected_mmode_trap

mmode_intr_handler:

        # Set STIP, clear MTIE

        li      t0, 0x20        # STIP
        csrs    mip, t0
        slli    t0, t0, 2       # MTIE
        csrc    mie, t0
        j       mmode_trap_done

mmode_excp_handler:
        # We support one S mode to M mode environment call, which is to re-arm
        # the timer interrupt.

        addi    t0, t0, -9
        bnez    t0, unexpected_mmode_trap

        # Clear STIP, set MTIE

        li      t0, 0x20        # STIP
        csrc    mip, t0
        slli    t0, t0, 2       # MTIE
        csrs    mie, t0

        # Advance mepc past ecall instruction

        csrr    t0, mepc
        addi    t0, t0, 4
        csrw    mepc, t0
       
mmode_trap_done:
        csrr    t0, mscratch
        mret


unexpected_mmode_trap:
        # We can call panic in M mode since panic does not rely on any kernel
        # virtual mappings (which, at the time of this writing, do not exist
        # anyway).

        la      a0, trap_mmode_cont
        call    panic

        .section        .rodata, "a", @progbits
trap_mmode_cont:
        .asciz          "Unexpected M-mode trap"

        .end
