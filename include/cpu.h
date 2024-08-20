#ifndef CPU_H_
#define CPU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>

#include "cputype.h"
#include "cpudef.h"
#include "cpu_utils.h"
#include "log.h"
#include "load.h"

// How to dump: `hexdump -e '16/1 "0x%02x, " "\n"' dmg_boot.bin`
static const BYTE BOOTROM[] = {
    0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb, 0x21, 0x26, 0xff, 0x0e,
    0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3, 0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96, 0x00, 0x13, 0x7b,
    0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22, 0x23, 0x05, 0x20, 0xf9,
    0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99, 0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20,
    0xf9, 0x2e, 0x0f, 0x18, 0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04,
    0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20, 0xf7, 0x1d, 0x20, 0xf2,
    0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62, 0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06,
    0x7b, 0xe2, 0x0c, 0x3e, 0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
    0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb, 0x11, 0x17, 0xc1, 0xcb, 0x11, 0x17,
    0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9, 0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
    0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc,
    0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e, 0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c,
    0x21, 0x04, 0x01, 0x11, 0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20,
    0xf5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe, 0x3e, 0x01, 0xe0, 0x50
};

typedef union
{
    WORD w;

    struct {
        BYTE l, h;
    } b;
} CPU_Reg;

typedef struct {
    // Registers
    CPU_Reg             ir;                        // Instruction register
    //CPU_Reg             ie;                        // Interrupt enable

    CPU_Reg             af;                        // Register AF (Accumulator + Flags)
    CPU_Reg             bc;                        // Register BC (B + C)
    CPU_Reg             de;                        // Register DE (D + E)
    CPU_Reg             hl;                        // Register HL (H + L)

    CPU_Reg             sp;                        // Stack pointer
    CPU_Reg             pc;                        // Program counter
    CPU_Reg             wz;                        // Memory pointer

    BYTE               *memory;
    BYTE               *rom;
    size_t              rom_size;

    WORD                m_div;                      // DIV timer

    int                 m_IME; // Interrup master enable [write only]

    unsigned            m_cycle_counter;

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
    CPU_Debug_Info      debug_info;
#endif
} CPU;

#define ROM_SIZE            (context->rom_size+0x100)
#define ORIGINAL_ROM_SIZE   (context->rom_size)

#define _CPU_INIT(context, src_rom_path) do {                                                                                       \
    IR                          = 0;                                                                                                \
    AF                          = 0;                                                                                                \
    BC                          = 0;                                                                                                \
    DE                          = 0;                                                                                                \
    HL                          = 0;                                                                                                \
    SP                          = 0;                                                                                                \
    PC                          = 0;                                                                                                \
    WZ                          = 0;                                                                                                \
    context->m_IME              = 0;                                                                                                \
    context->m_cycle_counter    = 0;                                                                                                \
    int rv = loadrom(src_rom_path, &context->rom, &context->rom_size);                                                              \
    if (rv != context->rom_size) {                                                                                                  \
        char *errmsg = "FAILED READING PROVIDED ROM";                                                                               \
        switch(rv) {                                                                                                                \
            case LOADROM_FAIL_OPEN:     errmsg = "FAILED OPENING ROM";                                                              \
            case LOADROM_FAIL_ALLOC:    errmsg = "FAILED ALLOCATING ROM MEMORY";                                                    \
        }                                                                                                                           \
        fprintf(stderr, "%s: %s\n", errmsg, src_rom_path);                                                                          \
        exit(EXIT_FAILURE);                                                                                                         \
    }                                                                                                                               \
    context->memory             = (BYTE*)(calloc(0xFFFF+1, sizeof(BYTE)));                                                          \
} while(0)

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
#define CPU_INIT(context, src_rom_path) do {                                                                                                             \
        _CPU_INIT(context, src_rom_path);                                                                                           \
        CPU_GAMEBOY_DOCTOR_SETUP(src_rom_path);                                                                                     \
} while(0)
#else
#define CPU_INIT _CPU_INIT
#endif

#define _CPU_FREE(context) do {                                                                                                     \
    if (context->memory)    free(context->memory);                                                                                  \
    if (context->rom)       free(context->rom);                                                                                     \
} while(0)

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
#define CPU_FREE(context) do {                                                                                                      \
        _CPU_FREE(context);                                                                                                         \
        CPU_GAMEBOY_DOCTOR_CLEANUP();                                                                                               \
} while(0)
#else
#define CPU_FREE _CPU_FREE
#endif

#define FETCH_CYCLE() do {                                                                                                          \
    LOG_CPU_STATE();                                                                                                                \
    IR = READ_MEMORY(PC); PC++;                                                                                                     \
    /* TODO: check interrupts */                                                                                                    \
} while(0)

#ifdef __cplusplus
}
#endif

#endif