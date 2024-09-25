#ifndef DECODE_H_
#define DECODE_H_

#include "instr.h"

#define OP_X    (IR >> 6)
#define OP_Y    ((IR >> 3)  & 7)
#define OP_Z    (IR         & 7)
#define OP_Q    (OP_Y & 1)
#define OP_P    (OP_Y >> 1)

#define LOG_ERR() do {} while(0)

/*-----------------------CB DECODING---------------------------*/

#define _DECODE_CB_X0() do {                                    \
    if (OP_Z == 6) {                                            \
        ROT_HL();                                               \
    } else {                                                    \
        ROT(RZ); /* HL is already checked so we use RZ */       \
    }                                                           \
} while(0)

#define _DECODE_CB() do {                                       \
    IR = READ_MEMORY(PC); PC++;                                 \
    switch(OP_X) {                                              \
        case 0: _DECODE_CB_X0();    break;                      \
        case 1: BIT_R();            break;                      \
        case 2: RES_R();            break;                      \
        case 3: SET_R();            break;                      \
    }                                                           \
} while(0)

/*-----------------------X0 DECODING---------------------------*/

#define _DECODE_X0_Z0() do {                                    \
    switch(OP_Y) {                                              \
        case 0: /* NOP(); */        break;                      \
        case 1: LD_DNN_SP();        break;                      \
        case 2: STOP();             break;                      \
        case 3: JP_DISP();          break;                      \
        default: JP_CC_DISP();      break;                      \
    }                                                           \
} while(0)

#define _DECODE_X0_Z1() do {                                    \
    switch(OP_Q) {                                              \
        case 0: LD_RP_NN();         break;                      \
        case 1: ADD_HL_RP();        break;                      \
    }                                                           \
} while(0)

#define _DECODE_X0_Z2_Q0() do {                                 \
    switch(OP_P) {                                              \
        case 0: LD_IBC_A();         break;                      \
        case 1: LD_IDE_A();         break;                      \
        case 2: LD_IHL_A_INC();     break;                      \
        case 3: LD_IHL_A_DEC();     break;                      \
    }                                                           \
} while(0)

#define _DECODE_X0_Z2_Q1() do {                                 \
    switch(OP_P) {                                              \
        case 0: LD_A_IBC();         break;                      \
        case 1: LD_A_IDE();         break;                      \
        case 2: LD_A_IHL_INC();     break;                      \
        case 3: LD_A_IHL_DEC();     break;                      \
    }                                                           \
} while(0)

#define _DECODE_X0_Z2() do {                                    \
    switch(OP_Q) {                                              \
        case 0: _DECODE_X0_Z2_Q0(); break;                      \
        case 1: _DECODE_X0_Z2_Q1(); break;                      \
    }                                                           \
} while(0)

#define _DECODE_X0_Z3() do {                                    \
    switch(OP_Q) {                                              \
        case 0: INC_RP();           break;                      \
        case 1: DEC_RP();           break;                      \
    }                                                           \
} while(0)

#define _DECODE_X0_Z7() do {                                    \
    switch(OP_Y) {                                              \
        case 0: RLCA();             break;                      \
        case 1: RRCA();             break;                      \
        case 2: RLA();              break;                      \
        case 3: RRA();              break;                      \
        case 4: DAA();              break;                      \
        case 5: CPL();              break;                      \
        case 6: SCF();              break;                      \
        case 7: CCF();              break;                      \
    }                                                           \
} while(0)

#define _DECODE_X0() do {                                       \
    switch(OP_Z) {                                              \
        case 0: _DECODE_X0_Z0();    break;                      \
        case 1: _DECODE_X0_Z1();    break;                      \
        case 2: _DECODE_X0_Z2();    break;                      \
        case 3: _DECODE_X0_Z3();    break;                      \
        case 4:     INC_R();        break;                      \
        case 5:     DEC_R();        break;                      \
        case 6:     LD_R_N();       break;                      \
        case 7: _DECODE_X0_Z7();    break;                      \
    }                                                           \
} while(0)

/*-----------------------X1 DECODING---------------------------*/

#define _DECODE_X1() do {                                       \
    if (OP_Y == 6 && OP_Z == 6) {                               \
        HALT();                                                 \
    } else {                                                    \
        LD_R_R();                                               \
    }                                                           \
} while(0)

/*-----------------------X3 DECODING---------------------------*/

#define _DECODE_X3_Z0() do {                                    \
    switch(OP_Y) {                                              \
        case 4: LDH_DN_A();         break;                      \
        case 5: ADD_SP_DISP();      break;                      \
        case 6: LDH_A_DN();         break;                      \
        case 7: LD_HL_SP_DISP();    break;                      \
        default: RET_CC();          break;                      \
    }                                                           \
} while(0)

#define _DECODE_X3_Z1_Q1() do {                                 \
    switch(OP_P) {                                              \
        case 0: RET();              break;                      \
        case 1: RETI();             break;                      \
        case 2: JP_HL();            break;                      \
        case 3: LD_SP_HL();         break;                      \
    }                                                           \
} while(0)

#define _DECODE_X3_Z1() do {                                    \
    switch(OP_Q) {                                              \
        case 0: POP_RP();           break;                      \
        case 1: _DECODE_X3_Z1_Q1(); break;                      \
    }                                                           \
} while(0)

#define _DECODE_X3_Z2() do {                                    \
    switch(OP_Y) {                                              \
        case 4: LDH_IC_A();         break;                      \
        case 5: LD_DNN_A();         break;                      \
        case 6: LDH_A_IC();         break;                      \
        case 7: LD_A_DNN();         break;                      \
        default: JP_CC_NN();        break;                      \
    }                                                           \
} while(0)

#define _DECODE_X3_Z3() do {                                    \
    switch(OP_Y) {                                              \
        case 0: JP_NN();            break;                      \
        case 1: _DECODE_CB();       break;                      \
        case 6: DI();               break;                      \
        case 7: EI();               break;                      \
        default: LOG_ERR();         break;                      \
    }                                                           \
} while(0)

#define _DECODE_X3_Z4() do {                                    \
    if (OP_Y >= 4) LOG_ERR();                                   \
    CALL_CC_NN();                                               \
} while(0)

#define _DECODE_X3_Z5() do {                                    \
    switch(OP_Q) {                                              \
        case 0: PUSH_RP();      break;                          \
        case 1:                                                 \
            if (OP_P != 0) LOG_ERR();                           \
            CALL_NN();                                          \
            break;                                              \
    }                                                           \
} while(0)

#define _DECODE_X3() do {                                       \
    switch(OP_Z) {                                              \
        case 0: _DECODE_X3_Z0();    break;                      \
        case 1: _DECODE_X3_Z1();    break;                      \
        case 2: _DECODE_X3_Z2();    break;                      \
        case 3: _DECODE_X3_Z3();    break;                      \
        case 4: _DECODE_X3_Z4();    break;                      \
        case 5: _DECODE_X3_Z5();    break;                      \
        case 6:     ALU_N();        break;                      \
        case 7:     RST_N();        break;                      \
    }                                                           \
} while(0)

/*----------------------FULL DECODING--------------------------*/

#define DECODE() do {                                           \
    switch(OP_X) {                                              \
        case 0: _DECODE_X0();       break;                      \
        case 1: _DECODE_X1();       break;                      \
        case 2:   ALU_R();          break;                      \
        case 3: _DECODE_X3();       break;                      \
    }                                                           \
} while(0)

#endif
