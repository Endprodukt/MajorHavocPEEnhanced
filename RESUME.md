# Major Havoc PE Enhanced: continuing

- Builds: `cmake --build build/pc` (MSYS2 gcc, SDL2; in Git Bash first
  `export PATH="/c/msys64/mingw64/bin:$PATH"`) and `cmake --build build/pi`
  (clang + lld, `cmake/toolchain-pi-clang.cmake`, sysroot of the Tempest
  2000 port). Both are configured. Details: BUILD.md. The SDK in
  `external/VectorPie` is a git clone (updated with `git pull`).
- Looking at the game without a screen: `build/pc/mhshot.exe --roms
  package/roms --seconds S --play "3 coin;5 jump;8 roll 40;10 aux 600"
  --svg _scratch/x.svg --at T,T --wav _scratch/x.wav [--speed 2]
  [--smooth] [--dump N] [--test]`. The self test's sound list: `--test`,
  `10 aux 600`, then `roll 130` and `jump 300` per step.
- To the cabinet: `sh tools/deploy.sh` (`/persistent/games/mhpeenhanced`).
  Package: bump `version` in `package/game.ini`, `sh tools/pack.sh` (ROMs
  included). Repository: https://github.com/Endprodukt/MajorHavocPEEnhanced (public,
  main; build/, upstream/, _scratch/, _ref/, external/ and videos stay
  out, `.gitignore`). Releasing as the other Enhanced games: a GitHub
  release with the .vpk (see Tempest Enhanced's RESUME.md); none yet.
- The game's options: `core/options.c` (EEPROM 20-22). Controls:
  `platform/controls.c` (SDK functions). Smoothness: `core/emu.c` (speed),
  `render/frames.c`, `render/smooth.c`. Sound: `sound/sound.c`,
  `sound/pokey.c pokey_run_opamp`, `platform/audio.c` (SDK stream).
  Enhancements: `render/enhance.c` (screens told by RAM: gamest 9B,
  frame+1 A1, animate BD, perm2 C7, brstat 854, tact 85B); the subtitle's
  shape: `python tools/logo/logotrace.py` (LOGO_OVERLAY=x.png to check)
  writes `render/logo_shape.h`. Headless: `mhshot --enhance`,
  `--poke "T ADDR VAL"`; Breakout: `--play "2 coin;4 jump;10.2 jump"`.
- Symbols: the listings in `upstream/ROMs/_builds/1.01/_listings`
  (extracted in `_scratch/lst/lst/`), RAM in `upstream/Source/a_ram.ah`,
  `g_ram.ah`. Reference: `upstream/` (jessaskey/mhavocpe), `_ref/hbmame/`,
  `F:/mame/src`, `F:/Tempest Enhanced`.
- Music tempo: `core/machine.c mh_machine_tick` (gamma_irq_steady,
  owed), `core/emu.c` (the gamma at the alpha's speed). `mhshot` prints
  "gamma irqs +N" a second (610 is right).
- Story text placement: `render/enhance.c mh_enhance_patch` (ROM page 0
  3E00/3E20, fixed ROM E418, page 0 2441; STORY_DY). Debugging display
  lists: `mhshot --dump N`.
- Roller: the game needs about 2400 counts a second for Rex's full speed;
  the cabinet's roller about 500 a turn (LETA 4x); standard 42 %
  (`render/config.c`, ORIGINAL_COUNTS in `platform/menu.c` and `main.c`).
- Builds (language, Tournament Edition): `main.c load_build`, `load_roms`,
  `find_builds`, `leave_menu`, `show_missing`; `rom/romset.c`
  (mh_romset_variant/use/fill, the file name rules). The package brings
  no ROMs (0.3.0); test sets in `_scratch/romtest/` (A split, B merged,
  C a release zip, D a folder; untracked). Test a build headless:
  `mhshot --roms upstream/ROMs/mhavocpede --enhance ...`.
- Never send simulated keys to the desktop while the owner works at the
  PC: they reached the owner's browser once.
- Next: 0.3.0 to the cabinet (ask; put the ROMs into
  /usr/local/share/advance/rom there first, the game's roms/ holds 0.2.9's
  zips); GitHub's cached old commits (STATUS). (0.2.9 is on the cabinet) the owner's verdict on the languages,
  the Tournament Edition, the joined menu pages, the
  music and the enhancements, the open items in STATUS.md.
