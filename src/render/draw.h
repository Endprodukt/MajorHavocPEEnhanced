/* draw.h - lines in board units (render/layout.h) onto the VectorPie
 * device layer, each in the colour its colour RAM entry gives (as MAME
 * shows it), scaled by its intensity and the picture's brightness.
 */
#ifndef MH_DRAW_H
#define MH_DRAW_H

#include "avg/avg.h"
#include "render/config.h"

#include <vectorpie/vp_dvg.h>

/* Draw the lines in list order, chaining strokes and setting the colour
 * only when it changes. */
void mh_draw_lines(vp_dvg *d, const mh_config *cfg, const mh_line *lines, int count);

#endif
