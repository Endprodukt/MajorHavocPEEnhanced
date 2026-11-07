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

The ROMs do not come with the package: bring your own (see ROMs below).

## ROMs

Major Havoc - The Promised End 1.01 by Jess M. Askey, with ADPCM speech
(builds with LPC speech are not supported). It comes in six builds:
English, German and French, each as The Promised End and as the
Tournament Edition (other mazes). One is enough; the service menu's GAME
SETTINGS (LANGUAGE, EDITION) choose among those that are there and mark
the others ROMS MISSING.

Where they come from: [jessaskey/mhavocpe](https://github.com/jessaskey/mhavocpe),
folder `ROMs/` (one folder per build, as HBMAME names them) or
`ROMs/_builds/1.01/` (the release zips). HBMAME's sets work as they are:
split, merged or non-merged.

Where they go: on the VectorPie cabinet into `/usr/local/share/advance/rom/`
(the cabinet's ROM folder, also on its network share), as zips or as
folders. Also looked in: the game's own `roms/` folder, `rom_path` in
`mhavocpe.ini`, `--roms PATH`, `$MH_ROMS`, AdvanceMAME's `dir_rom`.
Without them the game shows what is missing and where it goes.

| Build | HBMAME set | or the release zip | files |
|---|---|---|---|
| English | `mhavocpe.zip` | `mhavocpe_en_adpcm_101.zip` | `mhpe101.*` |
| German | `mhavocpede.zip` | `mhavocpe_de_adpcm_101.zip` | `mhpede101.*` |
| French | `mhavocpefr.zip` | `mhavocpe_fr_adpcm_101.zip` | `mhpefr101.*` |
| English, Tournament | `mhavocpete.zip` | `mhavocpe_en_tournament_adpcm_101.zip` | `mhpete101.*` |
| German, Tournament | `mhavocpedete.zip` | `mhavocpe_de_tournament_adpcm_101.zip` | `mhpedete101.*` |
| French, Tournament | `mhavocpefrte.zip` | `mhavocpe_fr_tournament_adpcm_101.zip` | `mhpefrte101.*` |

Each build's files (the file names may differ; the ending names the
board position):

| Position | Size | What | Needed |
|---|---|---|---|
| `.1mn` | 16 KB | alpha program | yes |
| `.1l` | 16 KB | alpha program | yes |
| `.1q` | 32 KB | alpha program, paged | yes |
| `.1np` | 32 KB | alpha program, paged | yes |
| `.6kl` | 8 KB | vector ROM | yes |
| `.6h` | 16 KB | vector ROM, paged | yes |
| `.6jk` | 16 KB | vector ROM, paged | yes |
| `.9s` | 16 KB | gamma (sound) program | yes |
| `.x1` | 256 KB | speech (the same in all six) | no: without it, no speech |
| `136002-125.6c` | 256 B | the vector generator's state PROM (Atari's) | no: without it, its timing is approximate |

A clone without speech (an HBMAME split set) takes it from `mhavocpe.zip`.

CRC32 of 1.01 (`x1`: `aade65d1`, `136002-125.6c`: `5903af03` in all six):

| Build | 1mn | 1l | 1q | 1np | 6kl | 6h | 6jk | 9s |
|---|---|---|---|---|---|---|---|---|
| mhavocpe | 8b4b1c7c | 90348169 | d5d86868 | 6f7b38a9 | ddcaab40 | 6b380183 | 7ecfd43b | 46cdd0fa |
| mhavocpede | a3ff05f2 | 214369f7 | ad51ea70 | 7d3c4e41 | 5971ccb7 | a7e9178f | b21ec237 | a3e4cbcb |
| mhavocpefr | 64b59f45 | bc0d539f | 12ef0140 | 14305109 | 0fcd62ef | 29eb2bda | 3c1cfe62 | 57eee0d9 |
| mhavocpete | 71fdffa0 | feba2023 | 428b51fd | 835c85fd | ddcaab40 | 6b380183 | 7ecfd43b | 46cdd0fa |
| mhavocpedete | 5949e62e | daf0e86a | 9401e3be | 9fe25082 | 5971ccb7 | a7e9178f | b21ec237 | a3e4cbcb |
| mhavocpefrte | 9e037c99 | 67eac8ae | 1db409a8 | 620015e9 | 0fcd62ef | 29eb2bda | 3c1cfe62 | 57eee0d9 |

The cabinet's AdvanceMAME brings only a pre-release (`mhavocpex7`), which
this game does not take.

## Controls

Through the VectorPie SDK's own controls: every function can be laid out
in the service menu (CONTROLS), kept in `save/controls.ini`. Standards:

| Function | Standard |
|---|---|
| Roller | the cabinet's spinner (`p1_dialx`; on a PC the mouse), 42 % (a 1200 ppr spinner turn = a turn of the original roller, about 500 counts: the LETA counts each pulse 4 times); everything on the CONTROLS page |
| Jump (button 1, also starts a game) | `p1_button1` |
| Shield (button 2) | `p1_button2` |
| Roll left / right | `p1_left` / `p1_right` (1600 counts a second) |
| Aux coin (self test: next screen) | F1, 7; in the self test also START 2 |
| Service menu | F2 or Tab always, START 1 and jump held 2 s |

START 1 / 2 and the coins are the cabinet's (both starts are button 1:
Major Havoc has no start buttons).

## Service menu

CONTROLS (the buttons, then the roller: what turns it: spinner, second
spinner, trackball, mouse or a stick's axis, laid out by moving it;
sensitivity in 1 % steps, shown as turns of the original roller; the
spinner's counts a turn; one turn as the original; direction; roller x2;
stick and key speeds; a live roller test), GAME SETTINGS (language,
edition, lives, difficulty, bonus life, roller x2, then the enhancements,
below), CREDIT
SETTINGS (price), SOUND SETTINGS (volume, speech, attract sounds, music
tempo), PICTURE (smooth motion, steady game frames, board rate, size,
position, mirroring, brightness, test pattern), the game's self test,
reset high scores and statistics. The
game's own settings are kept in its EEPROM; changing them starts the game
again when the menu is left.

## Enhancements

On the service menu's GAME SETTINGS page, under ENHANCEMENTS, each on or off (all off: as the
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
`game.ini`, `mhavocpe.ini`, `run.sh`, `marquee.png` (the ROMs: `/usr/local/share/advance/rom/`, above); `save/` holds `mhavocpe.ini` (the owner's
settings), `controls.ini` (the controls) and `eeprom.bin` (high scores, the game's options, statistics; the
Tournament Edition's in `eeprom_te.bin`, the languages share one).

Building, deploying, packaging: BUILD.md.
