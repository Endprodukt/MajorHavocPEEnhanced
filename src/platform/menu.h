/* menu.h - Major Havoc PE Enhanced's service menu (as Tempest Enhanced's).
 *
 * F2 or Tab opens it (or a control laid out for it, or START 1 and jump held two
 * seconds); the game stands still meanwhile. Pages:
 *
 *   CONTROLS          the game's functions (platform/controls.h), laid out
 *                     the SDK's way (vp_capture): START 1 or Enter on a
 *                     row, then the control; the first since the cursor
 *                     came to the row replaces, every further one adds.
 *                     Left / right: none or the standard. Delete: the
 *                     standard. Kept in save/controls.ini. Then the
 *                     roller (ROLLER): what turns it (START, then move the spinner, trackball,
 *                     mouse or stick; again: one more; left / right the
 *                     usual ones), sensitivity (MAME's percent, shown as
 *                     turns of the original roller), the spinner's counts a
 *                     turn, one turn as the original's, direction, the
 *                     game's roller x2, a stick's and the keys' speed, a
 *                     live test
 *   GAME SETTINGS     language (English, German, French), edition (The
 *                     Promised End, the Tournament Edition: other mazes),
 *                     lives, difficulty, bonus life, roller x2; then the
 *                     enhancements (ENHANCEMENTS, render/enhance.h)
 *   CREDIT SETTINGS   the price
 *   SOUND SETTINGS    volume, speech, sounds in the attract mode, music tempo
 *   PICTURE           smooth motion, steady frames, the board's rate, size,
 *                     place, mirroring, brightness, a test pattern
 *   A long page is set closer to fit the screen.
 *   the game's own self test, the high scores and statistics reset
 *
 * The game's own settings (game, credit, attract sounds) are in its EEPROM
 * (core/options.h): changed, they go there when the menu is left, and the
 * game starts again to take them.
 *
 *   up / down or the spinner   choose a line
 *   left / right               change its value
 *   START 1 or Enter           change it, open a page, or do what it says
 *   Esc                        back
 *   F2 or Tab                  save the settings and go back to the game
 *
 * The menu's own controls, never what is laid out (the SDK's menu
 * controls, mh_nav).
 */
#ifndef MH_MENU_H
#define MH_MENU_H

#include "core/options.h"
#include "platform/controls.h"
#include "render/config.h"

#include <vectorpie/vp.h>
#include <vectorpie/vp_controls.h>

enum {
    MH_MENU_NOTHING = 0,
    MH_MENU_CHANGED,        /* a setting of mh_config changed: take it over */
    MH_MENU_RESET_SCORES,   /* the high scores and statistics back to the factory's */
    MH_MENU_SELF_TEST,      /* the game's self test switch turned over (the game starts again) */
    MH_MENU_PATTERN,        /* the picture's test pattern */
    MH_MENU_ROLLER_TEST,    /* the roller's test screen */
    MH_MENU_CLOSE,          /* save and leave */
};

typedef struct mh_menu {
    int    open;
    int    page;
    int    item[8];         /* the line chosen on each page */
    int    confirm;         /* fire once more resets the high scores */
    int    dir_x, dir_y;
    double repeat_at;
    int    self_test;       /* the self test switch is on (shown) */
    mh_options opt;         /* the game's own settings as edited */
    mh_options orig;        /* as they were when the menu opened */
    int    opt_changed;     /* they differ from the game's: into the EEPROM on leaving */
    int    build_language, build_tournament;    /* the build running */
    int    build_have[6];   /* each build's ROMs are there (tournament * 3 + language) */
    vp_capture cap;         /* laying out a function's controls */
    int    rcap;            /* laying out the roller's source */
    int    radding;         /* a source was laid out since the cursor came to the row: the next adds */
    int    rep_n;           /* repeats of the held direction (the steps grow) */
    int    armed_x, armed_y;    /* the direction was at rest once since the menu opened */
} mh_menu;

/* opt: the game's settings now (core/options.h); the build running
 * (mh_config.language, tournament: changed, the game starts again). */
void mh_menu_open(mh_menu *u, const mh_options *opt, int self_test, int build_language, int build_tournament);
/* Once a picture while open, after mh_controls_read. */
int  mh_menu_update(mh_menu *u, mh_config *cfg, mh_controls *ctl, vp_input *in, double now);
void mh_menu_draw(mh_menu *u, const mh_config *cfg, vp_input *in, vp_dvg *d);

/* 1 while the PICTURE page is open: the game's picture goes behind it,
 * so that every change shows at once. */
int  mh_menu_on_picture(const mh_menu *u);

/* A line of text in the menu's look, for the screens it opens. */
void mh_menu_text(vp_dvg *d, const char *s, float y, float h, int highlight);

#endif
