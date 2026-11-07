#include "core/emu.h"

#include <string.h>

/* The cycle of the access: an instruction reads or writes its operand in
 * its last cycle. */
static void alpha_time(mh_emu *e)
{
    uint8_t n = e->alpha.op_cycles;
    e->m->now = e->m->cycle + (n ? (uint64_t)(n - 1 + e->frac) / e->speed : 0);
}

static uint8_t alpha_read(void *user, uint16_t a)
{
    mh_emu *e = (mh_emu *)user;
    alpha_time(e);
    return mh_alpha_rd(e->m, a);
}

static void alpha_write(void *user, uint16_t a, uint8_t v)
{
    mh_emu *e = (mh_emu *)user;
    alpha_time(e);
    mh_alpha_wr(e->m, a, v);
}

static void gamma_time(mh_emu *e)
{
    uint8_t n = e->gamma.op_cycles;
    e->m->now = e->gtime + (n ? ((uint64_t)(n - 1) * MH_GAMMA_DIV + e->gfrac) / e->speed : 0);
}

static uint8_t gamma_read(void *user, uint16_t a)
{
    mh_emu *e = (mh_emu *)user;
    gamma_time(e);
    return mh_gamma_rd(e->m, a);
}

static void gamma_write(void *user, uint16_t a, uint8_t v)
{
    mh_emu *e = (mh_emu *)user;
    gamma_time(e);
    mh_gamma_wr(e->m, a, v);
}

void mh_emu_init(mh_emu *e, mh_machine *m)
{
    memset(e, 0, sizeof *e);
    e->m = m;
    m6502_init(&e->alpha, alpha_read, alpha_write, e);
    m6502_init(&e->gamma, gamma_read, gamma_write, e);
    m6502_reset(&e->alpha);
    m6502_reset(&e->gamma);
    m->cycle = 0;
    m->gcycle = 0;
    e->gtime = 0;
    e->next_tick = MH_TICK_CYCLES;
    e->speed = 1;
}

void mh_emu_set_speed(mh_emu *e, uint32_t speed)
{
    e->speed = speed ? speed : 1;
    e->frac = 0;
    e->gfrac = 0;
}

void mh_emu_step(mh_emu *e)
{
    mh_machine *m = e->m;
    int gamma_first = e->gtime < m->cycle;
    uint64_t t = gamma_first ? e->gtime : m->cycle;
    while (e->next_tick <= t) {
        mh_machine_tick(m);
        e->next_tick += MH_TICK_CYCLES;
    }
    if (!gamma_first) {
        e->alpha.irq = m->alpha_irq;
        uint64_t before = e->alpha.cycles;
        int took = m->alpha_irq && !(e->alpha.p & M6502_I);
        m6502_step(&e->alpha);
        if (took) e->alpha_irqs++;
        e->frac += (uint32_t)(e->alpha.cycles - before);
        m->cycle += e->frac / e->speed;
        e->frac %= e->speed;
        return;
    }
    if (m->gamma_held) {
        /* held in reset: it stands while the alpha goes on */
        e->gtime = m->cycle;
        m->gcycle = e->gtime / MH_GAMMA_DIV;
        return;
    }
    if (m->gamma_released) {
        m->gamma_released = 0;
        m->gamma_nmi = 0;
        m6502_reset(&e->gamma);
    }
    if (m->gamma_nmi) {
        m->gamma_nmi = 0;
        m6502_nmi(&e->gamma);
        e->gamma_nmis++;
    }
    e->gamma.irq = m->gamma_irq;
    if (m->gamma_irq && !(e->gamma.p & M6502_I)) e->gamma_irqs++;
    uint64_t before = e->gamma.cycles;
    m6502_step(&e->gamma);
    e->gfrac += (uint32_t)((e->gamma.cycles - before) * MH_GAMMA_DIV);
    e->gtime += e->gfrac / e->speed;
    e->gfrac %= e->speed;
    m->gcycle = e->gtime / MH_GAMMA_DIV;        /* (real time: the POKEYs' and the sound's clock) */
}

void mh_emu_run(mh_emu *e, uint64_t cycles)
{
    uint64_t end = e->m->cycle + cycles;
    while (e->m->cycle < end) {
        mh_emu_step(e);
        if (e->alpha.jammed) break;
    }
}

int mh_emu_jammed(const mh_emu *e)
{
    return e->alpha.jammed || e->gamma.jammed;
}
