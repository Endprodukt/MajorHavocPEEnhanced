#include "render/layout.h"

#define HALF 2048.0f

static void place(const mh_config *cfg, float x, float y, float *bx, float *by)
{
    if (cfg->flip_x) x = -x;
    if (cfg->flip_y) y = -y;
    *bx = HALF + x / MH_FIELD_X * HALF * cfg->size_x + (float)cfg->pos_x;
    *by = HALF + y / MH_FIELD_Y * HALF * cfg->size_y + (float)cfg->pos_y;
}

int mh_layout(const mh_config *cfg, const mh_line *lines, int count, mh_line *out)
{
    for (int i = 0; i < count; i++) {
        mh_line l = lines[i];
        place(cfg, l.x0, l.y0, &l.x0, &l.y0);
        place(cfg, l.x1, l.y1, &l.x1, &l.y1);
        out[i] = l;
    }
    return count;
}

void mh_layout_point(const mh_config *cfg, float x, float y, int *bx, int *by)
{
    float fx, fy;
    place(cfg, x, y, &fx, &fy);
    *bx = (int)(fx + 0.5f);
    *by = (int)(fy + 0.5f);
}

void mh_layout_screen(const mh_config *cfg, float u, float v, int *bx, int *by)
{
    /* the screen is 4:3: u's 2/3 is the board's half width */
    mh_layout_point(cfg, u * 1.5f * MH_FIELD_X, -v * 2.0f * MH_FIELD_Y, bx, by);
}
