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

    if ((s->player = gst_element_factory_make("playbin", "player")) == NULL) {
        g_printerr("Can't create playbin element.\n");
        goto clean_up;
    }
    g_object_set(G_OBJECT(s->player), "uri", uri, NULL);

    GstBus *bus = gst_element_get_bus(s->player);
    gst_bus_add_watch(bus, bus_cb, s->player);
    gst_object_unref(bus);

    clean_up:
        g_free(uri);

    return s;
}

void
sound_play(sound *s) {
    if (s->player == NULL)
        return;

    if (GST_STATE(s->player) == GST_STATE_PLAYING)
        return;
    if (gst_element_set_state(s->player, GST_STATE_PLAYING) ==
            GST_STATE_CHANGE_FAILURE)
        g_printerr("Failed to set playbin's state to playing.\n");
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
            if (gst_element_set_state(player, GST_STATE_PAUSED) ==
                    GST_STATE_CHANGE_FAILURE)
                g_printerr("Failed to set playbin's state to paused.\n");
            if (!gst_element_seek_simple(player, GST_FORMAT_TIME,
                    GST_SEEK_FLAG_FLUSH, 0))
                g_printerr("Failed to seek to start of stream.\n");

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
