#include "platform/audio.h"

#include <vectorpie/vp_audio.h>
#include <vectorpie/vp_error.h>

#include <stdio.h>

#define MAX_QUEUED (MH_SOUND_RATE * 15 / 100)   /* 150 ms */

static vp_audio *g_audio;
static vp_voice g_stream;

int mh_audio_open(mh_sound *snd)
{
    snd->ring_r = snd->ring_w;
    if (vp_audio_open(&g_audio) != VP_OK || !g_audio) {
        fprintf(stderr, "mhavocpe: no sound: %s\n", vp_last_error());
        g_audio = NULL;
        return -1;
    }
    g_stream = vp_stream_play(g_audio, MH_SOUND_RATE, 1.0f);
    if (!g_stream) {
        fprintf(stderr, "mhavocpe: no sound stream\n");
        vp_audio_close(g_audio);
        g_audio = NULL;
        return -1;
    }
    /* 60 ms of silence ahead: the game's samples come a picture at a time */
    static const float quiet[2 * 960] = { 0 };
    for (int k = 0; k < 3; k++) vp_stream_write(g_audio, g_stream, quiet, 960);
    return 0;
}

void mh_audio_close(void)
{
    if (g_audio) vp_audio_close(g_audio);
    g_audio = NULL;
    g_stream = 0;
}

void mh_audio_feed(mh_sound *snd)
{
    if (!g_audio) {
        snd->ring_r = snd->ring_w;
        return;
    }
    /* behind: drop the oldest of what waits here */
    int queued = vp_stream_queued(g_audio, g_stream);
    uint32_t waiting = snd->ring_w - snd->ring_r;
    if (queued + (int)waiting > MAX_QUEUED) {
        uint32_t drop = (uint32_t)(queued + (int)waiting - MAX_QUEUED);
        if (drop > waiting) drop = waiting;
        snd->ring_r += drop;
    }
    float mono[512], frames[1024];
    for (int n; (n = mh_sound_read(snd, mono, 512)) > 0;) {
        for (int i = 0; i < n; i++) frames[2 * i] = frames[2 * i + 1] = mono[i];
        vp_stream_write(g_audio, g_stream, frames, n);
    }
}
