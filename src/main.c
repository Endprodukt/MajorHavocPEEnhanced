/* mhavocpe - Major Havoc - The Promised End (Jess M. Askey) on the board
 * of src/core, in real time.
 *
 * Finds the ROMs (the package brings them: roms/mhavocpe.zip), then runs
 * both original programs on two 6502 cores against the board (alpha 2.5
 * MHz, gamma 1.25 MHz, both faster for steady frames), with the sound of its
 * four POKEYs and the speech, and shows what its vector generator draws on
 * the horizontal screen (render/layout.h), with pictures between the
 * game's pictures (render/smooth.h).
 *
 *   mhavocpe [--roms PATH] [--config INI] [--nosound] [--hz N]
 *            [--dvg [PORT] | --window | --windowed] [--shot FILE.svg]
 *
 *   --shot  at the end, the last picture shown as SVG (and what the game is doing)
 *
 * Settings: mhavocpe.ini next to the program, the owner's in save/; the
 * service menu (F2, platform/menu.h) changes them. Controls:
 * platform/controls.h (save/controls.ini). The EEPROM (high scores, the
 * game's options, statistics) is kept in save/eeprom.bin, the Tournament
 * Edition's in save/eeprom_te.bin (its mazes are others; the languages
 * share one). The build (language, edition, rom/romset.h) is the
 * settings'.
 */
#include "avg/avg.h"
#include "core/emu.h"
#include "core/machine.h"
#include "core/options.h"
#include "platform/app.h"
#include "platform/audio.h"
#include "platform/controls.h"
#include "platform/menu.h"
#include "render/config.h"
#include "render/draw.h"
#include "render/enhance.h"
#include "render/frames.h"
#include "render/layout.h"
#include "render/svg.h"
#include "rom/romset.h"
#include "sound/sound.h"

#include <vectorpie/vp_credits.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATHS 48

/* More than this much real time behind (a stall, a debugger) is dropped
 * rather than caught up with. */
#define MAX_CATCHUP_S 0.25
#define SLICE 2500              /* alpha cycles (1 ms) the game runs between looks at its inputs and pictures */
#define STEADY_SPEED 2          /* the alpha's speed for steady frames (core/emu.h: 2 is enough, measured) */

enum { MODE_GAME, MODE_MENU, MODE_PATTERN, MODE_ROLLER_TEST };

#define ORIGINAL_COUNTS 500.0   /* the cabinet's roller: counts a turn (render/config.c) */
#define FULL_SPEED      2400.0  /* roller counts a second: Rex at full speed (render/config.c) */

static int rom_paths(int argc, char **argv, const mh_config *cfg, const char **paths)
{
    int n = 0;
    /* (a path given as it is, a zip or a folder of files, is the
     * standard build's; another build is looked for by its name in it) */
    int standard = !strcmp(mh_romset_variant(cfg->language, cfg->tournament), "mhavocpe");
    const char *arg = mh_arg(argc, argv, "--roms");
    if (arg) {
        n = mh_romset_add_dir(paths, n, MAX_PATHS, arg);
        if (standard) paths[n++] = arg;
    }
    if (cfg->rom_path[0]) {
        n = mh_romset_add_dir(paths, n, MAX_PATHS, cfg->rom_path);
        if (standard) paths[n++] = cfg->rom_path;
    }
    /* the package's own, next to the program */
    static char pkg[1024];
    vp_asset_path("roms", pkg, sizeof pkg);
    n = mh_romset_add_dir(paths, n, MAX_PATHS, pkg);
    /* The cabinet's own ROM folders: AdvanceMAME's dir_rom. */
    char rcpath[1024];
    vp_rc *rc = NULL;
    if (vp_rc_default_path(rcpath, sizeof rcpath) == VP_OK && vp_rc_load(rcpath, &rc) == VP_OK) {
        const char *dirs = vp_rc_get(rc, "dir_rom");
        if (dirs) {
            static char copy[2048];
            snprintf(copy, sizeof copy, "%s", dirs);
            for (char *p = strtok(copy, ":;"); p && n < MAX_PATHS; p = strtok(NULL, ":;"))
                n = mh_romset_add_dir(paths, n, MAX_PATHS, p);
        }
        vp_rc_free(rc);
    }
    n += mh_romset_default_paths(paths + n, MAX_PATHS - n);
    return n;
}

static int eeprom_load(uint8_t *ee, size_t size, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    int ok = fread(ee, 1, size, f) == size;
    fclose(f);
    return ok ? 0 : -1;
}

static void eeprom_save(mh_machine *m, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "mhavocpe: cannot write %s\n", path);
        return;
    }
    fwrite(m->eeprom, 1, sizeof m->eeprom, f);
    fclose(f);
    m->eeprom_dirty = 0;
}

/* --- the game ---------------------------------------------------------------------- */

static mh_config cfg;
static mh_romset roms;
static mh_machine m;
static mh_emu emu;
static mh_sound snd;
static mh_frames frames;
static mh_enhance enh;
static int have_sound;
static char ee_path[1024], cfg_path[1024];
static int g_argc;
static char **g_argv;
static int rom_language, rom_tournament;    /* the build running */

/* The ROMs of the build the settings name: the standard one first (it
 * brings the speech), then the other build over it. Not found, the
 * standard one runs. Returns -1 if not even that is found. */
static int load_roms(void)
{
    const char *paths[MAX_PATHS];
    static mh_romset alt;
    mh_romset_use("mhavocpe");
    mh_config std = cfg;
    std.language = std.tournament = 0;
    int n = rom_paths(g_argc, g_argv, &std, paths);
    if (mh_romset_load(&roms, paths, n) != 0) return -1;
    rom_language = rom_tournament = 0;
    const char *name = mh_romset_variant(cfg.language, cfg.tournament);
    if (strcmp(name, "mhavocpe")) {
        mh_romset_use(name);
        n = rom_paths(g_argc, g_argv, &cfg, paths);
        if (mh_romset_load(&alt, paths, n) == 0) {
            mh_romset_fill(&alt, &roms);
            roms = alt;
            rom_language = cfg.language;
            rom_tournament = cfg.tournament;
        } else {
            fprintf(stderr, "mhavocpe: %s not found: the English Promised End runs\n", name);
        }
        mh_romset_use("mhavocpe");
    }
    snprintf(ee_path, sizeof ee_path, "%s/%s", vp_save_dir(), rom_tournament ? "eeprom_te.bin" : "eeprom.bin");
    return 0;
}

static void apply_settings(void)
{
    m.in.dsw1 = cfg.dsw1;
    m.in.dsw2 = cfg.dsw2;
    mh_emu_set_speed(&emu, cfg.steady ? STEADY_SPEED : 1);
    m.gamma_irq_steady = (uint8_t)(cfg.steady_music != 0);
    snd.volume = cfg.volume;
    snd.speech = cfg.speech;
}

/* Power on: the machine with the ROMs, the EEPROM as given, the self
 * test switch as asked. The sound hook stays. */
static void power_on(const uint8_t *eeprom, int self_test)
{
    mh_sound_fn sw = m.sound_write;
    void *su = m.sound_user;
    mh_machine_init(&m);
    mh_roms_in r = { roms.r1mn, roms.r1l, roms.r1q, roms.r1np, roms.r6kl, roms.r6h, roms.r6jk, roms.r9s };
    mh_machine_set_rom(&m, &r);
    if (mh_romset_have(&roms, MH_R_6C)) mh_machine_set_avg_prom(&m, roms.prom);
    if (eeprom) memcpy(m.eeprom, eeprom, sizeof m.eeprom);
    m.in.self_test = (uint8_t)(self_test != 0);
    m.sound_write = sw;
    m.sound_user = su;
    mh_emu_init(&emu, &m);
    apply_settings();
    frames.starts = 0;
}

/* The game starts again (its options taken anew, the self test switch as
 * asked), with its EEPROM as it is now. */
static void restart(int self_test)
{
    static uint8_t ee[sizeof m.eeprom];
    memcpy(ee, m.eeprom, sizeof ee);
    int dirty = m.eeprom_dirty;
    power_on(ee, self_test);
    m.eeprom_dirty = dirty;
}

/* Run `cycles`, the roller's turn of this picture spread over them, the
 * coin switches as pulses; new pictures go to the frames. */
static void run_game(double *roll_pos, double roll, double *coin_left, double cycles)
{
    double total = cycles > 1.0 ? cycles : 1.0;
    while (cycles > 0.0 && !mh_emu_jammed(&emu)) {
        double slice = cycles < SLICE ? cycles : SLICE;
        *roll_pos += roll * slice / total;
        /* (the LETA counts down as the roller turns right: MAME reads it
         * with PORT_REVERSE; the game negates it again) */
        m.in.roller[0] = (uint8_t)(-(long)floor(*roll_pos) & 0xff);
        for (int k = 0; k < 2; k++)
            if (coin_left[k] > 0.0) coin_left[k] -= slice / MH_ALPHA_HZ;
        m.in.coin_l = coin_left[0] > 0.0;
        m.in.coin_r = coin_left[1] > 0.0;
        uint64_t c0 = m.cycle, g0 = m.gcycle;
        mh_emu_run(&emu, (uint64_t)slice + 1);
        cycles -= (double)(m.cycle - c0);
        if (have_sound) mh_sound_run(&snd, (int)(m.gcycle - g0));
        mh_frames_check(&frames, &m, &emu, cfg.smooth);
    }
}

/* --- the test pattern ------------------------------------------------------------- */

static void pattern_line(vp_dvg *d, int x0, int y0, int x1, int y1)
{
    vp_moveto(d, x0, y0);
    vp_lineto(d, x1, y1);
}

/* The game's viewport as it lands on the screen: its edge, with marks
 * at its corners and the middle of each side (they show where it runs off
 * the screen), in board units. */
static void draw_viewport(vp_dvg *d, const mh_config *c, uint8_t r, uint8_t g, uint8_t b)
{
    static const float X[4] = { -MH_FIELD_X, MH_FIELD_X, MH_FIELD_X, -MH_FIELD_X };
    static const float Y[4] = { -MH_FIELD_Y, -MH_FIELD_Y, MH_FIELD_Y, MH_FIELD_Y };
    int x[4], y[4];
    vp_color(d, r, g, b);
    for (int i = 0; i < 4; i++) mh_layout_point(c, X[i], Y[i], &x[i], &y[i]);
    for (int i = 0; i < 4; i++) pattern_line(d, x[i], y[i], x[(i + 1) & 3], y[(i + 1) & 3]);
    /* corner marks pointing inwards, 1/10 of the field */
    const float kx = MH_FIELD_X * 0.1f, ky = MH_FIELD_Y * 0.1f;
    for (int i = 0; i < 4; i++) {
        float sx = X[i] < 0 ? 1.0f : -1.0f, sy = Y[i] < 0 ? 1.0f : -1.0f;
        int a, bb, cc, e;
        mh_layout_point(c, X[i] + sx * kx, Y[i] + sy * ky, &a, &bb);
        mh_layout_point(c, X[i] + sx * kx * 2.0f, Y[i] + sy * ky * 2.0f, &cc, &e);
        pattern_line(d, a, bb, cc, e);
    }
    /* the middle of each side */
    int a, bb, cc, e;
    mh_layout_point(c, 0.0f, -MH_FIELD_Y, &a, &bb); mh_layout_point(c, 0.0f, -MH_FIELD_Y + ky, &cc, &e);
    pattern_line(d, a, bb, cc, e);
    mh_layout_point(c, 0.0f, MH_FIELD_Y, &a, &bb); mh_layout_point(c, 0.0f, MH_FIELD_Y - ky, &cc, &e);
    pattern_line(d, a, bb, cc, e);
    mh_layout_point(c, -MH_FIELD_X, 0.0f, &a, &bb); mh_layout_point(c, -MH_FIELD_X + kx, 0.0f, &cc, &e);
    pattern_line(d, a, bb, cc, e);
    mh_layout_point(c, MH_FIELD_X, 0.0f, &a, &bb); mh_layout_point(c, MH_FIELD_X - kx, 0.0f, &cc, &e);
    pattern_line(d, a, bb, cc, e);
}

/* The test pattern: the board's whole field (fixed, dim), the game's
 * viewport over it (as size, place and mirroring put it), its middle, a
 * circle that is round when the proportions are right, every brightness.
 * The arrows move the viewport, START 1 switches to its size (left / right
 * the width, up / down the height); held, the steps grow. */
typedef struct pattern_state {
    int    size_mode;
    int    dir_x, dir_y, armed;
    double repeat_at;
    int    rep_n;
} pattern_state;

static int pattern_adjust(pattern_state *p, const mh_controls *ctl, double now)
{
    if (ctl->nav.fire) { p->size_mode = !p->size_mode; return 0; }
    if (!ctl->nav.x && !ctl->nav.y) p->armed = 1;
    int x = p->armed ? ctl->nav.x : 0, y = p->armed ? ctl->nav.y : 0, dx = ctl->nav.px, dy = ctl->nav.py;
    if (!dx && !dy) {
        if (x != p->dir_x || y != p->dir_y) { dx = x; dy = y; p->repeat_at = now + 0.4; p->rep_n = 0; }
        else if ((x || y) && now >= p->repeat_at) { dx = x; dy = y; p->repeat_at = now + 0.06; p->rep_n++; }
    } else {
        p->repeat_at = now + 0.4;
        p->rep_n = 0;
    }
    p->dir_x = x;
    p->dir_y = y;
    if (!dx && !dy) return 0;
    int mult = p->rep_n < 10 ? 1 : 4;
    if (p->size_mode) {
        cfg.size_x += 0.005f * (float)(dx * mult);
        cfg.size_y -= 0.005f * (float)(dy * mult);      /* (up is -1) */
        if (cfg.size_x < 0.5f) cfg.size_x = 0.5f;
        if (cfg.size_x > 1.5f) cfg.size_x = 1.5f;
        if (cfg.size_y < 0.5f) cfg.size_y = 0.5f;
        if (cfg.size_y > 1.5f) cfg.size_y = 1.5f;
    } else {
        cfg.pos_x += 8 * dx * mult;
        cfg.pos_y -= 8 * dy * mult;
        if (cfg.pos_x < -1024) cfg.pos_x = -1024;
        if (cfg.pos_x > 1024) cfg.pos_x = 1024;
        if (cfg.pos_y < -1024) cfg.pos_y = -1024;
        if (cfg.pos_y > 1024) cfg.pos_y = 1024;
    }
    return 1;
}

static void draw_pattern(vp_dvg *d, const pattern_state *p)
{
    int a, b, c, e;
    /* the board's whole field: what the screen can show at all */
    vp_color(d, 90, 90, 90);
    pattern_line(d, 0, 0, 4095, 0);
    pattern_line(d, 4095, 0, 4095, 4095);
    pattern_line(d, 4095, 4095, 0, 4095);
    pattern_line(d, 0, 4095, 0, 0);
    /* the game's viewport */
    draw_viewport(d, &cfg, 255, 255, 255);
    float k = 0.05f;
    mh_layout_screen(&cfg, -k, 0.0f, &a, &b); mh_layout_screen(&cfg, k, 0.0f, &c, &e);
    pattern_line(d, a, b, c, e);
    mh_layout_screen(&cfg, 0.0f, -k, &a, &b); mh_layout_screen(&cfg, 0.0f, k, &c, &e);
    pattern_line(d, a, b, c, e);
    vp_color(d, 0, 200, 255);
    for (int i = 0; i < 64; i++) {
        float p0 = (float)i / 64.0f * 6.2831853f, p1 = (float)(i + 1) / 64.0f * 6.2831853f;
        mh_layout_screen(&cfg, 0.4f * cosf(p0), 0.4f * sinf(p0), &a, &b);
        mh_layout_screen(&cfg, 0.4f * cosf(p1), 0.4f * sinf(p1), &c, &e);
        pattern_line(d, a, b, c, e);
    }
    /* every brightness the game uses, in its green */
    for (int i = 1; i <= 15; i++) {
        float g = powf((float)i / 15.0f, cfg.gamma) * cfg.brightness;
        if (g > 1.0f) g = 1.0f;
        vp_color(d, 0, (uint8_t)(203.0f * g), 0);
        int bx = 2048 - (14 * 160 + 110) / 2 + (i - 1) * 160;     /* (centred) */
        pattern_line(d, bx, 1300, bx + 110, 1300);
        pattern_line(d, bx, 1330, bx + 110, 1330);
    }
    char line[96];
    mh_menu_text(d, "PICTURE TEST PATTERN", 2900, 90, 1);
    mh_menu_text(d, "WHITE FRAME THE GAME  GREY FRAME THE WHOLE SCREEN", 2750, 50, 0);
    snprintf(line, sizeof line, "POSITION %d %d   SIZE %d %d", cfg.pos_x, cfg.pos_y, (int)(cfg.size_x * 100.0f + 0.5f),
             (int)(cfg.size_y * 100.0f + 0.5f));
    mh_menu_text(d, line, 1050, 60, 1);
    mh_menu_text(d, p->size_mode ? "ARROWS SIZE   START 1 POSITION   ESC BACK" : "ARROWS POSITION   START 1 SIZE   ESC BACK",
                 900, 55, 0);
}

/* --- the roller's test -------------------------------------------------------------- */

typedef struct roller_test {
    double counts;          /* roller counts since START 1 */
    double raw;             /* the relative controls' own counts */
    double speed;           /* roller counts a second, smoothed */
    double repeat_at;
    int    dir, armed;
} roller_test;

/* Turn it and see what the game gets: a dial turning as the original
 * roller would, the counts, the turns, the speed against Rex's full
 * speed. Left / right change the sensitivity at once. */
static int roller_test_run(roller_test *t, mh_controls *ctl, vp_dvg *d, double dt, double now)
{
    int changed = 0, dx = ctl->nav.px ? ctl->nav.px : 0;
    if (!ctl->nav.x) t->armed = 1;      /* (held only once let go: menu.c) */
    int x = t->armed ? ctl->nav.x : 0;
    if (!dx && x && (x != t->dir || now >= t->repeat_at)) dx = x;
    if (dx) {
        t->repeat_at = now + (x == t->dir ? 0.06 : 0.4);
        cfg.spin_sens += (float)dx;
        if (cfg.spin_sens < 1.0f) cfg.spin_sens = 1.0f;
        if (cfg.spin_sens > 999.0f) cfg.spin_sens = 999.0f;
        mh_controls_configure(ctl, &cfg);
        changed = 1;
    }
    t->dir = x;
    if (ctl->nav.fire) t->counts = t->raw = 0.0;
    /* (left and right change the sensitivity here: rolling digitally is left out) */
    double spin = ctl->spin - ctl->spin_keys;
    t->counts += spin;
    t->raw += ctl->raw;
    if (dt > 0.0) t->speed += (spin / dt - t->speed) * (dt / 0.25 < 1.0 ? dt / 0.25 : 1.0);

    /* the dial: one turn of it, one turn of the original roller */
    const float cx = 2048.0f, cy = 2350.0f, r = 620.0f, rx = r * 0.75f;   /* (board units are 4:3 on the screen) */
    vp_color(d, 0, 160, 60);
    for (int i = 0; i < 48; i++) {
        float a0 = (float)i / 48.0f * 6.2831853f, a1 = (float)(i + 1) / 48.0f * 6.2831853f;
        vp_moveto(d, (int)(cx + rx * cosf(a0)), (int)(cy + r * sinf(a0)));
        vp_lineto(d, (int)(cx + rx * cosf(a1)), (int)(cy + r * sinf(a1)));
    }
    float a = (float)(t->counts / ORIGINAL_COUNTS * 6.2831853) + 1.5707963f;
    vp_color(d, 255, 255, 255);
    vp_moveto(d, (int)cx, (int)cy);
    vp_lineto(d, (int)(cx - rx * 0.9f * cosf(a)), (int)(cy + r * 0.9f * sinf(a)));

    char line[160], src[128];
    mh_menu_text(d, "ROLLER TEST", 3800, 90, 1);
    snprintf(line, sizeof line, "ROLLER  %s", mh_roller_spelled(cfg.roller, src, sizeof src));
    mh_menu_text(d, line, 3600, 60, 0);
    snprintf(line, sizeof line, "%.2f TURNS OF THE ORIGINAL ROLLER   %.0f COUNTS", t->counts / ORIGINAL_COUNTS, t->counts);
    mh_menu_text(d, line, 1550, 62, 1);
    snprintf(line, sizeof line, "SPINNER %.2f TURNS  %.0f COUNTS  AT %.0f A TURN", t->raw / cfg.spin_ppr, t->raw, cfg.spin_ppr);
    mh_menu_text(d, line, 1400, 55, 0);
    snprintf(line, sizeof line, "SPEED %.0f COUNTS A SECOND  %.0f %% OF REX'S FULL SPEED", fabs(t->speed),
             fabs(t->speed) / FULL_SPEED * 100.0);
    mh_menu_text(d, line, 1250, 55, 0);
    snprintf(line, sizeof line, "SENSITIVITY %.1f %%  ONE SPINNER TURN %.2f ROLLER TURNS", cfg.spin_sens,
             cfg.spin_sens / 100.0 * cfg.spin_ppr / ORIGINAL_COUNTS);
    mh_menu_text(d, line, 1050, 62, 1);
    mh_menu_text(d, "LEFT RIGHT SENSITIVITY   START 1 FROM 0   ESC BACK", 300, 50, 0);
    return changed;
}

/* --- main ---------------------------------------------------------------------------- */

static mh_line *g_placed;
static int g_placed_cap;

static void show(vp_dvg *d, const mh_config *c, const mh_avg_list *pic)
{
    if (g_placed_cap < pic->count) {
        free(g_placed);
        g_placed_cap = pic->count + 512;
        g_placed = (mh_line *)malloc((size_t)g_placed_cap * sizeof *g_placed);
        if (!g_placed) { g_placed_cap = 0; return; }
    }
    int n = mh_layout(c, pic->lines, pic->count, g_placed);
    mh_draw_lines(d, c, g_placed, n);
}

/* The menu left: the settings saved; the game's own changed, into its
 * EEPROM and the game started again to take them; another build chosen,
 * the game starts with it (its own EEPROM, the menu's options in it). */
static void leave_menu(int *mode, mh_menu *menu, mh_app *app)
{
    mh_config_save(&cfg, cfg_path);
    if (cfg.language != rom_language || cfg.tournament != rom_tournament) {
        if (menu->opt_changed) mh_options_put(&m, &menu->opt);
        eeprom_save(&m, ee_path);
        if (load_roms() == 0) {
            static uint8_t ee[sizeof m.eeprom];
            char buf[256];
            printf("mhavocpe: now %s from %s\n", mh_romset_describe(&roms, buf, sizeof buf), roms.source);
            memset(ee, 0xff, sizeof ee);
            eeprom_load(ee, sizeof ee, ee_path);
            power_on(ee, m.in.self_test);
            mh_options_put(&m, &menu->opt);         /* (the options as the menu shows them) */
            mh_enhance_init(&enh, &m);
        }
        /* (a build not found: the one that ran runs on, the setting as it was) */
        cfg.language = rom_language;
        cfg.tournament = rom_tournament;
        mh_config_save(&cfg, cfg_path);
        menu->opt_changed = 0;
    }
    if (menu->opt_changed) {
        mh_options_put(&m, &menu->opt);
        eeprom_save(&m, ee_path);
        restart(m.in.self_test);
        menu->opt_changed = 0;
    }
    *mode = MODE_GAME;
    menu->open = 0;
    app->hold_cancel = 0;
}

int main(int argc, char **argv)
{
    mh_config_defaults(&cfg);
    char buf[1024];
    const char *arg = mh_arg(argc, argv, "--config");
    if (arg) {
        snprintf(cfg_path, sizeof cfg_path, "%s", arg);
    } else {
        /* the package's defaults, then the owner's own in save/ */
        mh_config_load(&cfg, vp_asset_path("mhavocpe.ini", buf, sizeof buf));
        snprintf(cfg_path, sizeof cfg_path, "%s/mhavocpe.ini", vp_save_dir());
    }
    mh_config_load(&cfg, cfg_path);

    g_argc = argc;
    g_argv = argv;
    if (load_roms() != 0) {
        char missing[512];
        fprintf(stderr, "mhavocpe: ROMs missing: %s (roms/mhavocpe.zip of the package; --roms PATH)%s\n",
                mh_romset_missing(&roms, missing, sizeof missing),
                roms.lpc ? "; a build with LPC speech was found: only ADPCM builds are supported" : "");
        return 1;
    }
    printf("mhavocpe: ROMs from %s: %s%s%s\n", roms.source, mh_romset_describe(&roms, buf, sizeof buf),
           mh_romset_have(&roms, MH_R_X1) ? "" : ", no speech (X1 missing)",
           mh_romset_have(&roms, MH_R_6C) ? "" : ", no 136002-125 (the vector generator's timing is approximate)");

    mh_app app;
    if (mh_app_open(&app, "Major Havoc PE Enhanced", argc, argv, cfg.board_hz) != 0) return 1;

    static mh_controls ctl;
    static mh_menu menu;
    static mh_avg_list pic;
    static uint8_t ee[sizeof m.eeprom];
    static roller_test rtest;
    static pattern_state pattern;

    mh_sound_init(&snd, mh_romset_have(&roms, MH_R_X1) ? roms.x1 : NULL);
    if (!mh_flag(argc, argv, "--nosound")) {
        have_sound = mh_audio_open(&snd) == 0;
        if (have_sound) {
            m.sound_write = mh_sound_write;
            m.sound_user = &snd;
        }
    }
    mh_frames_init(&frames);
    memset(ee, 0xff, sizeof ee);
    eeprom_load(ee, sizeof ee, ee_path);
    power_on(ee, 0);
    mh_enhance_init(&enh, &m);
    mh_controls_init(&ctl, &cfg, app.in);

    int mode = MODE_GAME;
    double roll_pos = 0.0, ee_dirty_since = -1.0, clock = 0.0;

    while (!app.quit) {
        double dt;
        if (!mh_app_poll(&app, &dt)) break;
        if (dt > MAX_CATCHUP_S) dt = MAX_CATCHUP_S;
        clock += dt;
        mh_controls_read(&ctl, app.in, dt);

        if (ctl.menu && !menu.cap.capturing) {
            if (mode == MODE_GAME) {
                mh_options opt;
                mh_options_get(&m, &opt);
                mh_menu_open(&menu, &opt, m.in.self_test, rom_language, rom_tournament);
                mode = MODE_MENU;
                app.hold_cancel = 1;                /* Esc goes back in the menu, it does not end */
            } else {
                leave_menu(&mode, &menu, &app);     /* the menu's key again: save, back to the game */
            }
        }

        mh_app_begin_frame(&app);
        switch (mode) {
        case MODE_GAME:
            /* (Major Havoc starts a game with button 1: the starts are it
             * too; in the self test START 2 is the aux coin, its next screen) */
            m.in.b1[0] = (uint8_t)(ctl.jump || ctl.start1 || (ctl.start2 && !m.in.self_test));
            m.in.b2[0] = (uint8_t)ctl.shield;
            m.in.diag = (uint8_t)(ctl.aux || (ctl.start2 && m.in.self_test));
            mh_enhance_patch(&enh, cfg.fx, &m);
            run_game(&roll_pos, ctl.spin, ctl.coin_left, dt * MH_ALPHA_HZ);
            mh_frames_picture(&frames, m.cycle, cfg.smooth, &pic);
            mh_enhance_apply(&enh, cfg.fx, &m, &pic, dt);
            /* the roller's light (1600 D0) as the START 1 lamp */
            vp_lamps_set((m.outputs & 0x01) ? (unsigned)VP_LAMP_START1 : 0u);
            show(app.dvg, &cfg, &pic);
            break;
        case MODE_MENU:
            switch (mh_menu_update(&menu, &cfg, &ctl, app.in, clock)) {
            case MH_MENU_CHANGED:
                mh_controls_configure(&ctl, &cfg);
                apply_settings();
                if (app.on_board) app.frame_hz = cfg.board_hz;     /* (the picture page's BOARD RATE) */
                break;
            case MH_MENU_RESET_SCORES: {
                /* everything but the options back to new: the game sets
                 * its high scores, initials and statistics up anew */
                uint8_t keep[3];
                memcpy(keep, m.eeprom + MH_EE_OPTIONS, sizeof keep);
                memset(m.eeprom, 0xff, sizeof m.eeprom);
                memcpy(m.eeprom + MH_EE_OPTIONS, keep, sizeof keep);
                m.eeprom_dirty = 1;
                if (!menu.opt_changed) restart(m.in.self_test);    /* (else leaving restarts it) */
                eeprom_save(&m, ee_path);
                leave_menu(&mode, &menu, &app);
                break;
            }
            case MH_MENU_SELF_TEST:
                if (menu.opt_changed) mh_options_put(&m, &menu.opt);
                menu.opt_changed = 0;
                eeprom_save(&m, ee_path);
                restart(!m.in.self_test);
                leave_menu(&mode, &menu, &app);
                break;
            case MH_MENU_PATTERN:
                memset(&pattern, 0, sizeof pattern);
                mode = MODE_PATTERN;
                break;
            case MH_MENU_ROLLER_TEST:
                memset(&rtest, 0, sizeof rtest);
                mode = MODE_ROLLER_TEST;
                break;
            case MH_MENU_CLOSE:
                leave_menu(&mode, &menu, &app);
                break;
            }
            if (mode == MODE_MENU && mh_menu_on_picture(&menu)) {
                /* behind the picture page: the game's picture, half as
                 * bright, laid out as the settings say now */
                mh_config dim = cfg;
                dim.brightness *= 0.5f;
                show(app.dvg, &dim, &pic);
                draw_viewport(app.dvg, &cfg, 120, 120, 120);    /* (where the game's picture can reach) */
            }
            if (mode == MODE_MENU) mh_menu_draw(&menu, &cfg, app.in, app.dvg);
            break;
        case MODE_ROLLER_TEST:
            roller_test_run(&rtest, &ctl, app.dvg, dt, clock);
            if (ctl.nav.back) mode = MODE_MENU;
            break;
        default:
            if (pattern_adjust(&pattern, &ctl, clock)) apply_settings();
            draw_pattern(app.dvg, &pattern);
            if (ctl.nav.back) mode = MODE_MENU;
            break;
        }
        if (mh_app_end_frame(&app) != 0) break;
        if (mh_emu_jammed(&emu)) {
            fprintf(stderr, "mhavocpe: the %s CPU stopped at %04X\n", emu.alpha.jammed ? "alpha" : "gamma",
                    emu.alpha.jammed ? emu.alpha.pc : emu.gamma.pc);
            break;
        }
        if (have_sound) mh_audio_feed(&snd);

        /* the EEPROM: saved a second after the game last changed it */
        if (m.eeprom_dirty) {
            if (ee_dirty_since < 0.0) ee_dirty_since = clock;
            else if (clock - ee_dirty_since > 1.0) {
                eeprom_save(&m, ee_path);
                ee_dirty_since = -1.0;
            }
        }
    }

    if (m.eeprom_dirty) eeprom_save(&m, ee_path);
    const char *shot = mh_arg(argc, argv, "--shot");
    if (shot) {
        if (mh_svg_write(shot, pic.lines, pic.count) != 0) fprintf(stderr, "mhavocpe: cannot write %s\n", shot);
        printf("mhavocpe: %.1f s run, %llu alpha interrupts, %u pictures, %d lines in the last\n",
               (double)m.cycle / MH_ALPHA_HZ, (unsigned long long)emu.alpha_irqs, m.vg_starts, pic.count);
    }
    vp_lamps_set(0);
    if (have_sound) mh_audio_close();
    mh_app_close(&app);
    mh_frames_free(&frames);
    mh_avg_list_free(&pic);
    free(g_placed);
    return 0;
}
