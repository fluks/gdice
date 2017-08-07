#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "diceexpr.h"
#include "config.h"
#include "sound.h"
#include "numflow.h"

typedef struct {
    gint sides, number_rolls;
} dice;

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

static gboolean
roll_dices(GList *dices, int_least64_t *result, GString *result_string, GString *error);

static gboolean
add_modifier(gint modifier, int_least64_t *result, GString *result_string, GString *error);

static gboolean
add_dice_expression(const gchar *expr, int_least64_t *result, GString *result_string,
    GString *error);

static gboolean
validate_dice_expr(GtkWidget *entry, GdkEvent *event, gpointer builder);

static void
set_ui_based_on_dice_expression_validity(GtkWidget *roll_button, GtkWidget *dice_expr,
    gboolean valid_dice_expression);

static void
reset(GtkWidget *button, gpointer user_data);

static void
zero_spinbutton(gpointer data, gpointer user_data);

static gboolean
sounds_enabled(GtkBuilder *builder);

static void
minimize_window(GtkContainer *container, GtkWidget *widget, gpointer user_data);

static void
form_result_string(GString *s, int_least64_t result, GtkBuilder *builder);

static void
insert_string_to_buffer(GString *s, GtkBuilder *builder);

static void
set_window_icon(GtkWindow *window);

static void
load_css();

static void
set_widgets_same_size(GtkBuilder *builder, const gchar *src, const gchar *dst);

static void
add_dice_expr_completion(GtkEntry *entry);

static void
append_dice_expr_completion(GtkEntry *entry);

static void
load_preferences(GtkBuilder *builder);

static void
show_about_window(GtkMenuItem *menuitem, gpointer user_data);

static void
show_help_window(GtkMenuItem *menuitem, gpointer user_data);

static void
connect_help_window_signals(GtkBuilder *builder);

int
main(int argc, char **argv) {
    bindtextdomain(GETTEXT_PACKAGE, PROGRAMNAME_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    // For de_parse().
    srand(time(NULL));

    gtk_init(&argc, &argv);
    sound *s = sound_init(&argc, &argv, RESDIR "dices.ogg");

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, RESDIR "gdice.glade", NULL);

    gtk_builder_connect_signals(builder, NULL);

    GObject *dice_expr = gtk_builder_get_object(builder, "dice_expression");
    g_signal_connect(dice_expr, "key-release-event", G_CALLBACK(validate_dice_expr), builder);

    add_dice_expr_completion(GTK_ENTRY(dice_expr));

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
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    set_window_icon(GTK_WINDOW(window));

    GObject *variable_dices_box = gtk_builder_get_object(builder, "variable_dices_box");
    g_signal_connect(variable_dices_box, "remove", G_CALLBACK(minimize_window), window);

    GObject *about_menuitem = gtk_builder_get_object(builder, "about_menuitem");
    g_signal_connect(about_menuitem, "activate", G_CALLBACK(show_about_window), builder);

    connect_help_window_signals(builder);

    load_css();

    load_preferences(builder);

    gtk_widget_show_all(GTK_WIDGET(window));

    set_widgets_same_size(builder, "dice_expression_label", "dN");
#ifndef HAVE_GSTREAMER
    GObject *sound_checkbox = gtk_builder_get_object(builder, "sound_checkbox");
    gtk_widget_hide(GTK_WIDGET(sound_checkbox));
#endif

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
    int_least64_t result = 0;
    GString *result_string = g_string_new("");
    GString *error = g_string_new("");
    GList *const_dices = NULL, *var_dices = NULL;

    const gchar *expr = get_dice_expression(rp->builder);
    if (!add_dice_expression(expr, &result, result_string, error))
        goto error;

    const_dices = get_const_dices(rp->builder);
    if (!roll_dices(const_dices, &result, result_string, error))
        goto error;

    gint modifier = get_modifier(rp->builder);
    if (!add_modifier(modifier, &result, result_string, error))
        goto error;

    var_dices = get_var_dices(rp->builder);
    if (!roll_dices(var_dices, &result, result_string, error))
        goto error;

    /* No input. */
    if (result_string->len == 0)
        goto clean_up;

    append_dice_expr_completion(
        GTK_ENTRY(gtk_builder_get_object(rp->builder, "dice_expression")));

    if (sounds_enabled(rp->builder))
        sound_play(rp->s);

    form_result_string(result_string, result, rp->builder);
    insert_string_to_buffer(result_string, rp->builder);
    goto clean_up;

    error:
        insert_string_to_buffer(error, rp->builder);

    clean_up:
        if (const_dices != NULL)
            g_list_free_full(const_dices, g_free);
        if (var_dices != NULL)
            g_list_free_full(var_dices, g_free);
        g_string_free(result_string, TRUE);
        g_string_free(error, TRUE);
}

/** Form the result string, verbose or just the integer.
 * @param s Result string.
 * @param result Integer result.
 * @param builder
 */
static void
form_result_string(GString *s, int_least64_t result, GtkBuilder *builder) {
    if (is_verbose(builder)) {
        if (*(s->str) == '+')
            g_string_erase(s, 0, 1);
        g_string_append(s, " = ");
    }
    else
        g_string_erase(s, 0, -1);

    g_string_append_printf(s, "%" PRIdLEAST64 "\n", result);
}

/** Insert result string to the textview and scroll to the end of the textview.
 * @param s Result string.
 * @param builder
 */
static void
insert_string_to_buffer(GString *s, GtkBuilder *builder) {
    GObject *textview = gtk_builder_get_object(builder, "textview");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_insert_at_cursor(buffer, s->str, s->len);

    GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, "insert");
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview), mark, 0, FALSE, 0, 0);
}

/** Add a new variable dice.
 * @param button Add button, which was pressed. Not used.
 * @param user_data GtkBuilder object.
 */
static void
add_dice(GtkWidget *button, gpointer user_data) {
    GtkBuilder *builder = user_data;

    GtkWidget *variable_dice = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GObject *dN = gtk_builder_get_object(builder, "dN");
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(dN), &alloc);
    GtkWidget *label = gtk_label_new_with_mnemonic("d_N");
    gtk_widget_set_size_request(label, alloc.width, alloc.height);
    gtk_box_pack_start(GTK_BOX(variable_dice), label, FALSE, TRUE, 0);

    GtkWidget *sides = gtk_spin_button_new_with_range(0, 100000, 1);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), sides);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sides), 0);
    gtk_widget_set_tooltip_text(sides, _("Number of sides"));
    gtk_entry_set_activates_default(GTK_ENTRY(sides), TRUE);
    gtk_box_pack_start(GTK_BOX(variable_dice), sides, TRUE, TRUE, 0);

    label = gtk_label_new_with_mnemonic("_x");
    gtk_box_pack_start(GTK_BOX(variable_dice), label, FALSE, TRUE, 0);

    GtkWidget *number_rolls = gtk_spin_button_new_with_range(-100000, 100000, 1);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), number_rolls);
    gtk_widget_set_tooltip_text(number_rolls, _("Number of rolls"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(number_rolls), 0);
    gtk_entry_set_activates_default(GTK_ENTRY(number_rolls), TRUE);
    gtk_box_pack_start(GTK_BOX(variable_dice), number_rolls, TRUE, TRUE, 0);

    GtkWidget *remove_button = gtk_button_new();
    GtkWidget *remove_image = gtk_image_new_from_file(RESDIR "remove_12x12.svg");
    gtk_button_set_image(GTK_BUTTON(remove_button), remove_image);
    g_signal_connect(remove_button, "clicked", G_CALLBACK(remove_dice), variable_dice);
    gtk_widget_set_can_focus(remove_button, FALSE);
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
 * @param error
 * @return FALSE if integer overflows, TRUE otherwise.
 */
static gboolean
roll_dices(GList *dices, int_least64_t *result, GString *result_string, GString *error) {
    enum flow_type overflow;
    for (GList *it = dices; it != NULL; it = it->next) {
        dice *d = it->data;
        if (d->sides == 0 || d->number_rolls == 0)
            continue;

        int_least64_t sum = 0;
        gint sign = d->number_rolls < 0 ? -1 : 1;
        g_string_append_printf(result_string, "%c(", sign < 0 ? '-' : '+');
        for (gint i = 0; i < ABS(d->number_rolls); i++) {
            gint32 roll = g_random_int_range(1, d->sides + 1);
            NF_PLUS(sum, roll, INT_LEAST64, overflow);
            if (overflow != 0)
                goto integer_overflow;
            sum += roll;
            g_string_append_printf(result_string, "%" G_GINT32_FORMAT, roll);
            if (i != ABS(d->number_rolls) - 1)
                g_string_append_c(result_string, '+');
        }
        g_string_append_c(result_string, ')');
        NF_MULTIPLY(sum, sign, INT_LEAST64, overflow);
        if (overflow != 0)
            goto integer_overflow;
        sum *= sign;
        NF_PLUS(*result, sum, INT_LEAST64, overflow);
        if (overflow != 0)
            goto integer_overflow;
        *result += sum;
    }

    return TRUE;

    integer_overflow:
        g_string_append(error, _("integer overflow\n"));
        return FALSE;
}

/** Add modifier to results.
 * @param modifier
 * @param result
 * @param result_string
 * @param error
 * @return FALSE if integer overflows, TRUE otherwise.
 */
static gboolean
add_modifier(gint modifier, int_least64_t *result, GString *result_string, GString *error) {
    if (modifier == 0)
        return TRUE;

    enum flow_type overflow;
    NF_PLUS(*result, modifier, INT_LEAST64, overflow);
    if (overflow != 0) {
        g_string_append(error, _("integer overflow\n"));
        return FALSE;
    }

    *result += modifier;
    g_string_append_printf(result_string, "%+i", modifier);

    return TRUE;
}

/** Add result of a dice expression to results.
 * @param expr A dice expression. If it's empty string, do nothing.
 * @param result
 * @param result_string
 * @param error
 * @return TRUE if nothing failed or no input, FALSE otherwise.
 */
static gboolean
add_dice_expression(const gchar *expr, int_least64_t *result, GString *result_string,
    GString *error) {
    if (g_strcmp0(expr, "") == 0)
        return TRUE;

    int_least64_t res = 0;
    char *rolled_expr = NULL;
    enum parse_error e = de_parse(expr, &res, &rolled_expr);
    /* Overflow and syntax errors should be caught in the validator function, but
     * because that is called on key-release-event, a roll button press can be
     * registered if pressed very quickly before the roll button is disabled.
     */
    switch (e) {
        /* Fallthrough! */
        case DE_INVALID_CHARACTER : case DE_SYNTAX_ERROR : case DE_NROLLS :
        case DE_IGNORE : case DE_DICE: case DE_ROLLS_TOO_LARGE:
            g_string_assign(error, _("syntax error\n"));
            return FALSE;
        case DE_MEMORY:
            g_printerr("Out of memory\n");
            abort();
        case DE_OVERFLOW:
            g_string_assign(error, _("integer overflow\n"));
            return FALSE;
        default:
            *result = res;
            g_string_append(result_string, rolled_expr);
            free(rolled_expr);
            return TRUE;
    }
}

/** Validate dice expression.
 * If dice expression is invalid show it to the user and disable roll button.
 * @param entry Dice expression entry.
 * @param event
 * @param user_data GtkBuilder.
 * @return TRUE to stop other handlers for the event, FALSE otherwise.
 */
static gboolean
validate_dice_expr(GtkWidget *entry, GdkEvent *event, gpointer user_data) {
    GtkBuilder *builder = user_data;

    GObject *roll_button = gtk_builder_get_object(builder, "roll_button");
    const gchar *expr = gtk_entry_get_text(GTK_ENTRY(entry));
    if (g_strcmp0(expr, "") == 0) {
        set_ui_based_on_dice_expression_validity(GTK_WIDGET(roll_button), entry, TRUE);
        return FALSE;
    }

    int_least64_t result = 0;
    char *rolled_expr = NULL;
    enum parse_error e = de_parse(expr, &result, &rolled_expr);
    switch (e) {
        /* Don't catch DE_OVERFLOW here because same expression can sometimes
         * result to overflow and others not.
         */
        /* Fallthrough! */
        case DE_INVALID_CHARACTER: case DE_SYNTAX_ERROR: case DE_NROLLS:
        case DE_IGNORE: case DE_DICE: case DE_ROLLS_TOO_LARGE:
            set_ui_based_on_dice_expression_validity(GTK_WIDGET(roll_button), entry, FALSE);
            break;
        /* TODO Maybe just free rolled_expr, but then what? */
        case DE_MEMORY:
            g_printerr("Out of memory\n");
            abort();
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
set_ui_based_on_dice_expression_validity(GtkWidget *roll_button,
        GtkWidget *dice_expr, gboolean valid_dice_expression) {
    gtk_widget_set_sensitive(roll_button, valid_dice_expression);
    GtkStyleContext *ctx = gtk_widget_get_style_context(dice_expr);
    if (valid_dice_expression) {
        gtk_style_context_remove_class(ctx, "invalid");
        gtk_style_context_add_class(ctx, "valid");
    }
    else {
        gtk_style_context_remove_class(ctx, "valid");
        gtk_style_context_add_class(ctx, "invalid");
    }
}

/** Reset.
 * Set every spinbutton's and modifier's value to zero, set dice expression to
 * empty string and clear its completed dice expressions, remove results
 * from textview and enable roll button.
 * @param button
 * @param user_data GtkBuilder.
 */
static void
reset(GtkWidget *button, gpointer user_data) {
    GtkBuilder *builder = user_data;

    GObject *expr = gtk_builder_get_object(builder, "dice_expression");
    gtk_entry_set_text(GTK_ENTRY(expr), "");
    GtkEntryCompletion *completion = gtk_entry_get_completion(GTK_ENTRY(expr));
    GtkTreeModel *model = gtk_entry_completion_get_model(completion);
    gtk_list_store_clear(GTK_LIST_STORE(model));

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

/** Check if sounds are enabled.
 * @param builder
 * @return True if sounds are enabled, false otherwise.
 */
static gboolean
sounds_enabled(GtkBuilder *builder) {
    GObject *item = gtk_builder_get_object(builder, "sound_checkbox");
    return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
}

/** Set size of the main window as small as possible to show all widgets.
 * @param container Not used.
 * @param widget Not used.
 * @param user_data Main window. GtkWindow.
 */
static void
minimize_window(GtkContainer *container, GtkWidget *widget, gpointer user_data) {
    GtkWindow *window = user_data;
    gtk_window_resize(window, 1, 1);
}

/** Set icon for the main window.
 * @param window
 */
static void
set_window_icon(GtkWindow *window) {
    GError *error = NULL;
    GdkPixbuf *icon_buf = gdk_pixbuf_new_from_file(RESDIR "gdice.svg", &error);
    if (icon_buf) {
        gtk_window_set_icon(window, icon_buf);
        g_object_unref(icon_buf);
    }
    else {
        g_printerr("Can't load icon: %s\n", error->message);
        g_error_free(error);
    }
}

/** Load css.
 */
static void
load_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;
    gtk_css_provider_load_from_path(provider, RESDIR "gdice.css", &error);
    if (!error) {
        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER);
        g_object_unref(provider);
    }
    else {
        g_printerr("%s\n", error->message);
        g_error_free(error);
    }
}

/** Resize dst widget as same size as src. This function needs to be called
 * after gtk_widget_show_all(), otherwise dst won't be resized.
 * @param builder
 * @param src Source widget. dst will be resized to this size.
 * @param dst Destination widget which will be resized.
 */
static void
set_widgets_same_size(GtkBuilder *builder, const gchar *src, const gchar *dst) {
    GObject *s = gtk_builder_get_object(builder, src);
    if (!s)
        return;
    GObject *d = gtk_builder_get_object(builder, dst);
    if (!d)
        return;
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(s), &alloc);
    gtk_widget_set_size_request(GTK_WIDGET(d), alloc.width, alloc.height);
}

/** Add completion for dice expression entry.
 * @param entry
 */
static void
add_dice_expr_completion(GtkEntry *entry) {
    GtkEntryCompletion *completion = gtk_entry_completion_new();
    gtk_entry_completion_set_model(completion,
        GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING)));
    gtk_entry_completion_set_popup_completion(completion, TRUE);
    gtk_entry_completion_set_popup_single_match(completion, FALSE);
    gtk_entry_completion_set_inline_completion(completion, TRUE);
    gtk_entry_completion_set_text_column(completion, 0);
    gtk_entry_completion_set_minimum_key_length(completion, 1);
    gtk_entry_set_completion(entry, completion);
    g_object_unref(completion);
}

/** Append the dice expression string to the completion list, if it's not
 * in it already.
 * @param entry
 */
static void
append_dice_expr_completion(GtkEntry *entry) {
    GtkEntryCompletion *completion = gtk_entry_get_completion(entry);
    GtkTreeModel *model = gtk_entry_completion_get_model(completion);
    const gchar *dice_expr = gtk_entry_get_text(entry);

    gboolean dice_expr_in_list = FALSE;
    GtkTreeIter iter;
    for (gboolean has_next = gtk_tree_model_get_iter_first(model, &iter);
            has_next;
            has_next = gtk_tree_model_iter_next(model, &iter)) {
        GValue value = G_VALUE_INIT;
        gtk_tree_model_get_value(model, &iter, 0, &value);
        const gchar *list_text = g_value_get_string(&value);
        dice_expr_in_list = g_str_equal(dice_expr, list_text);
        g_value_unset(&value);
        if (dice_expr_in_list)
            break;
    }
    if (!dice_expr_in_list) {
        GtkTreeIter i;
        gtk_list_store_append(GTK_LIST_STORE(model), &i);
        gtk_list_store_set(GTK_LIST_STORE(model), &i, 0, dice_expr, -1);
    }
}

/** Load preferences and bind them to the GUI.
 */
static void
load_preferences(GtkBuilder *builder) {
    GSettings *settings = g_settings_new("com.github.fluks.GDice");
    GObject *object = gtk_builder_get_object(builder, "sound_checkbox");
    g_settings_bind(settings, "sound", object, "active", G_SETTINGS_BIND_DEFAULT);
    object = gtk_builder_get_object(builder, "verbose");
    g_settings_bind(settings, "verbose", object, "active", G_SETTINGS_BIND_DEFAULT);
}

/** Show the about window.
 * @param menuitem Not used.
 * @param user_data GtkBuilder.
 */
static void
show_about_window(GtkMenuItem *menuitem, gpointer user_data) {
    GtkBuilder *builder = user_data;
    GObject *about_window = gtk_builder_get_object(builder, "about_window");
    gint response_id = gtk_dialog_run(GTK_DIALOG(about_window));

    if (response_id == GTK_RESPONSE_DELETE_EVENT)
        gtk_widget_hide(GTK_WIDGET(about_window));
}

/**
 * Show the help window.
 * @param menuitem Not used.
 * @param user_data GtkBuilder.
 */
static void
show_help_window(GtkMenuItem *menuitem, gpointer user_data) {
    GtkBuilder *builder = user_data;

    GObject *help_window = gtk_builder_get_object(builder, "help_window");
    GtkButton *close_button = GTK_BUTTON(gtk_builder_get_object(builder, "help_window_close_button"));
    g_signal_connect_swapped(close_button, "clicked", G_CALLBACK(gtk_widget_hide), GTK_WIDGET(help_window));
    gtk_window_present(GTK_WINDOW(help_window));
}

/**
 * Connect signals related to the help window.
 * @param builder GtkBuilder.
 */
static void
connect_help_window_signals(GtkBuilder *builder) {
    GtkWidget *about_menuitem = GTK_WIDGET(gtk_builder_get_object(builder, "help_menuitem"));
    g_signal_connect(about_menuitem, "activate", G_CALLBACK(show_help_window), builder);
}
