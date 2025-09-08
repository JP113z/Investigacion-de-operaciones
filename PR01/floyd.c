
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 8
#define INF 1000000000

GtkWidget *grid_table;
GtkWidget *name_row[MAXN], *name_col[MAXN];
GtkWidget *entry_matrix[MAXN][MAXN];
int current_n = 0;

gboolean updating = FALSE;

void on_header_changed(GtkEditable *editable, gpointer index_ptr)
{
    if (updating)
        return;
    updating = TRUE;

    int idx = GPOINTER_TO_INT(index_ptr);
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));

    if (editable == GTK_EDITABLE(name_row[idx]))
    {
        gtk_entry_set_text(GTK_ENTRY(name_col[idx]), text);
    }
    else if (editable == GTK_EDITABLE(name_col[idx]))
    {
        gtk_entry_set_text(GTK_ENTRY(name_row[idx]), text);
    }

    updating = FALSE;
}

// crear el grid en interfaz
void build_table()
{
    // limpiar grid
    GList *children = gtk_container_get_children(GTK_CONTAINER(grid_table));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    // nombres por defecto A, B, C...
    for (int j = 0; j < current_n; j++)
    {
        GtkWidget *entry = gtk_entry_new();
        char buf[2] = {'A' + j, '\0'};
        gtk_entry_set_text(GTK_ENTRY(entry), buf);
        gtk_grid_attach(GTK_GRID(grid_table), entry, j + 1, 0, 1, 1);
        name_col[j] = entry;
    }

    for (int i = 0; i < current_n; i++)
    {
        GtkWidget *entry = gtk_entry_new();
        char buf[2] = {'A' + i, '\0'};
        gtk_entry_set_text(GTK_ENTRY(entry), buf);
        gtk_grid_attach(GTK_GRID(grid_table), entry, 0, i + 1, 1, 1);
        name_row[i] = entry;

        // 🔹 Conectar señales de sincronización
        g_signal_connect(entry, "changed", G_CALLBACK(on_header_changed), GINT_TO_POINTER(i));
        g_signal_connect(name_col[i], "changed", G_CALLBACK(on_header_changed), GINT_TO_POINTER(i));

        for (int j = 0; j < current_n; j++)
        {
            GtkWidget *e = gtk_entry_new();
            if (i == j)
            {
                gtk_entry_set_text(GTK_ENTRY(e), "0");
                gtk_editable_set_editable(GTK_EDITABLE(e), FALSE); // 🔹 Diagonal no editable
            }
            else
            {
                gtk_entry_set_text(GTK_ENTRY(e), "INF");
            }
            gtk_grid_attach(GTK_GRID(grid_table), e, j + 1, i + 1, 1, 1);
            entry_matrix[i][j] = e;
        }
    }

    gtk_widget_show_all(grid_table);
}

/* --- Parser: "INF" o número --- */
int parse_val(const char *txt)
{
    if (!txt)
        return INF;
    if (strcasecmp(txt, "INF") == 0 || strlen(txt) == 0)
        return INF;
    return atoi(txt);
}

/// Ejecutar floyd e imprimir tablas en consola
void run_floyd(GtkWidget *btn, gpointer data)
{
    int n = current_n;
    int D[MAXN][MAXN], P[MAXN][MAXN];

    // leer entradas
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            const char *txt = gtk_entry_get_text(GTK_ENTRY(entry_matrix[i][j]));
            D[i][j] = parse_val(txt);
            P[i][j] = -1;
        }
    }

    // imprimir D(0)
    printf("D(0):\n");
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (D[i][j] >= INF)
                printf("INF ");
            else
                printf("%3d ", D[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    // Floyd
    for (int k = 0; k < n; k++)
    {
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                if (D[i][k] < INF && D[k][j] < INF && D[i][j] > D[i][k] + D[k][j])
                {
                    D[i][j] = D[i][k] + D[k][j];
                    P[i][j] = k;
                }
            }
        }

        // imprimir tablas en consola
        printf("D(%d):\n", k + 1);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                if (D[i][j] >= INF)
                    printf("INF ");
                else
                    printf("%3d ", D[i][j]);
            }
            printf("\n");
        }
        printf("\n");

        printf("P(%d):\n", k + 1);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                if (P[i][j] == -1)
                    printf(" -  ");
                else
                    printf("%3d ", P[i][j]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void on_aceptar_clicked(GtkButton *button, gpointer dialog)
{
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

// Diálogo para preguntar cantidad de nodos
int ask_nodes_dialog()
{
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Cantidad de nodos");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    GtkWidget *label = gtk_label_new("Ingrese cantidad de nodos (1-8):");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);

    GtkWidget *spin = gtk_spin_button_new_with_range(1, MAXN, 1);
    gtk_box_pack_start(GTK_BOX(vbox), spin, FALSE, FALSE, 8);
    gtk_widget_set_halign(spin, GTK_ALIGN_CENTER);

    // Botón Aceptar
    GtkWidget *btn_aceptar = gtk_button_new_with_label("Aceptar");
    gtk_box_pack_start(GTK_BOX(vbox), btn_aceptar, FALSE, FALSE, 8);
    gtk_widget_set_halign(btn_aceptar, GTK_ALIGN_CENTER);

    // Botón Load (a implementar después)
    GtkWidget *btn_load = gtk_button_new_with_label("Load");
    gtk_box_pack_start(GTK_BOX(vbox), btn_load, FALSE, FALSE, 8);
    gtk_widget_set_halign(btn_load, GTK_ALIGN_CENTER);

    int response = -1;
    int val = 0;

    // Conectar señal para cerrar el diálogo con respuesta OK al presionar Aceptar
    g_signal_connect(btn_aceptar, "clicked", G_CALLBACK(on_aceptar_clicked), dialog);
    g_object_set_data(G_OBJECT(dialog), "spin", spin);

    gtk_widget_show_all(dialog);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_ACCEPT)
    {
        GtkWidget *spin_widget = g_object_get_data(G_OBJECT(dialog), "spin");
        val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_widget));
    }
    gtk_widget_destroy(dialog);
    return val;
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new_from_file("PR01/floyd.glade");
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window_floyd"));
    grid_table = GTK_WIDGET(gtk_builder_get_object(builder, "grid_table"));
    GtkWidget *btn_run = GTK_WIDGET(gtk_builder_get_object(builder, "btn_run"));
    GtkWidget *btn_close = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));

    // Preguntar cantidad de nodos antes de mostrar la ventana principal
    current_n = ask_nodes_dialog();
    if (current_n <= 0)
        return 0;

    build_table();

    g_signal_connect(btn_run, "clicked", G_CALLBACK(run_floyd), NULL);
    g_signal_connect(btn_close, "clicked", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}