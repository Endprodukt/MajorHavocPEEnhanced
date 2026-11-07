#include "sound/oki6295.h"

#include <math.h>
#include <string.h>

#define ROM_MASK 0x3ffff

static const int8_t INDEX_SHIFT[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
static const float VOLUME[16] = {
    0x20 / 32.0f, 0x16 / 32.0f, 0x10 / 32.0f, 0x0b / 32.0f, 0x08 / 32.0f, 0x06 / 32.0f, 0x04 / 32.0f, 0x03 / 32.0f,
    0x02 / 32.0f, 0, 0, 0, 0, 0, 0, 0,
};
static int g_diff[49 * 16];
static int g_tables;

static void tables(void)
{
    static const int8_t NBL2BIT[16][4] = {
        { 1, 0, 0, 0 }, { 1, 0, 0, 1 }, { 1, 0, 1, 0 }, { 1, 0, 1, 1 },
        { 1, 1, 0, 0 }, { 1, 1, 0, 1 }, { 1, 1, 1, 0 }, { 1, 1, 1, 1 },
        { -1, 0, 0, 0 }, { -1, 0, 0, 1 }, { -1, 0, 1, 0 }, { -1, 0, 1, 1 },
        { -1, 1, 0, 0 }, { -1, 1, 0, 1 }, { -1, 1, 1, 0 }, { -1, 1, 1, 1 },
    };
    if (g_tables) return;
    for (int step = 0; step <= 48; step++) {
        int stepval = (int)floor(16.0 * pow(11.0 / 10.0, (double)step));
        for (int nib = 0; nib < 16; nib++)
            g_diff[step * 16 + nib] = NBL2BIT[nib][0] *
                (stepval * NBL2BIT[nib][1] + stepval / 2 * NBL2BIT[nib][2] + stepval / 4 * NBL2BIT[nib][3] + stepval / 8);
    }
    g_tables = 1;
}

void mh_oki_init(mh_oki *o, const uint8_t *rom)
{
    tables();
    memset(o, 0, sizeof *o);
    o->rom = rom;
    o->command = -1;
}

static uint8_t rd(const mh_oki *o, uint32_t a)
{
    return o->rom ? o->rom[a & ROM_MASK] : 0;
}

void mh_oki_write(mh_oki *o, uint8_t data)
{
    if (o->command != -1) {
        unsigned mask = data >> 4;
        for (int i = 0; i < 4; i++, mask >>= 1) {
            mh_oki_voice *v = &o->v[i];
            if (!(mask & 1) || v->playing) continue;
            uint32_t base = (uint32_t)o->command * 8;
            uint32_t start = ((uint32_t)rd(o, base) << 16 | (uint32_t)rd(o, base + 1) << 8 | rd(o, base + 2)) & ROM_MASK;
            uint32_t stop = ((uint32_t)rd(o, base + 3) << 16 | (uint32_t)rd(o, base + 4) << 8 | rd(o, base + 5)) & ROM_MASK;
            if (start >= stop) continue;
            v->playing = 1;
            v->base = start;
            v->sample = 0;
            v->count = 2 * (stop - start + 1);
            v->signal = 0;
            v->step = 0;
            v->volume = VOLUME[data & 15];
            o->phrases++;
        }
        o->command = -1;
    } else if (data & 0x80) {
        o->command = data & 0x7f;
    } else {
        unsigned mask = data >> 3;
        for (int i = 0; i < 4; i++, mask >>= 1)
            if (mask & 1) o->v[i].playing = 0;
    }
}

float mh_oki_sample(mh_oki *o)
{
    float sum = 0.0f;
    for (int i = 0; i < 4; i++) {
        mh_oki_voice *v = &o->v[i];
        if (!v->playing) continue;
        unsigned nibble = (unsigned)(rd(o, v->base + v->sample / 2) >> (((v->sample & 1) << 2) ^ 4)) & 15;
        v->signal += g_diff[v->step * 16 + nibble];
        if (v->signal > 2047) v->signal = 2047;
        else if (v->signal < -2048) v->signal = -2048;
        v->step += INDEX_SHIFT[nibble & 7];
        if (v->step > 48) v->step = 48;
        else if (v->step < 0) v->step = 0;
        sum += (float)v->signal * v->volume / 2048.0f;
        if (++v->sample >= v->count) v->playing = 0;
    }
    return sum;
}
