/* menu.c - see menu.h. */
#include "platform/menu.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

enum {
    L_GAP = 0,
    L_PAGE_CONTROLS, L_PAGE_ROLLER, L_PAGE_GAME, L_PAGE_CREDIT, L_PAGE_SOUND, L_PAGE_PICTURE,
    L_SELF_TEST, L_RESET_SCORES, L_EXIT, L_BACK,
    L_MENU_HOLD, L_DEFAULTS,
    L_SENS, L_REVERSE, L_KEY_SPEED, L_RSOURCE, L_PPR, L_ONE_TO_ONE, L_TURN_NOTE, L_STICK_SPEED, L_ROLLER_TEST,
    L_LIVES, L_DIFFICULTY, L_BONUS, L_ROLLER_X2, L_RESTART_NOTE,
    L_PRICE,
    L_VOLUME, L_SPEECH, L_ATTRACT, L_MUSIC_TEMPO,
    L_SMOOTH, L_STEADY, L_BOARD_HZ, L_SIZE, L_WIDTH, L_HEIGHT, L_POS_X, L_POS_Y, L_FLIP_X, L_FLIP_Y, L_BRIGHT,
    L_PATTERN, L_PAGE_FX, L_FX_ALL_ON, L_FX_ALL_OFF,
    L_FX0, L_FX_LAST = L_FX0 + MH_FX_COUNT - 1,
    L_FN0,                      /* + MH_FN_xxx */
    L_END = 0xff
};

enum { P_MAIN, P_CONTROLS, P_ROLLER, P_GAME, P_CREDIT, P_SOUND, P_PICTURE, P_FX, N_PAGES };
static const char *const TITLE[N_PAGES] = {
    "MAJOR HAVOC PE ENHANCED  SERVICE MENU", "CONTROLS", "ROLLER", "GAME SETTINGS", "CREDIT SETTINGS",
    "SOUND SETTINGS", "PICTURE", "ENHANCEMENTS",
};

static const char *const FX_LABEL[MH_FX_COUNT] = {
    "MARQUEE SUBTITLE IN THE TITLE", "EACH SCREEN AS LARGE AS IT FITS", "BREAKOUT CLOSE UP",
};

static const char *const FN_LABEL[MH_FN_COUNT] = {
    "JUMP  BUTTON 1", "SHIELD  BUTTON 2", "ROLL LEFT", "ROLL RIGHT", "AUX COIN  SELF TEST NEXT", "SERVICE MENU",
};
static const char *const DIFFICULTY[4] = { "EASY", "MEDIUM", "HARD", "DEMO" };
static const char *const BONUS[4] = { "NONE", "50000", "100000", "200000" };
static const char *const PRICE[4] = { "FREE PLAY", "1 COIN 2 PLAYS", "1 COIN 1 PLAY", "2 COINS 1 PLAY" };

/* --- the pages -------------------------------------------------------------------- */

#define PAGE_MAX 24

static int page_lines(int page, uint8_t *out)
{
    int n = 0;
#define ADD(l) out[n++] = (uint8_t)(l)
    switch (page) {
    case P_CONTROLS:
        for (int f = 0; f < MH_FN_COUNT; f++) ADD(L_FN0 + f);
        ADD(L_MENU_HOLD); ADD(L_GAP); ADD(L_DEFAULTS); ADD(L_BACK);
        break;
    case P_ROLLER:
        ADD(L_RSOURCE); ADD(L_SENS); ADD(L_TURN_NOTE); ADD(L_PPR); ADD(L_ONE_TO_ONE); ADD(L_REVERSE); ADD(L_ROLLER_X2);
        ADD(L_GAP); ADD(L_STICK_SPEED); ADD(L_KEY_SPEED); ADD(L_GAP); ADD(L_ROLLER_TEST); ADD(L_BACK);
        break;
    case P_GAME:
        ADD(L_LIVES); ADD(L_DIFFICULTY); ADD(L_BONUS); ADD(L_ROLLER_X2); ADD(L_GAP); ADD(L_RESTART_NOTE); ADD(L_BACK);
        break;
    case P_CREDIT:
        ADD(L_PRICE); ADD(L_GAP); ADD(L_RESTART_NOTE); ADD(L_BACK);
        break;
    case P_SOUND:
        ADD(L_VOLUME); ADD(L_SPEECH); ADD(L_ATTRACT); ADD(L_MUSIC_TEMPO); ADD(L_GAP); ADD(L_BACK);
        break;
    case P_PICTURE:
        ADD(L_SMOOTH); ADD(L_STEADY); ADD(L_BOARD_HZ); ADD(L_GAP);
        ADD(L_SIZE); ADD(L_WIDTH); ADD(L_HEIGHT); ADD(L_POS_X); ADD(L_POS_Y); ADD(L_FLIP_X); ADD(L_FLIP_Y);
        ADD(L_BRIGHT); ADD(L_GAP); ADD(L_PATTERN); ADD(L_BACK);
        break;
    case P_FX:
        for (int f = 0; f < MH_FX_COUNT; f++) ADD(L_FX0 + f);
        ADD(L_GAP); ADD(L_FX_ALL_ON); ADD(L_FX_ALL_OFF); ADD(L_GAP); ADD(L_BACK);
        break;
    default:
        ADD(L_PAGE_CONTROLS); ADD(L_PAGE_ROLLER); ADD(L_PAGE_GAME); ADD(L_PAGE_CREDIT); ADD(L_PAGE_SOUND);
        ADD(L_PAGE_PICTURE); ADD(L_PAGE_FX); ADD(L_GAP); ADD(L_SELF_TEST); ADD(L_RESET_SCORES); ADD(L_GAP); ADD(L_EXIT);
        break;
    }
#undef ADD
    out[n] = L_END;
    return n;
}

/* The line chosen (gaps and notes cannot be chosen). */
static int selectable(int line) { return line != L_GAP && line != L_RESTART_NOTE && line != L_TURN_NOTE; }

#define ORIGINAL_COUNTS 250.0f  /* the cabinet's roller: counts a turn (render/config.c) */

static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

static int chosen_line(const uint8_t *lines, int item, int *count)
{
    int k = 0, line = L_GAP;
    for (int i = 0; lines[i] != L_END; i++) {
        if (!selectable(lines[i])) continue;
        if (k == item) line = lines[i];
        k++;
    }
    *count = k;
    return line;
}

static int step(int v, int d, int lo, int hi)
{
    v += d;
    return v > hi ? lo : v < lo ? hi : v;
}

static float stepf(float v, int d, float by, float lo, float hi)
{
    v += (float)d * by;
    return v < lo ? lo : v > hi ? hi : v;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

int mh_menu_on_picture(const mh_menu *u)
{
    return u->open && u->page == P_PICTURE;
}

void mh_menu_open(mh_menu *u, const mh_options *opt, int self_test)
{
    u->open = 1;
    u->page = P_MAIN;
    u->confirm = 0;
    u->dir_x = u->dir_y = 0;
    u->self_test = self_test;
    u->opt = *opt;
    u->orig = *opt;
    u->opt_changed = 0;
    u->rcap = u->radding = u->rep_n = 0;
    u->armed_x = u->armed_y = 0;
    memset(&u->cap, 0, sizeof u->cap);
}

/* A function's row: laying its controls out (vectorpie/vp_controls.h). */
static int function_row(mh_menu *u, vp_input *in, int fn, int fire, int left, int right, int del)
{
    const char *name = MH_FN_NAME[fn];
    if (fire) {
        vp_capture_begin(&u->cap, in, name);
    } else if (del) {
        vp_capture_standard(&u->cap, in, name);
        vp_input_save(in, NULL);
    } else if (left || right) {
        vp_capture_toggle(&u->cap, in, name);
        vp_input_save(in, NULL);
    }
    return MH_MENU_NOTHING;
}

int mh_menu_update(mh_menu *u, mh_config *cfg, mh_controls *ctl, vp_input *in, double now)
{
    if (u->page < 0 || u->page >= N_PAGES) u->page = P_MAIN;

    if (u->rcap) {
        char spec[48];
        int r = mh_roller_capture_poll(ctl, in, spec, sizeof spec);
        if (r == 0) return MH_MENU_NOTHING;
        u->rcap = 0;
        u->dir_x = ctl->nav.x;
        u->dir_y = ctl->nav.y;
        u->repeat_at = now + 0.6;
        if (r < 0) return MH_MENU_NOTHING;
        /* the first since the cursor came to the row replaces, every further one adds */
        if (!u->radding || !strcmp(cfg->roller, "none")) {
            snprintf(cfg->roller, sizeof cfg->roller, "%s", spec);
        } else if (!strstr(cfg->roller, spec)) {
            int n = 1;
            for (const char *q = cfg->roller; *q; q++) n += *q == '+';
            if (n < MH_ROLLER_SOURCES && strlen(cfg->roller) + strlen(spec) + 2 < sizeof cfg->roller) {
                strcat(cfg->roller, "+");
                strcat(cfg->roller, spec);
            }
        }
        u->radding = 1;
        return MH_MENU_CHANGED;
    }
    if (u->cap.capturing) {
        if (vp_capture_step(&u->cap, in) == VP_CAPTURE_CHANGED) vp_input_save(in, NULL);
        u->dir_x = ctl->nav.x;
        u->dir_y = ctl->nav.y;
        u->repeat_at = now + 0.6;
        return MH_MENU_NOTHING;
    }

    uint8_t lines[PAGE_MAX + 1];
    int n;
    page_lines(u->page, lines);
    int line = chosen_line(lines, u->item[u->page], &n);
    if (u->item[u->page] >= n) {
        u->item[u->page] = 0;
        line = chosen_line(lines, 0, &n);
    }

    if (ctl->nav.back) {
        vp_capture_moved(&u->cap);
        if (u->page == P_MAIN) return MH_MENU_CLOSE;
        u->page = P_MAIN;
        return MH_MENU_NOTHING;
    }

    /* a push counts once, and again and again while it is held */
    /* a direction held counts only once it was let go after the menu
     * opened (a stick or pedal resting off its middle would scroll on) */
    if (!ctl->nav.x) u->armed_x = 1;
    if (!ctl->nav.y) u->armed_y = 1;
    int x = u->armed_x ? ctl->nav.x : 0, y = u->armed_y ? ctl->nav.y : 0, dx = 0, dy = 0;
    if (ctl->nav.px || ctl->nav.py) {
        dx = ctl->nav.px;
        dy = ctl->nav.py;
        u->repeat_at = now + 0.45;
        u->rep_n = 0;
    } else if (x != u->dir_x || y != u->dir_y) {
        dx = x != u->dir_x ? x : 0;
        dy = y != u->dir_y ? y : 0;
        u->repeat_at = now + 0.45;
        u->rep_n = 0;
    } else if ((x || y) && now >= u->repeat_at) {
        dx = x;
        dy = y;
        u->repeat_at = now + 0.08;
        u->rep_n++;
    }
    /* held longer, the steps grow (the roller's values go far) */
    int mult = u->rep_n < 8 ? 1 : u->rep_n < 24 ? 5 : 25;
    u->dir_x = x;
    u->dir_y = y;
    dy += ctl->nav.spin;
    int fire = ctl->nav.fire;

    if (dy) {
        u->item[u->page] = clampi(u->item[u->page] + dy, 0, n - 1);
        u->confirm = 0;
        vp_capture_moved(&u->cap);      /* the next control laid out replaces again */
        u->radding = 0;
        return MH_MENU_NOTHING;
    }

    if (line == L_RSOURCE) {
        if (fire) { mh_roller_capture_begin(ctl, in); u->rcap = 1; return MH_MENU_NOTHING; }
        if (ctl->nav.del) { snprintf(cfg->roller, sizeof cfg->roller, "dial"); u->radding = 0; return MH_MENU_CHANGED; }
        if (dx) {
            snprintf(cfg->roller, sizeof cfg->roller, "%s", mh_roller_preset(cfg->roller, dx));
            u->radding = 0;
            return MH_MENU_CHANGED;
        }
        return MH_MENU_NOTHING;
    }

    if (line >= L_FX0 && line <= L_FX_LAST) {
        if (!dx && !fire) return MH_MENU_NOTHING;
        cfg->fx[line - L_FX0] = !cfg->fx[line - L_FX0];
        return MH_MENU_CHANGED;
    }
    if (line >= L_FN0) {
        int left = dx < 0, right = dx > 0;
        vp_capture_filter(&u->cap, &left, &right);
        return function_row(u, in, line - L_FN0, fire, left, right, ctl->nav.del);
    }

    int d = dx ? dx : (fire ? 1 : 0);
    if (!d) return MH_MENU_NOTHING;
    mh_options *o = &u->opt;
    switch (line) {
    case L_PAGE_CONTROLS: if (fire) { u->page = P_CONTROLS; vp_capture_moved(&u->cap); } break;
    case L_PAGE_ROLLER:   if (fire) u->page = P_ROLLER; break;
    case L_PAGE_GAME:     if (fire) u->page = P_GAME; break;
    case L_PAGE_CREDIT:   if (fire) u->page = P_CREDIT; break;
    case L_PAGE_SOUND:    if (fire) u->page = P_SOUND; break;
    case L_PAGE_PICTURE:  if (fire) u->page = P_PICTURE; break;
    case L_PAGE_FX:       if (fire) u->page = P_FX; break;
    case L_FX_ALL_ON:
    case L_FX_ALL_OFF:
        if (!fire) break;
        for (int f = 0; f < MH_FX_COUNT; f++) cfg->fx[f] = line == L_FX_ALL_ON;
        return MH_MENU_CHANGED;
    case L_BACK:          if (fire) u->page = P_MAIN; break;
    case L_EXIT:          if (fire) return MH_MENU_CLOSE; break;
    case L_SELF_TEST:     if (fire) return MH_MENU_SELF_TEST; break;
    case L_RESET_SCORES:
        if (!fire) break;
        if (!u->confirm) { u->confirm = 1; break; }
        u->confirm = 0;
        return MH_MENU_RESET_SCORES;
    case L_MENU_HOLD:     cfg->menu_hold = !cfg->menu_hold; return MH_MENU_CHANGED;
    case L_DEFAULTS:
        if (!fire) break;
        for (int f = 0; f < MH_FN_COUNT; f++) vp_input_set(in, MH_FN_NAME[f], "standard");
        vp_input_save(in, NULL);
        break;
    case L_SENS:          cfg->spin_sens = clampf(floorf(cfg->spin_sens + 0.5f) + (float)(d * mult), 1.0f, 999.0f);
                          return MH_MENU_CHANGED;
    case L_PPR:           cfg->spin_ppr = clampf(cfg->spin_ppr + (float)(d * mult * 10), 10.0f, 20000.0f); return MH_MENU_CHANGED;
    case L_ONE_TO_ONE:
        if (!fire) break;
        cfg->spin_sens = floorf(ORIGINAL_COUNTS / cfg->spin_ppr * 1000.0f + 0.5f) / 10.0f;
        return MH_MENU_CHANGED;
    case L_STICK_SPEED:   cfg->stick_speed = clampf(cfg->stick_speed + (float)(d * mult * 100), 100.0f, 20000.0f); return MH_MENU_CHANGED;
    case L_ROLLER_TEST:   if (fire) return MH_MENU_ROLLER_TEST; break;
    case L_REVERSE:       cfg->spin_reverse = !cfg->spin_reverse; return MH_MENU_CHANGED;
    case L_KEY_SPEED:     cfg->key_speed = clampf(cfg->key_speed + (float)(d * mult * 50), 50.0f, 20000.0f); return MH_MENU_CHANGED;
    case L_LIVES:         o->lives = step(o->lives, d, 3, 6); u->opt_changed = 1; break;
    case L_DIFFICULTY:    o->difficulty = step(o->difficulty, d, 0, 3); u->opt_changed = 1; break;
    case L_BONUS:         o->bonus = step(o->bonus, d, 0, 3); u->opt_changed = 1; break;
    case L_ROLLER_X2:     o->roller_x2 = !o->roller_x2; u->opt_changed = 1; break;
    case L_PRICE:         o->price = step(o->price, d, 0, 3); u->opt_changed = 1; break;
    case L_ATTRACT:       o->attract_sound = !o->attract_sound; u->opt_changed = 1; break;
    case L_VOLUME:        cfg->volume = stepf(cfg->volume, d, 0.1f, 0.0f, 1.0f); return MH_MENU_CHANGED;
    case L_MUSIC_TEMPO:   cfg->steady_music = !cfg->steady_music; return MH_MENU_CHANGED;
    case L_SPEECH:        cfg->speech = stepf(cfg->speech, d, 0.1f, 0.0f, 2.0f); return MH_MENU_CHANGED;
    case L_SMOOTH:        cfg->smooth = !cfg->smooth; return MH_MENU_CHANGED;
    case L_STEADY:        cfg->steady = !cfg->steady; return MH_MENU_CHANGED;
    case L_BOARD_HZ:      cfg->board_hz = stepf(cfg->board_hz, d, 5.0f, 40.0f, 60.0f); return MH_MENU_CHANGED;
    case L_SIZE:
        cfg->size_x = stepf(cfg->size_x, d, 0.01f, 0.5f, 1.5f);
        cfg->size_y = stepf(cfg->size_y, d, 0.01f, 0.5f, 1.5f);
        return MH_MENU_CHANGED;
    case L_WIDTH:         cfg->size_x = stepf(cfg->size_x, d, 0.01f, 0.5f, 1.5f); return MH_MENU_CHANGED;
    case L_HEIGHT:        cfg->size_y = stepf(cfg->size_y, d, 0.01f, 0.5f, 1.5f); return MH_MENU_CHANGED;
    case L_POS_X:         cfg->pos_x = clampi(cfg->pos_x + d * 16, -1024, 1024); return MH_MENU_CHANGED;
    case L_POS_Y:         cfg->pos_y = clampi(cfg->pos_y + d * 16, -1024, 1024); return MH_MENU_CHANGED;
    case L_FLIP_X:        cfg->flip_x = !cfg->flip_x; return MH_MENU_CHANGED;
    case L_FLIP_Y:        cfg->flip_y = !cfg->flip_y; return MH_MENU_CHANGED;
    case L_BRIGHT:        cfg->brightness = stepf(cfg->brightness, d, 0.05f, 0.3f, 2.0f); return MH_MENU_CHANGED;
    case L_PATTERN:       if (fire) return MH_MENU_PATTERN; break;
    }
    if (u->opt_changed) {
        const mh_options *a = &u->opt, *b = &u->orig;
        u->opt_changed = a->lives != b->lives || a->difficulty != b->difficulty || a->bonus != b->bonus ||
                         a->roller_x2 != b->roller_x2 || a->attract_sound != b->attract_sound || a->price != b->price;
    }
    return MH_MENU_NOTHING;
}

/* --- drawing ------------------------------------------------------------------------ */

static void describe(int line, const mh_menu *u, const mh_config *cfg, vp_input *in, const char **label, char *value,
                     size_t size)
{
    const mh_options *o = &u->opt;
    value[0] = '\0';
    if (line >= L_FN0) {
        int fn = line - L_FN0;
        *label = FN_LABEL[fn];
        if (u->cap.capturing && !strcmp(u->cap.action, MH_FN_NAME[fn])) {
            snprintf(value, size, "PRESS THE CONTROL");
        } else {
            char buf[256];
            vp_input_spelled(in, MH_FN_NAME[fn], vp_input_controls(in, MH_FN_NAME[fn]), buf, sizeof buf);
            size_t k = 0;
            for (const char *p = buf; *p && k + 1 < size; p++) value[k++] = (char)toupper((unsigned char)*p);
            value[k] = '\0';
            if (k > 44) strcpy(value + 41, "...");
        }
        return;
    }
    if (line >= L_FX0 && line <= L_FX_LAST) {
        *label = FX_LABEL[line - L_FX0];
        snprintf(value, size, "%s", cfg->fx[line - L_FX0] ? "ON" : "OFF");
        return;
    }
    switch (line) {
    case L_PAGE_CONTROLS: *label = "CONTROLS"; break;
    case L_PAGE_FX:       *label = "ENHANCEMENTS"; break;
    case L_FX_ALL_ON:     *label = "ALL ON"; break;
    case L_FX_ALL_OFF:    *label = "ALL OFF  AS THE ORIGINAL"; break;
    case L_PAGE_ROLLER:   *label = "ROLLER"; break;
    case L_PAGE_GAME:     *label = "GAME SETTINGS"; break;
    case L_PAGE_CREDIT:   *label = "CREDIT SETTINGS"; break;
    case L_PAGE_SOUND:    *label = "SOUND SETTINGS"; break;
    case L_PAGE_PICTURE:  *label = "PICTURE"; break;
    case L_SELF_TEST:     *label = "GAME SELF TEST"; snprintf(value, size, "%s", u->self_test ? "ON" : "OFF"); break;
    case L_RESET_SCORES:  *label = "RESET HIGH SCORES"; snprintf(value, size, "%s", u->confirm ? "START AGAIN" : "AND STATISTICS"); break;
    case L_EXIT:          *label = "SAVE AND EXIT"; break;
    case L_BACK:          *label = "BACK"; break;
    case L_MENU_HOLD:     *label = "START 1 AND JUMP HELD"; snprintf(value, size, "%s", cfg->menu_hold ? "OPENS MENU" : "OFF"); break;
    case L_DEFAULTS:      *label = "ALL BACK TO STANDARD"; break;
    case L_SENS:          *label = "SENSITIVITY"; snprintf(value, size, "%.1f %%", cfg->spin_sens); break;
    case L_RSOURCE:
        *label = "ROLLER";
        if (u->rcap) snprintf(value, size, "MOVE THE CONTROL");
        else mh_roller_spelled(cfg->roller, value, (int)size);
        break;
    case L_TURN_NOTE:
        snprintf(value, size, "ONE SPINNER TURN  %.2f TURNS OF THE ORIGINAL ROLLER",
                 cfg->spin_sens / 100.0f * cfg->spin_ppr / ORIGINAL_COUNTS);
        *label = value;
        break;
    case L_PPR:           *label = "SPINNER COUNTS A TURN"; snprintf(value, size, "%.0f", cfg->spin_ppr); break;
    case L_ONE_TO_ONE:    *label = "SET ONE TURN AS THE ORIGINAL"; break;
    case L_STICK_SPEED:   *label = "STICK SPEED"; snprintf(value, size, "%.0f COUNTS A SECOND", cfg->stick_speed); break;
    case L_ROLLER_TEST:   *label = "ROLLER TEST"; break;
    case L_REVERSE:       *label = "DIRECTION"; snprintf(value, size, "%s", cfg->spin_reverse ? "REVERSED" : "NORMAL"); break;
    case L_KEY_SPEED:     *label = "DIGITAL ROLLING SPEED"; snprintf(value, size, "%.0f COUNTS A SECOND", cfg->key_speed); break;
    case L_LIVES:         *label = "LIVES"; snprintf(value, size, "%d", o->lives); break;
    case L_DIFFICULTY:    *label = "DIFFICULTY"; snprintf(value, size, "%s", DIFFICULTY[o->difficulty & 3]); break;
    case L_BONUS:         *label = "BONUS LIFE"; snprintf(value, size, "%s", BONUS[o->bonus & 3]); break;
    case L_ROLLER_X2:     *label = "ROLLER MULTIPLIER"; snprintf(value, size, "%s", o->roller_x2 ? "X2" : "X1"); break;
    case L_RESTART_NOTE:  *label = u->opt_changed ? "CHANGED  THE GAME STARTS AGAIN ON EXIT" : "A CHANGE STARTS THE GAME AGAIN"; break;
    case L_PRICE:         *label = "PRICE"; snprintf(value, size, "%s", PRICE[o->price & 3]); break;
    case L_VOLUME:        *label = "VOLUME"; snprintf(value, size, "%d", (int)(cfg->volume * 10.0f + 0.5f)); break;
    case L_SPEECH:        *label = "SPEECH"; snprintf(value, size, "%d", (int)(cfg->speech * 10.0f + 0.5f)); break;
    case L_MUSIC_TEMPO:   *label = "MUSIC TEMPO"; snprintf(value, size, "%s", cfg->steady_music ? "STEADY" : "AS THE CABINET"); break;
    case L_ATTRACT:       *label = "SOUNDS IN THE ATTRACT MODE"; snprintf(value, size, "%s", o->attract_sound ? "ON" : "OFF"); break;
    case L_SMOOTH:        *label = "SMOOTH MOTION"; snprintf(value, size, "%s", cfg->smooth ? "ON" : "OFF"); break;
    case L_STEADY:        *label = "GAME FRAMES"; snprintf(value, size, "%s", cfg->steady ? "STEADY" : "AS THE CABINET"); break;
    case L_BOARD_HZ:      *label = "BOARD RATE"; snprintf(value, size, "%d PICTURES A SECOND", (int)(cfg->board_hz + 0.5f)); break;
    case L_SIZE:          *label = "SIZE"; snprintf(value, size, "%d", (int)(cfg->size_x * 100.0f + 0.5f)); break;
    case L_WIDTH:         *label = "WIDTH"; snprintf(value, size, "%d", (int)(cfg->size_x * 100.0f + 0.5f)); break;
    case L_HEIGHT:        *label = "HEIGHT"; snprintf(value, size, "%d", (int)(cfg->size_y * 100.0f + 0.5f)); break;
    case L_POS_X:         *label = "POSITION LEFT RIGHT"; snprintf(value, size, "%d", cfg->pos_x); break;
    case L_POS_Y:         *label = "POSITION DOWN UP"; snprintf(value, size, "%d", cfg->pos_y); break;
    case L_FLIP_X:        *label = "MIRROR LEFT RIGHT"; snprintf(value, size, "%s", cfg->flip_x ? "ON" : "OFF"); break;
    case L_FLIP_Y:        *label = "MIRROR UP DOWN"; snprintf(value, size, "%s", cfg->flip_y ? "ON" : "OFF"); break;
    case L_BRIGHT:        *label = "BRIGHTNESS"; snprintf(value, size, "%d", (int)(cfg->brightness * 100.0f + 0.5f)); break;
    case L_PATTERN:       *label = "TEST PATTERN"; break;
    default:              *label = "?"; break;
    }
}

void mh_menu_text(vp_dvg *d, const char *s, float y, float h, int highlight)
{
    vp_text_font(VP_FONT_ASTEROIDS);
    if (highlight) vp_color(d, 255, 220, 60);
    else vp_color(d, 150, 160, 255);
    vp_text(d, s, 2048, y, h, VP_TEXT_CENTER);
}

void mh_menu_draw(mh_menu *u, const mh_config *cfg, vp_input *in, vp_dvg *d)
{
    int page = u->page >= 0 && u->page < N_PAGES ? u->page : P_MAIN;
    const float STEP = 170.0f, GAP = 90.0f, H = 78.0f;
    const float LEFT = 520.0f, RIGHT = 3600.0f;
    uint8_t lines[PAGE_MAX + 1];
    page_lines(page, lines);

    vp_text_font(VP_FONT_ASTEROIDS);
    vp_color(d, 255, 220, 60);
    vp_text(d, TITLE[page], 2048, 3740, 95, VP_TEXT_CENTER);

    float height = 0.0f;
    for (int i = 0; lines[i] != L_END; i++) height += lines[i] ? STEP : GAP;
    float y = 2048.0f + height * 0.5f - STEP * 0.5f + 60.0f;
    int k = 0, chosen_id = L_GAP;
    for (int i = 0; lines[i] != L_END; i++) {
        int line = lines[i];
        if (!line) { y -= GAP; continue; }
        const char *label;
        char value[300];
        describe(line, u, cfg, in, &label, value, sizeof value);
        if (line == L_TURN_NOTE) {
            char note[160];
            snprintf(note, sizeof note, "%s", label);
            vp_color(d, 110, 130, 255);
            vp_text(d, note, LEFT + 80.0f, y + 20.0f, H * 0.62f, VP_TEXT_LEFT);
            y -= STEP * 0.7f;
            continue;
        }
        if (line == L_RESTART_NOTE) {
            if (u->opt_changed) vp_color(d, 255, 120, 60);
            else vp_color(d, 110, 130, 255);
            vp_text(d, label, LEFT, y, H * 0.75f, VP_TEXT_LEFT);
            y -= STEP;
            continue;
        }
        int chosen = k == u->item[page];
        if (chosen) {
            chosen_id = line;
            vp_color(d, 255, 255, 255);
            vp_moveto(d, (int)LEFT - 160, (int)y + 70);
            vp_lineto(d, (int)LEFT - 70, (int)y + 38);
            vp_lineto(d, (int)LEFT - 160, (int)y + 6);
        } else {
            vp_color(d, 60, 200, 90);
        }
        vp_text(d, label, LEFT, y, H, VP_TEXT_LEFT);
        if (value[0]) {
            float room = RIGHT - LEFT - vp_text_width(label, H) - 150.0f, w = vp_text_width(value, H), h = H;
            if (w > room && w > 0.0f) h = H * room / w;
            if (chosen) vp_color(d, 255, 255, 255);
            else vp_color(d, 80, 220, 255);
            if (u->cap.capturing && chosen) vp_color(d, 255, 60, 50);
            vp_text(d, value, RIGHT, y, h, VP_TEXT_RIGHT);
        }
        y -= STEP;
        k++;
    }
    const char *said = vp_capture_said(&u->cap);
    if (said) {
        vp_color(d, 255, 120, 60);
        vp_text(d, said, 2048, 430, 55, VP_TEXT_CENTER);
    }
    vp_color(d, 110, 130, 255);
    const char *one = "ROLLER OR UP DOWN CHOOSE   LEFT RIGHT CHANGE", *two = "START 1 OR ENTER DO   ESC BACK   F2 OR TAB SAVE";
    if (u->rcap) {
        one = "MOVE THE SPINNER, TRACKBALL, MOUSE OR STICK";
        two = "START 1, ENTER OR ESC - LEAVE IT";
    } else if (chosen_id == L_RSOURCE) {
        one = "START, ENTER - MOVE A CONTROL  (AGAIN: ONE MORE)";
        two = "LEFT RIGHT - CHOOSE   DEL - SPINNER";
    } else if (u->cap.capturing || chosen_id >= L_FN0) {
        const char *a = NULL, *b = NULL;
        vp_capture_help(&u->cap, &a, &b);
        if (a) one = a;
        if (b) two = b;
    }
    vp_text(d, one, 2048, 300, 50, VP_TEXT_CENTER);
    vp_text(d, two, 2048, 200, 50, VP_TEXT_CENTER);
}
