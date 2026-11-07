#include "platform/controls.h"

#include <vectorpie/vp_controls.h>

#include <SDL.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COIN_PULSE_S 0.1        /* the coin switch closed this long */
#define NAV_COUNTS   40.0       /* spinner counts for a step through a menu */
#define DEAD_ZONE    0.15f      /* a stick's axis nearer its middle than this does not roll */

const char *const MH_FN_NAME[MH_FN_COUNT] = { "jump", "shield", "roll_left", "roll_right", "aux", "service_menu" };
static const char *const STANDARD[MH_FN_COUNT] = {
    "p1_button1", "p1_button2", "p1_left", "p1_right", "key_F1+key_7", "key_F2+key_Tab",
};

/* --- the roller's sources --------------------------------------------------------- */

typedef struct rel { const char *name, *action, *spelled; } rel;
static const rel RELATIVE[] = {
    { "dial", "p1_dialx", "SPINNER" },
    { "dial2", "p2_dialx", "SPINNER 2" },
    { "trackball_x", "p1_trackballx", "TRACKBALL X" },
    { "trackball_y", "p1_trackbally", "TRACKBALL Y" },
    { "mouse_x", "p1_mousex", "MOUSE X" },
    { "mouse_y", "p1_mousey", "MOUSE Y" },
};
#define NREL ((int)(sizeof RELATIVE / sizeof RELATIVE[0]))
#define CAP_AXES 8

/* One source by its name; 0 if not known. */
static int parse_one(const char *s, mh_roller_source *o)
{
    memset(o, 0, sizeof *o);
    for (int i = 0; i < NREL; i++)
        if (!strcmp(s, RELATIVE[i].name)) {
            snprintf(o->action, sizeof o->action, "%s", RELATIVE[i].action);
            return 1;
        }
    int j, a;
    if (sscanf(s, "joy%d_axis%d", &j, &a) == 2 && j >= 1 && a >= 1) {
        o->joy = j - 1;
        o->axis = a - 1;
        return 1;
    }
    return 0;
}

static int parse_sources(const char *spec, mh_roller_source *out, int max)
{
    char buf[128];
    snprintf(buf, sizeof buf, "%s", spec);
    int n = 0;
    for (char *p = strtok(buf, "+ "); p && n < max; p = strtok(NULL, "+ "))
        if (parse_one(p, &out[n])) n++;
    return n;
}

const char *mh_roller_spelled(const char *spec, char *buf, int size)
{
    char tmp[128];
    snprintf(tmp, sizeof tmp, "%s", spec);
    int len = 0;
    buf[0] = '\0';
    for (char *p = strtok(tmp, "+ "); p; p = strtok(NULL, "+ ")) {
        char word[48];
        int j, a;
        word[0] = '\0';
        for (int i = 0; i < NREL; i++)
            if (!strcmp(p, RELATIVE[i].name)) snprintf(word, sizeof word, "%s", RELATIVE[i].spelled);
        if (!word[0] && sscanf(p, "joy%d_axis%d", &j, &a) == 2) snprintf(word, sizeof word, "JOY%d AXIS %d", j, a);
        if (!word[0]) continue;
        len += snprintf(buf + len, len < size ? (size_t)(size - len) : 0, "%s%s", len ? " + " : "", word);
        if (len >= size) break;
    }
    if (!buf[0]) snprintf(buf, (size_t)size, "NONE");
    return buf;
}

const char *mh_roller_preset(const char *spec, int dir)
{
    static const char *const PRESET[] = { "dial", "dial2", "trackball_x", "trackball_y", "mouse_x", "mouse_y",
                                          "joy1_axis1", "joy1_axis2", "none" };
    const int n = (int)(sizeof PRESET / sizeof PRESET[0]);
    int at = -1;
    for (int i = 0; i < n; i++)
        if (!strcmp(spec, PRESET[i])) at = i;
    if (at < 0) return PRESET[0];
    return PRESET[(at + (dir < 0 ? n - 1 : 1)) % n];
}

/* --- setting up ---------------------------------------------------------------------- */

void mh_controls_init(mh_controls *c, const mh_config *cfg, vp_input *in)
{
    memset(c, 0, sizeof *c);
    for (int i = 0; i < MH_FN_COUNT; i++) vp_input_define(in, MH_FN_NAME[i], STANDARD[i]);
    vp_input_reserve_key(in, SDL_SCANCODE_TAB);     /* the service menu's, as F2 (never laid out) */
    vp_input_load(in, NULL);
    mh_controls_configure(c, cfg);
}

void mh_controls_configure(mh_controls *c, const mh_config *cfg)
{
    float dir = cfg->spin_reverse ? -1.0f : 1.0f;
    c->sens = cfg->spin_sens / 100.0f * dir;
    c->stick_rate = cfg->stick_speed * dir;
    c->key_rate = cfg->key_speed;
    c->menu_hold = cfg->menu_hold;
    c->nsrc = parse_sources(cfg->roller, c->src, MH_ROLLER_SOURCES);
}

/* --- reading ------------------------------------------------------------------------- */

void mh_controls_read(mh_controls *c, vp_input *in, double dt)
{
    int left = vp_input_held(in, MH_FN_NAME[MH_FN_ROLL_LEFT]), right = vp_input_held(in, MH_FN_NAME[MH_FN_ROLL_RIGHT]);
    if (left && right) left = right = 0;
    int raw = 0;
    double stick = 0.0;
    for (int i = 0; i < c->nsrc; i++) {
        const mh_roller_source *s = &c->src[i];
        if (s->action[0]) {
            raw += vp_input_delta(in, s->action);
        } else {
            float v = vp_input_joy_axis(in, s->joy, s->axis);
            if (fabsf(v) > DEAD_ZONE) stick += (double)((v - (v > 0 ? DEAD_ZONE : -DEAD_ZONE)) / (1.0f - DEAD_ZONE));
        }
    }
    /* (the menus move by the cabinet's spinner, whatever the roller is) */
    int dial = vp_input_delta(in, "p1_dialx");
    c->raw = raw;
    c->spin_keys = (double)(right - left) * c->key_rate * dt;
    c->spin = (double)raw * c->sens + stick * c->stick_rate * dt + c->spin_keys;

    c->jump = vp_input_held(in, MH_FN_NAME[MH_FN_JUMP]);
    c->shield = vp_input_held(in, MH_FN_NAME[MH_FN_SHIELD]);
    c->aux = vp_input_held(in, MH_FN_NAME[MH_FN_AUX]);
    c->start1 = vp_input_held(in, "start1");
    c->start2 = vp_input_held(in, "start2");
    for (int k = 0; k < 2; k++) {
        int coin = vp_input_held(in, k ? "coin2" : "coin1");
        if (coin && !c->coin_was[k] && c->coin_left[k] <= 0.0) c->coin_left[k] = COIN_PULSE_S;
        c->coin_was[k] = coin;
    }

    /* For a menu: the SDK's fixed menu controls, whatever is laid out */
    mh_nav nav;
    memset(&nav, 0, sizeof nav);
    nav.x = vp_input_menu_way_x(in);
    nav.y = vp_input_menu_way_y(in);
    nav.px = vp_input_menu_right_pressed(in) - vp_input_menu_left_pressed(in);
    nav.py = vp_input_menu_down(in) - vp_input_menu_up(in);
    c->nav_spin += (double)dial * (c->sens < 0.0f ? -1.0 : 1.0);
    /* a line a picture at most (a jump of the mouse, as a window gets the
     * focus, is not a turn of the spinner) */
    if (c->nav_spin >= NAV_COUNTS) { nav.spin = 1; c->nav_spin = 0.0; }
    else if (c->nav_spin <= -NAV_COUNTS) { nav.spin = -1; c->nav_spin = 0.0; }
    nav.fire = vp_input_menu_confirm(in);
    nav.back = vp_input_key_pressed(in, SDL_SCANCODE_ESCAPE);
    nav.del = vp_input_key_pressed(in, SDL_SCANCODE_DELETE) || vp_input_key_pressed(in, SDL_SCANCODE_KP_PERIOD);
    c->nav = nav;

    int menu_now = vp_input_held(in, MH_FN_NAME[MH_FN_MENU]);
    int f2 = vp_input_key_pressed(in, SDL_SCANCODE_F2) || vp_input_key_pressed(in, SDL_SCANCODE_TAB);
    /* A cabinet without a key for it: start 1 and jump held together. */
    if (c->menu_hold && c->jump && c->start1) {
        if (!c->hold_since) c->hold_since = SDL_GetTicks() | 1u;
        else if (SDL_GetTicks() - c->hold_since >= 2000u) menu_now = 1;
    } else {
        c->hold_since = 0;
    }
    c->menu = f2 || (menu_now && !c->menu_was);
    c->menu_was = menu_now;
}

/* --- laying out the roller's source by moving it ------------------------------------- */

static void rest(mh_controls *c, vp_input *in)
{
    memset(c->rcap_sum, 0, sizeof c->rcap_sum);
    int joys = vp_input_joystick_count(in);
    for (int j = 0; j < 8; j++)
        for (int a = 0; a < CAP_AXES; a++) c->rcap_rest[j][a] = j < joys ? vp_input_joy_axis(in, j, a) : 0.0f;
}

void mh_roller_capture_begin(mh_controls *c, vp_input *in)
{
    c->rcap = 1;
    c->rcap_armed = 0;
    c->rcap_still = 0;
    rest(c, in);
}

int mh_roller_capture_poll(mh_controls *c, vp_input *in, char *spec, int size)
{
    if (!c->rcap) return -1;
    spec[0] = '\0';
    if (c->rcap_armed && (c->nav.back || c->nav.fire)) {
        c->rcap = 0;
        return -1;
    }
    int moving = 0, found = -1, fj = 0, fa = 0;
    for (int i = 0; i < NREL; i++) {
        int d = vp_input_delta(in, RELATIVE[i].action);
        if (d) moving = 1;
        c->rcap_sum[i] += abs(d);
        if (c->rcap_sum[i] > 60 && found < 0) found = i;
    }
    int joys = vp_input_joystick_count(in);
    for (int j = 0; j < joys && j < 8 && found < 0; j++)
        for (int a = 0; a < CAP_AXES && found < 0; a++) {
            float v = vp_input_joy_axis(in, j, a) - c->rcap_rest[j][a];
            if (fabsf(v) > 0.1f) moving = 1;
            if (fabsf(v) > 0.6f) { found = NREL; fj = j; fa = a; }
        }
    if (!c->rcap_armed) {
        /* what was moving when the row was chosen has to stand still first */
        if (moving) { c->rcap_still = 0; rest(c, in); }
        else if (++c->rcap_still >= 10) { c->rcap_armed = 1; rest(c, in); }
        return 0;
    }
    if (found < 0) return 0;
    if (found < NREL) snprintf(spec, (size_t)size, "%s", RELATIVE[found].name);
    else snprintf(spec, (size_t)size, "joy%d_axis%d", fj + 1, fa + 1);
    c->rcap = 0;
    return 1;
}
