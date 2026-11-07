/* romset.h - The Promised End's ROMs.
 *
 * Major Havoc - The Promised End is Jess M. Askey's; its ROMs come with
 * the package (package/roms/mhavocpe.zip, the release 1.01, English,
 * ADPCM speech, from upstream/ROMs/_builds/1.01). Any build of the same
 * layout is taken instead when given (--roms, rom_path): HBMAME's sets
 * (mhavocpe, mhavocpede, mhavocpete, ...) and the release zips alike.
 *
 * The files are told by the board position their name ends in (MAME's
 * names: mhpe101.1mn, mhavocpe.6kl, ...), all from one zip or folder,
 * never mixed:
 *
 *   1MN 1L     alpha program, 16K each        (8000-FFFF)
 *   1Q 1NP     alpha paged program, 32K each  (2000-3FFF, eight pages)
 *   6KL        vector ROM, 8K                 (5000-5FFF, 6000-7FFF)
 *   6H 6JK     vector ROM pages, 16K each     (the generator's 2000-3FFF)
 *   9S         gamma program, 16K             (8000-BFFF, C000-FFFF)
 *   X1         speech, 256K (optional: no speech without it)
 *   6C         136002-125, the vector generator's state PROM (optional:
 *              its exact timing)
 *
 * Builds with LPC speech (a 32K 9S for the TMS5220) are not supported
 * yet. Each ROM begins with four bytes: its position, the language (0
 * English, 1 German, 2 French), the version (major, minor).
 */
#ifndef MH_ROMSET_H
#define MH_ROMSET_H

#include <stdint.h>

enum { MH_R_1MN, MH_R_1L, MH_R_1Q, MH_R_1NP, MH_R_6KL, MH_R_6H, MH_R_6JK, MH_R_9S, MH_R_X1, MH_R_6C, MH_R_COUNT };

typedef struct mh_romset {
    uint8_t  r1mn[0x4000], r1l[0x4000], r1q[0x8000], r1np[0x8000];
    uint8_t  r6kl[0x2000], r6h[0x4000], r6jk[0x4000], r9s[0x4000];
    uint8_t  x1[0x40000];
    uint8_t  prom[0x100];
    uint32_t found;             /* a bit per MH_R_xxx */
    int      lpc;               /* a build with LPC speech was found (not supported) */
    char     source[1024];      /* the zip or folder it came from */
} mh_romset;

/* Look in each path (a .zip or a directory) in order until one holds a
 * complete set. Returns 0 when the game can run; otherwise -1 and
 * mh_romset_missing() names what is missing from the best one found. */
int mh_romset_load(mh_romset *rs, const char *const *paths, int npaths);

/* The ROMs not found, as a comma-separated list (empty when complete). */
const char *mh_romset_missing(const mh_romset *rs, char *buf, int size);

static inline int mh_romset_have(const mh_romset *rs, int r) { return (rs->found >> r) & 1; }

/* The language (0 English, 1 German, 2 French) and version of the set,
 * from 1MN's first bytes. */
const char *mh_romset_describe(const mh_romset *rs, char *buf, int size);

/* The default search list: $MH_ROMS (a ';'- or ':'-separated list), ./roms and the usual
 * AdvanceMAME/MAME rom folders, each tried as mhavocpe.zip and as a
 * directory (the package's roms/ next to the program is the caller's,
 * main.c). Returns the count. */
int mh_romset_default_paths(const char **out, int max);

/* The same forms for one more folder; appended at out[n], returns the new count. */
int mh_romset_add_dir(const char **out, int n, int max, const char *dir);

#endif
