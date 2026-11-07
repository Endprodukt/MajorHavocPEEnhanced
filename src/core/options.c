#include "core/options.h"

static const int LIVES[4] = { 3, 6, 5, 4 };     /* tw_main.asm gamlvs */

static void decode(uint8_t o0, uint8_t o1, mh_options *o)
{
    o->roller_x2 = o0 & 1;
    o->attract_sound = (o0 >> 1) & 1;
    o->bonus = (o0 >> 2) & 3;
    o->difficulty = (o0 >> 4) & 3;
    o->lives = LIVES[(o0 >> 6) & 3];
    o->price = o1 & 3;
    o->raw1 = o1;
}

int mh_options_get(const mh_machine *m, mh_options *o)
{
    const uint8_t *e = m->eeprom + MH_EE_OPTIONS;
    if ((uint8_t)~(uint8_t)(e[0] + e[1]) == e[2]) {
        decode(e[0], e[1], o);
        return 1;
    }
    /* mh_gamma.asm loadop: ALLPOT reads the switches as the pots see them */
    decode((uint8_t)(~m->in.dsw1 ^ 0xe5), (uint8_t)(m->in.dsw2 ^ 0x01), o);
    return 0;
}

void mh_options_put(mh_machine *m, const mh_options *o)
{
    int code = 0;
    for (int i = 0; i < 4; i++)
        if (LIVES[i] == o->lives) code = i;
    uint8_t o0 = (uint8_t)((o->roller_x2 & 1) | ((o->attract_sound & 1) << 1) | ((o->bonus & 3) << 2) |
                           ((o->difficulty & 3) << 4) | (code << 6));
    uint8_t o1 = (uint8_t)((o->raw1 & ~3) | (o->price & 3));
    uint8_t *e = m->eeprom + MH_EE_OPTIONS;
    e[0] = o0;
    e[1] = o1;
    e[2] = (uint8_t)~(uint8_t)(o0 + o1);
    m->eeprom_dirty = 1;
}
