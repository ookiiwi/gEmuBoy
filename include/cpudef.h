#ifndef CPUDEF_H_
#define CPUDEF_H_

#define A       (context->af.b.h)
#define F       (context->af.b.l)
#define B       (context->bc.b.h)
#define C       (context->bc.b.l)
#define D       (context->de.b.h)
#define E       (context->de.b.l)
#define H       (context->hl.b.h)
#define L       (context->hl.b.l)
#define SPh     (context->sp.b.h)
#define SPl     (context->sp.b.l)
#define PCh     (context->pc.b.h)
#define PCl     (context->pc.b.l)
#define W       (context->wz.b.h)
#define Z       (context->wz.b.l)

#define IR      (context->ir.w)
#define AF      (context->af.w)
#define BC      (context->bc.w)
#define DE      (context->de.w)
#define HL      (context->hl.w)
#define SP      (context->sp.w)
#define PC      (context->pc.w)
#define WZ      (context->wz.w)

#define _IME    (context->m_IME)

#define DIV     (context->memory[0xFF04])
#define TIMA    (context->memory[0xFF05])
#define TMA     (context->memory[0xFF06])
#define TAC     (context->memory[0xFF07])

#endif