#ifndef CPUDEF_H_
#define CPUDEF_H_

#define rA       (gb->cpu->af.b.h)
#define rF       (gb->cpu->af.b.l)
#define rB       (gb->cpu->bc.b.h)
#define rC       (gb->cpu->bc.b.l)
#define rD       (gb->cpu->de.b.h)
#define rE       (gb->cpu->de.b.l)
#define rH       (gb->cpu->hl.b.h)
#define rL       (gb->cpu->hl.b.l)
#define SPh      (gb->cpu->sp.b.h)
#define SPl      (gb->cpu->sp.b.l)
#define PCh      (gb->cpu->pc.b.h)
#define PCl      (gb->cpu->pc.b.l)
#define rW       (gb->cpu->wz.b.h)
#define rZ       (gb->cpu->wz.b.l)

#define PREV_IR (gb->cpu->prev_ir)
#define IR      (gb->cpu->ir)
#define rAF      (gb->cpu->af.w)
#define rBC      (gb->cpu->bc.w)
#define rDE      (gb->cpu->de.w)
#define rHL      (gb->cpu->hl.w)
#define SP      (gb->cpu->sp.w)
#define PC      (gb->cpu->pc.w)
#define WZ      (gb->cpu->wz.w)

#define _IME    (gb->cpu->IME)

#endif