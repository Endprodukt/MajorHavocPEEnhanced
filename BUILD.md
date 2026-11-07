# Building

Both builds are configured in `build/pc` and `build/pi`; CMake 3.21+,
Ninja.

## PC (Windows, MSYS2 gcc, SDL2)

In Git Bash, MSYS2's gcc must come first in `PATH` (otherwise another
`gcc` is found and the build fails without a message):

    export PATH="/c/msys64/mingw64/bin:$PATH"
    cmake -S . -B build/pc -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
    cmake --build build/pc

`build/pc/mhavocpe.exe --windowed` runs the game in a window (the ROMs are
copied to `build/pc/roms`). `-DMH_BUILD_GAME=OFF` builds only the board and
the tools (no SDK, no SDL).

## Raspberry Pi (the cabinet), clang + lld from Windows

`cmake/toolchain-pi-clang.cmake`, against a copy of the cabinet's `/usr`
and `/lib` (the Tempest 2000 port's):

    cmake -S . -B build/pi -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-pi-clang.cmake \
          -DMH_PI_SYSROOT="F:/Tempest 2000 Port/build/pi-sysroot" \
          -DCMAKE_C_COMPILER=C:/Users/ocare/.local/share/retcomm/toolchains/cmake-clang-v1/latest/bin/clang.exe \
          -DCMAKE_CXX_COMPILER=C:/Users/ocare/.local/share/retcomm/toolchains/cmake-clang-v1/latest/bin/clang++.exe \
          -DCMAKE_MAKE_PROGRAM=C:/Users/ocare/.local/share/retcomm/toolchains/cmake-clang-v1/latest/bin/ninja.exe
    cmake --build build/pi

## To the cabinet

    sh tools/deploy.sh

(ssh key `~/.ssh/id_ed25519_vectorpie`, `pi@192.168.0.122`): installs to
`/persistent/games/mhpeenhanced` with the ROMs, starts the game, shows
`run.log`; if another game has the board it only installs.

## Package

    sh tools/pack.sh

writes `build/vpk/mhpeenhanced-<version>.vpk` (program, `game.ini`,
`mhavocpe.ini`, `run.sh`, `roms/mhavocpe.zip`). Bump `version` in
`package/game.ini` first.

## Tools

`build/pc/mhshot.exe` runs the game without a screen:

    mhshot --roms package/roms --seconds 40 --play "3 coin;5 jump;8 roll 40" \
           --svg _scratch/g.svg --at 6,13,24 --wav _scratch/g.wav [--dump 80] [--test]

writes the pictures at those times as SVG (generator units, MAME's 300 x
260 field), the sound as WAV, prints pictures/interrupts/PCs each second;
`--dump N` decodes the display list, `--test` turns the self test switch
on, `--dsw1/--dsw2 HEX` set the option switches, `--eeprom FILE` starts
from (and keeps) an EEPROM image. SVG to PNG:
`magick -density 40 F.svg -background black -flatten F.png`.
