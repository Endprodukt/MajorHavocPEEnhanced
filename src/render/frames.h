/* frames.h - the game's pictures as they come, and the next one ahead
 * (from Tempest Enhanced).
 *
 * The alpha starts the vector generator on a complete list once a frame
 * (VGGO); the machine keeps the list as it was then (core/machine.h,
 * snap_*). An empty picture (the game shows one as a scene changes)
 * leaves the last standing. With smooth motion (render/smooth.h) a copy of
 * the machine then runs on to the next picture, with the controls as they
 * are, and the shown picture moves towards it.
 */
#ifndef MH_FRAMES_H
#define MH_FRAMES_H

#include "core/emu.h"
#include "render/smooth.h"

typedef struct mh_frames {
    uint32_t    starts;         /* the machine's vg_starts of the last picture */
    mh_avg_list pic;            /* the last picture (generator units) */
    mh_avg_list ahead;          /* the next, from the copy */
    uint64_t    t;              /* the cycle it came */
    uint32_t    pictures;       /* pictures so far */
    mh_smooth   sm;
    mh_machine *copy;           /* the machine run ahead (allocated once) */
    mh_emu     *copy_emu;
} mh_frames;

void mh_frames_init(mh_frames *f);
void mh_frames_free(mh_frames *f);

/* After the game ran a while: take a new picture if there is one (1),
 * and if smooth, run ahead to the next. */
int  mh_frames_check(mh_frames *f, const mh_machine *m, const mh_emu *e, int smooth);

/* The picture to show at cycle now. */
void mh_frames_picture(mh_frames *f, uint64_t now, int smooth, mh_avg_list *out);

/* The picture the generator was last started on (generator units). */
void mh_frames_render(const mh_machine *m, mh_avg_list *out);

#endif
