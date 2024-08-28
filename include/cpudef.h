#ifndef CPUDEF_H_
#define CPUDEF_H_

#define A       (gb->cpu->af.b.h)
#define F       (gb->cpu->af.b.l)
#define B       (gb->cpu->bc.b.h)
#define C       (gb->cpu->bc.b.l)
#define D       (gb->cpu->de.b.h)
#define E       (gb->cpu->de.b.l)
#define H       (gb->cpu->hl.b.h)
#define L       (gb->cpu->hl.b.l)
#define SPh     (gb->cpu->sp.b.h)
#define SPl     (gb->cpu->sp.b.l)
#define PCh     (gb->cpu->pc.b.h)
#define PCl     (gb->cpu->pc.b.l)
#define W       (gb->cpu->wz.b.h)
#define Z       (gb->cpu->wz.b.l)

#define IR      (gb->cpu->ir.w)
#define AF      (gb->cpu->af.w)
#define BC      (gb->cpu->bc.w)
#define DE      (gb->cpu->de.w)
#define HL      (gb->cpu->hl.w)
#define SP      (gb->cpu->sp.w)
#define PC      (gb->cpu->pc.w)
#define WZ      (gb->cpu->wz.w)

#define _IME    (gb->cpu->m_IME)

#define DIV     (gb->memory[0xFF04])
#define TIMA    (gb->memory[0xFF05])
#define TMA     (gb->memory[0xFF06])
#define TAC     (gb->memory[0xFF07])

#endif