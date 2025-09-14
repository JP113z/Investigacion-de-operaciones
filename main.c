#include <gtk/gtk.h>
#include <stdio.h>

void on_btn_clicked(GtkWidget *GtkWidget, gpointer d)
{
    system("./pending &");
}

void on_btn_clicked_floyd(GtkWidget *GtkWidget, gpointer d)
{
    system("./PR01/floyd &");
}

void on_btn_clicked_knapsack(GtkWidget *GtkWidget, gpointer d)
{
    system("./Knapsack/knapsack &");
}

void on_btn_exit_clicked(GtkWidget *GtkWidget, gpointer d)
{
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *label1;
    GtkBuilder *builder;
    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file("main.glade");
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

    gtk_builder_add_callback_symbols(
        builder,
        "on_btn_exit_clicked", G_CALLBACK(on_btn_exit_clicked),
        "on_btn_clicked", G_CALLBACK(on_btn_clicked),
        "on_btn_clicked_floyd", G_CALLBACK(on_btn_clicked_floyd),
        "on_btn_clicked_knapsack", G_CALLBACK(on_btn_clicked_knapsack),
        NULL);

    gtk_builder_connect_signals(builder, NULL);

    label1 = GTK_WIDGET(gtk_builder_get_object(builder, "label1"));

    gtk_widget_show(window);
    gtk_main();
    return EXIT_SUCCESS;
}
