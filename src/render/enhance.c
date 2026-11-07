#include "render/enhance.h"

#include "render/config.h"
#include "render/layout.h"
#include "render/logo_shape.h"

#include <math.h>
#include <string.h>

/* --- PE 1.01's addresses (the listings in upstream/ROMs/_builds/1.01/_listings) --- */
#define RAM_GAMEST   0x009b     /* D7: a game is played */
#define RAM_FRAME1   0x00a1     /* frame+1: the attract mode's page */
#define RAM_ANIMATE  0x00bd     /* the title: 0, 1 drawn in, 2 subtitle zooming, 3 going up */
#define RAM_PERM2    0x00c7     /* the subtitle's zoom (0..20) */
#define RAM_BRSTAT   0x0854     /* D7: Breakout is played */
#define RAM_TACT     0x085b     /* D7: the tactical display */
#define AVG_HAVOC1   0x1910     /* the title's routines, the generator's addresses */
#define AVG_HAVOC2   0x196c
#define AVG_HAVOC3   0x1a48

/* The story text (auxpgm_0: tw_story.asm ?drawwmsg, ROM page 0 at 2000):
 * it starts each line with jsr vgcntr and a vector to the line's place
 * (vgscalz 2); a line goes when it scrolls above the middle, and the
 * generator's window (tw_main.asm, E418: VGADD_SCALE(ywin_on,1,0) with the
 * beam in the middle) cuts off what is above the middle. Ours, in page 0's
 * free room (3DEB-3FFE, FF), in the attract mode (gamest D7 clear; the end
 * story is played in a game): a dark vector up after each line's CNTR, and
 * the beam moved up as much before the window is set (E418 then calls
 * stpg0 first, which selects page 0, then ours). */
#define STORY_CALL   0x0508     /* in page 0: jsr vgcntr; ldx #vdata; jsr vgvctr */
#define STORY_OURS   0x1e00     /* in page 0: ours (alpha 3E00) */
#define WIN_CALL     0xe418     /* lda #0; ldx #$79; jsr vgadd2; jsr stpg0 */
#define WIN_OURS     0x1e20     /* in page 0: ours (alpha 3E20) */
#define LINES_AT     0x0441     /* in page 0: scormsg's ldx #06, the lines it draws less 1 */
#define STORY_DY     64         /* the vector up, in the list's units (binary scale 2) */
#define VG_CNTR      0xd684
#define VG_ADD2      0xd688

#define HS_TOP       50.0f      /* the high scores: below this the table, */
#define HS_LIFT      26.0f      /* lifted this much */

#define SUB_WIDTH    0.70f      /* the subtitle's lettering: this much of MAJOR HAVOC's width */
#define SUB_GAP      3.0f       /* generator units between HAVOC and the subtitle */
#define EASE_S       0.25       /* the fit eases to a new screen in about this */
#define FIELD_X      MH_FIELD_X
#define FIELD_Y      MH_FIELD_Y

/* The subtitle levelled: the marquee's lettering rises to the right; turned
 * so that PROMISED END's line is level (its points' least squares line),
 * the letters' own slant kept. */
static float sub_pt[MH_SUB_SEGS][2], sub_x0, sub_x1, sub_y0, sub_y1;

static void level_subtitle(void)
{
    double n = 0, sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int p = 0; p < MH_SUB_PATHS; p++) {
        if (MH_SUB_PATH[p].part != 0) continue;
        for (int k = 0; k < MH_SUB_PATH[p].n; k++) {
            const float *a = MH_SUB_PT[MH_SUB_PATH[p].first + k];
            n++; sx += a[0]; sy += a[1]; sxx += (double)a[0] * a[0]; sxy += (double)a[0] * a[1];
        }
    }
    double slope = n > 1 ? (n * sxy - sx * sy) / (n * sxx - sx * sx) : 0.0;
    float c = (float)cos(atan(slope)), s = (float)sin(atan(slope));
    sub_x0 = sub_y0 = 1e9f; sub_x1 = sub_y1 = -1e9f;
    for (int i = 0; i < MH_SUB_SEGS; i++) {
        float x = MH_SUB_PT[i][0], y = MH_SUB_PT[i][1];
        sub_pt[i][0] = x * c + y * s;
        sub_pt[i][1] = -x * s + y * c;
        sub_x0 = fminf(sub_x0, sub_pt[i][0]); sub_x1 = fmaxf(sub_x1, sub_pt[i][0]);
        sub_y0 = fminf(sub_y0, sub_pt[i][1]); sub_y1 = fmaxf(sub_y1, sub_pt[i][1]);
    }
}

void mh_enhance_init(mh_enhance *e, const mh_machine *m)
{
    level_subtitle();
    memset(e, 0, sizeof *e);
    e->zoom = 1.0f;
    /* the title's routines where PE 1.01 has them (6KL: the generator's
     * 1000-1FFF is the alpha's 5000-5FFF) */
    static const uint8_t HAVOC1[4] = { 0xea, 0x1f, 0xf0, 0x00 }, HAVOC3[4] = { 0xdc, 0x1f, 0x1c, 0x1f };
    e->title_ok = !memcmp(m->amem + 0x4000 + AVG_HAVOC1, HAVOC1, 4) && !memcmp(m->amem + 0x4000 + AVG_HAVOC3, HAVOC3, 4);
    static const uint8_t CALL[8] = { 0x20, 0x84, 0xd6, 0xa2, 0x1f, 0x20, 0xc5, 0xd6 };
    static const uint8_t WIN[10] = { 0xa9, 0x00, 0xa2, 0x79, 0x20, 0x88, 0xd6, 0x20, 0x34, 0xd3 };
    e->story_ok = !memcmp(m->paged + STORY_CALL, CALL, 8) && !memcmp(m->amem + WIN_CALL, WIN, 10)
                  && m->paged[LINES_AT - 1] == 0xa2 && m->paged[LINES_AT] == 0x06;
    for (int a = STORY_OURS; a < STORY_OURS + 0x40; a++) e->story_ok &= m->paged[a] == 0xff;
}

void mh_enhance_patch(mh_enhance *e, const int *fx, mh_machine *m)
{
    int want = e->story_ok && fx[MH_FX_FIT] && !m->in.self_test;
    if (want == e->story_on) return;
    uint8_t *call = m->paged + STORY_CALL, *ours = m->paged + STORY_OURS;
    uint8_t *wcall = m->amem + WIN_CALL, *wours = m->paged + WIN_OURS;
    if (want) {
        const int dy = STORY_DY / 2;                    /* (at binary scale 1) */
        const uint8_t win[] = {
            0x24, RAM_GAMEST,                           /* bit gamest */
            0x30, 0x14,                                 /* bmi window (a game) */
            0xa9, 0x00, 0xa2, 0x71,                     /* SCAL binary 1, linear 0 (the window's, without it) */
            0x20, VG_ADD2 & 0xff, VG_ADD2 >> 8,
            0xa9, dy & 0xff, 0xa2, (dy >> 8) & 0x1f,    /* VCTR up, dark */
            0x20, VG_ADD2 & 0xff, VG_ADD2 >> 8,
            0xa9, 0x00, 0xaa,
            0x20, VG_ADD2 & 0xff, VG_ADD2 >> 8,
            0xa9, 0x00, 0xa2, 0x79,                     /* window: the game's */
            0x4c, VG_ADD2 & 0xff, VG_ADD2 >> 8,         /* jmp vgadd2 */
        };
        memcpy(wours, win, sizeof win);
        const uint8_t wc[10] = { 0x20, 0x34, 0xd3, 0x20, 0x20, 0x3e, 0xea, 0xea, 0xea, 0xea };  /* jsr stpg0; jsr 3E20 */
        memcpy(wcall, wc, 10);
        m->paged[LINES_AT] = 0x07;                      /* a line more: the list starts higher */
        const uint8_t code[] = {
            0x20, VG_CNTR & 0xff, VG_CNTR >> 8,         /* jsr vgcntr */
            0x24, RAM_GAMEST,                           /* bit gamest */
            0x30, 0x0d,                                 /* bmi done (a game) */
            0xa9, STORY_DY & 0xff,                      /* lda #<dy */
            0xa2, (STORY_DY >> 8) & 0x1f,               /* ldx #>dy: VCTR's first word, y */
            0x20, VG_ADD2 & 0xff, VG_ADD2 >> 8,         /* jsr vgadd2 */
            0xa9, 0x00,                                 /* lda #0 */
            0xaa,                                       /* tax: the second, dark, x 0 */
            0x4c, VG_ADD2 & 0xff, VG_ADD2 >> 8,         /* jmp vgadd2 */
            0x60,                                       /* done: rts */
        };
        memcpy(ours, code, sizeof code);
        call[1] = 0x00; call[2] = 0x3e;                 /* jsr 3E00 */
    } else {
        call[1] = VG_CNTR & 0xff; call[2] = VG_CNTR >> 8;
        memset(ours, 0xff, 0x20);
        static const uint8_t WIN[10] = { 0xa9, 0x00, 0xa2, 0x79, 0x20, 0x88, 0xd6, 0x20, 0x34, 0xd3 };
        memcpy(wcall, WIN, 10);
        m->paged[LINES_AT] = 0x06;
        memset(wours, 0xff, 0x20);
    }
    e->story_on = want;
}

/* --- SUBTITLE ---------------------------------------------------------------- */

static void subtitle(const mh_machine *m, mh_avg_list *pic)
{
    if (m->amem[RAM_ANIMATE] < 2) return;           /* (the game shows none yet) */
    int g3 = -1;
    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f;
    for (int i = 0; i < pic->count; i++) {
        const mh_line *l = &pic->lines[i];
        if (l->sub == AVG_HAVOC3) g3 = l->grp;
        if (l->sub == AVG_HAVOC1 || l->sub == AVG_HAVOC2) {
            x0 = fminf(x0, fminf(l->x0, l->x1));
            x1 = fmaxf(x1, fmaxf(l->x0, l->x1));
            y0 = fminf(y0, fminf(l->y0, l->y1));
        }
    }
    if (g3 < 0 || x1 < x0) return;
    /* the game's subtitle: the two groups after the red line */
    int n = 0;
    for (int i = 0; i < pic->count; i++) {
        int g = pic->lines[i].grp;
        if (g == g3 + 1 || g == g3 + 2) continue;
        pic->lines[n++] = pic->lines[i];
    }
    pic->count = n;

    /* ours: right under HAVOC, as on the marquee; zooming in as the game's */
    float w = (x1 - x0) * SUB_WIDTH;
    float cx = x1 - sub_x1 * w, cy = (y0 - SUB_GAP) + sub_y0 * w;
    if (m->amem[RAM_ANIMATE] == 2) {
        /* zoomed in as the game's: its middle comes from the screen's
         * middle to its place, its top stays under HAVOC */
        float k = (255.0f - 4.0f * (float)m->amem[RAM_PERM2]) / 127.0f;
        if (k > 1.0f) {
            const float sx = (sub_x0 + sub_x1) * 0.5f;
            const float kmax = 255.0f / 127.0f;
            float t = (k - 1.0f) / (kmax - 1.0f);
            if (t > 1.0f) t = 1.0f;
            float mx = (cx + sx * w) * (1.0f - t);
            /* at most as wide as the screen (nothing cut off) */
            float kfit = 2.0f * FIELD_X * 0.96f / ((sub_x1 - sub_x0) * w);
            if (kfit < 1.0f) kfit = 1.0f;
            w *= 1.0f + (kfit < kmax ? kfit - 1.0f : kmax - 1.0f) * t;
            cx = mx - sx * w;
            cy = (y0 - SUB_GAP) + sub_y0 * w;
        }
    }
    if (mh_avg_list_reserve(pic, pic->count + MH_SUB_SEGS + 1) != 0) return;
    for (int p = 0; p < MH_SUB_PATHS; p++) {
        const mh_sub_path *sp = &MH_SUB_PATH[p];
        int segs = sp->closed ? sp->n : sp->n - 1;
        for (int k = 0; k < segs; k++) {
            const float *a = sub_pt[sp->first + k], *b = sub_pt[sp->first + (k + 1) % sp->n];
            mh_line l;
            memset(&l, 0, sizeof l);
            l.x0 = cx + a[0] * w; l.y0 = cy - a[1] * w;
            l.x1 = cx + b[0] * w; l.y1 = cy - b[1] * w;
            l.intensity = 15;
            l.z = 7;
            l.r = sp->r; l.g = sp->g; l.b = sp->b;
            l.sub = 0xfff0;
            l.grp = (uint16_t)(g3 + 1);
            l.item = (uint16_t)(0x7000 + p);
            l.ikey = (uint16_t)(0xfff0 + sp->part);
            pic->lines[pic->count++] = l;
        }
    }
}

/* --- FIT and BREAKOUT ---------------------------------------------------------- */

typedef struct fbox { float x0, x1, y0, y1; } fbox;

/* What the screen uses (measured over long runs, render/enhance.h);
 * 0: all of the field. */
static int screen_box(const int *fx, const mh_machine *m, fbox *b)
{
    int game = m->amem[RAM_GAMEST] & 0x80, page = m->amem[RAM_FRAME1];
    if (m->amem[RAM_TACT] & 0x80) {
        if (fx[MH_FX_BREAKOUT] && (m->amem[RAM_BRSTAT] & 0x80)) {
            *b = (fbox){ 58.0f, 102.0f, -84.0f, -12.0f };       /* the Breakout box */
            return 1;
        }
        if (!fx[MH_FX_FIT]) return 0;
        *b = (fbox){ -126.0f, 120.0f, -101.0f, 113.0f };        /* the tactical display */
        return 1;
    }
    if (!fx[MH_FX_FIT] || game) return 0;
    if (page == 0x06 || page == 0x07) {
        *b = (fbox){ -74.0f, 96.0f, -100.0f + HS_LIFT, 89.0f };  /* the high scores (-66..88, -92..81; the table lifted), a margin */
        return 1;
    }
    if (page >= 0xe0) {
        *b = (fbox){ -70.0f, 92.0f, -95.0f, 111.0f };           /* info */
        return 1;
    }
    if (page >= 0x08 && page <= 0x1d) {
        *b = (fbox){ -121.0f, 129.0f, -131.0f, 95.0f };         /* the title and the story */
        return 1;
    }
    return 0;
}

/* A line cut to the field; 0 if nothing is left. */
static int clip(float *x0, float *y0, float *x1, float *y1)
{
    float t0 = 0.0f, t1 = 1.0f, dx = *x1 - *x0, dy = *y1 - *y0;
    const float p[4] = { -dx, dx, -dy, dy };
    const float q[4] = { *x0 + FIELD_X, FIELD_X - *x0, *y0 + FIELD_Y, FIELD_Y - *y0 };
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) return 0;
            continue;
        }
        float t = q[i] / p[i];
        if (p[i] < 0.0f) { if (t > t1) return 0; if (t > t0) t0 = t; }
        else             { if (t < t0) return 0; if (t < t1) t1 = t; }
    }
    float ax = *x0, ay = *y0;
    *x0 = ax + dx * t0; *y0 = ay + dy * t0;
    *x1 = ax + dx * t1; *y1 = ay + dy * t1;
    return 1;
}

static void fit(mh_enhance *e, const int *fx, const mh_machine *m, mh_avg_list *pic, double dt)
{
    fbox b;
    float tz = 1.0f, tx = 0.0f, ty = 0.0f;
    if (screen_box(fx, m, &b)) {
        float sx = 2.0f * FIELD_X / (b.x1 - b.x0), sy = 2.0f * FIELD_Y / (b.y1 - b.y0);
        tz = sx < sy ? sx : sy;
        if (tz < 1.0f) tz = 1.0f;
        if (tz > 4.0f) tz = 4.0f;
        tx = (b.x0 + b.x1) * 0.5f;
        ty = (b.y0 + b.y1) * 0.5f;
    }
    if (!e->have) {
        e->zoom = tz; e->cx = tx; e->cy = ty;
        e->have = 1;
    } else {
        float k = (float)(1.0 - exp(-dt / EASE_S));
        e->zoom += (tz - e->zoom) * k;
        e->cx += (tx - e->cx) * k;
        e->cy += (ty - e->cy) * k;
    }
    if (fabsf(e->zoom - 1.0f) < 0.002f && fabsf(e->cx) < 0.05f && fabsf(e->cy) < 0.05f) return;
    int n = 0;
    for (int i = 0; i < pic->count; i++) {
        mh_line l = pic->lines[i];
        l.x0 = (l.x0 - e->cx) * e->zoom; l.y0 = (l.y0 - e->cy) * e->zoom;
        l.x1 = (l.x1 - e->cx) * e->zoom; l.y1 = (l.y1 - e->cy) * e->zoom;
        if (!clip(&l.x0, &l.y0, &l.x1, &l.y1)) continue;
        pic->lines[n++] = l;
    }
    pic->count = n;
}

/* The high score table (attract pages 06, 07): its heading at 68..84,
 * the table from 24 down; the table lifted up to the heading (a row and a
 * half between them instead of three and a half). */
static void lift_scores(const mh_machine *m, mh_avg_list *pic)
{
    int page = m->amem[RAM_FRAME1];
    if ((m->amem[RAM_GAMEST] & 0x80) || (m->amem[RAM_TACT] & 0x80) || (page != 0x06 && page != 0x07)) return;
    for (int i = 0; i < pic->count; i++) {
        mh_line *l = &pic->lines[i];
        if (l->y0 < HS_TOP && l->y1 < HS_TOP) { l->y0 += HS_LIFT; l->y1 += HS_LIFT; }
    }
}

void mh_enhance_apply(mh_enhance *e, const int *fx, const mh_machine *m, mh_avg_list *pic, double dt)
{
    if (fx[MH_FX_SUBTITLE] && e->title_ok) subtitle(m, pic);
    if (fx[MH_FX_FIT]) lift_scores(m, pic);
    fit(e, fx, m, pic, dt);
}
