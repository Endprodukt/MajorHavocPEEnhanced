/* enhance.h - Major Havoc PE Enhanced's own touches to the game's picture
 * (as Tempest Enhanced's render/enhance.h), each switched in the service
 * menu (ENHANCEMENTS, mh_config.fx[]). All work on the picture in
 * generator units, after the smoothing, before the layout; the game's
 * routines stay as they are.
 *
 *   SUBTITLE  the attract mode's title: the game's MAJOR HAVOC stays, its
 *             small THE PROMISED END under it becomes the marquee's
 *             PROMISED END: Enhanced (render/logo_shape.h, traced from the
 *             marquee by tools/logo/logotrace.py), turned level, smaller,
 *             right aligned under HAVOC as on
 *             the marquee, zooming in as the game's did and going up with
 *             the logo.
 *   FIT       each screen as large as it can be: the screens that use only
 *             part of the field (measured: the tactical display before a
 *             wave, the story and title pages, the high score and info
 *             pages) are zoomed to fill it; the maze, space and the demos
 *             use all of it and stay. The zoom eases from screen to screen.
 *             The attract mode's story text (the enemy list, the
 *             backstory) is placed higher, so that it scrolls up to just
 *             under the subtitle before it goes (mh_enhance_patch: a few
 *             bytes in the program, not during the self test).
 *   BREAKOUT  while Breakout is played in the tactical display (RAM
 *             brstat D7), its box is zoomed up to the screen's height.
 *
 * The screens are told by the game's RAM (PE 1.01's addresses, from its
 * listings): gamest D7 (a game is played), frame+1 (the attract mode's
 * page), tact D7 (the tactical display), brstat D7, animate, perm1, perm2
 * (the title's phases, place and zoom); the same in all six builds of 1.01
 * (rom/romset.h). The title's routines are checked in the vector ROM, the
 * story's in the program (another build: they stay off).
 */
#ifndef MH_ENHANCE_H
#define MH_ENHANCE_H

#include "avg/avg.h"
#include "core/machine.h"
#include "render/config.h"     /* MH_FX_xxx */

typedef struct mh_enhance {
    int   title_ok;         /* the vector ROM holds PE 1.01's title routines */
    float zoom, cx, cy;     /* the fit now: scale and the field point that goes to the middle */
    int   have;
    int   story_ok;         /* the program holds PE 1.01's story routine and room for ours */
    int   story_on;         /* ours is patched in */
    int   win_call;         /* the window's call in the fixed program (the builds differ) */
    int   ours;             /* in page 0: ours (the builds' free room differs) */
    float story_x0, story_x1;   /* the story's widest line so far (FIT) */
} mh_enhance;

/* Once, with the machine holding its ROMs. */
void mh_enhance_init(mh_enhance *e, const mh_machine *m);

/* The program patched as the switches say (before the machine runs on);
 * nothing patched during the self test (it sums the ROMs). */
void mh_enhance_patch(mh_enhance *e, const int *fx, mh_machine *m);

/* The picture (generator units) changed as the switches say; dt the
 * seconds since the last picture shown. */
void mh_enhance_apply(mh_enhance *e, const int *fx, const mh_machine *m, mh_avg_list *pic, double dt);

#endif
