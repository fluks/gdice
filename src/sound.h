#ifndef SOUND_H
    #define SOUND_H

#include <gst/gst.h>
#include <glib.h>

typedef struct {
    GstElement *player;    
} sound;

/** Initialize playing audio.
 * @param argc
 * @param argv
 * @param file Sound file to play.
 * @return A new sound object or NULL if failed to initialize sounds.
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
