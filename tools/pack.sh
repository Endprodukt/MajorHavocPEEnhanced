#!/bin/sh
# pack.sh - the VectorPie package (.vpk) of the Pi build: the program, the
# package's files. The ROMs do not come with it: the owner brings them
# (README).
# Run from the project's folder, in Git Bash, after the Pi build. Writes
# build/vpk/mhpeenhanced-<version>.vpk.
set -e
S=build/vpk/stage/mhpeenhanced
rm -rf build/vpk/stage
mkdir -p $S
cp build/pi/mhavocpe $S/
cp package/game.ini package/mhavocpe.ini package/run.sh $S/
[ -f package/marquee.png ] && cp package/marquee.png $S/
sh external/VectorPie/sdk/tools/vp-pack/vp-pack.sh $S build/vpk
