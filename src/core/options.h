/* options.h - The Promised End's own options, in its EEPROM.
 *
 * The gamma keeps two bytes of options at EEPROM 20-21 (eerom = 6020,
 * "options" in g_ram.ah), their checksum at 22: the sum of both, inverted.
 * A new EEPROM (a bad checksum) takes them from the switches (mh_gamma.asm
 * loadop: 13/14S through POKEY 0's ALLPOT, inverted, xor E5; 8S xor 01).
 * The alpha reads them at power on (tw_slave.asm getoptn); its self test
 * changes them (tw_test2.asm savesets). So a change takes hold when the
 * game starts again.
 *
 *   options 0: D0 roller x2, D1 sounds in the attract mode, D2-D3 bonus
 *              life (none, 50000, 100000, 200000), D4-D5 difficulty
 *              (easy, medium, hard, demo), D6-D7 lives (3, 6, 5, 4)
 *   options 1: D0-D1 price (free play, 1 coin 2 plays, 1 coin 1 play,
 *              2 coins 1 play)
 */
#ifndef MH_OPTIONS_H
#define MH_OPTIONS_H

#include <stdint.h>

#include "core/machine.h"

#define MH_EE_OPTIONS 0x20

typedef struct mh_options {
    int lives;          /* 3..6 */
    int difficulty;     /* 0 easy, 1 medium, 2 hard, 3 demo */
    int bonus;          /* 0 none, 1 50000, 2 100000, 3 200000 */
    int roller_x2;
    int attract_sound;
    int price;          /* 0 free play, 1 1 coin 2 plays, 2 1 coin 1 play, 3 2 coins 1 play */
    uint8_t raw1;       /* options 1 as it was (its other bits are kept) */
} mh_options;

/* The options as the game will use them: from the EEPROM, or, if that is
 * new, as the game will take them from the switches. Returns 1 if they
 * came from the EEPROM. */
int  mh_options_get(const mh_machine *m, mh_options *o);
/* Into the EEPROM, with their checksum (the game takes them at its next
 * power on). */
void mh_options_put(mh_machine *m, const mh_options *o);

#endif
