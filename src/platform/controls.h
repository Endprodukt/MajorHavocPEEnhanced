/* controls.h - the cabinet's controls as the Major Havoc board's inputs,
 * through the SDK's own controls (vectorpie/vp_controls.h).
 *
 * The game's functions, each laid out by the player in the service menu
 * (CONTROLS, vp_capture; kept in save/controls.ini), with its standard
 * the cabinet's own (advmame.rc):
 *
 *   jump          p1_button1   button 1 (also starts a game)
 *   shield        p1_button2   button 2
 *   roll_left     p1_left      rolling with keys or a stick
 *   roll_right    p1_right
 *   aux           F1, 7        the aux coin: the self test's next screen
 *   service_menu  F2, Tab      the service menu (F2 and Tab work whatever is laid out)
 *
 * The roller, all its settings the owner's (service menu, ROLLER): what
 * turns it (mh_config.roller, up to MH_ROLLER_SOURCES of them): a relative
 * control - the cabinet's spinner (p1_dialx, on a PC the mouse), a second
 * spinner (p2_dialx), a trackball's or the mouse's x or y - its counts
 * times the sensitivity (mh_config.spin_sens, MAME's percent); a stick's
 * axis, as fast as it is pushed (mh_config.stick_speed counts a second
 * all the way); reversed if asked; rolling digitally at
 * mh_config.key_speed counts a second. The source is laid out by moving
 * it (mh_roller_capture_*).
 * START 1 / 2 and the coins are the cabinet's (never laid out): both
 * starts are button 1 (Major Havoc has no start buttons), a coin a 100 ms
 * pulse. START 1 and jump held two seconds open the service menu (if
 * mh_config.menu_hold).
 *
 * The menus never go by what is laid out: mh_nav is their own controls
 * (the SDK's menu controls).
 */
#ifndef MH_CONTROLS_H
#define MH_CONTROLS_H

#include "render/config.h"

#include <vectorpie/vp_input.h>

/* The game's functions, by their names in controls.ini. */
enum { MH_FN_JUMP, MH_FN_SHIELD, MH_FN_ROLL_LEFT, MH_FN_ROLL_RIGHT, MH_FN_AUX, MH_FN_MENU, MH_FN_COUNT };
extern const char *const MH_FN_NAME[MH_FN_COUNT];

/* Where the menu's own controls point: x and y -1, 0, 1 (y: up is -1),
 * held; fire (START 1, Enter) and back (Esc) as they go down; the spinner
 * moves y by whole steps. */
typedef struct mh_nav {
    int x, y, fire, back;
    int px, py;             /* went down since the last read: -1, 0, 1 (short taps count too) */
    int spin;
    int del;                /* Delete (also the keypad's) went down */
} mh_nav;

#define MH_ROLLER_SOURCES 4

typedef struct mh_roller_source {
    char    action[24];     /* a relative action of the SDK, or "" for an axis */
    int     joy, axis;      /* the axis (from 0) */
} mh_roller_source;

typedef struct mh_controls {
    float   sens;           /* game counts per relative count (the sign: the direction) */
    float   stick_rate;     /* game counts a second, a stick all the way */
    float   key_rate;       /* game counts a second, rolling digitally */
    int     menu_hold;
    mh_roller_source src[MH_ROLLER_SOURCES];
    int     nsrc;

    /* laying out the roller's source by moving it */
    int     rcap, rcap_armed, rcap_still;
    int     rcap_sum[8];
    float   rcap_rest[8][8];

    /* what the last read found */
    double  spin;           /* game counts this picture */
    double  spin_keys;      /* of them, rolled digitally */
    int     raw;            /* the relative controls' own counts this picture */
    int     jump, shield, start1, start2, aux;
    double  coin_left[2];   /* seconds left of the coin switch's pulse */
    mh_nav  nav;
    int     menu;           /* the service menu's key went down */
    double  nav_spin;       /* spinner counts not yet a menu step */
    int     coin_was[2], menu_was;
    uint32_t hold_since;
} mh_controls;

/* Defines the functions on `in` and loads the player's (save/controls.ini). */
void mh_controls_init(mh_controls *c, const mh_config *cfg, vp_input *in);
/* Take the roller's settings (again after they changed). */
void mh_controls_configure(mh_controls *c, const mh_config *cfg);
/* Once per picture: sample everything; dt the seconds since the last. */
void mh_controls_read(mh_controls *c, vp_input *in, double dt);

/* The roller's source in words ("SPINNER + JOY1 AXIS 2"). */
const char *mh_roller_spelled(const char *spec, char *buf, int size);
/* The next / previous of the usual sources (left / right on the menu's row). */
const char *mh_roller_preset(const char *spec, int dir);
/* Laying out the roller's source: begin, then once a picture after
 * mh_controls_read: 0 waiting, 1 a control moved (its name in spec), -1
 * left as it was (Esc, Enter, START 1). */
void mh_roller_capture_begin(mh_controls *c, vp_input *in);
int  mh_roller_capture_poll(mh_controls *c, vp_input *in, char *spec, int size);

#endif
