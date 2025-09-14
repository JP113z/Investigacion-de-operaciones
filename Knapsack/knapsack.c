#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Knapsack 1/0, Bounded y Unbounded
// implementar un dialogo primero que pida la capacidad maxima de la mochila (entre 0 y 20) y la cantidad de objetos a considerar (entre 1 y 10). con boton de aceptar y cargar
// luego despliega la ventana para editar el costo, valor y cantidad disponible de cada objeto (permitiendo el infinito con un *). con boton de ejecutar, guardar y cerrar.

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // cargar interfaz
    GtkBuilder *builder = gtk_builder_new_from_file("Knapsack/knapsack.glade");
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window_knapsack"));
    GtkWidget *btn_run = GTK_WIDGET(gtk_builder_get_object(builder, "btn_run"));
    GtkWidget *btn_close = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));
    GtkWidget *btn_save = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save"));


    
    g_signal_connect(btn_close, "clicked", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
