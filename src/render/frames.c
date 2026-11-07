#include "render/frames.h"

#include <stdlib.h>
#include <string.h>

#define AHEAD_MAX_CYCLES 250000     /* alpha cycles; a picture comes every 49000 or so (35 a second at worst) */

void mh_frames_init(mh_frames *f)
{
    memset(f, 0, sizeof *f);
    mh_smooth_init(&f->sm);
}

void mh_frames_free(mh_frames *f)
{
    mh_avg_list_free(&f->pic);
    mh_avg_list_free(&f->ahead);
    mh_smooth_free(&f->sm);
    free(f->copy);
    free(f->copy_emu);
    memset(f, 0, sizeof *f);
}

void mh_frames_render(const mh_machine *m, mh_avg_list *out)
{
    mh_vgmem v;
    mh_machine_snap_mem(m, &v);
    mh_avg_state st = m->snap_state;
    mh_avg_run(&v, m->snap_colram, &st, m->snap_flip_x, m->snap_flip_y, out, 0);
}

int mh_frames_check(mh_frames *f, const mh_machine *m, const mh_emu *e, int smooth)
{
    if (m->vg_starts == f->starts) return 0;
    f->starts = m->vg_starts;
    static mh_avg_list fresh;
    mh_frames_render(m, &fresh);
    if (!fresh.count) return 0;     /* an empty picture: the last stays */
    mh_avg_list keep = f->pic;
    f->pic = fresh;
    fresh = keep;
    f->t = m->snap_at;
    f->pictures++;
    if (!smooth) {
        mh_smooth_set(&f->sm, &f->pic, f->t, NULL, 0);
        return 1;
    }

    /* a copy runs on to the next picture */
    if (!f->copy) {
        f->copy = (mh_machine *)malloc(sizeof *f->copy);
        f->copy_emu = (mh_emu *)malloc(sizeof *f->copy_emu);
        if (!f->copy || !f->copy_emu) {
            free(f->copy); free(f->copy_emu);
            f->copy = NULL; f->copy_emu = NULL;
            mh_smooth_set(&f->sm, &f->pic, f->t, NULL, 0);
            return 1;
        }
    }
    mh_machine *c = f->copy;
    mh_emu *ce = f->copy_emu;
    memcpy(c, m, sizeof *c);
    *ce = *e;
    ce->m = c;
    ce->alpha.user = ce;
    ce->gamma.user = ce;
    c->sound_write = NULL;
    c->vg.trace = NULL;
    uint64_t start = c->cycle;
    uint32_t starts = c->vg_starts;
    while (c->cycle - start < AHEAD_MAX_CYCLES && !mh_emu_jammed(ce)) {
        mh_emu_step(ce);
        if (c->vg_starts != starts) break;
    }
    if (c->vg_starts != starts && c->snap_at > f->t) {
        mh_frames_render(c, &f->ahead);
        if (f->ahead.count) {
            mh_smooth_set(&f->sm, &f->pic, f->t, &f->ahead, (uint32_t)(c->snap_at - f->t));
            return 1;
        }
    }
    mh_smooth_set(&f->sm, &f->pic, f->t, NULL, 0);
    return 1;
}

void mh_frames_picture(mh_frames *f, uint64_t now, int smooth, mh_avg_list *out)
{
    if (!smooth) {
        /* (copy, so that out never aliases the picture kept here) */
        mh_smooth s0;
        memset(&s0, 0, sizeof s0);
        s0.cur = f->pic;
        mh_smooth_picture(&s0, now, out);
        return;
    }
    mh_smooth_picture(&f->sm, now, out);
}
