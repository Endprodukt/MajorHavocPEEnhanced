/* mhshot - run Major Havoc PE without a screen and write its picture as SVG.
 *
 *   mhshot [--roms PATH] [--seconds S] [--play "T WHAT [ARG];..."] [--svg FILE] [--at T,T,...]
 *          [--eeprom FILE] [--test] [--dsw1 HEX] [--dsw2 HEX] [--dump N] [--wav FILE]
 *
 * --play: a script of times (seconds) and actions: coin, coin2, jump,
 * shield, aux (held 0.1 s, or ARG milliseconds), roll N (the roller N counts over the next 0.5 s;
 * negative the other way).
 * --at: the pictures at these times (FILE becomes FILE_T.svg); default
 * the end, to FILE.
 * --eeprom: start from (and keep) this EEPROM image.
 * --test: the self test switch on.
 * --dsw1 HEX, --dsw2 HEX: the option switches (core/machine.h).
 * --dump N: the first N instructions of each picture written, decoded.
 * --wav FILE: the sound (POKEYs and speech, 48 kHz mono) as a WAV file.
 * --speed N: the alpha N times as fast as the cabinet's (core/emu.h).
 * --enhance: the pictures written with the enhancements as standard (render/enhance.h,
 * each snapped to its screen's zoom).
 * --poke "T ADDR VAL;...": write the alpha's RAM byte ADDR (hex) with VAL
 * (hex) from T seconds on, each picture (to try screens).
 * --steady-music: the gamma's interrupt every 8 ticks however late
 * acknowledged (core/machine.h gamma_irq_steady), as the game's standard.
 * --smooth: run ahead as the game does (render/frames.h) and print each
 * second how many of the pictures' lines found a partner in the next.
 * Prints what the CPUs did each second: pictures, interrupts, the gamma's
 * NMIs, both PCs, and the lines of each picture written.
 */
#include "avg/avg.h"
#include "core/emu.h"
#include "core/machine.h"
#include "render/enhance.h"
#include "render/frames.h"
#include "render/svg.h"
#include "rom/romset.h"
#include "sound/sound.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct act { double t; char what[16]; int arg; } act;

static int parse_play(const char *s, act *out, int max)
{
    int n = 0;
    char buf[16384];
    snprintf(buf, sizeof buf, "%s", s);
    for (char *e = strtok(buf, ";"); e && n < max; e = strtok(NULL, ";")) {
        act a;
        memset(&a, 0, sizeof a);
        int k = sscanf(e, "%lf %15s %d", &a.t, a.what, &a.arg);
        if (k >= 2) out[n++] = a;
    }
    return n;
}

static int g_dump;

/* The generator's list as the picture began, instruction by instruction
 * (calls followed). */
static void dump(const mh_machine *m)
{
    static const char *const OP[8] = { "VCTR", "HALT", "SVEC", "STAT", "CNTR", "JSRL", "RTSL", "JMPL" };
    mh_vgmem v;
    mh_machine_snap_mem(m, &v);
    unsigned pc = 0, sp = 0, stack[4] = { 0 }, map = m->snap_state.map;
    for (int n = 0; n < g_dump; n++) {
        unsigned w = mh_vgmem_byte(&v, pc, map) | (mh_vgmem_byte(&v, pc + 1, map) << 8);
        printf("    %04X: %04X %s", pc, w, OP[w >> 13]);
        pc += 2;
        switch (w >> 13) {
        case 0: {
            unsigned w2 = mh_vgmem_byte(&v, pc, map) | (mh_vgmem_byte(&v, pc + 1, map) << 8);
            pc += 2;
            int dy = (int)(w & 0x1fff), dx = (int)(w2 & 0x1fff);
            if (dy & 0x1000) dy -= 0x2000;
            if (dx & 0x1000) dx -= 0x2000;
            printf(" dx %d dy %d z %u", dx, dy, w2 >> 13);
            break;
        }
        case 2: {
            int dy = (int)((w >> 8) & 0x1f), dx = (int)(w & 0x1f);
            if (dy & 0x10) dy -= 0x20;
            if (dx & 0x10) dx -= 0x20;
            printf(" dx %d dy %d z %u", dx * 2, dy * 2, (w >> 5) & 7);
            break;
        }
        case 3:
            if (w & 0x1000) printf(" SCAL bin %u lin %u%s", (w >> 8) & 7, w & 0xff, (w & 0x800) ? " clip" : "");
            else {
                map = (w >> 8) & 3;
                printf(" colour %u intensity %u page %u%s%s", w & 15, (w >> 4) & 15, map, (w & 0x400) ? " xflip" : "",
                       (w & 0x800) ? " sparkle" : "");
            }
            break;
        case 5: stack[sp & 3] = pc; sp++; pc = (w & 0x1fff) << 1; printf(" %04X", pc); break;
        case 6: sp--; pc = stack[sp & 3]; break;
        case 7: pc = (w & 0x1fff) << 1; printf(" %04X", pc); break;
        }
        putchar('\n');
        if ((w >> 13) == 1) break;
    }
}

static int g_enhance;
static mh_enhance g_enh;

static void shot(const mh_machine *m, const char *path)
{
    if (g_dump) dump(m);
    static mh_avg_list l;
    mh_vgmem v;
    mh_machine_snap_mem(m, &v);
    mh_avg_state st = m->snap_state;
    mh_avg_run(&v, m->snap_colram, &st, m->snap_flip_x, m->snap_flip_y, &l, 0);
    if (g_enhance) {
        static mh_config c;
        mh_config_defaults(&c);
        g_enh.have = 0;
        mh_enhance_apply(&g_enh, c.fx, m, &l, 1.0);
        printf("  enhance: story %d/%d zoom %.2f about %.0f,%.0f; ram 9b %02X a1 %02X bd %02X c7 %02X 854 %02X 85b %02X\n",
               g_enh.story_ok, g_enh.story_on, g_enh.zoom, g_enh.cx, g_enh.cy, m->amem[0x9b], m->amem[0xa1], m->amem[0xbd], m->amem[0xc7],
               m->amem[0x854], m->amem[0x85b]);
    }
    if (mh_svg_write(path, l.lines, l.count) != 0) fprintf(stderr, "mhshot: cannot write %s\n", path);
    printf("  %s: picture %u, %d lines, %d instructions%s\n", path, m->snap_starts, l.count, l.instructions,
           l.status == MH_AVG_OK ? "" : " (runaway list)");
}

static void put32(FILE *f, uint32_t v)
{
    uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) };
    fwrite(b, 1, 4, f);
}

static void put16(FILE *f, uint16_t v)
{
    uint8_t b[2] = { (uint8_t)v, (uint8_t)(v >> 8) };
    fwrite(b, 1, 2, f);
}

/* A WAV header for `samples` 16-bit mono samples at MH_SOUND_RATE. */
static void wav_header(FILE *f, uint32_t samples)
{
    fwrite("RIFF", 1, 4, f); put32(f, 36 + samples * 2);
    fwrite("WAVEfmt ", 1, 8, f); put32(f, 16);
    put16(f, 1); put16(f, 1); put32(f, MH_SOUND_RATE); put32(f, MH_SOUND_RATE * 2); put16(f, 2); put16(f, 16);
    fwrite("data", 1, 4, f); put32(f, samples * 2);
}

int main(int argc, char **argv)
{
    const char *roms = NULL, *svg = "mhshot.svg", *play = "", *at = NULL, *eepath = NULL, *wav = NULL;
    double seconds = 10.0;
    const char *poke = "";
    int steady_music = 0;
    int test = 0, dsw1 = -1, dsw2 = -1, speed = 1, smooth = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--roms") && i + 1 < argc) roms = argv[++i];
        else if (!strcmp(argv[i], "--seconds") && i + 1 < argc) seconds = atof(argv[++i]);
        else if (!strcmp(argv[i], "--play") && i + 1 < argc) play = argv[++i];
        else if (!strcmp(argv[i], "--svg") && i + 1 < argc) svg = argv[++i];
        else if (!strcmp(argv[i], "--at") && i + 1 < argc) at = argv[++i];
        else if (!strcmp(argv[i], "--eeprom") && i + 1 < argc) eepath = argv[++i];
        else if (!strcmp(argv[i], "--test")) test = 1;
        else if (!strcmp(argv[i], "--dsw1") && i + 1 < argc) dsw1 = (int)strtol(argv[++i], NULL, 16);
        else if (!strcmp(argv[i], "--dsw2") && i + 1 < argc) dsw2 = (int)strtol(argv[++i], NULL, 16);
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc) g_dump = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--wav") && i + 1 < argc) wav = argv[++i];
        else if (!strcmp(argv[i], "--speed") && i + 1 < argc) speed = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--smooth")) smooth = 1;
        else if (!strcmp(argv[i], "--enhance")) g_enhance = 1;
        else if (!strcmp(argv[i], "--steady-music")) steady_music = 1;
        else if (!strcmp(argv[i], "--poke") && i + 1 < argc) poke = argv[++i];
        else { fprintf(stderr, "mhshot: unknown option %s\n", argv[i]); return 2; }
    }

    const char *paths[64];
    int np = 0;
    if (roms) { np = mh_romset_add_dir(paths, np, 64, roms); paths[np++] = roms; }
    np += mh_romset_default_paths(paths + np, 64 - np);
    static mh_romset rs;
    if (mh_romset_load(&rs, paths, np) != 0) {
        char miss[256];
        fprintf(stderr, "mhshot: ROMs missing: %s%s\n", mh_romset_missing(&rs, miss, sizeof miss),
                rs.lpc ? " (an LPC build was found: not supported)" : "");
        return 1;
    }
    char desc[128];
    printf("mhshot: ROMs from %s: %s\n", rs.source, mh_romset_describe(&rs, desc, sizeof desc));

    static mh_machine m;
    static mh_emu e;
    mh_machine_init(&m);
    mh_roms_in r = { rs.r1mn, rs.r1l, rs.r1q, rs.r1np, rs.r6kl, rs.r6h, rs.r6jk, rs.r9s };
    mh_machine_set_rom(&m, &r);
    if (mh_romset_have(&rs, MH_R_6C)) mh_machine_set_avg_prom(&m, rs.prom);
    if (eepath) {
        FILE *f = fopen(eepath, "rb");
        if (f) {
            if (fread(m.eeprom, 1, sizeof m.eeprom, f) != sizeof m.eeprom) fprintf(stderr, "mhshot: short EEPROM\n");
            fclose(f);
        }
    }
    m.in.self_test = (uint8_t)test;
    m.gamma_irq_steady = (uint8_t)steady_music;
    if (dsw1 >= 0) m.in.dsw1 = (uint8_t)dsw1;
    if (dsw2 >= 0) m.in.dsw2 = (uint8_t)dsw2;

    static mh_sound snd;
    FILE *wf = NULL;
    uint32_t wav_samples = 0;
    if (wav) {
        mh_sound_init(&snd, mh_romset_have(&rs, MH_R_X1) ? rs.x1 : NULL);
        m.sound_write = mh_sound_write;
        m.sound_user = &snd;
        wf = fopen(wav, "wb");
        if (wf) wav_header(wf, 0);
        else fprintf(stderr, "mhshot: cannot write %s\n", wav);
    }
    mh_emu_init(&e, &m);
    mh_emu_set_speed(&e, (uint32_t)speed);
    mh_enhance_init(&g_enh, &m);

    double poke_t[16];
    unsigned poke_a[16], poke_v[16];
    int npk = 0;
    {
        char buf[512];
        snprintf(buf, sizeof buf, "%s", poke);
        for (char *t = strtok(buf, ";"); t && npk < 16; t = strtok(NULL, ";"))
            if (sscanf(t, "%lf %x %x", &poke_t[npk], &poke_a[npk], &poke_v[npk]) == 3) npk++;
    }
    static act acts[512];
    int na = parse_play(play, acts, 512);
    double times[64];
    int nt = 0;
    if (at) {
        char buf[1024];
        snprintf(buf, sizeof buf, "%s", at);
        for (char *t = strtok(buf, ","); t && nt < 64; t = strtok(NULL, ",")) times[nt++] = atof(t);
    }

    const double step = 0.001;
    double roll_pos = 0.0, roll_rate = 0.0, roll_until = 0.0;
    double coin_until[2] = { -1, -1 }, jump_until = -1, shield_until = -1, aux_until = -1;
    int ti = 0;
    uint32_t last_starts = 0;
    static mh_frames fr;
    mh_frames_init(&fr);
    uint64_t lines_seen = 0, lines_paired = 0;
    for (double t = 0.0; t < seconds; t += step) {
        for (int i = 0; i < na; i++) {
            if (acts[i].t < t - step || acts[i].t >= t) continue;
            if (!strcmp(acts[i].what, "coin")) coin_until[0] = t + 0.1;
            else if (!strcmp(acts[i].what, "coin2")) coin_until[1] = t + 0.1;
            else if (!strcmp(acts[i].what, "jump")) jump_until = t + (acts[i].arg > 0 ? acts[i].arg / 1000.0 : 0.1);
            else if (!strcmp(acts[i].what, "shield")) shield_until = t + (acts[i].arg > 0 ? acts[i].arg / 1000.0 : 0.1);
            else if (!strcmp(acts[i].what, "aux")) aux_until = t + (acts[i].arg > 0 ? acts[i].arg / 1000.0 : 0.1);
            else if (!strcmp(acts[i].what, "roll")) { roll_rate = acts[i].arg / 0.5; roll_until = t + 0.5; }
        }
        m.in.coin_l = t < coin_until[0];
        m.in.coin_r = t < coin_until[1];
        m.in.b1[0] = t < jump_until;
        m.in.b2[0] = t < shield_until;
        m.in.diag = t < aux_until;
        if (t < roll_until) roll_pos += roll_rate * step;
        m.in.roller[0] = (uint8_t)(-(long)floor(roll_pos) & 0xff);    /* (as main.c: + is right) */
        for (int i = 0; i < npk; i++)
            if (t >= poke_t[i]) m.amem[poke_a[i] & 0x1fff] = (uint8_t)poke_v[i];
        if (g_enhance) {
            static mh_config c;
            static int once;
            if (!once) { mh_config_defaults(&c); once = 1; }
            mh_enhance_patch(&g_enh, c.fx, &m);
        }
        uint64_t g0 = m.gcycle;
        mh_emu_run(&e, (uint64_t)(step * MH_ALPHA_HZ));
        if (wf) {
            float mono[256];
            mh_sound_run(&snd, (int)(m.gcycle - g0));
            for (int n; (n = mh_sound_read(&snd, mono, 256)) > 0;)
                for (int k = 0; k < n; k++, wav_samples++) put16(wf, (uint16_t)(int16_t)lrintf(mono[k] * 32767.0f));
        }
        if (smooth && mh_frames_check(&fr, &m, &e, 1) && fr.sm.span) {
            lines_seen += (uint64_t)fr.sm.cur.count;
            for (int k = 0; k < fr.sm.cur.count; k++) lines_paired += fr.sm.pair[k] >= 0;
        }
        if (mh_emu_jammed(&e)) {
            printf("mhshot: the %s CPU stopped at %04X after %.3f s\n", e.alpha.jammed ? "alpha" : "gamma",
                   e.alpha.jammed ? e.alpha.pc : e.gamma.pc, t);
            break;
        }
        if (floor(t + step) != floor(t)) {
            static uint64_t last_girqs;
            printf("  %2.0f s: gamma irqs +%llu\n", t + step, (unsigned long long)(e.gamma_irqs - last_girqs));
            last_girqs = e.gamma_irqs;
            printf("  %2.0f s: pictures %u (+%u), alpha irqs %llu, gamma nmis %llu, alpha pc %04X, gamma pc %04X, "
                   "page %u, gamma %s\n", t + step, m.vg_starts, m.vg_starts - last_starts,
                   (unsigned long long)e.alpha_irqs, (unsigned long long)e.gamma_nmis, e.alpha.pc, e.gamma.pc, m.rompg,
                   m.gamma_held ? "held" : "runs");
            last_starts = m.vg_starts;
            if (smooth && lines_seen)
                printf("        smooth: %.1f %% of %llu lines paired\n", 100.0 * (double)lines_paired / (double)lines_seen,
                       (unsigned long long)lines_seen);
            lines_seen = lines_paired = 0;
        }
        while (ti < nt && t + step >= times[ti]) {
            char path[1024];
            const char *dot = strrchr(svg, '.');
            int stem = dot ? (int)(dot - svg) : (int)strlen(svg);
            snprintf(path, sizeof path, "%.*s_%05.1f.svg", stem, svg, times[ti]);
            shot(&m, path);
            ti++;
        }
    }
    if (!nt) shot(&m, svg);
    if (wf) {
        fseek(wf, 0, SEEK_SET);
        wav_header(wf, wav_samples);
        fclose(wf);
        printf("  %s: %.1f s of sound, peak %.2f, %u speech phrases\n", wav, wav_samples / (double)MH_SOUND_RATE,
               snd.peak, snd.oki.phrases);
    }
    if (eepath && m.eeprom_dirty) {
        FILE *f = fopen(eepath, "wb");
        if (f) { fwrite(m.eeprom, 1, sizeof m.eeprom, f); fclose(f); }
    }
    return 0;
}
