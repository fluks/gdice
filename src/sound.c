#include <stddef.h>
#include <stdlib.h>
#include "sound.h"

static gboolean
bus_cb(GstBus *bus, GstMessage *message, gpointer user_data);

sound*
sound_init(int *argc, char ***argv, const gchar *file) {
    gst_init(argc, argv);

    sound *s = g_new(sound, 1);
    s->player = NULL;

    GError *error = NULL;
    gchar *uri = gst_filename_to_uri(file, &error);
    if (uri == NULL) {
        g_printerr("%s\n", error->message);
        g_error_free(error);
        return s;
    }

    s->player = gst_element_factory_make("playbin", "player");
    g_object_set(G_OBJECT(s->player), "uri", uri, NULL);
    g_free(uri);

    GstBus *bus = gst_element_get_bus(s->player);
    gst_bus_add_watch(bus, bus_cb, s->player);
    gst_object_unref(bus);

    return s;
}

void
sound_play(sound *s) {
    if (s->player == NULL)
        return;

    if (GST_STATE(s->player) == GST_STATE_PLAYING)
        return;
    gst_element_set_state(s->player, GST_STATE_PLAYING);
}

void
sound_end(sound *s) {
    if (s->player != NULL) {
        gst_element_set_state(s->player, GST_STATE_NULL);
        gst_object_unref(s->player);
    }
    g_free(s);
}

static gboolean
bus_cb(GstBus *bus, GstMessage *message, gpointer user_data) {
    GstElement *player = user_data;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            gst_element_set_state(player, GST_STATE_PAUSED);
            gst_element_seek_simple(player, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, 0);
            break;
        case GST_MESSAGE_ERROR: {
            GError *error = NULL;
            gchar *debug = NULL;
            gst_message_parse_error(message, &error, &debug);
            g_printerr("Error from element %s: %s\n",
                GST_OBJECT_NAME(message->src), error->message);
            g_error_free(error);
            g_free(debug);
            break;
        }
        default: ;
    }

    return TRUE;
}
