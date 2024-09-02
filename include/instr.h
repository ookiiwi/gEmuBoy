#ifndef INSTR_H_
#define INSTR_H_

#include "cpu.h"
#include "cpudef.h"
#include "mmu.h"
#include "log.h"
#include "interrupt.h"
#include "timer.h"
#include "defs.h"

#include <stddef.h>

#define ABS(n) (n < 0 ? -n : n)

#define ZF_TOGGLE (0x80)
#define NF_TOGGLE (0x40)
#define HF_TOGGLE (0x20)
#define CF_TOGGLE (0x10)

/// Extracts flag from F register
#define F_Z (F & ZF_TOGGLE)
#define F_N (F & NF_TOGGLE)
#define F_H (F & HF_TOGGLE)
#define F_C (F & CF_TOGGLE)
#define F_NHC (F_N | F_H | F_C)

/// Extract flag value as boolean
#define ZF  (F >> 7)
#define NF ((F >> 6) & 1)
#define HF ((F >> 5) & 1)
#define CF ((F >> 4) & 1)

#define ZF_CHECK(byte)          ( ( ( byte & 0xFF ) == 0 ) << 7 )
#define HF_ADC_CHECK(a, b, c)   ( ( ( (a & 0xF) + (b & 0xF) + (c & 0xF) ) & 0x10 ) << 1 )          /* Half-carry bit = bit 3   */
#define HF_SBC_CHECK(a, b, c)   ( ( ( (a & 0xF) - (b & 0xF) - (c & 0xF) ) & 0x10 ) << 1 )          /* Half-carry bit = bit 3   */
#define HF_ADD_CHECK(a, b)      HF_ADC_CHECK(a, b, 0)
#define HF_SUB_CHECK(a, b)      HF_SBC_CHECK(a, b, 0)
#define CF_CHECK(a, b)          ( ( a > 0xFF - b ) << 4 )  /* Carry bit = bit 7        */
#define CF_BORROW(a, b)         ( ( a < ABS(b) )   << 4 )

#define HF_CHECK16(res, a, b)   ( ( ( ( res ^ a ^ b ) >> 8 ) & 0x10 ) << 1 )
#define CF_CHECK16(a, b)        ( ( a > 0xFFFF - b ) << 4 )

#define MSB(word)               ( ( word >> 8 ) & 0xFF  )
#define LSB(word)               (   word        & 0xFF  )

#define MSb(byte)               ( ( byte & 0xFF ) >> 7  )
#define LSb(byte)               (   byte &  1           )

#define HNIBBLE(byte)           ( ( byte >> 4 ) & 0x0F  )
#define LNIBBLE(byte)           (   byte        & 0x0F  )

static inline BYTE read_memory(GB_gameboy_t *gb, WORD addr) { 
    BYTE res = GB_mem_read(gb, addr); 
    INC_CYCLE();
    return res; 
}

#define READ_MEMORY(addr) read_memory(gb, addr)
#define WRITE_MEMORY(addr, data) do {               \
    GB_mem_write(gb, addr, data);                   \
    INC_CYCLE();                                    \
} while(0)

static long REGISTER_OFFSET_TABLE[8] = {
    offsetof(GB_cpu_t, bc.b.h),  /* B */
    offsetof(GB_cpu_t, bc.b.l),  /* C */
    offsetof(GB_cpu_t, de.b.h),  /* D */
    offsetof(GB_cpu_t, de.b.l),  /* E */
    offsetof(GB_cpu_t, hl.b.h),  /* H */
    offsetof(GB_cpu_t, hl.b.l),  /* L */
    0,                          /* (HL) */
    offsetof(GB_cpu_t, af.b.h)   /* A */
};

static long REGISTER_PAIR_OFFSET_TABLE[4] = {
    offsetof(GB_cpu_t, bc.w),    /* BC */
    offsetof(GB_cpu_t, de.w),    /* DE */
    offsetof(GB_cpu_t, hl.w),    /* HL */
    offsetof(GB_cpu_t, sp.w)     /* SP */
};

static long REGISTER_PAIR2_OFFSET_TABLE[4] = {
    offsetof(GB_cpu_t, bc.w),    /* BC */
    offsetof(GB_cpu_t, de.w),    /* DE */
    offsetof(GB_cpu_t, hl.w),    /* HL */
    offsetof(GB_cpu_t, af.w)     /* AF */
};

/* Select register in opcode byte */
/* |7|6|5|4|3|2|1|0|    */
/*      ~~~~~ ~~~~~     */
/*        |     |____ Z */
/*        |__________ Y */
#define REGISTER(index)      ( *( (BYTE*)(gb->cpu) + REGISTER_OFFSET_TABLE[ index & 7 ] ) ) // Note to self: 0b111 = 7
#define REGISTER_PAIR(table) ( *(WORD *)(void *)( (char*)(gb->cpu) + table[ OP_P ] ) )

#define RY  REGISTER(OP_Y)
#define RZ  REGISTER(OP_Z)

#define RP  ( REGISTER_PAIR(REGISTER_PAIR_OFFSET_TABLE) )
#define RP2 ( REGISTER_PAIR(REGISTER_PAIR2_OFFSET_TABLE) )

// Note: Set and Get register macros allow operating on registers without worriying about r/w (HL)

#define SET_REGISTER(op, value) do {        			\
    if (op == 6) {                          			\
        WRITE_MEMORY(HL, value);            			\
    } else {                                			\
        REGISTER(op) = value;               			\
    }                                       			\
} while (0)

#define SET_RY(value)       SET_REGISTER(OP_Y, value)
#define SET_RZ(value)       SET_REGISTER(OP_Z, value)

#define _READ_MEMORY_DUMMY_PARAM(addr, dummy) READ_MEMORY(addr) // Wrapper over READ_MEMORY to allow calling INC_CYCLE and returning read value
#define GET_REGISTER(op)    (op == 6 ? _READ_MEMORY_DUMMY_PARAM(HL, INC_CYCLE()) : REGISTER(op))
#define GET_RY()            GET_REGISTER(OP_Y)
#define GET_RZ()            GET_REGISTER(OP_Z)

static const BYTE CC_TABLE[4] = { ZF_TOGGLE, ZF_TOGGLE, CF_TOGGLE, CF_TOGGLE };
#define CC(index) ( !( F & CC_TABLE[index] ) ^ (index & 1) ) /* Condition Check */


/*------------------------------------------------------8-bit load instructions-------------------------------------------------------*/

/**
 * LD r, r': Load register (register)
 * 
 * Flags: -
 * M-Cycles: 1
*/
#define LD_R_R() do {                       			\
    BYTE rz = GET_RZ();                     			\
    SET_RY(rz);                             			\
} while(0)


/**
 * LD r, n: Load register (immediate)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_R_N() do {                       			\
    Z = READ_MEMORY(PC++);                  			\
    SET_RY(Z);                              			\
} while(0)

#define _LD_A_INDIRECT(addr) do {           			\
    A = Z = READ_MEMORY(addr);              			\
} while(0)

#define _LD_INDIRECT_A(addr) do {           			\
    WRITE_MEMORY(addr, A);                  			\
} while(0)

/**
 * LD A, (BC): Load accumulator (indirect BC)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_A_IBC() do {                     			\
    _LD_A_INDIRECT(BC);                     			\
} while(0)

/**
 * LD A, (DE): Load accumulator (indirect DE)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_A_IDE() do {                     			\
    _LD_A_INDIRECT(DE);                     			\
} while(0)

/**
 * LD (BC), A: Load accumulator (indirect BC)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_IBC_A() do {                     			\
    _LD_INDIRECT_A(BC);                     			\
} while(0)

/**
 * LD (DE), A: Load accumulator (indirect DE)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_IDE_A() do {                     			\
    _LD_INDIRECT_A(DE);                     			\
} while(0)

/**
 * LD A, (nn): Load accumulator (direct)
 * 
 * Flags: -
 * M-Cycles: 4
*/
#define LD_A_DNN() do {                     			\
    Z = READ_MEMORY(PC++);                  			\
    W = READ_MEMORY(PC++);                  			\
    _LD_A_INDIRECT(WZ);                     			\
} while(0)

/**
 * LD (nn), A: Load accumulator (direct)
 * 
 * Flags: -
 * M-Cycles: 4
*/
#define LD_DNN_A() do {                     			\
    Z = READ_MEMORY(PC++);                  			\
    W = READ_MEMORY(PC++);                  			\
    _LD_INDIRECT_A(WZ);                     			\
} while(0)

/**
 * LDH A, (_): Load accumulator (indirect 0xFF00+lsb)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define _LDH_A_INDIRECT(addr_lsb) do {      			\
    int addr = 0xFF00 + addr_lsb;           			\
    Z = READ_MEMORY(addr);                  			\
    A = Z;                                  			\
} while(0)

/**
 * LDH (_), A: Load from accumulator (indirect 0xFF00+lsb)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define _LDH_INDIRECT_A(addr_lsb) do {      			\
    int addr = 0xFF00 + addr_lsb;           			\
    WRITE_MEMORY(addr, A);                  			\
} while(0)

/**
 * LDH A, (C): Load accumulator (indirect 0xFF00+C)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LDH_A_IC() do {                     			\
    _LDH_A_INDIRECT(C);                     			\
} while(0)

/**
 * LDH (C), A: Load from accumulator (indirect 0xFF00+C)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LDH_IC_A() do {                     			\
    _LDH_INDIRECT_A(C);                     			\
} while(0)

/**
 * LDH A, (n): Load accumulator (direct 0xFF00+n)
 * 
 * Flags: -
 * M-Cycles: 3
*/
#define LDH_A_DN() do {                     			\
    Z = READ_MEMORY(PC++);                  			\
    _LDH_A_INDIRECT(Z);                     			\
} while(0)

/**
 * LDH (n), A: Load from accumulator (direct 0xFF00+n)
 * 
 * Flags: -
 * M-Cycles: 3
*/
#define LDH_DN_A() do {                     			\
    Z = READ_MEMORY(PC++);                  			\
    _LDH_INDIRECT_A(Z);                     			\
} while(0)

/**
 * LD A, (HL-): Load accumulator (indirect HL, decrement)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_A_IHL_DEC() do {                 			\
    Z = READ_MEMORY(HL); HL--;              			\
    A = Z;                                  			\
} while(0)

/**
 * LD (HL-), A: Load from accumulator (indirect HL, decrement)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_IHL_A_DEC() do {                 			\
    WRITE_MEMORY(HL, A); HL--;              			\
} while(0)

/**
 * LD A, (HL+): Load accumulator (indirect HL, increment)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_A_IHL_INC() do {                 			\
    Z = READ_MEMORY(HL); HL++;              			\
    A = Z;                                  			\
} while(0);

/**
 * LD (HL+), A: Load from accumulator (indirect HL, increment)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_IHL_A_INC() do {                 			\
    WRITE_MEMORY(HL, A); HL++;              			\
} while(0);

/*-----------------------------------------------------16-bit load instructions-------------------------------------------------------*/

/**
 * LD rr, nn: Load 16-bit register / register pair
 * 
 * Flags: -
 * M-Cycles: 3
*/
#define LD_RP_NN() do {                     			\
    Z   = READ_MEMORY(PC++);                			\
    W   = READ_MEMORY(PC++);                			\
    RP  = WZ;                               			\
} while(0)

/**
 * LD (nn), SP: Load from stack pointer (direct)
 * 
 * Flags: -
 * M-Cycles: 5
*/
#define LD_DNN_SP() do {                    			\
    Z   = READ_MEMORY(PC++);                			\
    W   = READ_MEMORY(PC++);                			\
    WRITE_MEMORY(WZ, SPl); WZ++; /* lsb */  			\
    WRITE_MEMORY(WZ, SPh);       /* msb */  			\
} while(0)

/**
 * LD SP, HL: Load stack pointer from HL
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define LD_SP_HL() do {                     			\
    SP = HL;                                			\
    INC_CYCLE();                            			\
} while(0)

/**
 * PUSH rr: Push to stack
 * 
 * Flags: -
 * M-Cycles: 4
*/
#define PUSH_RP() do {                      			\
    WORD rr = RP2;                          			\
    SP--; WRITE_MEMORY(SP, MSB(rr)); /* M2/M3 */		\
    SP--; WRITE_MEMORY(SP, LSB(rr)); /* M4 */   		\
} while(0)

/**
 * POP rr: POP from stack
 * 
 * Flags: Flags are affected except that POP AF overrides all flags
 * M-Cycles: 3
*/
#define POP_RP() do {                       			\
    Z = READ_MEMORY(SP++);      /* M2 */    			\
    W = READ_MEMORY(SP++);      /* M3 */    			\
    RP2 = WZ;                   /* M4 */    			\
    F &= 0xF0; /* Ensure low nibble is 0 */ 			\
} while(0)

/**
 * LD HL, SP+e: Load HL from adjusted stack pointer
 * 
 * Flags: Z = 0, N = 0, H = half-carry b.3, C = carry b.7
 * M-Cycles: 3
*/
#define LD_HL_SP_DISP() do {                			\
    Z = READ_MEMORY(PC++);                  			\
    int res = SP + (SIGNED_BYTE)Z;          			\
    HL = res;                               			\
    F = HF_ADD_CHECK(SPl, (SIGNED_BYTE)Z) 		|		\
        CF_CHECK(SPl, Z);                    			\
    INC_CYCLE();                            			\
} while(0)

/*--------------------------------------------8-bit arithmetic and logical instructions-----------------------------------------------*/

/**
 * Below set of instructions designed optimized to avoid repeting unnecessary code, especially when operating on (HL) or a byte
 * Therefore the burden of the different handling of (HL) is taken care of by GET_REGISTER macro 
*/

/**
 * ADD r: Add (register)
 * 
 * Flags: Z = zero, N = 0, H = half-carry, C = carry
 * M-Cycles: 1
*/
#define ADD_R(rhs) do {                     			\
    int res = A + rhs;                      			\
    F = ZF_CHECK(res)               			|		\
        0                           			|		\
        HF_ADD_CHECK(A, rhs)        			|		\
        CF_CHECK(A, rhs);                   			\
    A = res;                                			\
} while(0)

//#define ADD_R(rhs) _ADD(rhs, 0, CF_CHECK)

/**
 * ADC r: Add with carry(register)
 * 
 * Flags: Z = zero, N = 0, H = half-carry, C = carry
 * M-Cycles: 1
*/
#define ADC_R(rhs) do {                     			\
    int res = A + rhs + CF;                 			\
    F = ZF_CHECK(res)               			|		\
        0                           			|		\
        HF_ADC_CHECK(A, rhs, CF)    			|		\
        CF_CHECK(A, (rhs+CF));              			\
    A = res;                                			\
} while(0)

/**
 * SUB r: Subtract (register)
 * 
 * Flags: Z = zero, N = 1, H = half-carry, C = carry
 * M-Cycles: 1
*/
#define SUB_R(rhs) do {                     			\
    int res = A - rhs;                      			\
    F = ZF_CHECK(res)               			|		\
        NF_TOGGLE                   			|		\
        HF_SUB_CHECK(A, rhs)        			|		\
        CF_BORROW(A, rhs);                  			\
    A = res;                                			\
} while(0)

/**
 * SBC r: Subtract with carry(register)
 * 
 * Flags: Z = zero, N = 1, H = half-carry, C = carry
 * M-Cycles: 1
*/
#define SBC_R(rhs) do {                     			\
    /* NOT SURE ABOUT THIS ONE */           			\
    int res = A - rhs - CF;                 			\
    F = ZF_CHECK(res)               			|		\
        NF_TOGGLE                   			|		\
        HF_SBC_CHECK(A, rhs, CF)    			|		\
        CF_BORROW(A, (rhs+CF));             			\
    A = res;                                			\
} while(0)

/**
 * CP r: Compare (register)
 * 
 * Flags: Z = zero, N = 1, H = half-carry, C = carry
 * M-Cycles: 1
*/
#define CP_R(rhs) do {                      			\
    BYTE a = A;                             			\
    SUB_R(rhs);                             			\
    A = a;                                  			\
} while(0)

#define _INC_R(inc_val, nf, hf_check) do {  			\
    BYTE ry = GET_RY();                     			\
    int res = ry + inc_val;                 			\
    SET_RY(res);                            			\
    F = ZF_CHECK(res)               			|		\
        nf                          			|		\
        hf_check(ry, ABS(inc_val))  			|		\
        F_C; /* Carry flag not affected */  			\
} while(0)

/**
 * INC r: Increment (register)
 * 
 * Flags: Z = zero, N = 0, H = half-carry, C = not affected
 * M-Cycles: 1
*/
#define INC_R() _INC_R(1, 0, HF_ADD_CHECK)

/**
 * DEC r: Deccrement (register)
 * 
 * Flags: Z = zero, N = 1, H = half-carry, C = not affected
 * M-Cycles: 1
*/
#define DEC_R() _INC_R(-1, NF_TOGGLE, HF_SUB_CHECK)

/**
 * Customizable macro for bitwise operation on accumulator
 * 
 * Flags: Z = zero, N = 0, H = depends, C = 0
 * M-Cycles: 1
*/
#define _BITWISE_R(rhs, operator, hf) do {  			\
    A operator##= rhs;                      			\
    F = ZF_CHECK(A)                 			|		\
        0                           			|		\
        hf                          			|		\
        0;                                  			\
} while(0)

/**
 * AND r: Bitwise AND (register)
 * 
 * Flags: Z = zero, N = 0, H = 1, C = 0
 * M-Cycles: 1
*/
#define AND_R(rhs) _BITWISE_R(rhs, &, HF_TOGGLE);

/**
 * OR r: Bitwise OR (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = 0
 * M-Cycles: 1
*/
#define OR_R(rhs) _BITWISE_R(rhs, |, 0);

/**
 * XOR r: Bitwise XOR (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = 0
 * M-Cycles: 1
*/
#define XOR_R(rhs) _BITWISE_R(rhs, ^, 0);

#define ALU(rhs) do {                       			\
    BYTE b = rhs;                           			\
    switch(OP_Y) {                          			\
        case 0: ADD_R(b);   break;          			\
        case 1: ADC_R(b);   break;          			\
        case 2: SUB_R(b);   break;          			\
        case 3: SBC_R(b);   break;          			\
        case 4: AND_R(b);   break;          			\
        case 5: XOR_R(b);   break;          			\
        case 6: OR_R(b);    break;          			\
        case 7: CP_R(b);    break;          			\
    }                                       			\
} while(0)

#define ALU_R() do {                        			\
    BYTE rz = GET_RZ();                     			\
    ALU(rz);                                			\
} while(0)

/**
 * ALU[y] n
 * 
 * Flags: ...
 * M-Cycles: n+1
*/
#define ALU_N() do {                        			\
    Z = READ_MEMORY(PC++);                  			\
    ALU(Z);                                 			\
} while(0)

/**
 * CCF: Complement carry flag
 * 
 * Flags: Z = not affected, N = 0, H = 0, C = !carry
 * M-Cycles: 1
*/
#define CCF() do {                          			\
    F = F_Z                         			|		\
        0                           			|		\
        0                           			|		\
        (!CF << 4);                         			\
} while(0)

/**
 * SCF: Set carry flag
 * 
 * Flags: Z = not affected, N = 0, H = 0, C = 1
 * M-Cycles: 1
*/
#define SCF() do {                          			\
    F = F_Z                         			|		\
        0                           			|		\
        0                           			|		\
        CF_TOGGLE;            			                \
} while(0)

/**
 * DAA: Decimal adjust accumulator
 * 
 * Flags: Z = zero, N = not affected, H = 0, C = carry
 * M-Cycles: 1
 * 
 * See  - https://blog.ollien.com/posts/gb-daa/
 *      - https://stackoverflow.com/a/45246967/12313137
 *      - https://github.com/superzazu/z80/blob/d64fe10a2274e5e40019b1086bf7d8990cbc5f23/z80.c#L604
 * 
 * Explanation: 
 * A register is BCD corrected so that if non-BCD value ( > 9)
 * is contained in the low nibble or half-carry flag is set
 * 0x06 is added/subtracted to A. Moreover, if the high nibble 
 * is higher than 9 or carry flag is on, 0x60 is added/subtracted.
 * 
 * However, subtraction adjustement is only performed if HF or CF are ON
*/
#define DAA() do {                          			\
    BYTE res =                              			\
        (HF || (!NF && LNIBBLE(A) > 0x09))  			\
                    ? 6 : 0;                			\
    BYTE bcd_carry =                        			\
        (!NF && A > 0x99) || CF;            			\
    if (bcd_carry) res |= 0x60;             			\
    res = NF ? (A - res) : (A + res);       			\
    F = ZF_CHECK(res)               			|		\
        F_N                         			|		\
        0                           			|		\
        (bcd_carry << 4);                   			\
    A = res;                                			\
} while(0)

/**
 * CPL: Complement accumulator
 * 
 * Flags: Z = not affected, N = 1, H = 1, C = not affected
 * M-Cycles: 1
*/
#define CPL() do {                          			\
    A = ~A;                                 			\
    F = F_Z                         			|		\
        NF_TOGGLE                   			|		\
        HF_TOGGLE                   			|		\
        F_C;                                			\
} while(0)

/*--------------------------------------------------16-bit arithmetic instructions---------------------------------------------------*/

/**
 * INC rr: Increment 16-bit register
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define INC_RP() do {                       			\
    RP++;                                   			\
    INC_CYCLE();                            			\
} while(0)

/**
 * DEC rr: Decrement 16-bit register
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define DEC_RP() do {                       			\
    RP--;                                   			\
    INC_CYCLE();                            			\
} while(0)

/**
 * ADD HL, rr: Add (16-bit register)
 * 
 * Flags: Z = not affected, N = 0, H = half-carry, C = carry
 * M-Cycles: 2
 * 
 * Note:
 * 16-bit addition is proceed at once 
 * instead of breaking it into 8-bit addition
*/
#define ADD_HL_RP() do {                    			\
    WORD rp = RP;                           			\
    int res = HL + rp;                      			\
    F = F_Z                         			|		\
        0                           			|		\
        HF_CHECK16(res, HL, rp)     			|		\
        CF_CHECK16(HL, rp);                 			\
    HL = res;                               			\
    INC_CYCLE();                            			\
} while(0)

/**
 * ADD SP, e: Add to stack pointer (relative)
 * 
 * Flags: Z = 0, N = 0, H = half-carry, C = carry
 * M-Cycles: 4
 * 
 * Note:
 * As W and Z should be used as intermediate variables and finally be assigned to SP.
 * Even though we tried to simplify the process, WZ should have the same expected result
*/
#define ADD_SP_DISP() do {                  			\
    Z = READ_MEMORY(PC++);                  			\
    int res = SP + (SIGNED_BYTE)Z;          			\
    F = 0                           			|		\
        0                           			|		\
        HF_ADD_CHECK(SPl, (SIGNED_BYTE)Z) 		|		\
        CF_CHECK(SPl, Z);                   			\
    SP = WZ = res;                          			\
    INC_CYCLE(2);                           			\
} while(0)

/*-------------------------------------------Rotate, shift, and bit operation instructions--------------------------------------------*/

/**
 * RLC: Rotate left circular (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = carry
 * M-Cycles: 2 (M1 = CB)
 * 
 * Explanation:
 * Copy bit 7 to carry flag and bit 0.
 * Then rotate left
 *       __________________
 *      /                  \
 * |CF|<-|7|6|5|4|3|2|1|0|<-
*/
#define RLC(reg) do {                       			\
    reg = (reg << 1) | (reg >> 7);          			\
    F = ZF_CHECK(reg)               			|		\
        0                           			|		\
        0                           			|		\
        (LSb(reg) << 4);                    			\
} while(0);

/**
 * RLCA: Rotate left circular (accumulator)
 * 
 * Flags: Z = 0, N = 0, H = 0, C = carry
 * M-Cycles: 1
*/
#define RLCA() do {                         			\
    RLC(A);                                 			\
    F = F_C; /* Only keep carry flag */     			\
} while(0);                                 			\

/**
 * RRC: Rotate right circular (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = carry
 * M-Cycles: 2
 * 
 * Explanation:
 * Copy bit 0 to carry flag and bit 7.
 * Then rotate right
 *  __________________
 * /                  \
 * ->|7|6|5|4|3|2|1|0|->|CF|
*/
#define RRC(reg) do {                       			\
    reg = (reg >> 1) | (LSb(reg) << 7);     			\
    F = ZF_CHECK(reg)               			|		\
        0                           			|		\
        0                           			|		\
        ( MSb(reg) << 4 );                  			\
} while(0)

/**
 * RRCA: Rotate right circular (accumulator)
 * 
 * Flags: Z = 0, N = 0, H = 0, C = carry
 * M-Cycles: 1
*/
#define RRCA() do {                         			\
    RRC(A);                                 			\
    F = F_C; /* Only keep carry flag */     			\
} while(0);     

/**
 * RL: Rotate left (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = carry
 * M-Cycles: 2
 * 
 *  _________________________
 * /                         \
 * --|CF|<-|7|6|5|4|3|2|1|0|<-
*/
#define RL(reg) do {                        			\
    int res = (reg << 1) | CF;              			\
    F = ZF_CHECK(res)               			|		\
        0                           			|		\
        0                           			|		\
        ( MSb(reg) << 4 );                  			\
    reg = res;                              			\
} while(0)

/**
 * RLA: Rotate left (accumulator)
 * 
 * Flags: Z = 0, N = 0, H = 0, C = carry
 * M-Cycles: 1
*/
#define RLA() do {                          			\
    RL(A);                                  			\
    F = F_C;                                			\
} while(0)

/**
 * RR: Rotate right (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = carry
 * M-Cycles: 2
 * 
 *  _________________________
 * /                         \
 * ->|7|6|5|4|3|2|1|0|->|CF|--
*/
#define RR(reg) do {                        			\
    int cf = LSb(reg) << 4;                 			\
    reg = (reg >> 1) | (CF<<7);             			\
    F = ZF_CHECK(reg)               			|		\
        0                           			|		\
        0                           			|		\
        cf;                                 			\
} while(0)

/**
 * RRA: Rotate right (accumulator)
 * 
 * Flags: Z = 0, N = 0, H = 0, C = carry
 * M-Cycles: 1
*/
#define RRA() do {                          			\
    RR(A);                                  			\
    F = F_C;                                			\
} while(0)

/**
 * SLA r: Shift left arithmetic (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = carry
 * M-Cycles: 1
 * 
 * |CF| <- |7|6|5|4|3|2|1|0| <- 0
*/
#define SLA(reg) do {                       			\
    BYTE cf = MSb(reg) << 4;                			\
    reg <<= 1;                              			\
    F = ZF_CHECK(reg)               			|		\
        0                           			|		\
        0                           			|		\
        cf;                                 			\
} while(0)

/**
 * SRA r: Shift right arithmetic (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = carry
 * M-Cycles: 2
 *  __
 * |  |
 *  -|7|6|5|4|3|2|1|0| -> |CF|
*/
#define SRA(reg) do {                       			\
    int res = (reg & 0x80) | (reg>>1);      			\
    F = ZF_CHECK(res)               			|		\
        0                           			|		\
        0                           			|		\
        (LSb(reg) << 4);                    			\
    reg = res;                              			\
} while(0)

/**
 * SRL r: Shift right logical (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = carry
 * M-Cycles: 2
 * 
 * 0 -> |7|6|5|4|3|2|1|0| -> |CF|
*/
#define SRL(reg) do {                       			\
    int cf = LSb(reg) << 4;                 			\
    reg >>= 1;                              			\
    F = ZF_CHECK(reg)               			|		\
        0                           			|		\
        0                           			|		\
        cf;                                 			\
} while(0)

/**
 * SWAP r: Swap nibbles (register)
 * 
 * Flags: Z = zero, N = 0, H = 0, C = 0
 * M-Cycles: 2
*/
#define SWAP_R(reg) do {                    			\
    reg = (LNIBBLE(reg)<<4) | HNIBBLE(reg); 			\
    F = ZF_CHECK(reg);                      			\
} while(0)

#define ROT(reg) do {                       			\
    switch(OP_Y) {                          			\
        case 0: RLC(reg);   break;          			\
        case 1: RRC(reg);   break;          			\
        case 2: RL(reg);    break;          			\
        case 3: RR(reg);    break;          			\
        case 4: SLA(reg);   break;          			\
        case 5: SRA(reg);   break;          			\
        case 6: SWAP_R(reg);break;          			\
        case 7: SRL(reg);   break;          			\
    }                                       			\
} while(0)

#define ROT_HL() do {                       			\
    Z = READ_MEMORY(HL);                    			\
    ROT(Z);                                 			\
    WRITE_MEMORY(HL, Z);                    			\
} while(0)

/**
 * BIT b, r: Test bit (register)
 * 
 * Flags: Z = zero, N = 0, H = 1, C = not affected
 * M-Cycles: 2
*/
#define BIT_R() do {                        			\
    int test = GET_RZ() & (1 << OP_Y);      			\
    F = ZF_CHECK(test)              			|		\
        0                           			|		\
        HF_TOGGLE                   			|		\
        F_C;                                			\
} while(0)

/**
 * RES b, r: Reset bit (register)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define RES_R() do {                        			\
    int res = GET_RZ() & (~(1 << OP_Y));    			\
    SET_RZ(res);                            			\
} while(0)

/**
 * SET b, r: Set bit (register)
 * 
 * Flags: -
 * M-Cycles: 2
*/
#define SET_R() do {                        			\
    int res = GET_RZ() | (1 << OP_Y);       			\
    SET_RZ(res);                            			\
} while(0)

/*--------------------------------------------------Control flow instructions--------------------------------------------------------*/

/**
 * JP nn: Jump
 * 
 * Flags: -
 * M-Cycles: 4
*/
#define JP_NN() do {                        			\
    Z   = READ_MEMORY(PC++);        /* M2 */   			\
    W   = READ_MEMORY(PC++);        /* M3 */   			\
    PC  = WZ;                       /* M4 */       	    \
    INC_CYCLE();                            			\
} while(0)

/**
 * JP HL: Jump to HL
 * 
 * Flags: -
 * M-Cycles: 1
*/
#define JP_HL() do {                        			\
    PC  = HL;                               			\
} while(0)

/**
 * JP cc, nn: Jump (conditional)
 * 
 * Flags: -
 * M-Cycles: 4 (cc true)
 *           3 (cc false)
*/
#define JP_CC_NN() do {                     			\
    Z = READ_MEMORY(PC++);                  			\
    W = READ_MEMORY(PC++);                  			\
    if (CC(OP_Y)) {                         			\
        PC = WZ;                            			\
        INC_CYCLE();                        			\
    }                                       			\
} while(0)

/**
 * JR e: Relative jump
 * 
 * Flags: -
 * M-Cycles: 3
*/
#define JP_DISP() do {                      			\
    Z = READ_MEMORY(PC++);                  			\
    WZ = PC + (SIGNED_BYTE)Z;               			\
    PC = WZ;                                			\
    INC_CYCLE();                            			\
} while(0)

/**
 * JP cc, nn: Jump (conditional)
 * 
 * Flags: -
 * M-Cycles: 3 (cc true)
 *           2 (cc false)
*/
#define JP_CC_DISP() do {                   			\
    Z = READ_MEMORY(PC++);                  			\
    WZ = PC + (SIGNED_BYTE)Z;               			\
    if (CC(OP_Y-4)) {                       			\
        PC = WZ;                            			\
        INC_CYCLE();                        			\
    }                                       			\
} while(0)

/**
 * CALL nn: Call function
 * 
 * Flags: -
 * M-Cycles: 6
*/
#define CALL_NN() do {                      			\
    Z = READ_MEMORY(PC++);                  			\
    W = READ_MEMORY(PC++);                  			\
    SP--; WRITE_MEMORY(SP, PCh);            			\
    SP--; WRITE_MEMORY(SP, PCl);            			\
    PC = WZ;                                			\
    INC_CYCLE();                            			\
} while(0)

/**
 * CALL cc, nn: Call function (conditional)
 * 
 * Flags: -
 * M-Cycles: 6 (cc true)
 *           3 (cc false)
*/
#define CALL_CC_NN() do {                   			\
    Z = READ_MEMORY(PC++);                  			\
    W = READ_MEMORY(PC++);                  			\
    if (CC(OP_Y)) {                         			\
        SP--; WRITE_MEMORY(SP, PCh);        			\
        SP--; WRITE_MEMORY(SP, PCl);        			\
        PC = WZ;                            			\
        INC_CYCLE();                        			\
    }                                       			\
} while(0)

/**
 * RET: Return from function
 * 
 * Flags: -
 * M-Cycles: 4
*/
#define RET() do {                          			\
    Z = READ_MEMORY(SP++);                  			\
    W = READ_MEMORY(SP++);                  			\
    PC = WZ;                                			\
    INC_CYCLE();                            			\
} while(0)

/**
 * RET cc: Return from function (conditional)
 * 
 * Flags: -
 * M-Cycles: 5 (cc true)
 *           2 (cc false)
*/
#define RET_CC() do {                       			\
    if (CC(OP_Y)) {                         			\
        RET();                              			\
    }                                       			\
    INC_CYCLE();                            			\
} while(0)

/**
 * RETI: Return from interrupt handler
 * 
 * Flags: -
 * M-Cycles: 4
*/
#define RETI() do {                         			\
    RET();                                  			\
    _IME = 1;                               			\
} while(0)

/**
 * RST n: Restart / Call function (implied)
 * 
 * Flags: -
 * M-Cycles: 4
*/
#define RST_N() do {                        			\
    WRITE_MEMORY(--SP, PCh);            			    \
    WRITE_MEMORY(--SP, PCl);            			    \
    PC = (OP_Y*8) & 0xFFFF;                 			\
    INC_CYCLE();                            			\
} while(0)

/*-------------------------------------------------Miscellaneous instructions--------------------------------------------------------*/

/**
 * HALT: Halt system clock
 * 
 * Flags: -
 * M-Cycles: 1
*/
#define HALT() do {                         			\
    gb->cpu->m_is_halted = 1;                           \
} while(0)

/**
 * STOP: Stop system and main clocks
 * 
 * Flags: -
 * M-Cycles: 1
*/
#define STOP() do {                         			\
    /* TODO: reset DIV */                   			\
} while(0)

/**
 * DI: Disable interrupts
 * 
 * Flags: -
 * M-Cycles: 1
*/
#define DI() do {                           			\
    _IME = 0;                               			\
} while(0)

/**
 * EI: Enable interrupts
 * 
 * Flags: -
 * M-Cycles: 1
*/
#define EI() do {                           			\
    gb->cpu->m_ei_delay = 2;                            \
} while(0)

#endif

