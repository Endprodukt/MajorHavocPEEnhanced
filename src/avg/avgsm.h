/* avgsm.h - the vector generator's state machine, clock by clock, for its
 * timing: when it halts (IN0 bit 0), so that the program sees the bit
 * when it would.
 *
 * The board's state machine is a 256x4 PROM (136002-125) behind a latch,
 * clocked at 1.512 MHz; three bits of the state strobe the parts of the
 * generator. This follows MAME's avg_device and avg_mhavoc_device
 * (src/devices/video/avgdvg.cpp, BSD-3-Clause, Mathis Rosenhauer) down to
 * how MAME runs it: in batches of at least 10000 master clock cycles,
 * each reading vector memory as it is at the batch's start, the next
 * batch due when the time the batch took has passed. The halt becomes
 * visible after the cycles it took within its batch (MAME: vg_halt_timer).
 * (From Tempest Enhanced, with Major Havoc's memory: STAT's page bits
 * choose the vector ROM page the generator reads at 2000-3FFF.)
 *
 * Times are in master clock cycles (MAME's 12.096 MHz for every AVG).
 */
#ifndef MH_AVGSM_H
#define MH_AVGSM_H

#include <stdint.h>
#include <stdio.h>

#define MH_AVGSM_NEVER (~(uint64_t)0)

/* What the generator reads: its 0000-1FFF is the alpha's 4000-5FFF
 * (vector RAM, the first 4K of 6KL), its 2000-3FFF a page of 6H/6JK. */
typedef struct mh_vgmem {
    const uint8_t *low;     /* 8K */
    const uint8_t *bank;    /* 32K: four pages of 8K */
} mh_vgmem;

static inline uint8_t mh_vgmem_byte(const mh_vgmem *m, unsigned addr, unsigned map)
{
    addr &= 0x3fff;
    if (addr & 0x2000) return m->bank[((map & 3) << 13) | (addr & 0x1fff)];
    return m->low[addr & 0x1fff];
}

typedef struct mh_avgsm {
    uint8_t  prom[256];
    int      have_prom;
    uint8_t  state_latch, op, data, int_latch, dvy12, halt, sync_halt;
    uint16_t pc, sp, stack[4], dvx, dvy, timer;
    uint8_t  scale, bin_scale, map;
    uint64_t run_at;        /* next batch; MH_AVGSM_NEVER: none due */
    uint64_t halt_at;       /* the pending halt becomes visible; MH_AVGSM_NEVER: none */
    uint32_t batches;
    FILE    *trace;         /* not NULL: every batch and halt is noted here */
} mh_avgsm;

void mh_avgsm_init(mh_avgsm *v, const uint8_t *prom);

/* Bring the generator up to master time `t`: run every batch and fire
 * every halt due at or before it. */
void mh_avgsm_advance(mh_avgsm *v, const mh_vgmem *mem, uint64_t t);

void mh_avgsm_go(mh_avgsm *v, const mh_vgmem *mem, uint64_t t);   /* a write to VGGO at time t */
void mh_avgsm_reset(mh_avgsm *v);                                   /* a write to VGRST */

static inline int mh_avgsm_halted(const mh_avgsm *v) { return v->sync_halt; }

#endif
