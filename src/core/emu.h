/* emu.h - the two original programs on the board.
 *
 * Two 6502 cores (cpu/m6502.h) run the alpha's and the gamma's programs
 * from the ROMs against mh_machine, interleaved instruction by
 * instruction in time: the one behind goes next. Time is counted in alpha
 * cycles (2.5 MHz); a gamma cycle (1.25 MHz) is two of them. Every 512
 * alpha cycles the 5 kHz clock ticks both interrupts' counters
 * (mh_machine_tick). The alpha's write to the gamma's latch gives the
 * gamma an NMI; while the alpha holds the gamma in reset it stands, and
 * it starts from its reset vector when let go.
 */
#ifndef MH_EMU_H
#define MH_EMU_H

#include "core/machine.h"
#include "cpu/m6502.h"

typedef struct mh_emu {
    mh_machine *m;
    m6502       alpha, gamma;
    uint64_t    gtime;      /* the gamma's time, in alpha cycles */
    uint64_t    next_tick;  /* the next 5 kHz tick, in alpha cycles */
    uint64_t    alpha_irqs, gamma_nmis, gamma_irqs;
    uint32_t    speed;      /* both CPUs run this many times as fast as the cabinet's (1: as the cabinet) */
    uint32_t    frac;       /* the alpha's cycles not yet a whole cycle of time */
    uint32_t    gfrac;      /* the gamma's the same (in alpha cycles) */
} mh_emu;

/* The machine must be initialized and have its ROMs. Resets both CPUs. */
void mh_emu_init(mh_emu *e, mh_machine *m);

/* One instruction of whichever CPU is behind. */
void mh_emu_step(mh_emu *e);

/* Steady frames: the CPUs `speed` times as fast as the cabinet's (1: as
 * the cabinet). The game draws a picture every 8 interrupts (50.9 a
 * second) when it is done in time; on the cabinet a busy scene takes
 * longer and the game slows down (down to about 35 pictures a second).
 * Faster, every picture is done in time. The gamma goes as fast as the
 * alpha: the alpha's commands (NMIs) then take the gamma as much of its
 * time as on the board (a faster alpha alone sent twice as many a second
 * and held up the gamma's interrupt, the music). The interrupts, the
 * vector generator, the POKEYs, the speech and the sound keep real time. */
void mh_emu_set_speed(mh_emu *e, uint32_t speed);

/* Run until `cycles` more alpha cycles of time have passed. */
void mh_emu_run(mh_emu *e, uint64_t cycles);

/* 1 if either CPU met an undocumented opcode and stands. */
int mh_emu_jammed(const mh_emu *e);

#endif
