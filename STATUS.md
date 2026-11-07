# Major Havoc PE Enhanced: status

## Current state (2026-11-07)

- Version 0.3.0 built: the package without ROMs (the owner brings them,
  README "ROMs"). Not committed, not on the cabinet. The cabinet runs
  0.2.9 (its roms/ still holds the zips 0.2.9 brought).
- Open: the public repository's history (commits 2016ef6, 689af96) and
  the v0.2.9 release's .vpk still hold the ROMs; the owner decides.
- Version 0.2.9 (the roller's standard 42 %, the service menu's
  pages joined, language and Tournament Edition) on the cabinet since
  2026-11-07, started by deploy.sh: 50 fps, 5-7 ms a frame, the six ROM
  zips in roms/; its saved sensitivity 41 % (the owner's) stays.
- Earlier, version 0.2.2: the port runs on PC (window) and is built for the Pi
  (`build/pi/mhavocpe`, md5 `f800c0e933c5668d28f33acd8068d13b`, the same
  program as 0.2.1); package `build/vpk/mhpeenhanced-0.2.2.vpk` (1.6M,
  ROMs and marquee included, md5 `08ae23092d60056b3ef727f3a3e4b984`).
  Installed on the cabinet 2026-11-07 (`pi@192.168.0.122:
  /persistent/games/mhpeenhanced`, md5s checked, all libraries found) and
  started there (Raspberry Pi 5, USB-DVG on /dev/ttyACM0, SDK 0.9.0, 1
  joystick): attract mode at 50.0 fps, 280-350 vectors, 4-8 ms a frame
  (peak 8.5), the process about 2 % of the CPU. About 30 s after the start
  the picture turned to a still one of 860-1170 vectors (as the service
  menu): presumably opened at the cabinet; not confirmed. Not yet played
  or heard on the cabinet by me.
- Both original programs (The Promised End 1.01, English, ADPCM) run on two
  6502 cores; board, rendering, sound, controls, menu and tools are C.
- Git: public repository https://github.com/Endprodukt/MajorHavocPEEnhanced
  (branch main, created 2026-11-07 with 0.2.8). Release v0.2.9
  (2026-11-07, commit 689af96) with `mhpeenhanced-0.2.9.vpk`:
  https://github.com/Endprodukt/MajorHavocPEEnhanced/releases/tag/v0.2.9
  (its notes: `_scratch/release_notes_0.2.9.md`).

## 0.3.0: no ROMs in the package

- Built for the Pi (`build/pi/mhavocpe` md5
  `7a2d79931d488295094f86ad88953209`), package
  `build/vpk/mhpeenhanced-0.3.0.vpk` (md5
  `13b75bea4c98267ac40286ff41211f4d`, 1.5M: program, ini files, run.sh,
  marquee; no ROMs).
- The owner: the ROMs are not to come with the game. package/roms/ is
  gone (the zips kept for tests in `_scratch/romtest/`, untracked);
  pack.sh, deploy.sh and CMake copy none. README "ROMs": the six builds,
  their HBMAME sets and release zips, the files by board position, sizes,
  CRC32, where they come from (jessaskey/mhavocpe ROMs/, ROMs/_builds/1.01)
  and where they go (/usr/local/share/advance/rom on VectorPie).
- The loader (rom/romset.c): each build alone (no English set needed);
  per folder `<set>.zip`, `<set>/`, the release zip, `mhavocpe.zip` (a
  merged set) and the folder; a file named for another build (mhpe101.*,
  mhpede101.* ...) is never taken, one named for none only for the
  English build or in the build's own zip or folder; 1MN's language byte
  checked; speech and 6C from anywhere. The build set not there: the
  English one, else any there runs. The menu marks builds without ROMs
  (ROMS MISSING); leaving with one, the running one stays.
- No ROMs at all: a screen says what is missing and where it goes (the
  path in Hershey letters), any button back to the cabinet's menu.
- Checked on the PC: none (the screen, seen in a window), split HBMAME
  (English parent, German clone without speech), merged HBMAME (all six
  in mhavocpe.zip: English, German, French Tournament each right), a
  release zip alone (French Tournament; set to English it runs the French
  Tournament), an HBMAME folder. Not checked: the menu's ROMS MISSING mark
  on screen (simulated keys reached the owner's browser instead of the
  window: no more key tests while the owner works at the PC), the cabinet.

## 0.2.9: the roller's standard 42 % (the LETA counts 4 times), the menu's pages joined, language and Tournament Edition

- Built for the Pi (`build/pi/mhavocpe` md5
  `7366875fdef138d8468a3409d709d761`), package
  `build/vpk/mhpeenhanced-0.2.9.vpk` (md5
  `571739b103794d86f616a2dde6390751`, 2.2M with the six builds). On the cabinet (deploy.sh now copies all
  ROM zips).

- The owner doubted the 21 % (and had set 41 % on the cabinet). The game
  (tw_main.asm, tw_motion.asm): it reads the LETA every second interrupt
  (measured 407 interrupts, 51 pictures a second), adds half of each
  change and halves the sum each picture; Rex's speed goes to twice the
  sum, full speed from about 2400 counts a second. With the LETA counting
  twice (250 a turn of the 3.25" roller) that took 10 turns of the big
  roller a second: not the cabinet. The LETA (a quadrature decoder, 2x or
  4x; the arcadecontrols wiki lists Major Havoc among its games) counted 4
  times: about 500 a turn, full speed from 5 turns a second. Now the
  standard 42 % (500 / 1200), ONE TURN AS THE ORIGINAL and the roller test
  reckon with 500. The settings file now carries `settings_version = 2`;
  an older one holding the old standard 21 % takes 42 %, an owner's own
  value stays (the cabinet's 41 %).
- Language and edition (the owner): The Promised End's own self test
  settings (price, difficulty, lives, bonus, attract sounds, roller x2)
  were all in the menu already; its other "settings" are builds:
  English, German, French, each also as the Tournament Edition (other
  mazes; upstream/ROMs). All six now come with the package (the five
  others' programs only, 89-90 KB each zipped; speech and 6C from
  mhavocpe.zip: the speech is the same English in all), chosen on GAME
  SETTINGS (LANGUAGE, EDITION; `language`, `tournament` in the ini).
  Leaving the menu with another build: its ROMs, its EEPROM
  (`eeprom_te.bin` for the Tournament Edition, the languages share
  `eeprom.bin`), the menu's options in it, the game started again
  (`main.c load_roms`, `leave_menu`; `rom/romset.c mh_romset_variant`,
  `mh_romset_use`, `mh_romset_fill`). A build not found: the English one
  runs. The cabinet's AdvanceMAME has only a pre-release (mhavocpex7).
- The enhancements in all six: the title's routines are the same; the
  story patch now looks for the window's call (E418, the Tournament
  Edition's E408) and puts its code into page 0's free room where it is
  (3E00 English; 3E40 German, 3F50 French: their room starts later).
  FIT: the high score and info pages are as wide as their lines (the
  German HOHE ERGEBNISSE was cut off at x1.60, now x1.47); the story as
  wide as its widest line so far (German and French lines are longer:
  x1.11 / x1.08 after the first long one, English stays x1.15). Checked
  headless in all six (title, story, high scores, story pages in three
  languages) and in a window: LANGUAGE DEUTSCH, leaving, the game in
  German; a start in the French Tournament Edition (`eeprom_te.bin`).
  Not checked: the info pages (E0+) in German and French, a game played
  in each build, the cabinet.
- The service menu (the owner): the ROLLER page joined CONTROLS (under a
  ROLLER heading; its first line now TURNED BY), ENHANCEMENTS joined GAME
  SETTINGS (under an ENHANCEMENTS heading); a page too long for the screen
  is set closer (CONTROLS now 20 lines). Checked in a window: both pages,
  ESC back, 42 % shown from the old PC settings.
- Game speed: the interrupts, pictures (50.9 a second when done in time),
  music and sound keep the cabinet's time. GAME FRAMES STEADY runs the
  CPUs twice as fast, so the busy scenes that slowed the cabinet down (to
  about 35 pictures) do not slow down; AS THE CABINET is the original's
  speed throughout.

## 0.2.8: the story text up to the subtitle, the high scores higher

- Built for the Pi (`build/pi/mhavocpe` md5
  `2274019c3a43eaf48f7e2d5e5f2b05c7`), package
  `build/vpk/mhpeenhanced-0.2.8.vpk` (md5
  `65de6b107fb44f82d022193fe7e1b8dc`). On the cabinet since 2026-11-07,
  started by deploy.sh: 50 fps, 4.6-5.3 ms a frame in the attract mode.
  Not yet seen there by me.
- The owner: under the title the enemy list (and the backstory) went
  away far below the subtitle. The game cuts the story text off at the
  screen's middle twice: a line goes when it scrolls above the middle
  (`tw_story.asm ?setupln`), and the generator's window (SCAL bit 11:
  nothing above the beam's height; `tw_main.asm` E418
  `VGADD_SCALE(ywin_on,1,0)` with the beam in the middle) cuts what is
  above. Now (FIT on, attract mode only, not during the self test, which
  sums the ROMs): `mh_enhance_patch` puts two routines into ROM page 0's
  free room (3E00, 3E20; 3DEB-3FFE are FF): after each story line's CNTR a
  dark vector up (64 at binary scale 2), and before the window the beam
  moved up as much (E418 now calls stpg0 first, then 3E20); the enemy list
  draws a line more (page 0 2441: 06 -> 07) so its bottom stays full. The
  text now goes a little under "Enhanced". In a game (gamest D7) the
  routines do nothing (the end story).
- The high score table (pages 06/07: -66..88, -92..81) had the box of the
  info pages (up to +111), so it sat low: its own box now, with a margin.
  The owner also wanted the table itself higher: its heading is at 68..84,
  the table from 24 down (three and a half rows between); lines below 50
  are lifted by 26 (`lift_scores`, FIT on, pages 06/07 in the attract
  mode), the box refitted: x1.60, centred.
- Checked headless (list, backstory, high scores, self test with the
  patch off: all ROMs P, a game) and in a window (the list).

## 0.2.7: the subtitle larger

- Built for the Pi (`build/pi/mhavocpe` md5
  `153a1cd225107d9b024f5aecacf02443`), package
  `build/vpk/mhpeenhanced-0.2.7.vpk` (md5
  `14b24c4ade3bd39a1c6cfa58380b9267`). Not yet on the cabinet.
- The owner found the one-line PROMISED END: Enhanced (0.42 of MAJOR
  HAVOC's width, the marquee's proportion) hard to read: now 0.70
  (`SUB_WIDTH`), right aligned under HAVOC; zooming in it grows to at
  most the screen's width (before, it ran off both sides). Checked
  headless (the zoom-in) and in a window.

## 0.2.6: the music at its tempo

- Built for the Pi (`build/pi/mhavocpe` md5 `afb68680e1655586eff5591573940fc2`), package
  `build/vpk/mhpeenhanced-0.2.6.vpk` (md5 `69f8c3da96e2c0090faac34e142df509`). On the cabinet since 2026-11-07
  (md5 checked), started by deploy.sh: 50 fps, 4.7-7.8 ms a frame (peak
  10.8) in the attract mode, the process 39 % of one of the Pi's 4 cores
  (top over 3 s). The music not yet heard there by me.
- The owner heard the music change its speed at times. The music is
  played by the gamma's interrupt (`rpm.asm g_irq`: every second one
  updates two of the four POKEYs). On the board (and in MAME) the
  interrupt comes 8 ticks (610 a second) after it was acknowledged; the
  gamma acknowledges late while it is busy with the alpha's commands
  (NMIs, which mask it), and every later one is late too. Measured
  headless (`mhshot` now prints the gamma's interrupts a second) over 120 s
  of attract and play: as the cabinet 441-613 a second (avg 592); with
  steady frames (the alpha x2, twice as many commands a second: 2023 NMIs
  against 861) 355-620 (avg 509), i.e. the music down to 58 % at times.
- Now: (1) MUSIC TEMPO STEADY (`steady_music`, standard on;
  `core/machine.c`): the interrupt is raised every 8 ticks whenever it was
  acknowledged, one due while one is still up is given right after the
  acknowledge (at most 2 owed, none while the gamma is held in reset).
  (2) Steady frames run the gamma as fast as the alpha (`core/emu.c`,
  `gfrac`), so the commands take it as much of its time as on the board;
  the POKEYs, the speech and the sound clock keep real time (`gcycle` is
  real time). Measured: steady frames 610-612 a second throughout, as the
  cabinet's speed 609-624 (making up the lost ones).
- Checked: the self test at x2 (alpha and gamma pages all P, the EEPROM
  written by the faster gamma passes), the sound list (102 speech phrases,
  peak and length the same at x1 and x2), the menu entry in a window.
  `mhshot --steady-music`; its play script now takes 512 actions.

## 0.2.5: enhancements (subtitle, fit, Breakout close-up), new marquee

- Built for the Pi (`build/pi/mhavocpe` md5
  `c65e2d325467b68a42249f4a7af8effc`), package
  `build/vpk/mhpeenhanced-0.2.5.vpk` (md5
  `27ab994da4e428f9af40dd377b3aad52`). On the cabinet since 2026-11-07
  (program md5 checked), started there (the digital rain screensaver had
  the board; deploy.sh only installed, started by hand with run.sh): 50 fps,
  3.7-4.8 ms a frame in the attract mode, the only program on the board.
  The SDK saw 0 joysticks this time (1 before). Not yet seen or played on
  the cabinet by me.
- The owner's second marquee (2000 x 375, "PROMISED END: Enhanced" on one
  line; source `_scratch/marquee/source-2.png`, also
  `tools/logo/mhavoc_marquee.png`) as `package/marquee.png` (1920 x 360,
  the same proportions, no bars). The first one's trace source is kept in
  `_scratch/marquee/mhavoc_marquee_old.png`.
- `render/enhance.c`, switched on the new ENHANCEMENTS page
  (`fx_subtitle`, `fx_fit`, `fx_breakout`, default on), applied to each
  picture after the smoothing:
  - SUBTITLE: the title's THE PROMISED END (the two groups after HAVOC3's
    red line) replaced by the marquee's PROMISED END: Enhanced
    (`render/logo_shape.h` from `tools/logo/logotrace.py`, traced from the
    second marquee: one line), turned level
    (least squares of PROMISED END's points), 0.70 of MAJOR HAVOC's width
    (0.2.7; 0.42 before),
    right aligned under HAVOC; zooming in while `animate` = 2 as the game
    (`perm2`), from the screen's middle, its top under HAVOC. Only when the
    vector ROM holds PE 1.01's title routines. The tracer now keeps the
    letters' holes (P, R, O, D, a, e, d) and the colon (thin pieces are
    dropped by their thickness instead of their height).
  - FIT: tactical display (`tact` D7) x1.21, attract pages 06/07 (high
    scores) x1.60 with the table lifted (0.2.8), E0+ (info) x1.26, pages 08-1D (title, story)
    x1.15, eased in
    0.25 s, lines cut at the field's edge.
  - BREAKOUT (off by default; the owner wants the whole tactical display
    larger, which FIT does, x1.21, edge to edge: more would cut it off):
    `brstat` D7 during the tactical display: its box (58..102, -84..-12)
    x3.61, the rest of the tactical display out of view.
- Checked headless (`mhshot --enhance`, `--poke "T ADDR VAL"` new) and in a
  window: title phases, the story, high scores, tactical display, Breakout
  started with jump, the menu page.

## 0.2.4: roller direction as MAME, the service menu's keys shown

- On the cabinet since 2026-11-07 (program md5
  `f5b509b65695d0093d1c2d62f91b1306`), started there: 50 fps, 4.7-6.2 ms
  a frame in the attract mode.

- The roller turned the wrong way (keys and spinner alike): MAME reads the
  LETA with PORT_REVERSE (the game negates the count again). Now as MAME:
  rolling right moves the ship right (checked headless: its middle from
  x -3 to +98). `mhshot`'s "roll N" the same way.
- The CONTROLS page's SERVICE MENU row read NONE: its standard is now
  F2 + Tab (they work whatever is laid out; more can be added).
- The test pattern's brightness bars centred (they ran 900..3250, now
  symmetric about 2048); the rest was centred already.

## 0.2.3: the game's viewport framed in the test pattern

- The test pattern (PICTURE, TEST PATTERN) draws the board's whole field
  as a fixed grey frame and the game's viewport (MAME's 300 x 260) as a
  white frame with marks at its corners and the middle of each side, laid
  out as size, place and mirroring say. There the arrows move it (8 board
  units a step), START 1 switches them to its size (0.5 % a step: left /
  right the width, up / down the height); held, the steps grow. The
  PICTURE page draws the viewport's frame grey over the dimmed game.
  Checked in a window. At size 100 the two frames are the same (the game
  fills the board's field); on the tube make the white one fit the glass.

## 0.2.2: the marquee, under Atari

- `package/marquee.png`: the owner's marquee ("The Adventures of Major
  Havoc / Promised End: Enhanced", 2088x520, source kept in
  `_scratch/marquee/source-1.png`) in proportion on black for the 1920x360
  marquee screen (1446x360 in the middle: `magick SRC -resize x360
  -background black -gravity center -extent 1920x360`); `game.ini` names
  it. The menu group is `manufacturer = Atari` (as before). Not yet seen on
  the cabinet.

## 0.2.1: the roller entirely the owner's

- The service menu's ROLLER page: what turns it (`mh_config.roller`: the
  cabinet's spinner `dial`, `dial2`, `trackball_x/y`, `mouse_x/y`, a
  stick's axis `joyN_axisM`, up to four joined by "+"; START then move the
  control, again adds one more; left / right the usual ones; Delete the
  spinner), sensitivity 1-999 % in steps of 1 (held, the steps grow to 5
  and 25), shown as turns of the original roller a spinner turn, the
  spinner's counts a turn (1200), "set one turn as the original"
  (250 / counts a turn), direction, the game's roller x2, a stick's speed
  (2400 counts a second all the way), the keys' speed (1600 counts a
  second), and a ROLLER TEST (a dial turning as the original roller, turns,
  counts, speed against Rex's full speed; left / right change the
  sensitivity at once).
- Menu fixes: a direction held when the menu opens (a stick or pedal
  resting off its middle, as on this PC with four joysticks) no longer
  scrolls; the spinner moves the cursor a line a picture at most (the
  mouse jumps as a window gets the focus).
- Checked in a window: the page, cycling the source, the sensitivity, the
  test. Laying out the source by moving it could not be checked by script
  (the window lost the focus); to try by hand.

## 0.2.0: service menu, the SDK's controls, smooth motion, sound as HBMAME

- **Service menu** (F2 or Tab, or START 1 and jump held 2 s), as Tempest
  Enhanced's: CONTROLS, ROLLER, GAME SETTINGS (lives 3-6, difficulty,
  bonus life, roller x1/x2), CREDIT SETTINGS (price), SOUND SETTINGS
  (volume, speech, sounds in the attract mode), PICTURE (smooth motion,
  game frames steady/as the cabinet, board rate, size, place, mirroring,
  brightness, test pattern), the game's self test, reset of high scores and
  statistics. The game's own settings live in its EEPROM (`core/options.c`:
  bytes 20-21, checksum 22 = the sum inverted, decoded after
  `tw_slave.asm getoptn` / `tw_test2.asm savesets`); a change is written
  when the menu is left and the game starts again (the menu says so).
  Checked in a window by script (`_scratch` screenshots): every page, a
  change shown and undone.
- **Controls through the SDK** (`vectorpie/vp_controls.h`, SDK updated to
  065d93a): functions jump (p1_button1), shield (p1_button2), roll_left /
  roll_right (p1_left/right), aux (F1, 7), service_menu (none), laid out
  with `vp_capture`, kept in `save/controls.ini`; menus on the SDK's fixed
  menu controls. Tempest's own binding code (`vpbind`) is gone. Tab is
  reserved for the menu.
- **Smooth**: steady frames (the alpha at twice its speed; measured: busy
  maze scenes 35-38 pictures a second as the cabinet, 49-51 steady; the
  game's main loop waits for 8 interrupts a picture, so a slow picture
  loses time, which on the cabinet slows the game down; steady, it no
  longer does: the maze then runs at its full pace). Pictures between the
  game's pictures by running a copy ahead (`render/frames.c`,
  `render/smooth.c`): lines grouped by the list's CNTR groups and objects;
  97-99 % of the lines paired in attract and maze, about 85 % in the space
  battle (3D enemies whose line count changes change over halfway).
- **Sound as HBMAME mixes it** (MAME's "how loud"): each POKEY through
  MAME's op-amp stage (`set_output_opamp(1k, ...)`: 1000 times the
  channels' conductance, `pokey_run_opamp`), 0.25 each to the speaker, the
  OKI at 1.0; full scale 1.0. The speech alone reaches full scale (as in
  HBMAME); above 0.8 a soft knee instead of MAME's hard clip. Output through
  the SDK's stream (`vp_stream_play`): on the cabinet it follows the
  VectorPie menu's (music) volume.
- **Speech checked**: the self test's sound list played through (82
  phrases); the spectrogram shows voiced speech
  (`_scratch/speech_sample.wav`). In the attract mode the game speaks
  rarely or not at all (none in 120 s).
- **Roller for a 1200 ppr spinner**: the cabinet's roller (3.25" roller on
  a 0.625" one, 24 tooth wheel: about 125 pulses a turn, counted twice by
  the LETA without R186) gave about 250 counts a turn (0.2.9: wrong, the
  LETA counts 4 times, 500 a turn, 42 %); 21 % makes a turn
  of the spinner a turn of the roller (sources: craigb-spinner's MAME game
  config page, the KLOV thread on Tempest to Major Havoc conversions).
  Digital rolling 1600 counts a second.
- **POKEYs run from power on** (a POKEY has no reset pin; MAME starts them
  in reset, and the gamma's self test then reported Q-POKEY bad). Now it
  passes.
- **Defaults of a new EEPROM**: 3 lives, medium, bonus at 100000, attract
  sounds, roller x1, 1 coin 1 play (dsw1 00, dsw2 FF through the game's
  `loadop`).

## Findings (0.1.0)

- MAME's visible area for Major Havoc is 300 x 260; HBMAME's PE driver
  keeps the Alpha One prototype's 580 x 500.
- On the PE board the alpha's 6000-7FFF is the vector ROM page the AVG's
  last STAT chose (the self test sums VP0-3 so); MAME maps 6KL's empty half
  there. Done as the board.
- PE executes $1A at power on (65C02 test): the 6502 core has the
  undocumented NOPs.
- The self test's roller counts a step only when the 8-bit counter changes
  sign (128 counts).

## Known issues and limitations

- Not run on the cabinet: frame rate and CPU load on the Pi (headless on
  the PC 40 s take 2.8 s with steady frames and run-ahead), the roller's
  feel at 21 %, the stream's latency, the picture's size.
- Laying out the roller's source by moving a control: not tried yet.
- Steady frames change the pace of busy scenes against the cabinet (no
  slowdown); "AS THE CABINET" keeps the original.
- Lines of objects whose number of lines changes are not moved between
  pictures (they change over halfway).
- No frame-by-frame comparison with HBMAME (no HBMAME build here); sound
  levels follow HBMAME's mixing by its source, not by a recording.
- LPC builds (TMS5220) refused. `tests/` empty.
- Breakout close-up (off by default) hides the rest of the tactical
  display until it ends; FIT's boxes are measured
  per screen, not computed from each picture.
- Keys shorter than a picture are missed (the SDK reads the keyboard once
  a picture); real presses are longer. Window tests must hold keys
  (`menutest.ps1` in the session scratchpad did it with 150 ms).

## Outstanding tasks

- Try 0.2.0 on the cabinet (`sh tools/deploy.sh`): picture, smoothness,
  roller at 21 %, buttons, sound and speech level, the menu, frame rate.
- Try 0.2.5's enhancements on the cabinet (subtitle size, the zooms).
- Optionally the other builds (German, French, Tournament) as a choice.
