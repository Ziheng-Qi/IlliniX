// csr.h - Access to RISC-V CSRs
//

#ifndef _CSR_H_
#define _CSR_H_

#include <stdint.h>

// mcause

#define RISCV_MCAUSE_INTR_EXCODE_SSI 1
#define RISCV_MCAUSE_INTR_EXCODE_MSI 3
#define RISCV_MCAUSE_INTR_EXCODE_STI 5
#define RISCV_MCAUSE_INTR_EXCODE_MTI 7
#define RISCV_MCAUSE_INTR_EXCODE_SEI 9
#define RISCV_MCAUSE_INTR_EXCODE_MEI 11

static inline intptr_t csrr_mcause(void) {
    intptr_t val;
    asm inline ("csrr %0, mcause" : "=r" (val));
    return val;
}

// scause

#define RISCV_SCAUSE_INTR_EXCODE_SSI 1
#define RISCV_SCAUSE_INTR_EXCODE_STI 5
#define RISCV_SCAUSE_INTR_EXCODE_SEI 9

#define RISCV_SCAUSE_INSTR_ADDR_MISALIGNED  0
#define RISCV_SCAUSE_INSTR_ACCESS_FAULT     1
#define RISCV_SCAUSE_ILLEGAL_INSTR          2
#define RISCV_SCAUSE_BREAKPOINT             3
#define RISCV_SCAUSE_LOAD_ADDR_MISALIGNED   4
#define RISCV_SCAUSE_LOAD_ACCESS_FAULT      5
#define RISCV_SCAUSE_STORE_ADDR_MISALIGNED  6
#define RISCV_SCAUSE_STORE_ACCESS_FAULT     7
#define RISCV_SCAUSE_ECALL_FROM_UMODE       8
#define RISCV_SCAUSE_ECALL_FROM_SMODE       9
#define RISCV_SCAUSE_INSTR_PAGE_FAULT       12
#define RISCV_SCAUSE_LOAD_PAGE_FAULT        13
#define RISCV_SCAUSE_STORE_PAGE_FAULT       15

static inline intptr_t csrr_scause(void) {
    intptr_t val;
    asm inline ("csrr %0, scause" : "=r" (val));
    return val;
}

// mtval

static inline uintptr_t csrr_mtval(void) {
    uintptr_t mtval_cur;
    asm inline ("csrr %0, mtval" : "=r" (mtval_cur));
    return mtval_cur;
}

// stval

static inline uintptr_t csrr_stval(void) {
    uintptr_t stval_cur;
    asm inline ("csrr %0, stval" : "=r" (stval_cur));
    return stval_cur;
}

// mepc

static inline void csrw_mepc(uintptr_t mepc_new) {
    asm inline ("csrw mepc, %0" :: "r" (mepc_new));
}

static inline uintptr_t csrr_mepc(void) {
    uintptr_t mepc_cur;

    asm inline ("csrr %0, mepc" : "=r" (mepc_cur));
    return mepc_cur;
}

// sepc

static inline void csrw_sepc(uintptr_t sepc_new) {
    asm inline ("csrw sepc, %0" :: "r" (sepc_new));
}

static inline uintptr_t csrr_sepc(void) {
    uintptr_t sepc_cur;

    asm inline ("csrr %0, sepc" : "=r" (sepc_cur));
    return sepc_cur;
}

// mscratch

static inline void csrw_mscratch(uintptr_t mscratch_new) {
    asm inline ("csrw mscratch, %0" :: "r" (mscratch_new));
}

static inline uintptr_t csrr_mscratch(void) {
    uintptr_t mscratch_cur;

    asm inline ("csrr %0, mscratch" : "=r" (mscratch_cur));
    return mscratch_cur;
}

// sscratch

static inline void csrw_sscratch(uintptr_t sscratch_new) {
    asm inline ("csrw sscratch, %0" :: "r" (sscratch_new));
}

static inline uintptr_t csrr_sscratch(void) {
    uintptr_t sscratch_cur;

    asm inline ("csrr %0, sscratch" : "=r" (sscratch_cur));
    return sscratch_cur;
}

// mtvec

#define RISCV_MTVEC_MODE_shift 0
#define RISCV_MTVEC_MODE_nbits 2
#define RISCV_MTVEC_BASE_shift 2
#define RISCV_MTVEC_BASE_nbits 60

static inline void csrw_mtvec(void (*handler)(void)) {
    asm inline ("csrw mtvec, %0" :: "r" (handler));
}

// stvec

#define RISCV_STVEC_MODE_shift 0
#define RISCV_STVEC_MODE_nbits 2
#define RISCV_STVEC_BASE_shift 2
#define RISCV_STVEC_BASE_nbits 60

static inline void csrw_stvec(void (*handler)(void)) {
    asm inline ("csrw stvec, %0" :: "r" (handler));
}

// mie

#define RISCV_MIE_SSIE (1 << 1)
#define RISCV_MIE_MSIE (1 << 3)
#define RISCV_MIE_STIE (1 << 5)
#define RISCV_MIE_MTIE (1 << 7)
#define RISCV_MIE_SEIE (1 << 9)
#define RISCV_MIE_MEIE (1 << 11)

static inline void csrw_mie(intptr_t mask) {
    asm inline ("csrw mie, %0" :: "r" (mask));
}

static inline void csrs_mie(intptr_t mask) {
    asm inline ("csrrs zero, mie, %0" :: "r" (mask));
}

static inline void csrc_mie(intptr_t mask) {
    asm inline ("csrrc %0, mie, %0" :: "r" (mask));
}

// sie

#define RISCV_SIE_SSIE (1 << 1)
#define RISCV_SIE_STIE (1 << 5)
#define RISCV_SIE_SEIE (1 << 9)

static inline void csrw_sie(intptr_t mask) {
    asm inline ("csrw sie, %0" :: "r" (mask));
}

static inline void csrs_sie(intptr_t mask) {
    asm inline ("csrrs zero, sie, %0" :: "r" (mask));
}

static inline void csrc_sie(intptr_t mask) {
    asm inline ("csrrc %0, sie, %0" :: "r" (mask));
}

// mip

#define RISCV_MIP_SSIP (1 << 1)
#define RISCV_MIP_MSIP (1 << 3)
#define RISCV_MIP_STIP (1 << 5)
#define RISCV_MIP_MTIP (1 << 7)
#define RISCV_MIP_SEIP (1 << 9)
#define RISCV_MIP_MEIP (1 << 11)

static inline void csrw_mip(intptr_t mask) {
    asm inline ("csrw mip, %0" :: "r" (mask));
}

static inline void csrs_mip(intptr_t mask) {
    asm inline ("csrrs zero, mip, %0" :: "r" (mask));
}

static inline void csrc_mip(intptr_t mask) {
    asm inline ("csrrc %0, mip, %0" :: "r" (mask));
}

// sip

#define RISCV_SIP_SSIP (1 << 1)
#define RISCV_SIP_STIP (1 << 5)
#define RISCV_SIP_SEIP (1 << 9)

static inline void csrw_sip(intptr_t mask) {
    asm inline ("csrw sip, %0" :: "r" (mask));
}

static inline void csrs_sip(intptr_t mask) {
    asm inline ("csrrs zero, sip, %0" :: "r" (mask));
}

static inline void csrc_sip(intptr_t mask) {
    asm inline ("csrrc %0, sip, %0" :: "r" (mask));
}

// mstatus

#define RISCV_MSTATUS_SIE (1UL << 1)
#define RISCV_MSTATUS_MIE (1UL << 3)
#define RISCV_MSTATUS_SPIE (1UL << 3)
#define RISCV_MSTATUS_MPIE (1UL << 3)
#define RISCV_MSTATUS_SPP (1UL << 8)
#define RISCV_MSTATUS_MPP_shift 11
#define RISCV_MSTATUS_MPP_nbits 2
#define RISCV_MSTATUS_SUM (1UL << 18)

static inline intptr_t csrr_mstatus(void) {
    intptr_t val;

    asm inline ("csrr %0, mstatus" : "=r" (val));
    return val;
}

static inline void csrs_mstatus(intptr_t mask) {
    asm inline ("csrrs zero, mstatus, %0" :: "r" (mask));
}

static inline void csrc_mstatus(intptr_t mask) {
    asm inline ("csrrc zero, mstatus, %0" :: "r" (mask));
}

// sstatus

#define RISCV_SSTATUS_SIE (1UL << 1)
#define RISCV_SSTATUS_SPIE (1UL << 3)
#define RISCV_SSTATUS_SPP (1UL << 8)
#define RISCV_SSTATUS_SUM (1UL << 18)

static inline intptr_t csrr_sstatus(void) {
    intptr_t val;

    asm inline ("csrr %0, sstatus" : "=r" (val));
    return val;
}

static inline void csrs_sstatus(intptr_t mask) {
    asm inline ("csrrs zero, sstatus, %0" :: "r" (mask));
}

static inline void csrc_sstatus(intptr_t mask) {
    asm inline ("csrrc zero, sstatus, %0" :: "r" (mask));
}

// satp

#define RISCV_SATP_MODE_Sv39 8
#define RISCV_SATP_MODE_Sv48 9
#define RISCV_SATP_MODE_Sv57 10
#define RISCV_SATP_MODE_Sv64 11

#define RISCV_SATP_MODE_shift 60
#define RISCV_SATP_MODE_nbits 4
#define RISCV_SATP_ASID_shift 44
#define RISCV_SATP_ASID_nbits 16
#define RISCV_SATP_PPN_shift 0
#define RISCV_SATP_PPN_nbits 44

static inline uintptr_t csrr_satp(void) {
    uintptr_t satp_cur;

    asm inline volatile ("csrr %0, satp" : "=r" (satp_cur));
    return satp_cur;
}

static inline void csrw_satp(uintptr_t satp_new) {
    asm inline volatile("csrw satp, %0" :: "r" (satp_new));
}

static inline uintptr_t csrrw_satp(uintptr_t satp_new) {
    uintptr_t satp_old;

    asm inline volatile (
    "csrrw %0, satp, %1"
    : "=r"(satp_old)
    : "r" (satp_new));
    return satp_old;
}

#endif // _CSR_H_
