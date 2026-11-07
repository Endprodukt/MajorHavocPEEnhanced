/* avg.h - Major Havoc's analog vector generator (with colour), as a
 * display-list interpreter.
 *
 * 16-bit words, low byte first. The generator's own addresses: 0000-1FFF
 * are the alpha's 4000-5FFF (vector RAM, 6KL's first 4K), 2000-3FFF the
 * vector ROM page STAT chose (6H, 6JK: four pages of 8K). The list starts
 * at 0000. Opcode in bits 15-13:
 *
 *   000 VCTR  two words: DY (13-bit signed), then Z<<13 | DX (13-bit signed)
 *   001 HALT
 *   010 SVEC  one word: DY/2 in bits 12-8, Z in 7-5, DX/2 in 4-0
 *   011 STAT  bit 12 clear: colour (bits 3-0, an index into colour RAM
 *             1400-140F), intensity (7-4), vector ROM page (9-8), X
 *             inverted (10), sparkle (11)
 *       SCAL  bit 12 set: binary scale in bits 10-8, linear scale in 7-0;
 *             bit 11: the next instruction sets the clip (below)
 *   100 CNTR  beam to the centre
 *   101 JSRL  call word address (bits 12-0), 4-deep stack
 *   110 RTSL
 *   111 JMPL
 *
 * Z (0..7) is the vector's brightness: 0 a move, 1 "as STAT set it",
 * otherwise twice itself. A colour RAM entry (active low): bits 3 and 2
 * red (strong, weak), bit 1 green, bit 0 blue. Sparkle: the vector is
 * drawn in steps whose colour comes from colour RAM 0F-1E by a 7-bit
 * shift register. The clip: after a SCAL with bit 11, the next
 * instruction cuts off everything above the beam's height as it is then
 * (the maze scrolls under the score).
 *
 * Arithmetic and colours after MAME's avg_mhavoc_device (src/devices/
 * video/avgdvg.cpp, BSD-3-Clause, Mathis Rosenhauer): the deltas are
 * normalized as the board does it and the beam is kept in 16.16 fixed
 * point. Positions are the generator's own, centre 0,0, x right, y up;
 * MAME shows x -150..150 and y -130..130.
 *
 * The generator keeps colour, intensity, page, scale and the beam from
 * one picture to the next (only VGRST clears some): mh_avg_state carries
 * them.
 */
#ifndef MH_AVG_H
#define MH_AVG_H

#include <stdint.h>

#include "avg/avgsm.h"

typedef struct mh_line {
    float    x0, y0, x1, y1;
    uint8_t  intensity;     /* 1..15 */
    uint8_t  z;             /* the vector's own 1..7 */
    uint8_t  color;         /* index into colour RAM */
    uint8_t  r, g, b;       /* as MAME shows that entry */
    uint16_t src;           /* address of the list's instruction that drew it (or called the routine that did) */
    uint16_t sub;           /* the routine that instruction called, or 0xffff */
    /* What the line belongs to, for matching it with the same line of the
     * next picture (render/smooth.h). The game's list is groups, each from
     * a CNTR of the list itself (beam to the middle, a move to the place,
     * scale and colour, then what is drawn there); an object (item) is one
     * instruction of the list: a call of a shape (ikey its address) or a
     * vector of its own (ikey 0). */
    uint16_t grp, item, ikey;
} mh_line;

enum mh_avg_status {
    MH_AVG_OK = 0,          /* reached HALT */
    MH_AVG_LIMIT,           /* instruction limit: a runaway or endless list */
};

typedef struct mh_avg_list {
    mh_line *lines;
    int      count;
    int      cap;
    int      instructions;  /* executed by the last run */
    int      status;
} mh_avg_list;

/* What the generator keeps between pictures. */
typedef struct mh_avg_state {
    int32_t  xpos, ypos;    /* 16.16 */
    uint8_t  scale, bin_scale, color, intensity, map, sparkle, xinv;
    uint8_t  lst;           /* 0: the next instruction sets the clip */
    uint8_t  spkl;          /* the sparkle's shift register */
} mh_avg_state;

void mh_avg_list_free(mh_avg_list *l);
int  mh_avg_list_reserve(mh_avg_list *l, int n);

/* VGRST: what it clears. */
void mh_avg_state_reset(mh_avg_state *s);

/* Run the display list from 0000 until HALT (or until it jumps back to
 * 0000) with the colour RAM `colram` (32 entries), starting from and
 * updating *st. Lines are appended to out (cleared first; NULL: only the
 * state is followed). flip_x, flip_y: the board's invert bits.
 * max_instructions <= 0 uses a default. Returns the status. */
int mh_avg_run(const mh_vgmem *mem, const uint8_t *colram, mh_avg_state *st, int flip_x, int flip_y,
               mh_avg_list *out, int max_instructions);

/* How long the board takes for the list until HALT, in cycles of the
 * 12.096 MHz clock (without the state PROM: an estimate). */
uint32_t mh_avg_cycles(const mh_vgmem *mem, uint8_t map);

#endif
