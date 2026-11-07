/* machine.h - the Major Havoc board (as The Promised End uses it), as its
 * two programs see it.
 *
 * Both programs run from the ROMs on two 6502 cores (core/emu.h);
 * everything they talk to is here: the alpha's RAM with its paged half,
 * the paged program ROM, colour RAM, vector RAM and the vector generator,
 * the two latches between the CPUs, the 5 kHz clock behind both
 * interrupts, the gamma's four POKEYs, switches, roller and EEPROM. The
 * OKI 6295's writes go to the sound (sound/sound.h). Memory map:
 * core/mh_map.h.
 */
#ifndef MH_MACHINE_H
#define MH_MACHINE_H

#include <stdint.h>

#include "avg/avg.h"
#include "avg/avgsm.h"
#include "core/mh_map.h"
#include "core/pokeyio.h"

typedef struct mh_inputs {
    uint8_t coin_l, coin_r;     /* 1: the coin switch closed */
    uint8_t diag;               /* 1: the diagnostic step (aux coin) switch closed */
    uint8_t self_test;          /* 1: the self test switch on */
    uint8_t b1[2], b2[2];       /* each player's buttons: 1 jump, 2 shield; 1 pressed */
    uint8_t roller[2];          /* each player's roller count */
    uint8_t dsw1;               /* 13/14S, on POKEY 0's pots: a bit 1 is a switch off (ALLPOT reads it so) */
    uint8_t dsw2;               /* 8S, read at 4000: a bit 1 is a switch off */
} mh_inputs;

/* A write the gamma makes to its POKEYs (2000-27FF) or the OKI (5800, 5900). */
typedef void (*mh_sound_fn)(void *user, uint16_t addr, uint8_t value);

typedef struct mh_machine {
    /* --- alpha --- */
    uint8_t     amem[0x10000];  /* fixed RAM, vector RAM and ROM, the program; I/O and paged parts unused */
    uint8_t     zram[2][0x600]; /* the paged RAM (0200-07FF and 0A00-0FFF show the same bank) */
    uint8_t     paged[0x10000]; /* 1Q, 1NP: eight pages of 8K at 2000 */
    uint8_t     vgbank[0x8000]; /* 6H, 6JK: the generator's four pages at its 2000 */
    uint8_t     colram[32];
    uint8_t     rampg, rompg;   /* the RAM bank, the ROM page (0..7) */
    uint8_t     outputs;        /* 1600 */
    uint8_t     alpha_irq;      /* the interrupt line */
    uint8_t     alpha_irq_clock, alpha_irq_enable;
    uint32_t    watchdog;

    /* --- gamma --- */
    uint8_t     gram[0x800];
    uint8_t     grom[0x4000];   /* 9S at 8000, again at C000 */
    uint8_t     gamma_irq;
    uint8_t     gamma_irq_clock;
    uint8_t     gamma_irq_steady;   /* 1: every 8 ticks however late acknowledged (steady music); 0: as the board */
    uint8_t     gamma_irq_owed;     /* steady: interrupts due while one was still up, given after the acknowledge */
    uint8_t     gamma_nmi;      /* an NMI is due (the alpha wrote its latch) */
    uint8_t     gamma_held;     /* held in reset by the alpha (1600 D3 clear) */
    uint8_t     gamma_released; /* it has just been let go: reset it */
    uint8_t     coin_counter;
    mh_pokeyio  pk[4];
    uint8_t     eeprom[512];
    int         eeprom_dirty;

    /* --- the latches --- */
    uint8_t     alpha_data, alpha_rcvd, alpha_xmtd;     /* alpha -> gamma */
    uint8_t     gamma_data, gamma_rcvd, gamma_xmtd;     /* gamma -> alpha */

    /* --- the vector generator --- */
    mh_avgsm    vg;             /* its state machine (timing), with the PROM */
    uint64_t    vg_halt_at;     /* without the PROM: the cycle it halts */
    mh_avg_state vg_state;      /* what it keeps from picture to picture */
    uint32_t    vg_starts;      /* writes to VGGO: a new picture */
    /* The picture as the generator was started on it: its memory and
     * state then (the renderer runs on this, avg/avg.h). */
    uint8_t     snap_low[0x2000];
    uint8_t     snap_colram[32];
    mh_avg_state snap_state;
    uint8_t     snap_flip_x, snap_flip_y;
    uint32_t    snap_starts;
    uint64_t    snap_at;        /* the alpha cycle it was started */

    int         have_rom, have_prom;
    mh_inputs   in;

    uint64_t    cycle;          /* alpha cycles since power on (the time base) */
    uint64_t    now;            /* the time of the access being made (alpha cycles) */
    uint64_t    gcycle;         /* gamma cycles since power on */

    mh_sound_fn sound_write;
    void       *sound_user;
} mh_machine;

void mh_machine_init(mh_machine *m);

/* The ROMs (sizes as the chips): 1MN, 1L 16K; 1Q, 1NP 32K; 6KL 8K; 6H,
 * 6JK 16K; 9S 16K. */
typedef struct mh_roms_in {
    const uint8_t *r1mn, *r1l, *r1q, *r1np, *r6kl, *r6h, *r6jk, *r9s;
} mh_roms_in;
void mh_machine_set_rom(mh_machine *m, const mh_roms_in *r);
/* The vector generator's state PROM (136002-125): with it the halt bit
 * comes clock-exact (avg/avgsm.h); without, from the list's length. */
void mh_machine_set_avg_prom(mh_machine *m, const uint8_t *prom);

uint8_t mh_alpha_rd(mh_machine *m, uint16_t addr);
void    mh_alpha_wr(mh_machine *m, uint16_t addr, uint8_t v);
uint8_t mh_gamma_rd(mh_machine *m, uint16_t addr);
void    mh_gamma_wr(mh_machine *m, uint16_t addr, uint8_t v);

/* A tick of the 5 kHz clock: both interrupts' counters. */
void mh_machine_tick(mh_machine *m);

/* The generator's view of the last picture started (for avg/avg.h). */
void mh_machine_snap_mem(const mh_machine *m, mh_vgmem *out);

#endif
