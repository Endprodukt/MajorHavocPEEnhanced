#include "render/smooth.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OBJS    2048
#define MAX_GROUPS  512
#define MAX_CANDS   65536
#define FAR_JUMP    40.0f       /* generator units (the picture is 300 x 260): farther apart is a jump, not a move */
#define OTHER_SHAPE 4.0f        /* a partner of another shape (a sprite's next step) only if none of its own is as near */

void mh_smooth_init(mh_smooth *s)
{
    memset(s, 0, sizeof *s);
}

void mh_smooth_free(mh_smooth *s)
{
    mh_avg_list_free(&s->cur);
    mh_avg_list_free(&s->next);
    free(s->pair);
    free(s->taken);
    memset(s, 0, sizeof *s);
}

static void copy_list(mh_avg_list *dst, const mh_avg_list *src)
{
    if (mh_avg_list_reserve(dst, src->count > 0 ? src->count : 1) != 0) {
        dst->count = 0;
        return;
    }
    if (src->count) memcpy(dst->lines, src->lines, (size_t)src->count * sizeof *src->lines);
    dst->count = src->count;
    dst->status = src->status;
    dst->instructions = src->instructions;
}

typedef struct obj { int first, n; uint16_t key, grp, item; uint8_t used; float x, y; } obj;
typedef struct group { int first, n; uint32_t hash; uint8_t used; } group;

static int joins(const mh_line *p, const mh_line *q)
{
    return fabsf(p->x1 - q->x0) < 0.01f && fabsf(p->y1 - q->y0) < 0.01f;
}

/* The picture's objects, and its groups of them. */
static int objects(const mh_avg_list *l, obj *o, int max, group *g, int maxg, int *ng)
{
    int n = 0;
    *ng = 0;
    for (int i = 0; i < l->count; i++) {
        const mh_line *q = &l->lines[i];
        if (n && o[n - 1].grp == q->grp) {
            if (o[n - 1].item == q->item) { o[n - 1].n++; continue; }
            if (!q->ikey && !o[n - 1].key && joins(&l->lines[i - 1], q)) {
                o[n - 1].n++;
                o[n - 1].item = q->item;
                continue;
            }
        }
        if (n == max) break;
        o[n].first = i;
        o[n].n = 1;
        o[n].key = q->ikey;
        o[n].grp = q->grp;
        o[n].item = q->item;
        o[n].used = 0;
        n++;
    }
    for (int k = 0; k < n; k++) {
        float sx = 0.0f, sy = 0.0f;
        for (int j = 0; j < o[k].n; j++) {
            const mh_line *q = &l->lines[o[k].first + j];
            sx += q->x0 + q->x1;
            sy += q->y0 + q->y1;
        }
        o[k].x = sx / (float)(2 * o[k].n);
        o[k].y = sy / (float)(2 * o[k].n);
        if (k == 0 || o[k].grp != o[k - 1].grp) {
            if (*ng == maxg) continue;
            g[*ng].first = k;
            g[*ng].n = 0;
            g[*ng].hash = 2166136261u;
            g[*ng].used = 0;
            (*ng)++;
        }
        if (*ng && g[*ng - 1].first + g[*ng - 1].n == k) {
            group *gg = &g[*ng - 1];
            gg->n++;
            gg->hash = (gg->hash ^ o[k].key) * 16777619u;
            gg->hash = (gg->hash ^ (uint32_t)o[k].n) * 16777619u;
        }
    }
    return n;
}

static void pair_two(mh_smooth *s, obj *a, obj *b)
{
    for (int k = 0; k < a->n; k++) {
        s->pair[a->first + k] = b->first + k;
        s->taken[b->first + k] = 1;
    }
    a->used = b->used = 1;
}

typedef struct cand { float d; int i, j; } cand;

static int by_distance(const void *p, const void *q)
{
    float a = ((const cand *)p)->d, b = ((const cand *)q)->d;
    return a < b ? -1 : a > b;
}

void mh_smooth_set(mh_smooth *s, const mh_avg_list *cur, uint64_t t, const mh_avg_list *next, uint32_t span)
{
    copy_list(&s->cur, cur);
    s->t_cur = t;
    s->span = 0;
    if (!next || !span || !next->count) return;
    copy_list(&s->next, next);
    s->span = span;

    if (s->cap_pair < s->cur.count) {
        free(s->pair);
        s->cap_pair = s->cur.count + 256;
        s->pair = (int *)malloc((size_t)s->cap_pair * sizeof *s->pair);
    }
    if (s->cap_taken < s->next.count) {
        free(s->taken);
        s->cap_taken = s->next.count + 256;
        s->taken = (uint8_t *)malloc((size_t)s->cap_taken);
    }
    if (!s->pair || !s->taken) { s->span = 0; s->cap_pair = s->cap_taken = 0; return; }
    for (int i = 0; i < s->cur.count; i++) s->pair[i] = -1;
    memset(s->taken, 0, (size_t)s->next.count);

    static obj oa[MAX_OBJS], ob[MAX_OBJS];
    static group ga[MAX_GROUPS], gb[MAX_GROUPS];
    int nga, ngb;
    int na = objects(&s->cur, oa, MAX_OBJS, ga, MAX_GROUPS, &nga);
    int nb = objects(&s->next, ob, MAX_OBJS, gb, MAX_GROUPS, &ngb);

    /* groups the same in both: the k-th with the k-th of that kind */
    for (int i = 0; i < nga; i++) {
        for (int j = 0; j < ngb; j++) {
            if (gb[j].used || gb[j].hash != ga[i].hash || gb[j].n != ga[i].n) continue;
            gb[j].used = ga[i].used = 1;
            for (int k = 0; k < ga[i].n; k++) pair_two(s, &oa[ga[i].first + k], &ob[gb[j].first + k]);
            break;
        }
    }

    /* the rest: each object with the nearest of the same size */
    static cand c[MAX_CANDS];
    int nc = 0;
    for (int i = 0; i < na && nc < MAX_CANDS; i++) {
        if (oa[i].used) continue;
        for (int j = 0; j < nb && nc < MAX_CANDS; j++) {
            if (ob[j].used || ob[j].n != oa[i].n) continue;
            float d = hypotf(ob[j].x - oa[i].x, ob[j].y - oa[i].y);
            if (d > FAR_JUMP) continue;
            if (ob[j].key != oa[i].key) d += OTHER_SHAPE;
            c[nc].d = d; c[nc].i = i; c[nc].j = j;
            nc++;
        }
    }
    qsort(c, (size_t)nc, sizeof *c, by_distance);
    for (int k = 0; k < nc; k++) {
        if (oa[c[k].i].used || ob[c[k].j].used) continue;
        pair_two(s, &oa[c[k].i], &ob[c[k].j]);
    }

    /* partners too far apart: a jump */
    for (int k = 0; k < s->cur.count; k++) {
        int p = s->pair[k];
        if (p < 0) continue;
        const mh_line *a = &s->cur.lines[k], *b = &s->next.lines[p];
        if (fabsf(a->x0 - b->x0) > FAR_JUMP || fabsf(a->y0 - b->y0) > FAR_JUMP ||
            fabsf(a->x1 - b->x1) > FAR_JUMP || fabsf(a->y1 - b->y1) > FAR_JUMP) {
            s->pair[k] = -1;
            s->taken[p] = 0;
        }
    }
}

void mh_smooth_picture(mh_smooth *s, uint64_t now, mh_avg_list *out)
{
    out->count = 0;
    if (!s->span) {
        copy_list(out, &s->cur);
        return;
    }
    float a = (float)((double)(now - s->t_cur) / (double)s->span);
    if (now < s->t_cur) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    if (mh_avg_list_reserve(out, s->cur.count + s->next.count + 1) != 0) return;
    for (int k = 0; k < s->cur.count; k++) {
        int p = s->pair[k];
        if (p < 0) {
            if (a < 0.5f) out->lines[out->count++] = s->cur.lines[k];
            continue;
        }
        const mh_line *x = &s->cur.lines[k], *y = &s->next.lines[p];
        mh_line l = a < 0.5f ? *x : *y;
        l.x0 = x->x0 + (y->x0 - x->x0) * a; l.y0 = x->y0 + (y->y0 - x->y0) * a;
        l.x1 = x->x1 + (y->x1 - x->x1) * a; l.y1 = x->y1 + (y->y1 - x->y1) * a;
        out->lines[out->count++] = l;
    }
    if (a >= 0.5f)
        for (int k = 0; k < s->next.count; k++)
            if (!s->taken[k]) out->lines[out->count++] = s->next.lines[k];
}
