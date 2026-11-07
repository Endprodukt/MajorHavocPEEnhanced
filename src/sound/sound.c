#include "sound/sound.h"

#include "core/mh_map.h"

#include <math.h>
#include <string.h>

#define POKEY_HZ         (MH_MASTER_HZ / 8.0)
#define TICKS_PER_SAMPLE (POKEY_HZ / MH_SOUND_RATE)
#define POKEY_R          1000.0     /* HBMAME: set_output_opamp(RES_K(1), CAP_U(0.001), 5.0) */
#define POKEY_MIX        0.25       /* HBMAME: each POKEY to the speaker at 0.25 */
#define SPEECH_MIX       1.0        /* HBMAME: the OKI at 1.0 */
#define KNEE             0.8        /* below this the level is HBMAME's exactly */

void mh_sound_init(mh_sound *s, const uint8_t *speech_rom)
{
    memset(s, 0, sizeof *s);
    for (int i = 0; i < 4; i++) pokey_reset(&s->pk[i]);
    mh_oki_init(&s->oki, speech_rom);
    s->volume = 1.0f;
    s->speech = 1.0f;
}

void mh_sound_write(void *user, uint16_t addr, uint8_t value)
{
    mh_sound *s = (mh_sound *)user;
    if (addr >= MH_G_POKEY && addr < MH_G_POKEY + 0x40)
        pokey_write(&s->pk[(addr - MH_G_POKEY) >> 4], addr & 15, value);
    else if (addr == MH_G_OKI)
        mh_oki_write(&s->oki, value);
}

void mh_sound_run(mh_sound *s, int gamma_cycles)
{
    s->tick_frac += gamma_cycles;
    while (s->tick_frac >= TICKS_PER_SAMPLE) {
        /* (26.04 ticks a sample: whole ticks, the rest carried) */
        s->tick_carry += TICKS_PER_SAMPLE;
        unsigned ticks = (unsigned)s->tick_carry;
        s->tick_carry -= ticks;
        s->tick_frac -= TICKS_PER_SAMPLE;
        double x = 0.0;
        for (int i = 0; i < 4; i++) x += POKEY_MIX * pokey_run_opamp(&s->pk[i], ticks, POKEY_R);
        /* the speech at 8 kHz, a line between its samples */
        s->oki_frac += (double)MH_OKI_RATE / MH_SOUND_RATE;
        while (s->oki_frac >= 1.0) {
            s->oki_frac -= 1.0;
            s->oki_last = s->oki_cur;
            s->oki_cur = mh_oki_sample(&s->oki);
        }
        x += SPEECH_MIX * s->speech * (s->oki_last + (s->oki_cur - s->oki_last) * (float)s->oki_frac);
        s->dc += (x - s->dc) * 0.0005;      /* about 4 Hz: the coupling capacitor */
        x = (x - s->dc) * s->volume;
        /* MAME clips at full scale; the speech alone reaches it, and with
         * the POKEYs it goes past: above 0.8 a soft knee instead, up to 1 */
        if (x > KNEE) x = KNEE + (1.0 - KNEE) * tanh((x - KNEE) / (1.0 - KNEE));
        else if (x < -KNEE) x = -KNEE - (1.0 - KNEE) * tanh((-x - KNEE) / (1.0 - KNEE));
        if (fabs(x) > s->peak) s->peak = (float)fabs(x);
        uint32_t w = s->ring_w;
        if (w - s->ring_r < MH_SOUND_RING) {
            s->ring[w % MH_SOUND_RING] = (float)x;
            s->ring_w = w + 1;
        }
    }
}

int mh_sound_read(mh_sound *s, float *out, int n)
{
    uint32_t r = s->ring_r, have = s->ring_w - r;
    if ((uint32_t)n > have) n = (int)have;
    for (int i = 0; i < n; i++) out[i] = s->ring[(r + (uint32_t)i) % MH_SOUND_RING];
    s->ring_r = r + (uint32_t)n;
    return n;
}
