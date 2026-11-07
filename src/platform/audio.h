/* audio.h - the sound's samples to the speaker, through the SDK.
 *
 * The game's sound is made as it runs (sound/sound.h), so it goes to the
 * SDK's audio as a stream of its own (vp_stream_play): it is resampled to
 * the device, and on the cabinet it follows the volume set in the
 * VectorPie menu (a stream is at its MUSIC VOLUME). With no sound device
 * the game runs silent.
 */
#ifndef MH_AUDIO_H
#define MH_AUDIO_H

#include "sound/sound.h"

/* Returns 0, or -1 if there is no sound (a message is printed). */
int  mh_audio_open(mh_sound *snd);
void mh_audio_close(void);

/* Once a picture: what the game made since goes to the stream. Kept about
 * 60 ms ahead; more than 150 ms waiting (the device takes samples a little
 * slower than the game makes them) and the oldest are dropped. */
void mh_audio_feed(mh_sound *snd);

#endif
