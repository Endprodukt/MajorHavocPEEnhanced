#include "rom/romset.h"

#include "miniz.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct part {
    const char *pos;        /* the name's ending after the last '.' */
    uint32_t    size;
    int         optional;
} part;

static const part PARTS[MH_R_COUNT] = {
    { "1mn", 0x4000, 0 }, { "1l", 0x4000, 0 }, { "1q", 0x8000, 0 }, { "1np", 0x8000, 0 },
    { "6kl", 0x2000, 0 }, { "6h", 0x4000, 0 }, { "6jk", 0x4000, 0 }, { "9s", 0x4000, 0 },
    { "x1", 0x40000, 1 }, { "6c", 0x100, 1 },
};
#define REQUIRED ((1u << MH_R_X1) - 1)

static uint8_t *dest(mh_romset *rs, int r)
{
    switch (r) {
    case MH_R_1MN: return rs->r1mn;
    case MH_R_1L:  return rs->r1l;
    case MH_R_1Q:  return rs->r1q;
    case MH_R_1NP: return rs->r1np;
    case MH_R_6KL: return rs->r6kl;
    case MH_R_6H:  return rs->r6h;
    case MH_R_6JK: return rs->r6jk;
    case MH_R_9S:  return rs->r9s;
    case MH_R_X1:  return rs->x1;
    default:       return rs->prom;
    }
}

/* Which part a file is, by its name's ending (-1: none). */
static int part_of(const char *name)
{
    const char *base = name;
    for (const char *p = name; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    const char *dot = strrchr(base, '.');
    if (!dot) return -1;
    char ext[8];
    size_t n = strlen(dot + 1);
    if (n == 0 || n >= sizeof ext) return -1;
    for (size_t i = 0; i <= n; i++) ext[i] = (char)tolower((unsigned char)dot[1 + i]);
    for (int r = 0; r < MH_R_COUNT; r++)
        if (!strcmp(ext, PARTS[r].pos)) return r;
    return -1;
}

static void offer(mh_romset *rs, int r, const uint8_t *data, size_t size)
{
    if (r < 0 || (rs->found & (1u << r))) return;
    if (r == MH_R_9S && size == 0x8000) { rs->lpc = 1; return; }
    if (size != PARTS[r].size) return;
    memcpy(dest(rs, r), data, size);
    rs->found |= 1u << r;
}

static void scan_zip(mh_romset *rs, const char *path)
{
    mz_zip_archive zip;
    memset(&zip, 0, sizeof zip);
    if (!mz_zip_reader_init_file(&zip, path, 0)) return;
    mz_uint n = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < n; i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st) || st.m_is_directory) continue;
        int r = part_of(st.m_filename);
        if (r < 0 || st.m_uncomp_size > 0x40000) continue;
        size_t size = 0;
        void *data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        if (data) {
            offer(rs, r, (const uint8_t *)data, size);
            mz_free(data);
        }
    }
    mz_zip_reader_end(&zip);
}

static void scan_file(mh_romset *rs, const char *path)
{
    int r = part_of(path);
    if (r < 0) return;
    FILE *f = fopen(path, "rb");
    if (!f) return;
    static uint8_t buf[0x40000 + 1];
    size_t n = fread(buf, 1, sizeof buf, f);
    fclose(f);
    if (n < sizeof buf) offer(rs, r, buf, n);
}

static int is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int ends_with_zip(const char *path)
{
    size_t n = strlen(path);
    return n > 4 && (strcmp(path + n - 4, ".zip") == 0 || strcmp(path + n - 4, ".ZIP") == 0);
}

static void scan_dir(mh_romset *rs, const char *path)
{
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *de;
    char full[2048];
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        snprintf(full, sizeof full, "%s/%s", path, de->d_name);
        if (!is_dir(full) && !ends_with_zip(full)) scan_file(rs, full);
    }
    closedir(d);
}

static int count_bits(uint32_t v)
{
    int n = 0;
    for (; v; v &= v - 1) n++;
    return n;
}

int mh_romset_load(mh_romset *rs, const char *const *paths, int npaths)
{
    static mh_romset one;
    uint32_t best = 0;
    int lpc = 0;
    memset(rs, 0, sizeof *rs);
    for (int i = 0; i < npaths; i++) {
        if (!paths[i] || !*paths[i]) continue;
        memset(&one, 0, sizeof one);
        if (is_dir(paths[i])) scan_dir(&one, paths[i]);
        else if (ends_with_zip(paths[i])) scan_zip(&one, paths[i]);
        else continue;
        lpc |= one.lpc;
        if ((one.found & REQUIRED) == REQUIRED) {
            *rs = one;
            snprintf(rs->source, sizeof rs->source, "%s", paths[i]);
            return 0;
        }
        if (count_bits(one.found & REQUIRED) > count_bits(best & REQUIRED)) {
            *rs = one;
            best = one.found;
            snprintf(rs->source, sizeof rs->source, "%s", paths[i]);
        }
    }
    rs->lpc = lpc;
    return -1;
}

const char *mh_romset_missing(const mh_romset *rs, char *buf, int size)
{
    int len = 0;
    buf[0] = '\0';
    for (int r = 0; r < MH_R_COUNT; r++) {
        if (PARTS[r].optional || mh_romset_have(rs, r)) continue;
        len += snprintf(buf + len, len < size ? (size_t)(size - len) : 0, "%s%s", len ? ", " : "", PARTS[r].pos);
        if (len >= size) break;
    }
    return buf;
}

const char *mh_romset_describe(const mh_romset *rs, char *buf, int size)
{
    static const char *const LANG[3] = { "English", "German", "French" };
    uint8_t lang = rs->r1mn[1], major = rs->r1mn[2], minor = rs->r1mn[3];
    snprintf(buf, (size_t)size, "%s, version %u.%02u%s", lang < 3 ? LANG[lang] : "language ?", major, minor,
             major == 0 ? " (beta)" : "");
    return buf;
}

#define MAX_STORE 48
static char g_store[MAX_STORE][1024];
static int  g_nstore;

int mh_romset_add_dir(const char **out, int n, int max, const char *dir)
{
    static const char *const forms[] = { "%s/mhavocpe.zip", "%s" };
    for (int f = 0; f < 2 && n < max && g_nstore < MAX_STORE; f++) {
        snprintf(g_store[g_nstore], sizeof g_store[g_nstore], forms[f], dir);
        out[n++] = g_store[g_nstore++];
    }
    return n;
}

int mh_romset_default_paths(const char **out, int max)
{
    int n = 0;
    static char envcopy[2048];

    const char *env = getenv("MH_ROMS");
    if (env && *env) {
        snprintf(envcopy, sizeof envcopy, "%s", env);
        /* ';' separates on Windows, where a path holds a ':' */
        const char *sep = strchr(envcopy, ';') || (envcopy[1] == ':') ? ";" : ":";
        for (char *p = strtok(envcopy, sep); p && n < max; p = strtok(NULL, sep))
            n = mh_romset_add_dir(out, n, max, p);
    }

    n = mh_romset_add_dir(out, n, max, "roms");
    const char *home = getenv("HOME");
    static char h1[1024], h2[1024];
    if (home) {
        snprintf(h1, sizeof h1, "%s/.advance/rom", home);
        snprintf(h2, sizeof h2, "%s/mame/roms", home);
        n = mh_romset_add_dir(out, n, max, h1);
        n = mh_romset_add_dir(out, n, max, h2);
    }
    n = mh_romset_add_dir(out, n, max, "/home/pi/.advance/rom");
    n = mh_romset_add_dir(out, n, max, "/usr/local/share/advance/rom");
    return n;
}
