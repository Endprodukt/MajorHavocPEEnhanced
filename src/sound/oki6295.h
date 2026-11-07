/* oki6295.h - the OKI MSM6295 ADPCM speech chip of The Promised End's
 * daughter card (ROM X1, 256K), on the gamma's 5800/5900.
 *
 * Four voices; a command is the phrase number (with bit 7 set), then the
 * voices to start it on (bits 7-4) and its attenuation (bits 3-0); a byte
 * without bit 7 stops the voices of bits 6-3. Clocked at 1.056 MHz with
 * pin 7 high: 8000 samples a second, 12 bits each.
 *
 * After MAME's okim6295_device and oki_adpcm_state (src/devices/sound/
 * okim6295.cpp, okiadpcm.cpp, BSD-3-Clause, Mirko Buffoni, Aaron Giles).
 */
#ifndef MH_OKI6295_H
#define MH_OKI6295_H

#include <stdint.h>

#define MH_OKI_RATE 8000    /* 1056000 / 132 */

typedef struct mh_oki_voice {
    int      playing;
    uint32_t base, sample, count;
    int      signal, step;
    float    volume;
} mh_oki_voice;

typedef struct mh_oki {
    const uint8_t *rom;     /* 256K, NULL: no speech */
    int      command;       /* -1: none pending */
    mh_oki_voice v[4];
    uint32_t phrases;       /* phrases started (for the tools) */
} mh_oki;

void  mh_oki_init(mh_oki *o, const uint8_t *rom);
void  mh_oki_write(mh_oki *o, uint8_t data);
/* The next sample (at MH_OKI_RATE), -1..1. */
float mh_oki_sample(mh_oki *o);

#endif
