/* smooth.h - pictures between the game's pictures (from Tempest Enhanced).
 *
 * The game draws about 51 pictures a second (fewer in busy scenes, as the
 * cabinet, unless the frames are steady: core/emu.h); the screen shows 50
 * or 60. The program runs from the ROM, so no picture of an in-between
 * moment can be drawn by the game itself. Instead, when a picture is new,
 * a copy of the machine runs ahead to the next one (render/frames.h); each
 * line of the picture then moves towards the same line of that next one
 * as the time between them passes. The picture is never behind the game.
 *
 * Which line is the same: the game's list is groups (avg/avg.h:
 * mh_line.grp: a CNTR, a move to the place, then what is drawn there),
 * each a row of objects (a shape called, or a vector of the list's own;
 * vectors joined end to start are one figure). A group whose objects are
 * the same (shape and size, in order) in the next picture is paired with
 * it object by object, the k-th such group with the k-th. What is left
 * pairs object by object with the nearest of the same size, of its own
 * shape before another. Then the lines of paired objects by their number.
 * Lines without a partner, and partners too far apart (a jump), change
 * over halfway.
 */
#ifndef MH_SMOOTH_H
#define MH_SMOOTH_H

#include "avg/avg.h"

#include <stdint.h>

typedef struct mh_smooth {
    mh_avg_list cur;        /* the picture shown now (generator units) */
    mh_avg_list next;       /* the one expected next */
    uint64_t    t_cur;      /* the alpha cycle the shown one came */
    uint32_t    span;       /* cycles until the next one is expected; 0: none */
    int        *pair;       /* for each line of cur: its line in next, or -1 */
    uint8_t    *taken;      /* for each line of next: it has a partner */
    int         cap_pair, cap_taken;
} mh_smooth;

void mh_smooth_init(mh_smooth *s);
void mh_smooth_free(mh_smooth *s);

/* A new picture came at cycle t; next: the one expected `span` cycles
 * later (NULL or span 0: none known). Both are copied. */
void mh_smooth_set(mh_smooth *s, const mh_avg_list *cur, uint64_t t, const mh_avg_list *next, uint32_t span);

/* The picture at cycle now, into out (grown as needed). */
void mh_smooth_picture(mh_smooth *s, uint64_t now, mh_avg_list *out);

#endif
