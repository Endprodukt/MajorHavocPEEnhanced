/* layout.h - where the game's picture goes on the screen.
 *
 * Major Havoc's monitor is horizontal, as the cabinet's: the generator's
 * picture (avg/avg.h: centre 0,0, x right, y up) as MAME shows it, x
 * -150..150 and y -130..130 (MAME's mhavoc; HBMAME's 580 x 500 is the
 * Alpha One prototype's) over the whole 4:3 screen, to the board's
 * 0..4095 square. Size, place and mirroring (mh_config) apply.
 */
#ifndef MH_LAYOUT_H
#define MH_LAYOUT_H

#include "avg/avg.h"
#include "render/config.h"

#define MH_FIELD_X 150.0f       /* half the picture's width, generator units (MAME's visible area, 300 x 260) */
#define MH_FIELD_Y 130.0f       /* half its height */

/* The lines in board units (0..4095, y up), written to out (room for
 * count; out may be lines). Returns how many. */
int  mh_layout(const mh_config *cfg, const mh_line *lines, int count, mh_line *out);

/* A point of the picture (generator units) in board units. */
void mh_layout_point(const mh_config *cfg, float x, float y, int *bx, int *by);

/* A point of the screen (u across -2/3..2/3, v down -1/2..1/2, a unit as
 * tall as the screen) in board units. */
void mh_layout_screen(const mh_config *cfg, float u, float v, int *bx, int *by);

#endif
