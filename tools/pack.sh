#!/bin/sh
# pack.sh - the VectorPie package (.vpk) of the Pi build: the program, the
# package's files and the ROMs (The Promised End's come with the game).
# Run from the project's folder, in Git Bash, after the Pi build. Writes
# build/vpk/mhpeenhanced-<version>.vpk.
set -e
S=build/vpk/stage/mhpeenhanced
rm -rf build/vpk/stage
mkdir -p $S/roms
cp build/pi/mhavocpe $S/
cp package/game.ini package/mhavocpe.ini package/run.sh $S/
cp package/roms/mhavocpe.zip $S/roms/
[ -f package/marquee.png ] && cp package/marquee.png $S/
sh external/VectorPie/sdk/tools/vp-pack/vp-pack.sh $S build/vpk
