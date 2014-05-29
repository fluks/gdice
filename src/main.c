#include <gtk/gtk.h>
#include <glib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "diceexpr.h"
#include "config.h"
#include "sound.h"

typedef struct {
    gint sides, number_rolls;
} dice;

typedef struct {
    GtkWindow *window;
    gboolean *is_fullscreen;
} fullscreen_window;

typedef struct {
    GtkBuilder *builder;
    sound *s;
} roll_param;

static void
roll(GtkWidget *button, gpointer user_data);

static void
add_dice(GtkWidget *button, gpointer user_data);

static void
remove_dice(GtkWidget *button, gpointer user_data);

static GList*
get_const_dices(GtkBuilder *builder);

static GList*
get_var_dices(GtkBuilder *builder);

static gint
get_modifier(GtkBuilder *builder);

static const gchar*
get_dice_expression(GtkBuilder *builder);

static gboolean
is_verbose(GtkBuilder *builder);

static void
roll_dices(GList *dices, gint64 *result, GString *result_string);

static void
add_modifier(gint modifier, gint64 *result, GString *result_string);

static void
add_dice_expression(const gchar *expr, gint64 *result, GString *result_string);

static gboolean
validate_dice_expr(GtkWidget *entry, GdkEvent *event, gpointer builder);

static void
set_ui_based_on_dice_expression_validity(GtkWidget *roll_button, GtkWidget *dice_expr,
    gboolean valid_dice_expression);

static void
reset(GtkWidget *button, gpointer user_data);

static void
zero_spinbutton(gpointer data, gpointer user_data);

static void
toggle_fullscreen(GtkMenuItem *item, gpointer user_data);

static gboolean
is_window_fullscreen(GtkWidget *window, GdkEvent *event, gpointer user_data);

static gboolean
sounds_enabled(GtkBuilder *builder);

int
main(int argc, char **argv) {
    // For de_parse().
    srand(time(NULL));

    gtk_init(&argc, &argv);
    sound *s = sound_init(&argc, &argv, CONFIG_DICE_SOUND);

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, CONFIG_UI_DEFINITION_PATH, NULL);

    gtk_builder_connect_signals(builder, NULL);

    GObject *dice_expr = gtk_builder_get_object(builder, "dice_expression");
    g_signal_connect(dice_expr, "key-release-event", G_CALLBACK(validate_dice_expr), builder);

    GObject *roll_button = gtk_builder_get_object(builder, "roll_button");
    roll_param rp = { builder, s };
    g_signal_connect(roll_button, "clicked", G_CALLBACK(roll), &rp);
    gtk_widget_set_can_default(GTK_WIDGET(roll_button), TRUE);

    GObject *reset_button = gtk_builder_get_object(builder, "reset_button");
    g_signal_connect(reset_button, "clicked", G_CALLBACK(reset), builder);

    GObject *add_button = gtk_builder_get_object(builder, "add_button");
    g_signal_connect(GTK_WIDGET(add_button), "clicked", G_CALLBACK(add_dice), builder);

    GObject *window = gtk_builder_get_object(builder, "window");
    gtk_window_set_default(GTK_WINDOW(window), GTK_WIDGET(roll_button));

    gboolean is_fullscreen = FALSE;
    g_signal_connect(window, "window-state-event", G_CALLBACK(is_window_fullscreen), &is_fullscreen);
    GObject *fullscreen = gtk_builder_get_object(builder, "fullscreen");
    fullscreen_window fw = { GTK_WINDOW(window), &is_fullscreen };
    g_signal_connect(fullscreen, "activate", G_CALLBACK(toggle_fullscreen), &fw);

    gtk_widget_show_all(GTK_WIDGET(window));

    gtk_main();

    sound_end(s);
}

/** Roll dices and put result to TextView.
 * @param button Roll button. Not used.
 * @param user_data roll_param struct.
 */
static void
roll(GtkWidget *button, gpointer user_data) {
    roll_param *rp = user_data;
    gint64 result = 0;
    GString *result_string = g_string_new("");

    const gchar *expr = get_dice_expression(rp->builder);
    add_dice_expression(expr, &result, result_string);

    GList *const_dices = get_const_dices(rp->builder);
    roll_dices(const_dices, &result, result_string);

    gint modifier = get_modifier(rp->builder);
    add_modifier(modifier, &result, result_string);

    GList *var_dices = get_var_dices(rp->builder);
    roll_dices(var_dices, &result, result_string);

    if (result_string->len == 0)
        goto clean_up;

    if (sounds_enabled(rp->builder))
        sound_play(rp->s);

    if (*(result_string->str) == '+')
        g_string_erase(result_string, 0, 1);
    g_string_append(result_string, " = ");

    GObject *textview = gtk_builder_get_object(rp->builder, "textview");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    if (is_verbose(rp->builder))
        gtk_text_buffer_insert_at_cursor(buffer, result_string->str, result_string->len);
    g_string_erase(result_string, 0, -1);
    g_string_append_printf(result_string, "%" G_GINT64_FORMAT "\n", result);
    gtk_text_buffer_insert_at_cursor(buffer, result_string->str, result_string->len);

    GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, "insert");
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview), mark, 0, FALSE, 0, 0);

    clean_up:
        g_list_free_full(const_dices, g_free);
        g_list_free_full(var_dices, g_free);
        g_string_free(result_string, TRUE);
}

/** Add a new variable dice.
 * @param button Add button, which was pressed. Not used.
 * @param user_data GtkBuilder object.
 */
static void
add_dice(GtkWidget *button, gpointer user_data) {
    GtkBuilder *builder = user_data;

    GtkWidget *variable_dice = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget *label = gtk_label_new_with_mnemonic("d_N");
    gtk_box_pack_start(GTK_BOX(variable_dice), label, FALSE, TRUE, 0);

    GtkWidget *sides = gtk_spin_button_new_with_range(0, 100000, 1);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), sides);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sides), 0);
    gtk_box_pack_start(GTK_BOX(variable_dice), sides, TRUE, TRUE, 0);

    label = gtk_label_new_with_mnemonic("_x");
    gtk_box_pack_start(GTK_BOX(variable_dice), label, FALSE, TRUE, 0);

    GtkWidget *number_rolls = gtk_spin_button_new_with_range(-100000, 100000, 1);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), number_rolls);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(number_rolls), 0);
    gtk_box_pack_start(GTK_BOX(variable_dice), number_rolls, TRUE, TRUE, 0);

    GtkWidget *remove_button = gtk_button_new_with_mnemonic("_Remove");
    g_signal_connect(remove_button, "clicked", G_CALLBACK(remove_dice), variable_dice);
    gtk_box_pack_start(GTK_BOX(variable_dice), remove_button, FALSE, TRUE, 0);

    GObject *box = gtk_builder_get_object(builder, "variable_dices_box");
    gtk_box_pack_start(GTK_BOX(box), variable_dice, FALSE, TRUE, 0);

    gtk_widget_show_all(variable_dice);
}

/** Remove variable dice.
 * @param button Remove button, which was pressed. Not used.
 * @param user_data GtkBox which contains variable dice to be removed.
 */
static void
remove_dice(GtkWidget *button, gpointer user_data) {
    GtkWidget *w = user_data;
    gtk_widget_destroy(w);
}

/** Get constant dices.
 * Memory of returned list and its elements need to be freed after use.
 * @param builder
 * @return List of dices.
 */
static GList*
get_const_dices(GtkBuilder *builder) {
    GList *dices = NULL;

    GObject *box = gtk_builder_get_object(builder, "const_dices");
    GList *dice_spin_buttons = gtk_container_get_children(GTK_CONTAINER(box));
    gint sides[] = { 4, 6, 8, 10, 12, 20, 100 };
    gsize i = 0;
    for (GList *it = dice_spin_buttons; it != NULL; it = it->next, i++) {
        GtkSpinButton *sp = it->data;
        dice *d = g_new(dice, 1);
        d->sides = sides[i];
        d->number_rolls = gtk_spin_button_get_value_as_int(sp);
        dices = g_list_prepend(dices, d);
    }
    g_list_free(dice_spin_buttons);

    return g_list_reverse(dices);
}

/** Get variable dices.
 * Memory of returned list and its elements need to be freed after use.
 * @param builder
 * @return List of dices.
 */
static GList*
get_var_dices(GtkBuilder *builder) {
    GList *dices = NULL;

    GObject *outer_box = gtk_builder_get_object(builder, "variable_dices_box");
    GList *boxes = gtk_container_get_children(GTK_CONTAINER(outer_box));
    for (GList *it = boxes; it != NULL; it = it->next) {
        GtkContainer *box = it->data;
        GList *children = gtk_container_get_children(box);

        dice *d = g_new(dice, 1);
        GtkSpinButton *sides = g_list_nth_data(children, 1);
        d->sides = gtk_spin_button_get_value_as_int(sides);

        GtkSpinButton *number_rolls = g_list_nth_data(children, 3);
        d->number_rolls = gtk_spin_button_get_value_as_int(number_rolls);

        dices = g_list_prepend(dices, d);

        g_list_free(children);
    }
    g_list_free(boxes);

    return g_list_reverse(dices);
}

/** Get modifier.
 * @param builder
 * @return
 */
static gint
get_modifier(GtkBuilder *builder) {
    GObject *mod = gtk_builder_get_object(builder, "modifier");
    return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mod));
}

/** Get dice expression.
 * @param builder
 * @return Dice expression. Do not free.
 */
static const gchar*
get_dice_expression(GtkBuilder *builder) {
    GObject *expr = gtk_builder_get_object(builder, "dice_expression");
    return gtk_entry_get_text(GTK_ENTRY(expr));
}

/** Get verbosity.
 * If returns true, print values of dices rolled etc. More than just the result.
 * @param builder
 * @return Verbosity.
 */
static gboolean
is_verbose(GtkBuilder *builder) {
    GObject *item = gtk_builder_get_object(builder, "verbose");
    return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
}

/** Roll many dices.
 * @param dices List of dices.
 * @param result
 * @param result_string
 */
static void
roll_dices(GList *dices, gint64 *result, GString *result_string) {
    for (GList *it = dices; it != NULL; it = it->next) {
        dice *d = it->data;
        if (d->sides == 0 || d->number_rolls == 0)
            continue;

        gint64 sum = 0;
        gint sign = d->number_rolls < 0 ? -1 : 1;
        g_string_append_printf(result_string, "%c(", sign < 0 ? '-' : '+');
        for (gint i = 0; i < ABS(d->number_rolls); i++) {
            gint32 roll = g_random_int_range(1, d->sides + 1);
            sum += roll;
            g_string_append_printf(result_string, "%" G_GINT32_FORMAT, roll);
            if (i != ABS(d->number_rolls) - 1)
                g_string_append_c(result_string, '+');
        }
        g_string_append_c(result_string, ')');
        *result += sign * sum;
    }
}

/** Add modifier to results.
 * @param modifier
 * @param result
 * @param result_string
 */
static void
add_modifier(gint modifier, gint64 *result, GString *result_string) {
    if (modifier == 0)
        return;
    *result += modifier;
    g_string_append_printf(result_string, "%+i", modifier);
}

/** Add result of a dice expression to results.
 * @param expr A dice expression. If it's empty string, do nothing.
 * @param result
 * @param result_string
 */
static void
add_dice_expression(const gchar *expr, gint64 *result, GString *result_string) {
    if (g_strcmp0(expr, "") == 0)
        return;

    int_least64_t res = 0;
    char *rolled_expr = NULL;
    // Assume out of memory can't happen and input is validated before.
    enum parse_error error = de_parse(expr, &res, &rolled_expr);
    *result += res;
    const gchar *prefix = "";
    if (*rolled_expr != '+' && *rolled_expr != '-')
        prefix = "+";
    g_string_append_printf(result_string, "%s%s", prefix, rolled_expr);
    free(rolled_expr);
}

/** Validate dice expression.
 * If dice expression is invalid show it to the user and disable roll button.
 * @param entry Dice expression entry.
 * @param event
 * @param user_data GtkBuilder.
 * @return
 */
static gboolean
validate_dice_expr(GtkWidget *entry, GdkEvent *event, gpointer user_data) {
    GtkBuilder *builder = user_data;
    int_least64_t result = 0;
    char *rolled_expr = NULL;

    GObject *roll_button = gtk_builder_get_object(builder, "roll_button");
    const gchar *expr = gtk_entry_get_text(GTK_ENTRY(entry));
    if (g_strcmp0(expr, "") == 0) {
        set_ui_based_on_dice_expression_validity(GTK_WIDGET(roll_button), entry, TRUE);
        return FALSE;
    }

    enum parse_error error = de_parse(expr, &result, &rolled_expr);
    switch (error) {
        // Fallthrough!
        case DE_INVALID_CHARACTER: case DE_SYNTAX_ERROR: case DE_NROLLS:
        case DE_IGNORE: case DE_DICE:
            set_ui_based_on_dice_expression_validity(GTK_WIDGET(roll_button), entry, FALSE);
            break;
        default:
            set_ui_based_on_dice_expression_validity(GTK_WIDGET(roll_button), entry, TRUE);
    }
    free(rolled_expr);

    return FALSE;
}

/** Change widgets to reflect validity of dice expression.
 * @param roll_button
 * @param dice_expr
 * @param valid_dice_expression
 */
static void
set_ui_based_on_dice_expression_validity(GtkWidget *roll_button, GtkWidget *dice_expr,
    gboolean valid_dice_expression) {
    gtk_widget_set_sensitive(GTK_WIDGET(roll_button), valid_dice_expression);
    if (valid_dice_expression)
        gtk_widget_override_background_color(dice_expr, GTK_STATE_FLAG_NORMAL, NULL);
    else {
        const GdkRGBA light_red = { 1.0, 0.0, 0.0, 0.5 };
        gtk_widget_override_background_color(dice_expr, GTK_STATE_FLAG_NORMAL, &light_red);
    }
}

/** Reset.
 * Set every spinbutton's and modifier's value to zero, set dice expression to
 * empty string, remove results from textview and enable roll button.
 * @param button
 * @param user_data GtkBuilder.
 */
static void
reset(GtkWidget *button, gpointer user_data) {
    GtkBuilder *builder = user_data;

    GObject *expr = gtk_builder_get_object(builder, "dice_expression");
    gtk_entry_set_text(GTK_ENTRY(expr), "");

    GObject *roll_button = gtk_builder_get_object(builder, "roll_button");
    set_ui_based_on_dice_expression_validity(GTK_WIDGET(roll_button), GTK_WIDGET(expr), TRUE);

    GObject *modifier = gtk_builder_get_object(builder, "modifier");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(modifier), 0.0);

    GObject *textview = gtk_builder_get_object(builder, "textview");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(buffer, "", -1);

    GObject *box = gtk_builder_get_object(builder, "const_dices");
    GList *const_spin_buttons = gtk_container_get_children(GTK_CONTAINER(box));
    g_list_foreach(const_spin_buttons, zero_spinbutton, NULL);
    g_list_free(const_spin_buttons);

    GObject *outer_box = gtk_builder_get_object(builder, "variable_dices_box");
    GList *boxes = gtk_container_get_children(GTK_CONTAINER(outer_box));
    for (GList *it = boxes; it != NULL; it = it->next) {
        box = it->data;
        GList *children = gtk_container_get_children(GTK_CONTAINER(box));

        GtkSpinButton *sides = g_list_nth_data(children, 1);
        gtk_spin_button_set_value(sides, 0.0);
        GtkSpinButton *number_rolls = g_list_nth_data(children, 3);
        gtk_spin_button_set_value(number_rolls, 0.0);

        g_list_free(children);
    }
    g_list_free(boxes);
}

/** Set spinbutton value to zero.
 * @param data
 * @param user_data
 */
static void
zero_spinbutton(gpointer data, gpointer user_data) {
    GtkSpinButton *sp = data;
    gtk_spin_button_set_value(sp, 0.0);
}

/** Set fullscreen flag of the window.
 * @param window
 * @param event
 * @param user_data Boolean fullscreen flag.
 * @return
 */
static gboolean
is_window_fullscreen(GtkWidget *window, GdkEvent *event, gpointer user_data) {
    gboolean *is_fullscreen = user_data;

    *is_fullscreen = event->window_state.new_window_state & GDK_WINDOW_STATE_FULLSCREEN;

    return FALSE;
}

/** Toggle fullscreen.
 * @param item
 * @param user_data Main GtkWindow.
 */
static void
toggle_fullscreen(GtkMenuItem *item, gpointer user_data) {
    fullscreen_window *fw = user_data;
    if (*(fw->is_fullscreen))
        gtk_window_unfullscreen(GTK_WINDOW(fw->window));
    else
        gtk_window_fullscreen(GTK_WINDOW(fw->window));
}

/** Check if sounds are enabled.
 * @param builder
 * @return True if sounds are enabled, false otherwise.
 */
static gboolean
sounds_enabled(GtkBuilder *builder) {
    GObject *item = gtk_builder_get_object(builder, "sound_checkbox");
    return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
}
