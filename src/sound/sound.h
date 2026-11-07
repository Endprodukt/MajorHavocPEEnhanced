/* sound.h - Major Havoc's sound: four POKEYs and The Promised End's OKI
 * 6295 speech on one speaker, as loud as HBMAME makes it.
 *
 * The gamma writes the POKEYs' registers (2000-27FF) and the OKI (5800,
 * 5900); the machine's sound_write hook delivers them here (a POKEY's as
 * MH_G_POKEY + chip * 16 + register). HBMAME's driver (mhavocpe_adpcm):
 * each POKEY through an op-amp stage (set_output_opamp 1k, 0.001 uF, 5 V,
 * sound/pokey.h pokey_run_opamp) at 0.25 to the speaker, the speech at 1.0,
 * full scale 1.0 (MAME's -wavwrite clips there; the speech alone reaches
 * it). The output's coupling capacitor takes the DC away (MAME keeps it;
 * it is not heard). Then the owner's volume and speech level (1: as
 * HBMAME), and above 0.8 a soft knee instead of MAME's hard clip.
 *
 * mh_sound_run makes the samples of the gamma time that has passed into a
 * ring the sound output reads (platform/audio.h). Both channels of the
 * output carry the one speaker.
 */
#ifndef MH_SOUND_H
#define MH_SOUND_H

#include <stdint.h>

#include "sound/oki6295.h"
#include "sound/pokey.h"

#define MH_SOUND_RATE 48000
#define MH_SOUND_RING 16384     /* a third of a second */

typedef struct mh_sound {
    pokey    pk[4];
    mh_oki   oki;
    double   tick_frac;         /* POKEY clock ticks owed */
    double   tick_carry;        /* the part of a tick the last sample did not take */
    double   oki_frac;          /* speech samples owed */
    float    oki_last, oki_cur; /* the speech's last two samples (in between: a line) */
    double   dc;                /* the output's coupling capacitor (takes the DC away) */
    float    volume;            /* 0..1 */
    float    speech;            /* the speech's level against the POKEYs (1: HBMAME's) */
    float    peak;              /* the loudest sample made (0..1) */

    float    ring[MH_SOUND_RING];
    volatile uint32_t ring_w, ring_r;
} mh_sound;

/* speech_rom: the 256K of X1, or NULL (no speech). */
void mh_sound_init(mh_sound *s, const uint8_t *speech_rom);
/* The machine's sound_write hook (user: the mh_sound). */
void mh_sound_write(void *user, uint16_t addr, uint8_t value);
/* Make the samples of `gamma_cycles` cycles at 1.25 MHz. */
void mh_sound_run(mh_sound *s, int gamma_cycles);
/* Up to n samples (-1..1, mono); returns how many there were. */
int  mh_sound_read(mh_sound *s, float *out, int n);

#endif
