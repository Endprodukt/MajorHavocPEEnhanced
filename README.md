# Major Havoc PE Enhanced

Major Havoc - The Promised End (Jess M. Askey, https://github.com/jessaskey/mhavocpe)
for the VectorPie cabinet (and a window on a PC), on the VectorPie SDK.

The game is the original program: both of the board's programs (alpha: the
game, gamma: sound, switches, roller, EEPROM) run from The Promised End's
ROMs on two 6502 cores against a board written in C (`src/core`). The
vector generator's lists are drawn on the vector board, the four POKEYs and
the OKI 6295 speech go to the speaker. Built from Tempest Enhanced
(https://github.com/Endprodukt/TempestEnhanced): its 6502 core, AVG state machine, POKEY, platform
layer (app, menu) and tools were taken over; the controls are the SDK's own.

The ROMs come with the package (`package/roms/mhavocpe.zip`: release 1.01,
English, ADPCM speech, from `upstream/ROMs/_builds/1.01`); there is no
licence problem with them (the owner's word).

## Controls

Through the VectorPie SDK's own controls: every function can be laid out
in the service menu (CONTROLS), kept in `save/controls.ini`. Standards:

| Function | Standard |
|---|---|
| Roller | the cabinet's spinner (`p1_dialx`; on a PC the mouse), 21 % (a 1200 ppr spinner turn = a turn of the original roller); everything on the ROLLER page |
| Jump (button 1, also starts a game) | `p1_button1` |
| Shield (button 2) | `p1_button2` |
| Roll left / right | `p1_left` / `p1_right` (1600 counts a second) |
| Aux coin (self test: next screen) | F1, 7; in the self test also START 2 |
| Service menu | F2 or Tab always, START 1 and jump held 2 s |

START 1 / 2 and the coins are the cabinet's (both starts are button 1:
Major Havoc has no start buttons).

## Service menu

CONTROLS, ROLLER (what turns it: spinner, second spinner, trackball,
mouse or a stick's axis, laid out by moving it; sensitivity in 1 % steps,
shown as turns of the original roller; the spinner's counts a turn; one
turn as the original; direction; roller x2; stick and key speeds; a live
roller test), GAME SETTINGS
(lives, difficulty, bonus life, roller x2), CREDIT SETTINGS (price), SOUND
SETTINGS (volume, speech, attract sounds), PICTURE (smooth motion, steady
game frames, board rate, size, position, mirroring, brightness, test
pattern), ENHANCEMENTS (below), the game's self test, reset high scores
and statistics. The
game's own settings are kept in its EEPROM; changing them starts the game
again when the menu is left.

## Enhancements

On the service menu's ENHANCEMENTS page, each on or off (all off: as the
original):

- MARQUEE SUBTITLE: the title's MAJOR HAVOC stays the game's; the small
  THE PROMISED END under it becomes the marquee's "PROMISED END:
  Enhanced" on one line, level, under HAVOC, zooming in as the game's did.
- EACH SCREEN AS LARGE AS IT FITS: the tactical display before a wave, the
  story and title pages and the high score and info pages use only part
  of the field; they are zoomed to fill it (x1.15 to x1.60; the high
  score table moved up under its heading; the tactical
  display x1.21, edge to edge, Breakout in it too). The maze, space and the
  demos use all of it and stay. Under the title, the enemy list and the
  backstory scroll up to just under the subtitle (the game cut them off
  at the screen's middle).
- BREAKOUT CLOSE UP (off as standard): while Breakout is played in the
  tactical display (press jump there), its box alone is zoomed up to the
  screen's height (x3.6).

## Smooth motion

The game's pictures are steady (both CPUs run faster than the cabinet's,
so busy scenes keep 51 pictures a second instead of slowing down), and
pictures between them are made by running a copy of the machine ahead
(as Tempest Enhanced). Both can be switched off (PICTURE page).

## Sound

As loud as HBMAME mixes it: four POKEYs through MAME's op-amp stage at a
quarter each, the speech at full; a soft knee above 0.8 instead of a hard
clip. On the cabinet the sound follows the VectorPie menu's volume.

The music keeps its tempo (SOUND SETTINGS, MUSIC TEMPO: STEADY). On the
board the sound CPU's interrupt, which plays the music, counts from when
it was answered; while the sound CPU is busy with the main CPU's commands
it answers late and the music slows down (measured: down to about 72 %
of its tempo at times). Steady, the interrupt comes on its beat whatever,
and one missed is made up. AS THE CABINET keeps the original.

## Files on the cabinet

`/persistent/games/mhpeenhanced` (the menu's group: Atari): `mhavocpe`,
`game.ini`, `mhavocpe.ini`, `run.sh`, `marquee.png`, `roms/mhavocpe.zip`; `save/` holds `mhavocpe.ini` (the owner's
settings), `controls.ini` (the controls) and `eeprom.bin` (high scores, the game's options, statistics).

Building, deploying, packaging: BUILD.md.
