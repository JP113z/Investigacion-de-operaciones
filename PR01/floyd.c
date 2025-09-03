#include <gtk/gtk.h>

void btnClose_floyd_clicked(GtkWidget *GtkWidget, gpointer d)
{
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkBuilder *builder;
    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file("floyd.glade");
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_floyd"));

    gtk_builder_add_callback_symbols(builder, "btnClose_floyd_clicked", G_CALLBACK(btnClose_floyd_clicked), NULL);
    gtk_builder_connect_signals(builder, NULL);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
