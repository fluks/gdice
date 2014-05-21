#include <gtk/gtk.h>
#include <glib.h>
#include <stdint.h>

static void
roll_dices(GtkWidget *button, gpointer user_data);
static void
add_dice(GtkWidget *button, gpointer user_data);
static void
remove_dice(GtkWidget *button, gpointer user_data);

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "gtk_dice.glade", NULL);

    gtk_builder_connect_signals(builder, NULL);

    GObject *roll_button = gtk_builder_get_object(builder, "roll_button");
    g_signal_connect(roll_button, G_CALLBACK(roll_dices), builder);

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
roll_dices(GtkWidget *button, gpointer user_data) {
    GtkBuilder *builder = user_data;
    
    int_least64_t result = 0;
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

    GtkWidget *sides = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), sides);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sides), 0);
    gtk_box_pack_start(GTK_BOX(variable_dice), sides, TRUE, TRUE, 0);

    label = gtk_label_new_with_mnemonic("_x");
    gtk_box_pack_start(GTK_BOX(variable_dice), label, FALSE, TRUE, 0);

    GtkWidget *number_rolls = gtk_spin_button_new_with_range(-100, 100, 1);
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
