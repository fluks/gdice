#ifndef SOUND_H
    #define SOUND_H

#include "config.h"
#ifdef HAVE_GSTREAMER
    #include <gst/gst.h>
#endif
#include <glib.h>

typedef struct {
#ifdef HAVE_GSTREAMER
    GstElement *player;    
#else
    void *not_used;
#endif
} sound;

/** Initialize playing audio.
 * @param argc
 * @param argv
 * @param file Sound file to play.
 * @return sound object.
 */
sound*
sound_init(int *argc, char ***argv, const gchar *file);

/** Play a sound if not already playing.
 * @param s
 */
void
sound_play(sound *s);

/** Free sound resources.
 * @param s
 */
void
sound_end(sound *s);

#endif // SOUND_H
