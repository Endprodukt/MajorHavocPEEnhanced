#include "render/config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mh_config_defaults(mh_config *c)
{
    memset(c, 0, sizeof *c);
    c->smooth = 1;
    c->steady = 1;
    c->steady_music = 1;
    c->size_x = c->size_y = 1.0f;
    c->brightness = 1.0f;
    c->gamma = 0.8f;
    c->board_hz = 50.0f;
    /* what a new EEPROM takes (core/options.h): 3 lives, medium, bonus at
     * 100000, attract sounds, roller x1; 1 coin 1 play */
    c->dsw1 = 0x00;
    c->dsw2 = 0xff;
    /* The roller: what the game needs. It reads the LETA every second
     * interrupt (203 a second), adds half of each change up and halves the
     * sum each picture (tw_main.asm, tw_motion.asm rgdr); Rex's speed on
     * the ground goes to twice the sum: full speed ($60) from about 12
     * counts a read, about 2400 counts a second (measured: 407 interrupts,
     * 51 pictures a second; the ship from about 3200).
     * The cabinet's roller: a 3.25" roller drives a 0.625" one on a 24
     * tooth encoder wheel, 5.2 x 24 = about 125 pulses a turn. The LETA, a
     * quadrature decoder, counts each 2 or 4 times; at 2 (250 a turn) Rex's
     * full speed took 10 turns of the big roller a second, so the cabinet
     * counted 4 times: about 500 a turn, full speed from 5 turns a second,
     * a swipe of the hand on its 10" round.
     * A 1200 ppr spinner gives 1200 a turn: 42 % makes a turn of it a turn
     * of the roller (the menu's ONE TURN AS THE ORIGINAL; the owner had
     * found 41 % by feel on the cabinet).
     * A stick all the way: 2400 counts a second (Rex's full speed);
     * rolling digitally: 1600, about two thirds of it. */
    snprintf(c->roller, sizeof c->roller, "dial");
    c->spin_sens = 42.0f;
    c->spin_ppr = 1200.0f;
    c->stick_speed = 2400.0f;
    c->key_speed = 1600.0f;
    c->menu_hold = 1;
    c->volume = 1.0f;
    c->speech = 1.0f;
    for (int i = 0; i < MH_FX_COUNT; i++) c->fx[i] = 1;
    c->fx[MH_FX_BREAKOUT] = 0;     /* (the whole tactical display stays in view) */
}

static const char *const FX_NAME[MH_FX_COUNT] = { "subtitle", "fit", "breakout" };

const char *mh_config_fx_name(int fx)
{
    return fx >= 0 && fx < MH_FX_COUNT ? FX_NAME[fx] : "";
}

static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
    return s;
}

/* The settings' version: 2 since 0.2.9 (the roller's standard 42 %). */
#define SETTINGS_VERSION 2

static int set(mh_config *c, const char *key, const char *val, int *version)
{
    if (!strcmp(key, "settings_version")) { *version = atoi(val); return 0; }
    struct { const char *k; float *f; } floats[] = {
        { "size_x", &c->size_x }, { "size_y", &c->size_y }, { "brightness", &c->brightness },
        { "gamma", &c->gamma }, { "board_hz", &c->board_hz }, { "spin_sens", &c->spin_sens },
        { "key_speed", &c->key_speed }, { "volume", &c->volume }, { "speech", &c->speech },
        { "spin_ppr", &c->spin_ppr }, { "stick_speed", &c->stick_speed },
    };
    struct { const char *k; int *i; } ints[] = {
        { "smooth", &c->smooth }, { "steady", &c->steady }, { "steady_music", &c->steady_music },
        { "pos_x", &c->pos_x }, { "pos_y", &c->pos_y },
        { "flip_x", &c->flip_x }, { "flip_y", &c->flip_y },
        { "spin_reverse", &c->spin_reverse }, { "menu_hold", &c->menu_hold },
        { "language", &c->language }, { "tournament", &c->tournament },
    };
    for (unsigned i = 0; i < sizeof floats / sizeof floats[0]; i++)
        if (!strcmp(key, floats[i].k)) { *floats[i].f = (float)atof(val); return 0; }
    for (unsigned i = 0; i < sizeof ints / sizeof ints[0]; i++)
        if (!strcmp(key, ints[i].k)) { *ints[i].i = atoi(val); return 0; }
    if (!strcmp(key, "dsw1")) { c->dsw1 = (uint8_t)strtol(val, NULL, 16); return 0; }
    if (!strcmp(key, "dsw2")) { c->dsw2 = (uint8_t)strtol(val, NULL, 16); return 0; }
    if (!strcmp(key, "rom_path")) { snprintf(c->rom_path, sizeof c->rom_path, "%s", val); return 0; }
    if (!strncmp(key, "fx_", 3)) {
        for (int i = 0; i < MH_FX_COUNT; i++)
            if (!strcmp(key + 3, FX_NAME[i])) { c->fx[i] = atoi(val) != 0; return 0; }
        return -1;
    }
    if (!strcmp(key, "roller")) { snprintf(c->roller, sizeof c->roller, "%s", *val ? val : "dial"); return 0; }
    return -1;
}

int mh_config_load(mh_config *c, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char line[1024];
    int version = 1;
    while (fgets(line, sizeof line, f)) {
        char *p = trim(line);
        if (!*p || *p == ';' || *p == '#') continue;
        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim(p), *val = trim(eq + 1);
        if (set(c, key, val, &version) != 0) fprintf(stderr, "%s: setting not understood: %s = %s\n", path, key, val);
    }
    fclose(f);
    /* (0.1/0.2.0 wrote the keys' speed in sixteens of a count) */
    if (c->key_speed > 0.0f && c->key_speed < 50.0f) c->key_speed *= 16.0f;
    /* (up to 0.2.8 the roller's standard was 21 %, the LETA reckoned to
     * count twice; a file of then holding it takes the new standard, an
     * owner's own value stays) */
    if (version < 2 && c->spin_sens == 21.0f) c->spin_sens = 42.0f;
    return 0;
}

int mh_config_save(const mh_config *c, const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "; Major Havoc PE Enhanced: the owner's settings (see the README)\n");
    fprintf(f, "settings_version = %d\n", SETTINGS_VERSION);
    fprintf(f, "rom_path = %s\n", c->rom_path);
    fprintf(f, "smooth = %d\nsteady = %d\n", c->smooth, c->steady);
    fprintf(f, "size_x = %.3f\nsize_y = %.3f\npos_x = %d\npos_y = %d\nflip_x = %d\nflip_y = %d\n",
            c->size_x, c->size_y, c->pos_x, c->pos_y, c->flip_x, c->flip_y);
    fprintf(f, "brightness = %.2f\ngamma = %.2f\nboard_hz = %.0f\n", c->brightness, c->gamma, c->board_hz);
    fprintf(f, "dsw1 = %02X\ndsw2 = %02X\n", c->dsw1, c->dsw2);
    fprintf(f, "roller = %s\nspin_sens = %.1f\nspin_ppr = %.0f\nspin_reverse = %d\nstick_speed = %.0f\n"
               "key_speed = %.0f\nmenu_hold = %d\n",
            c->roller, c->spin_sens, c->spin_ppr, c->spin_reverse, c->stick_speed, c->key_speed, c->menu_hold);
    fprintf(f, "volume = %.2f\nspeech = %.2f\nsteady_music = %d\n", c->volume, c->speech, c->steady_music);
    for (int i = 0; i < MH_FX_COUNT; i++) fprintf(f, "fx_%s = %d\n", FX_NAME[i], c->fx[i]);
    fprintf(f, "language = %d\ntournament = %d\n", c->language, c->tournament);
    fclose(f);
    return 0;
}
