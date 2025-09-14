#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Knapsack 1/0, Bounded y Unbounded
// implementar un dialogo primero que pida la capacidad maxima de la mochila (entre 0 y 20) y la cantidad de objetos a considerar (entre 1 y 10). con boton de aceptar y cargar
// luego despliega la ventana para editar el costo, valor y cantidad disponible de cada objeto (permitiendo el infinito con un *). con boton de ejecutar, guardar y cerrar.

void on_aceptar_clicked(GtkButton *button, gpointer dialog);
void on_entry_insert_quantity(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data);
void on_entry_insert_val(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data);

#define MAX_OBJECTS 10
#define MAX_CAPACITY 20

GtkWidget *object_table;
int backpack_capacity = 0;
int object_count = 0;

// Dialogo inicial para capacidad y objetos
int initial_dialog()
{
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Configuración de la mochila");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    GtkWidget *label1 = gtk_label_new("Capacidad máxima de la mochila (0-20):");
    gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 8);
    GtkWidget *spin_capacity = gtk_spin_button_new_with_range(0, MAX_CAPACITY, 1);
    gtk_box_pack_start(GTK_BOX(vbox), spin_capacity, FALSE, FALSE, 8);

    GtkWidget *label2 = gtk_label_new("Cantidad de objetos (1-10):");
    gtk_box_pack_start(GTK_BOX(vbox), label2, FALSE, FALSE, 8);
    GtkWidget *spin_objects = gtk_spin_button_new_with_range(1, MAX_OBJECTS, 1);
    gtk_box_pack_start(GTK_BOX(vbox), spin_objects, FALSE, FALSE, 8);

    GtkWidget *btn_aceptar = gtk_button_new_with_label("Aceptar");
    gtk_box_pack_start(GTK_BOX(vbox), btn_aceptar, FALSE, FALSE, 8);

    GtkWidget *btn_load = gtk_button_new_with_label("Cargar");
    gtk_box_pack_start(GTK_BOX(vbox), btn_load, FALSE, FALSE, 8);

    int response = -1;
    g_signal_connect(btn_aceptar, "clicked", G_CALLBACK(on_aceptar_clicked), dialog);

    g_object_set_data(G_OBJECT(dialog), "spin_capacity", spin_capacity);
    g_object_set_data(G_OBJECT(dialog), "spin_objects", spin_objects);

    gtk_widget_show_all(dialog);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_ACCEPT)
    {
        GtkWidget *spin1 = g_object_get_data(G_OBJECT(dialog), "spin_capacity");
        GtkWidget *spin2 = g_object_get_data(G_OBJECT(dialog), "spin_objects");
        backpack_capacity = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin1));
        object_count = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin2));
    }
    gtk_widget_destroy(dialog);
    return (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_ACCEPT);
}

void on_aceptar_clicked(GtkButton *button, gpointer dialog)
{
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

// Construye la tabla editable de objetos
void edit_objects()
{
    // Limpia el grid si ya existe
    GList *children = gtk_container_get_children(GTK_CONTAINER(object_table));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    // Encabezados
    const char *headers[] = {"Objeto", "Costo", "Valor", "Cantidad"};
    for (int j = 0; j < 4; j++)
    {
        GtkWidget *lbl = gtk_label_new(headers[j]);
        gtk_grid_attach(GTK_GRID(object_table), lbl, j, 0, 1, 1);
    }

    // Objetos
    for (int i = 0; i < object_count; i++)
    {
        char name[8];
        snprintf(name, sizeof(name), "Obj %d", i + 1);
        GtkWidget *lbl = gtk_label_new(name);
        gtk_grid_attach(GTK_GRID(object_table), lbl, 0, i + 1, 1, 1);

        for (int j = 1; j < 4; j++)
        {
            GtkWidget *entry = gtk_entry_new();
            gtk_grid_attach(GTK_GRID(object_table), entry, j, i + 1, 1, 1);
            if (j == 1 || j == 2) // Costo y Valor
                g_signal_connect(entry, "insert-text", G_CALLBACK(on_entry_insert_val), NULL);
            else if (j == 3) // Cantidad
                g_signal_connect(entry, "insert-text", G_CALLBACK(on_entry_insert_quantity), NULL);
        }
    }
    gtk_widget_show_all(object_table);
}

// Cantidad permite dígitos o * para infinito
void on_entry_insert_quantity(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data)
{
    gboolean valid = TRUE;
    for (int i = 0; i < length; i++){
        char c = text[i];
        if (!g_ascii_isdigit(c) && c != '*') {
            valid = FALSE;
            break;
        }
    }
    if (!valid)
        g_signal_stop_emission_by_name(editable, "insert-text");
}

// Valor y costo solo permite digitos
void on_entry_insert_val(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data)
{
    gboolean valid = TRUE;
    for (int i = 0; i < length; i++){
        if (!g_ascii_isdigit(text[i])){
            valid = FALSE;
            break;
        }
    }
    if (!valid)
        g_signal_stop_emission_by_name(editable, "insert-text");
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new_from_file("Knapsack/knapsack.glade");
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window_knapsack"));
    GtkWidget *btn_run = GTK_WIDGET(gtk_builder_get_object(builder, "btn_run"));
    GtkWidget *btn_close = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));
    GtkWidget *btn_save = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save"));

    // Obtén el grid ya creado en Glade
    object_table = GTK_WIDGET(gtk_builder_get_object(builder, "object_table"));

    // Mostrar dialogo inicial
    if (!initial_dialog())
        return 0;

    // Llena el grid de objetos
    edit_objects();

    g_signal_connect(btn_close, "clicked", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
