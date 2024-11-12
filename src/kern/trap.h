// trap.h - Trap entry and initialization
// 

#ifndef _TRAP_H_
#define _TRAP_H_

#include <stdint.h>

// All traps are handled by _trap_entry defined in trap.s, which saves the
// current context in a struct trap_frame on the stack and dispatches to one of
// the handlers listed below. It arranges to restore the saved context when the
// handler returns.

#define TFR_ZERO    0
#define TFR_RA      1
#define TFR_SP      2
#define TFR_GP      3
#define TFR_TP      4
#define TFR_T0      5
#define TFR_T1      6
#define TFR_T2      7
#define TFR_S0      8
#define TFR_S1      9
#define TFR_A0      10
#define TFR_A1      11
#define TFR_A2      12
#define TFR_A3      13
#define TFR_A4      14
#define TFR_A5      15
#define TFR_A6      16
#define TFR_A7      17
#define TFR_S2      18
#define TFR_S3      19
#define TFR_S4      20
#define TFR_S5      21
#define TFR_S6      22
#define TFR_S7      23
#define TFR_S8      24
#define TFR_S9      25
#define TFR_S10     26
#define TFR_S11     27
#define TFR_T3      28
#define TFR_T4      29
#define TFR_T5      30
#define TFR_T6      31

struct trap_frame {
    uint64_t x[32]; // x[0] used to save tp when in U mode
    uint64_t sstatus;
    uint64_t sepc;
};

// _trap_entry_from_Xmode in trapasm.s dispatches to:
// 
//   Xmode_excp_handler defined in excp.c or
//   Xmode_intr_handler defined in intr.c
//
// where X is either s or u.

extern void smode_excp_handler(unsigned int code, struct trap_frame * tfr);
extern void umode_excp_handler(unsigned int code, struct trap_frame * tfr);
extern void intr_handler(int code, struct trap_frame * tfr);

#endif // _TRAP_H_
