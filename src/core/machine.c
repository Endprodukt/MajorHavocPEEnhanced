#include "core/machine.h"

#include <string.h>

void mh_machine_init(mh_machine *m)
{
    memset(m, 0, sizeof *m);
    m->alpha_irq_enable = 1;
    for (int i = 0; i < 4; i++) mh_pokeyio_init(&m->pk[i]);
    memset(m->eeprom, 0xff, sizeof m->eeprom);  /* erased */
    /* what a new EEPROM takes (core/options.h, the game's loadop): 13/14S
     * ALLPOT 00 gives options 1A (3 lives, medium, bonus at 100000, attract
     * sounds, roller x1); 8S FF gives price 2, 1 coin 1 play (tried: FF one
     * coin one play, FE two coins one play, FD free play, FC one coin two
     * plays). The service menu sets them in the EEPROM. */
    m->in.dsw1 = 0x00;
    m->in.dsw2 = 0xff;
    m->vg_halt_at = ~(uint64_t)0;
    mh_avgsm_init(&m->vg, NULL);
    m->vg.sync_halt = 1;        /* MAME: halted until the program starts it */
}

void mh_machine_set_rom(mh_machine *m, const mh_roms_in *r)
{
    memcpy(m->amem + MH_A_PROGRAM, r->r1mn, 0x4000);
    memcpy(m->amem + MH_A_PROGRAM + 0x4000, r->r1l, 0x4000);
    memcpy(m->paged, r->r1q, 0x8000);
    memcpy(m->paged + 0x8000, r->r1np, 0x8000);
    memcpy(m->amem + MH_A_VGROM, r->r6kl, 0x1000);   /* (its second 4K is empty: 6000-7FFF is the page, below) */
    memcpy(m->vgbank, r->r6h, 0x4000);
    memcpy(m->vgbank + 0x4000, r->r6jk, 0x4000);
    memcpy(m->grom, r->r9s, 0x4000);
    m->have_rom = 1;
}

void mh_machine_set_avg_prom(mh_machine *m, const uint8_t *prom)
{
    mh_avgsm_init(&m->vg, prom);
    m->vg.sync_halt = 1;
    m->have_prom = 1;
}

/* --- the vector generator ----------------------------------------------------- */

static void live_mem(const mh_machine *m, mh_vgmem *v)
{
    v->low = m->amem + MH_A_VGRAM;
    v->bank = m->vgbank;
}

void mh_machine_snap_mem(const mh_machine *m, mh_vgmem *out)
{
    out->low = m->snap_low;
    out->bank = m->vgbank;
}

/* The generator's clock at the access being made. */
static uint64_t vg_time(const mh_machine *m)
{
    return m->now * 12096u / 2500u;
}

static int vg_halted(mh_machine *m)
{
    if (m->have_prom) {
        mh_vgmem v;
        live_mem(m, &v);
        mh_avgsm_advance(&m->vg, &v, vg_time(m));
        return mh_avgsm_halted(&m->vg);
    }
    return m->now >= m->vg_halt_at;
}

/* The page the generator has selected by now. */
static unsigned vg_page(mh_machine *m)
{
    if (m->have_prom) {
        mh_vgmem v;
        live_mem(m, &v);
        mh_avgsm_advance(&m->vg, &v, vg_time(m));
        return m->vg.map & 3;
    }
    return m->vg_state.map & 3;
}

static void vg_go(mh_machine *m)
{
    mh_vgmem v;
    live_mem(m, &v);
    /* the picture as it is now, for the renderer; the generator's state
     * followed through it for the next one */
    memcpy(m->snap_low, m->amem + MH_A_VGRAM, sizeof m->snap_low);
    memcpy(m->snap_colram, m->colram, sizeof m->snap_colram);
    m->snap_state = m->vg_state;
    m->snap_flip_x = (uint8_t)((m->outputs >> 6) & 1);
    m->snap_flip_y = (uint8_t)((m->outputs >> 7) & 1);
    m->snap_starts = ++m->vg_starts;
    m->snap_at = m->now;
    mh_avg_run(&v, m->colram, &m->vg_state, 0, 0, NULL, 0);
    if (m->have_prom) {
        mh_avgsm_go(&m->vg, &v, vg_time(m));
    } else {
        uint32_t c = mh_avg_cycles(&v, m->vg_state.map);
        m->vg_halt_at = m->now + (uint64_t)c * 2500u / 12096u;
    }
}

static void vg_reset(mh_machine *m)
{
    if (m->have_prom) {
        mh_vgmem v;
        live_mem(m, &v);
        mh_avgsm_advance(&m->vg, &v, vg_time(m));
        mh_avgsm_reset(&m->vg);
    }
    m->vg_halt_at = m->now;
    mh_avg_state_reset(&m->vg_state);
}

/* --- interrupts ------------------------------------------------------------------ */

void mh_machine_tick(mh_machine *m)
{
    /* the LS161 behind the alpha's interrupt: raised on the 12th tick
     * after the acknowledge */
    if (m->alpha_irq_enable) {
        m->alpha_irq_clock++;
        if ((m->alpha_irq_clock & 0x0c) == 0x0c) {
            m->alpha_irq = 1;
            m->alpha_irq_enable = 0;
        }
    }
    /* the gamma's: up for 8 ticks of 16, unless acknowledged; the
     * acknowledge starts the count again, so a late one (the gamma busy
     * with the alpha's commands, NMIs) makes every later interrupt late,
     * and the music (rpm.asm g_irq) slower. Steady: raised every 8 ticks
     * whenever it was acknowledged (up until it is), the music at its
     * tempo. */
    m->gamma_irq_clock++;
    if (m->gamma_irq_steady) {
        /* one due while one is still up (the gamma busy longer than a
         * beat) is given right after the acknowledge: the music catches
         * up (at most 2; none while the gamma is held in reset) */
        if ((m->gamma_irq_clock & 0x07) == 0 && !m->gamma_held) {
            if (m->gamma_irq && m->gamma_irq_owed < 2) m->gamma_irq_owed++;
            m->gamma_irq = 1;
        }
    } else {
        m->gamma_irq = (m->gamma_irq_clock & 0x08) ? 1 : 0;
    }
}

/* --- alpha ---------------------------------------------------------------------- */

static uint8_t *zram_at(mh_machine *m, uint16_t a)
{
    if (a >= MH_A_ZRAM_LO && a < MH_A_RAM1) return &m->zram[m->rampg][a - MH_A_ZRAM_LO];
    if (a >= MH_A_ZRAM_HI && a < MH_A_PORTRD) return &m->zram[m->rampg][a - MH_A_ZRAM_HI];
    return NULL;
}

static uint8_t in0(mh_machine *m)
{
    const mh_inputs *in = &m->in;
    uint8_t v = 0;
    if (vg_halted(m)) v |= MH_IN0_VGHALT;
    if (!(m->now & 0x400)) v |= MH_IN0_2K4;
    if (m->gamma_xmtd) v |= MH_IN0_GXMTD;
    if (m->gamma_rcvd) v |= MH_IN0_GRCVD;
    v |= MH_IN0_UNUSED;
    if (!in->diag) v |= MH_IN0_DIAG;
    if (m->outputs & 0x20) {
        /* player select 1: the cabinet switch, self test */
        v |= MH_IN0_BIT6;
        if (!in->self_test) v |= MH_IN0_BIT7;
    } else {
        if (!in->coin_r) v |= MH_IN0_BIT6;
        if (!in->coin_l) v |= MH_IN0_BIT7;
    }
    return v;
}

uint8_t mh_alpha_rd(mh_machine *m, uint16_t a)
{
    if (a < 0x1000) {
        uint8_t *z = zram_at(m, a);
        return z ? *z : m->amem[a];
    }
    if (a >= MH_A_PAGED && a < MH_A_VGRAM) return m->paged[(unsigned)m->rompg * 0x2000 + (a - MH_A_PAGED)];
    if (a >= MH_A_VGROM2 && a < MH_A_PROGRAM) {
        /* the vector ROM page the generator's last STAT chose (the self
         * test sets it so to sum the pages; MAME shows 6KL's empty second
         * half here) */
        return m->vgbank[((unsigned)vg_page(m) << 13) | (a & 0x1fff)];
    }
    if (a >= MH_A_COMRAM) return m->amem[a];       /* COMRAM, vector RAM and ROM, the program */
    if (a >= MH_A_COLRAM && a < MH_A_COLRAM + 0x20) return m->colram[a & 31];
    switch (a) {
    case MH_A_PORTRD:
        m->alpha_rcvd = 1;
        m->gamma_xmtd = 0;
        return m->gamma_data;
    case MH_A_INPUTS:
        return in0(m);
    default:
        return 0x00;
    }
}

void mh_alpha_wr(mh_machine *m, uint16_t a, uint8_t v)
{
    if (a < 0x1000) {
        uint8_t *z = zram_at(m, a);
        if (z) *z = v;
        else m->amem[a] = v;
        return;
    }
    if (a >= MH_A_COMRAM && a < MH_A_PAGED) { m->amem[a] = v; return; }
    if (a >= MH_A_VGRAM && a < MH_A_VGROM) {
        /* the generator's batches due by now read vector RAM as it was */
        if (m->have_prom) {
            mh_vgmem vm;
            live_mem(m, &vm);
            mh_avgsm_advance(&m->vg, &vm, vg_time(m));
        }
        m->amem[a] = v;
        return;
    }
    if (a >= MH_A_COLRAM && a < MH_A_COLRAM + 0x20) { m->colram[a & 31] = v; return; }
    switch (a) {
    case MH_A_OUTPUTS:
        m->outputs = v;
        if (!(v & 0x08)) {
            /* the gamma held in reset: the latches clear */
            m->gamma_held = 1;
            m->gamma_irq = 0;
            m->gamma_irq_owed = 0;
            m->alpha_rcvd = m->alpha_xmtd = 0;
            m->gamma_rcvd = m->gamma_xmtd = 0;
        } else if (m->gamma_held) {
            m->gamma_held = 0;
            m->gamma_released = 1;
        }
        return;
    case MH_A_VGGO:     vg_go(m); return;
    case MH_A_WATCHDOG: m->watchdog++; return;
    case MH_A_VGRST:    vg_reset(m); return;
    case MH_A_INTACK:
        m->alpha_irq = 0;
        m->alpha_irq_clock = 0;
        m->alpha_irq_enable = 1;
        return;
    case MH_A_ROMPG:
        /* (the PE board's page bits: D0, D2, D1 as page bits 0, 1, 2) */
        m->rompg = (uint8_t)((v & 1) | ((v & 2) << 1) | ((v & 4) >> 1));
        return;
    case MH_A_RAMPG:    m->rampg = v & 1; return;
    case MH_A_PORTWR:
        m->gamma_rcvd = 0;
        m->alpha_xmtd = 1;
        m->alpha_data = v;
        m->gamma_nmi = 1;
        return;
    default:
        return;
    }
}

/* --- gamma ---------------------------------------------------------------------- */

/* The quad POKEY: address bits 0-2 the register's, 3-4 the chip, 5 the
 * register's bit 3. */
static void pokey_of(uint16_t a, int *chip, int *reg)
{
    unsigned off = a & 0x3f;
    *chip = (int)((off >> 3) & 3);
    *reg = (int)((off & 7) | ((off & 0x20) >> 2));
}

static void update_pots(mh_machine *m)
{
    /* 13/14S on POKEY 0's pots: a switch off is a pot that counts, its
     * ALLPOT bit 1 right after POTGO (when the game reads it); the other
     * chips' pots are open */
    m->pk[0].inputs = m->in.dsw1;
}

uint8_t mh_gamma_rd(mh_machine *m, uint16_t a)
{
    if (a < MH_G_POKEY) return m->gram[a & 0x7ff];
    if (a >= MH_G_ROM) return m->grom[a & 0x3fff];
    if (a < MH_G_INPUT) {
        int chip, reg;
        pokey_of(a, &chip, &reg);
        if (chip == 0) update_pots(m);
        return mh_pokeyio_read(&m->pk[chip], (unsigned)reg, m->now / MH_GAMMA_DIV);
    }
    if (a < MH_G_INDATA) {
        const mh_inputs *in = &m->in;
        uint8_t v = MH_IN1_UNUSED;
        if (m->alpha_xmtd) v |= MH_IN1_AXMTD;
        if (m->alpha_rcvd) v |= MH_IN1_ARCVD;
        if (!in->b2[1]) v |= MH_IN1_P2_B2;
        if (!in->b1[1]) v |= MH_IN1_P2_B1;
        if (!in->b2[0]) v |= MH_IN1_B2;
        if (!in->b1[0]) v |= MH_IN1_B1;
        return v;
    }
    if (a < MH_G_LETA) {
        m->gamma_rcvd = 1;
        m->alpha_xmtd = 0;
        return m->alpha_data;
    }
    if (a == MH_G_LETA) return m->in.roller[0];
    if (a == MH_G_LETA + 1) return m->in.roller[1];
    if (a >= MH_G_INTACK && a < MH_G_COUNTER) return m->in.dsw2;
    if (a >= MH_G_EEROM && a < MH_G_ROM) return m->eeprom[a & 0x1ff];
    return 0x00;
}

void mh_gamma_wr(mh_machine *m, uint16_t a, uint8_t v)
{
    if (a < MH_G_POKEY) { m->gram[a & 0x7ff] = v; return; }
    if (a < MH_G_INPUT) {
        int chip, reg;
        pokey_of(a, &chip, &reg);
        if (chip == 0 && reg == MH_PK_POTGO) update_pots(m);
        mh_pokeyio_write(&m->pk[chip], (unsigned)reg, v, m->now / MH_GAMMA_DIV);
        if (m->sound_write) m->sound_write(m->sound_user, (uint16_t)(MH_G_POKEY + chip * 16 + reg), v);
        return;
    }
    if (a >= MH_G_INTACK && a < MH_G_COUNTER) {
        m->gamma_irq = 0;
        if (!m->gamma_irq_steady) m->gamma_irq_clock = 0;
        else if (m->gamma_irq_owed) { m->gamma_irq_owed--; m->gamma_irq = 1; }
        return;
    }
    if (a >= MH_G_COUNTER && a < MH_G_OUTDATA) { m->coin_counter = v; return; }
    if (a >= MH_G_OUTDATA && a < MH_G_OKI) {
        m->alpha_rcvd = 0;
        m->gamma_xmtd = 1;
        m->gamma_data = v;
        return;
    }
    if (a == MH_G_OKI || a == MH_G_OKI2) {
        if (m->sound_write) m->sound_write(m->sound_user, MH_G_OKI, v);
        return;
    }
    if (a >= MH_G_EEROM && a < MH_G_ROM) {
        unsigned i = a & 0x1ff;
        if (m->eeprom[i] != v) {
            m->eeprom[i] = v;
            m->eeprom_dirty = 1;
        }
        return;
    }
}
