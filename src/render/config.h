/* config.h - the settings: the picture (size, place, brightness, smooth
 * motion), the pace, the roller, the sound, the cabinet's option switches.
 *
 * Read from mhavocpe.ini (key = value, ';' or '#' comments): the
 * package's next to the program, then the owner's in save/. The service
 * menu (F2, platform/menu.h) changes them and writes the owner's file. The
 * controls are the SDK's (save/controls.ini, platform/controls.h); the
 * game's own options (lives, difficulty, price ...) are in its EEPROM
 * (core/options.h).
 */
#ifndef MH_CONFIG_H
#define MH_CONFIG_H

#include <stdint.h>

/* The enhancements (render/enhance.h), each 1 on, 0 off. */
enum { MH_FX_SUBTITLE, MH_FX_FIT, MH_FX_BREAKOUT, MH_FX_COUNT };

typedef struct mh_config {
    /* --- picture --- */
    int   smooth;               /* 1: pictures between the game's pictures (render/smooth.h) */
    int   steady;               /* 1: the game's pictures steady, never slowed down (core/emu.h) */
    int   steady_music;         /* 1: the music at its tempo, however busy the gamma (core/machine.h) */
    float size_x, size_y;       /* 1.0: the picture as large as the screen allows */
    int   pos_x, pos_y;         /* board units the picture is moved by */
    int   flip_x, flip_y;
    float brightness, gamma;
    float board_hz;             /* pictures a second sent to the vector board (package/mhavocpe.ini) */

    /* --- the cabinet's option switches, a bit 1 a switch off: the game
     * takes them only into a new EEPROM (core/options.h) --- */
    uint8_t dsw1;               /* 13/14S, through POKEY 0's pots */
    uint8_t dsw2;               /* 8S */

    /* --- the roller (platform/controls.h) --- */
    char  roller[128];          /* what turns it: "dial", "dial2", "trackball_x", "trackball_y", "mouse_x",
                                 * "mouse_y", "joyN_axisM" (from 1), joined by "+"; "none" */
    float spin_sens;            /* percent: game counts per count of a relative control * 100 (MAME's sensitivity) */
    float spin_ppr;             /* the owner's spinner's counts a turn (for the menu's "turns of the original") */
    int   spin_reverse;
    float stick_speed;          /* a stick axis all the way: game counts a second */
    float key_speed;            /* digital rolling: game counts a second */
    int   menu_hold;            /* start 1 and jump held two seconds open the service menu */

    /* --- sound --- */
    float volume;               /* 0..1 */
    float speech;               /* the speech's level against the POKEYs, 0..2 (1: HBMAME's) */
    int   fx[MH_FX_COUNT];      /* the enhancements */
    char  rom_path[512];
} mh_config;

void mh_config_defaults(mh_config *c);
int  mh_config_load(mh_config *c, const char *path);    /* overlays what the file has; -1: no file */
int  mh_config_save(const mh_config *c, const char *path);
const char *mh_config_fx_name(int fx);     /* its setting's name without "fx_" */

#endif
