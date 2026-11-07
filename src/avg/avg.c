#include "avg/avg.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_MAX_INSTRUCTIONS 50000
#define UNIT 65536.0f           /* the beam's position is 16.16 fixed point */
#define SPARKLE_PIECES 24       /* a sparkling vector drawn in at most this many pieces */

void mh_avg_list_free(mh_avg_list *l)
{
    free(l->lines);
    memset(l, 0, sizeof *l);
}

int mh_avg_list_reserve(mh_avg_list *l, int n)
{
    if (n <= l->cap) return 0;
    int cap = l->cap ? l->cap : 1024;
    while (cap < n) cap *= 2;
    mh_line *p = (mh_line *)realloc(l->lines, (size_t)cap * sizeof *p);
    if (!p) return -1;
    l->lines = p;
    l->cap = cap;
    return 0;
}

void mh_avg_state_reset(mh_avg_state *s)
{
    /* MAME: avg_vgrst clears the scales and the colour, mhavoc_vgrst the sparkle */
    s->bin_scale = 0;
    s->scale = 0;
    s->color = 0;
    s->sparkle = 0;
}

static int push_line(mh_avg_list *l, const mh_line *ln)
{
    if (l->count == l->cap && mh_avg_list_reserve(l, l->count + 1) != 0) return -1;
    l->lines[l->count++] = *ln;
    return 0;
}

static unsigned word_at(const mh_vgmem *mem, unsigned addr, unsigned map)
{
    return (unsigned)mh_vgmem_byte(mem, addr, map) | ((unsigned)mh_vgmem_byte(mem, addr + 1, map) << 8);
}

/* The normalization before a vector: both deltas are shifted up until one
 * of them uses its top bit; each shift halves the time the vector timer
 * runs. Returns the shifts. */
static int normalize(unsigned *dvy, unsigned *dvx)
{
    int i = 0;
    while (((*dvy ^ (*dvy << 1)) & 0x1000) == 0 && ((*dvx ^ (*dvx << 1)) & 0x1000) == 0 && i < 16) {
        i++;
        *dvy = (*dvy & 0x1000) | ((*dvy << 1) & 0x1fff);
        *dvx = (*dvx & 0x1000) | ((*dvx << 1) & 0x1fff);
    }
    return i;
}

/* A colour RAM entry as MAME shows it (active low bits). */
static void entry_rgb(uint8_t data, uint8_t *r, uint8_t *g, uint8_t *b)
{
    unsigned d = (unsigned)~data;
    *r = (uint8_t)(((d >> 3) & 1) * 0xcb + ((d >> 2) & 1) * 0x34);
    *g = (uint8_t)(((d >> 1) & 1) * 0xcb);
    *b = (uint8_t)((d & 1) * 0xcb);
}

static unsigned bitswap4(unsigned v, int b3, int b2, int b1, int b0)
{
    return (((v >> b3) & 1) << 3) | (((v >> b2) & 1) << 2) | (((v >> b1) & 1) << 1) | ((v >> b0) & 1);
}

static unsigned sparkle_step(unsigned s)
{
    s = ((((s >> 6) ^ (s >> 5)) & 1) ^ 1) | (s << 1);
    if ((s & 0x7f) == 0x7f) s = 0;
    return s & 0xff;
}

typedef struct run {
    mh_avg_list *out;
    int          clip;          /* 1: nothing above clip_y */
    float        clip_y;
    int          flip_x, flip_y;
} run;

/* A piece of a vector (generator units, before the mirroring), cut at
 * the clip. */
static int emit(run *r, mh_line *ln, float x0, float y0, float x1, float y1)
{
    if (r->clip) {
        float c = r->clip_y;
        if (y0 > c && y1 > c) return 0;
        if (y0 > c) { x0 = x1 + (x0 - x1) * (c - y1) / (y0 - y1); y0 = c; }
        else if (y1 > c) { x1 = x0 + (x1 - x0) * (c - y0) / (y1 - y0); y1 = c; }
    }
    if (r->flip_x) { x0 = -x0; x1 = -x1; }
    if (r->flip_y) { y0 = -y0; y1 = -y1; }
    ln->x0 = x0; ln->y0 = y0; ln->x1 = x1; ln->y1 = y1;
    return push_line(r->out, ln);
}

int mh_avg_run(const mh_vgmem *mem, const uint8_t *colram, mh_avg_state *st, int flip_x, int flip_y,
               mh_avg_list *out, int max_instructions)
{
    unsigned pc = 0;
    unsigned stack[4] = { 0, 0, 0, 0 };
    unsigned sp = 0;
    int depth = 0;
    unsigned src = 0, sub = 0xffff;
    unsigned grp = 0, item = 0, ikey = 0;
    int instructions = 0, status = MH_AVG_OK;
    run r = { out, 0, 0.0f, flip_x, flip_y };

    if (max_instructions <= 0) max_instructions = DEFAULT_MAX_INSTRUCTIONS;
    if (out) out->count = 0;

    for (;;) {
        if (instructions >= max_instructions) { status = MH_AVG_LIMIT; break; }
        instructions++;
        if (depth == 0) {
            src = pc;
            sub = 0xffff;
            item++;
            ikey = 0;
        }
        unsigned w = word_at(mem, pc, st->map);
        pc += 2;
        unsigned op = w >> 13;

        /* the clip: set by the first instruction after a SCAL with bit 11
         * (MAME: mhavoc_latch1), at the beam's height */
        if (!st->lst) {
            r.clip = 1;
            r.clip_y = (float)(st->ypos / UNIT);
            st->lst = 1;
        }

        unsigned dvy = 0, dvx = 0, z = 0;
        int k, vector = 0, cycles = 0;
        int halt = 0;

        switch (op) {
        case 0:  /* VCTR */
            dvy = w & 0x1fff;
            w = word_at(mem, pc, st->map);
            pc += 2;
            dvx = w & 0x1fff;
            z = w >> 13;
            k = normalize(&dvy, &dvx) + (int)st->bin_scale;
            cycles = k < 15 ? 0x8000 >> k : 1;
            vector = 1;
            break;
        case 1:  /* HALT */
            halt = 1;
            break;
        case 2:  /* SVEC */
            dvy = (w >> 8) & 0x1f;
            dvy = ((dvy & 0x10) << 8) | ((dvy & 0x0f) << 8);
            dvx = w & 0x1f;
            dvx = ((dvx & 0x10) << 8) | ((dvx & 0x0f) << 8);
            z = (w >> 5) & 7;
            k = normalize(&dvy, &dvx) + (int)st->bin_scale;
            cycles = k < 8 ? 0x100 >> k : 1;
            vector = 1;
            break;
        case 3:
            if (w & 0x1000) {           /* SCAL */
                st->scale = (uint8_t)(w & 0xff);
                st->bin_scale = (uint8_t)((w >> 8) & 7);
                if (w & 0x0800) st->lst = 0;
            } else {                    /* STAT */
                st->color = (uint8_t)(w & 0x0f);
                st->intensity = (uint8_t)((w >> 4) & 0x0f);
                st->map = (uint8_t)((w >> 8) & 3);
                st->xinv = (uint8_t)((w >> 10) & 1);
                st->sparkle = (uint8_t)((w >> 11) & 1);
                if (st->sparkle)
                    st->spkl = (uint8_t)(bitswap4(w, 0, 1, 2, 3) | ((unsigned)(rand() & 7) << 4));
            }
            break;
        case 4:  /* CNTR */
            st->xpos = st->ypos = 0;
            if (depth == 0) grp++;
            break;
        case 5:  /* JSRL */
            stack[sp & 3] = pc;
            sp = (sp + 1) & 0xf;
            pc = (w & 0x1fff) << 1;
            if (depth == 0) sub = ikey = pc;
            depth++;
            break;
        case 6:  /* RTSL */
            sp = (sp - 1) & 0xf;
            pc = stack[sp & 3];
            if (depth > 0) depth--;
            break;
        case 7:  /* JMPL */
            pc = (w & 0x1fff) << 1;
            /* back to the start: the picture is complete */
            if (pc == 0 && instructions > 1) halt = 1;
            break;
        }
        if (halt) break;

        if (vector) {
            /* The DACs take the top ten bits of each delta; X may be
             * inverted (xor 1FF instead of 200). */
            unsigned xxor = st->xinv ? 0x1ff : 0x200;
            int32_t dxs = (int32_t)(((dvx >> 3) ^ xxor)) - 0x200;
            int32_t dys = (int32_t)(((dvy >> 3) ^ 0x200)) - 0x200;
            int64_t dx = (int64_t)dxs * (int32_t)(st->scale ^ 0xff);
            int64_t dy = (int64_t)dys * (int32_t)(st->scale ^ 0xff);
            int32_t nx = st->xpos + (int32_t)((dx * cycles) >> 4), ny = st->ypos + (int32_t)((dy * cycles) >> 4);
            unsigned lum = z == 1 ? st->intensity : z << 1;
            if (out && lum) {
                mh_line ln;
                memset(&ln, 0, sizeof ln);
                ln.intensity = (uint8_t)lum;
                ln.z = (uint8_t)z;
                ln.src = (uint16_t)src;
                ln.sub = (uint16_t)sub;
                ln.grp = (uint16_t)grp;
                ln.item = (uint16_t)item;
                ln.ikey = (uint16_t)ikey;
                if (!st->sparkle) {
                    ln.color = st->color;
                    entry_rgb(colram[st->color & 15], &ln.r, &ln.g, &ln.b);
                    if (emit(&r, &ln, st->xpos / UNIT, st->ypos / UNIT, nx / UNIT, ny / UNIT) != 0) {
                        status = MH_AVG_LIMIT;
                        break;
                    }
                } else {
                    /* steps of 8 clocks, each its own colour; drawn in at
                     * most SPARKLE_PIECES pieces, each the colour of its
                     * first step */
                    int steps = cycles / 8 > 0 ? cycles / 8 : 1;
                    int per = (steps + SPARKLE_PIECES - 1) / SPARKLE_PIECES;
                    unsigned s = st->spkl;
                    for (int i = 0; i < steps; i += per) {
                        int n = i + per <= steps ? per : steps - i;
                        unsigned idx = 0x0f + bitswap4(s, 0, 2, 4, 6);
                        ln.color = (uint8_t)idx;
                        entry_rgb(colram[idx & 31], &ln.r, &ln.g, &ln.b);
                        float t0 = (float)i / (float)steps, t1 = (float)(i + n) / (float)steps;
                        float x0 = st->xpos / UNIT, y0 = st->ypos / UNIT, x1 = nx / UNIT, y1 = ny / UNIT;
                        if (emit(&r, &ln, x0 + (x1 - x0) * t0, y0 + (y1 - y0) * t0,
                                 x0 + (x1 - x0) * t1, y0 + (y1 - y0) * t1) != 0) {
                            status = MH_AVG_LIMIT;
                            break;
                        }
                        for (int j = 0; j < n; j++) s = sparkle_step(s);
                    }
                    st->spkl = (uint8_t)s;
                    if (status != MH_AVG_OK) break;
                }
            } else if (st->sparkle) {
                int steps = cycles / 8;
                for (int j = 0; j < steps; j++) st->spkl = (uint8_t)sparkle_step(st->spkl);
            }
            st->xpos = nx;
            st->ypos = ny;
        }
    }
    if (out) {
        out->instructions = instructions;
        out->status = status;
    }
    return status;
}

uint32_t mh_avg_cycles(const mh_vgmem *mem, uint8_t map)
{
    /* States the machine goes through per instruction, 8 clock cycles each
     * (the state PROM 136002-125). */
    static const uint8_t STATES[8] = { 8, 4, 6, 7, 5, 5, 3, 2 };
    unsigned pc = 0, sp = 0, bin_scale = 0;
    unsigned stack[4] = { 0, 0, 0, 0 };
    uint32_t cycles = 0;

    for (int n = 0; n < DEFAULT_MAX_INSTRUCTIONS; n++) {
        unsigned w = word_at(mem, pc, map), dvy, dvx;
        int k;
        pc += 2;
        cycles += 8u * STATES[w >> 13];
        switch (w >> 13) {
        case 0:
            dvy = w & 0x1fff;
            dvx = word_at(mem, pc, map) & 0x1fff;
            pc += 2;
            k = normalize(&dvy, &dvx) + (int)bin_scale;
            cycles += k < 15 ? 0x8000u >> k : 1u;
            break;
        case 1:
            return cycles;
        case 2:
            dvy = (w & 0x1f00);
            dvx = (w & 0x1f) << 8;
            k = normalize(&dvy, &dvx) + (int)bin_scale;
            cycles += k < 8 ? 0x100u >> k : 1u;
            break;
        case 3:
            if (w & 0x1000) bin_scale = (w >> 8) & 7;
            else map = (uint8_t)((w >> 8) & 3);
            break;
        case 4:
            cycles += 0x8000u;
            break;
        case 5:
            stack[sp & 3] = pc;
            sp++;
            pc = (w & 0x1fff) << 1;
            break;
        case 6:
            sp--;
            pc = stack[sp & 3];
            break;
        case 7:
            pc = (w & 0x1fff) << 1;
            if (pc == 0) return cycles;
            break;
        }
    }
    return cycles;
}
