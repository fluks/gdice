#include <gtk/gtk.h>
#include <glib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    gint sides, number_rolls;
} dice;

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

int
main(int argc, char **argv) {
    // For de_parse().
    srand(time(NULL));

    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "res/gdice.glade", NULL);

    gtk_builder_connect_signals(builder, NULL);

    GObject *roll_button = gtk_builder_get_object(builder, "roll_button");
    g_signal_connect(roll_button, "clicked", G_CALLBACK(roll), builder);

    GObject *add_button = gtk_builder_get_object(builder, "add_button");
    g_signal_connect(GTK_WIDGET(add_button), "clicked", G_CALLBACK(add_dice), builder);

    GObject *window = gtk_builder_get_object(builder, "window");
    gtk_widget_show_all(GTK_WIDGET(window));

    gtk_main();
}

/** Roll dices and put result to TextView.
 * @param button Roll button. Not used.
 * @param user_data GtkBuilder object.
 */
static void
roll(GtkWidget *button, gpointer user_data) {
    GtkBuilder *builder = user_data;
    
    gint64 result = 0;
    GString *result_string = g_string_new("");

    //const gchar *expr = get_dice_expression(builder);

    GList *const_dices = get_const_dices(builder);
    roll_dices(const_dices, &result, result_string);
    g_list_free_full(const_dices, g_free);

    gint modifier = get_modifier(builder);
    add_modifier(modifier, &result, result_string);

    GList *var_dices = get_var_dices(builder);
    roll_dices(var_dices, &result, result_string);
    g_list_free_full(var_dices, g_free);

    g_string_append(result_string, " = ");

    GObject *textview = gtk_builder_get_object(builder, "textview");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    if (is_verbose(builder))
        gtk_text_buffer_insert_at_cursor(buffer, result_string->str, result_string->len);
    g_string_erase(result_string, 0, -1);
    g_string_append_printf(result_string, "%" G_GINT64_FORMAT "\n", result);
    gtk_text_buffer_insert_at_cursor(buffer, result_string->str, result_string->len);
    g_string_free(result_string, TRUE);

    //GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, NULL, "underline", PANGO_UNDERLINE_SINGLE,
        //NULL);
    //GtkTextIter start, *end;
    //gtk_text_buffer_get_end_iter(buffer, &start);
    //end = gtk_text_iter_copy(&start);
    //gtk_text_iter_backward_chars(&start, 2);
    //gtk_text_buffer_apply_tag(buffer, tag, &start, end);

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

static void
add_modifier(gint modifier, gint64 *result, GString *result_string) {
    if (modifier == 0)
        return;
    *result += modifier;
    g_string_append_printf(result_string, "%+i", modifier);
}
