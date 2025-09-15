#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define MAX_OBJECTS 10
#define MAX_CAPACITY 20
#define INF -1

GtkWidget *object_table;
int backpack_capacity = 0;
int object_count = 0;

GtkWidget *cost_entries[MAX_OBJECTS];
GtkWidget *value_entries[MAX_OBJECTS];
GtkWidget *quantity_entries[MAX_OBJECTS];

typedef struct {
    int cost;
    int value;
    int quantity;
} Object;
Object objects[MAX_OBJECTS];

int table[MAX_CAPACITY + 1][MAX_OBJECTS + 1];   // Table[i][j] = capacidad i con j objetos
int copies[MAX_CAPACITY + 1][MAX_OBJECTS + 1];  // k usado
int is_tie[MAX_CAPACITY + 1][MAX_OBJECTS + 1];  // empate

// Prototipos
void on_aceptar_clicked(GtkButton *button, gpointer dialog);
void knapsack(const char *filename, int n, int W, Object *objects);
void get_solutions(int i, int j, int *current, int n, int **solutions, int *solution_count, int *sol_capacity);

// Dialogo para preguntar capacidad de la mochila y cantidad de objetos
int initial_dialog()
{
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Configuración de la mochila");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // Capacidad
    GtkWidget *label1 = gtk_label_new("Capacidad máxima de la mochila (0-20):");
    gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 8);
    gtk_widget_set_halign(label1, GTK_ALIGN_CENTER);
    GtkWidget *spin_capacity = gtk_spin_button_new_with_range(0, MAX_CAPACITY, 1);
    gtk_box_pack_start(GTK_BOX(vbox), spin_capacity, FALSE, FALSE, 8);
    gtk_widget_set_halign(spin_capacity, GTK_ALIGN_CENTER);

    // Cantidad de objetos
    GtkWidget *label2 = gtk_label_new("Cantidad de objetos (1-10):");
    gtk_box_pack_start(GTK_BOX(vbox), label2, FALSE, FALSE, 8);
    gtk_widget_set_halign(label2, GTK_ALIGN_CENTER);
    GtkWidget *spin_objects = gtk_spin_button_new_with_range(1, MAX_OBJECTS, 1);
    gtk_box_pack_start(GTK_BOX(vbox), spin_objects, FALSE, FALSE, 8);
    gtk_widget_set_halign(spin_objects, GTK_ALIGN_CENTER);

    // Botón aceptar y cargar
    GtkWidget *btn_aceptar = gtk_button_new_with_label("Aceptar");
    gtk_box_pack_start(GTK_BOX(vbox), btn_aceptar, FALSE, FALSE, 8);
    gtk_widget_set_halign(btn_aceptar, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(btn_aceptar, "Genera una tabla con la cantidad de objetos y capacidad seleccionados.");
    GtkWidget *btn_load = gtk_button_new_with_label("Cargar");
    gtk_box_pack_end(GTK_BOX(vbox), btn_load, FALSE, FALSE, 8);
    gtk_widget_set_halign(btn_load, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(btn_load, "Carga un archivo .txt con un ejercicio previamente guardado.");

    g_object_set_data(G_OBJECT(dialog), "spin_capacity", spin_capacity);
    g_object_set_data(G_OBJECT(dialog), "spin_objects", spin_objects);

    // Señales
    g_signal_connect(btn_aceptar, "clicked", G_CALLBACK(on_aceptar_clicked), dialog);
    //g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_button_clicked), dialog);

    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK)
    {
        GtkWidget *spin1 = g_object_get_data(G_OBJECT(dialog), "spin_capacity");
        GtkWidget *spin2 = g_object_get_data(G_OBJECT(dialog), "spin_objects");
        backpack_capacity = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin1));
        object_count = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin2));
        gtk_widget_destroy(dialog);
        return 1;
    }
    
    gtk_widget_destroy(dialog);
    return 0;
}


void on_aceptar_clicked(GtkButton *button, gpointer dialog)
{
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

// Validar cantidad, solo enteros o infinito (*)
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
// Validar valor y costo, solo enteros positivos
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

// Ventana para editar objetos
void edit_objects()
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(object_table));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    // Encabezados
    const char *headers[] = {"Objeto", "Costo", "Valor", "Cantidad (*)"};
    for (int j = 0; j < 4; j++)
    {
        GtkWidget *lbl = gtk_label_new(headers[j]);
        gtk_grid_attach(GTK_GRID(object_table), lbl, j, 0, 1, 1);
    }

    // Crear filas y guardar sus valores 
    for (int i = 0; i < object_count; i++)
    {
        char name[8];
        snprintf(name, sizeof(name), "Obj %d", i + 1);
        GtkWidget *lbl = gtk_label_new(name);
        gtk_grid_attach(GTK_GRID(object_table), lbl, 0, i + 1, 1, 1);

        // Costo
        GtkWidget *entry_cost = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(object_table), entry_cost, 1, i + 1, 1, 1);
        g_signal_connect(entry_cost, "insert-text", G_CALLBACK(on_entry_insert_val), NULL);
        cost_entries[i] = entry_cost;

        // Valor
        GtkWidget *entry_val = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(object_table), entry_val, 2, i + 1, 1, 1);
        g_signal_connect(entry_val, "insert-text", G_CALLBACK(on_entry_insert_val), NULL);
        value_entries[i] = entry_val;

        // Cantidad
        GtkWidget *entry_qty = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(object_table), entry_qty, 3, i + 1, 1, 1);
        g_signal_connect(entry_qty, "insert-text", G_CALLBACK(on_entry_insert_quantity), NULL);
        quantity_entries[i] = entry_qty;
    }
    gtk_widget_show_all(object_table);
}

// Ejecutar knapsack
void on_run_clicked(GtkButton *button, gpointer user_data)
{
    // Lee las entradas
    for (int i = 0; i < object_count; i++) {
        const gchar *cost = gtk_entry_get_text(GTK_ENTRY(cost_entries[i]));
        const gchar *value  = gtk_entry_get_text(GTK_ENTRY(value_entries[i]));
        const gchar *quantity  = gtk_entry_get_text(GTK_ENTRY(quantity_entries[i]));

        if (!cost || strlen(cost) == 0) { g_printerr("Debe ingresar el costo para objeto %d\n", i+1); return; }
        if (!value  || strlen(value)  == 0) { g_printerr("Debe ingresar el valor para objeto %d\n", i+1); return; }
        if (!quantity  || strlen(quantity)  == 0) { g_printerr("Debe ingresar la cantidad para objeto %d\n", i+1); return; }

        objects[i].cost = atoi(cost);
        objects[i].value = atoi(value);
        if (strcmp(quantity, "*") == 0) objects[i].quantity = INF;
        else objects[i].quantity = atoi(quantity);
        if (objects[i].cost <= 0) { g_printerr("Costo debe ser positivo en objeto %d\n", i+1); return; }
        if (objects[i].value < 0) { g_printerr("Valor negativo en objeto %d\n", i+1); return; }
    }

    // Ejecutar knapsack
    knapsack("knapsack.tex", object_count, backpack_capacity, objects);
}

// Escribir el archivo .tex 
void write_file(const char *filename, int n, int W, Object *objects, int solution_count, int **solutions)
{
    FILE *f = fopen(filename, "i");
    if (!f) {
        g_printerr("No pude crear %s\n", filename);
        return;
    }

    fprintf(f, "%% Archivo generado automáticamente por knapsack.c\n");
    fprintf(f, "\\documentclass[12pt]{article}\n");
    fprintf(f, "\\usepackage[utf8]{inputenc}\n");
    fprintf(f, "\\usepackage{array,booktabs}\n");
    fprintf(f, "\\usepackage[table]{xcolor}\n");
    fprintf(f, "\\usepackage{longtable}\n");
    fprintf(f, "\\usepackage{geometry}\n");
    fprintf(f, "\\geometry{margin=0.8in}\n");
    fprintf(f, "\\begin{document}\n");
    fprintf(f, "\\section*{Problema}\n");

    // Mostrar datos del problema en notación matemática
    fprintf(f, "Capacidad de la mochila: $W = %d$\\\\\n", W);
    fprintf(f, "Número de objetos: $n = %d$\\\\\n", n);
    fprintf(f, "\\begin{itemize}\n");
    for (int i = 0; i < n; i++) {
        if (objects[i].quantity == INF)
            fprintf(f, "\\item Objeto %d: $c_{%d}=%d$, $v_{%d}=%d$, $q_{%d}=\\infty$ (unbounded)\n", i+1, i+1, objects[i].cost, i+1, objects[i].value, i+1);
        else
            fprintf(f, "\\item Objeto %d: $c_{%d}=%d$, $v_{%d}=%d$, $q_{%d}=%d$ (bounded)\n", i+1, i+1, objects[i].cost, i+1, objects[i].value, i+1, objects[i].quantity);
    }
    fprintf(f, "\\end{itemize}\n");

    fprintf(f, "\\section*{Tabla de trabajo}\n");
    fprintf(f, "Color: {\\color{green}verde} = se toma el objeto en esa celda, {\\color{red}rojo} = no se toma, {\\color{yellow}amarillo} = empate.\\\\\n");

    // Muestra la tabla vista en clase con soluciones para cada objeto
    for (int i = 1; i <= n; i++) {
        fprintf(f, "\\subsection*{Objeto %d}\n", i);

        fprintf(f, "\\begin{longtable}{c");
        for (int j = 1; j <= i; j++) fprintf(f, "c");
        fprintf(f, "}\n");

        // Encabezado
        fprintf(f, "\\toprule\n");
        fprintf(f, "Peso ");
        for (int j = 1; j <= i; j++) {
            fprintf(f, "& Obj %d ", j);
        }
        fprintf(f, "\\\\\n\\midrule\n");

        // Muestra las filas de la tabla
        for (int i = 0; i <= W; i++) {
            fprintf(f, "%d ", i);
            for (int j = 1; j <= i; j++) {
                int val = table[i][j];
                int k   = copies[i][j];
                int tie = is_tie[i][j];
                const char *color;
                if (tie) color = "yellow!50";
                else if (k > 0) color = "green!40";
                else color = "red!20";

                if (val <= INT_MIN/2) {
                    fprintf(f, "& \\cellcolor{%s}{}", color);
                } else {
                    fprintf(f, "& \\cellcolor{%s}%d~(k=%d)", color, val, k);
                }
            }
            fprintf(f, " \\\\\n");
        }

        fprintf(f, "\\bottomrule\n");
        fprintf(f, "\\end{longtable}\n");
    }

    // Mostrar soluciones óptimas
    fprintf(f, "\\section*{Soluciones óptimas}\n");
    int bestZ = table[W][n];
    fprintf(f, "El valor óptimo es de $Z = %d$ con las siguientes cantidades de cada objeto:\\\\\n", bestZ);

    if (solution_count == 0) {
        fprintf(f, "No se encontraron soluciones (algo anduvo mal en el DP).\\\\\n");
    } else {
        fprintf(f, "\\begin{itemize}\n");
        for (int s = 0; s < solution_count; s++) {
            fprintf(f, "\\item ");
            for (int i = 0; i < n; i++) {
                fprintf(f, "x_{%d} = %d", i+1, solutions[s][i]);
                if (i < n-1) fprintf(f, ", ");
            }
            fprintf(f, "\\\\\n");
        }
        fprintf(f, "\\end{itemize}\n");
    }
    fprintf(f, "\\end{document}\n");
    fclose(f);
}

// Resuelve knapsack y llama a generar el archivo
void knapsack(const char *filename, int n, int capacity, Object *objects)
{
    // Inicializar fila capacidad=0
    for (int j = 0; j <= n; j++) {
        table[0][j] = 0;
        copies[0][j] = 0;
        is_tie[0][j] = 0;
    }

    // Para cada capacidad i y objeto j aplica la ecuacion de Bellman
    for (int i = 1; i <= capacity; i++) {
        table[i][0] = 0; // 0 objetos = 0 valor
        for (int j = 1; j <= n; j++) { 
            int c = objects[j-1].cost;
            int v = objects[j-1].value;
            int quantity = objects[j-1].quantity;
            int best_case = INT_MIN; // mejor valor, se utiliza INT_MIN como un minimo imposible para probar
            int best_copies = 0;
            int tie = 0;
            int max_copies;

            // Calcular maximo de copias k que se pueden usar
            if (quantity == INF) max_copies = i / c;
            else {
                max_copies = quantity;
                if (max_copies > i / c) max_copies = i / c;
            }

            // Probar todas las casos posibles de k
            for (int k = 0; k <= max_copies; k++) {
                int caseValue = table[i - k*c][j-1] + k*v;
                if (caseValue > best_case) {
                    best_case = caseValue;
                    best_copies = k;
                    tie = 0;
                } else if (caseValue == best_case && k != best_copies) { // si hay empate
                    tie = 1; 
                }
            }

            // 
            if (best_case == INT_MIN) best_case = INT_MIN + 1;
            table[i][j] = best_case;
            copies[i][j] = best_copies;
            is_tie[i][j] = tie;
        }
    }

    // Soluciones optimas con backtracking
    int max_solutions = 10000;
    int **solutions = malloc(sizeof(int*) * max_solutions);
    for (int i = 0; i < max_solutions; i++) solutions[i] = NULL;
    int solution_count = 0;
    int *current = calloc(n, sizeof(int));
    get_solutions(capacity, n, current, n, solutions, &solution_count, &max_solutions);

    // Escribir archivo con las tablas y soluciones
    write_file(filename, n, capacity, objects, solution_count, solutions);

    // Liberar
    free(current);
    for (int s = 0; s < solution_count; s++) free(solutions[s]);
    free(solutions);

    // Compilar pdflatex y abrir PDF
    system("pdflatex -interaction=nonstopmode knapsack.tex > /dev/null 2>&1");
    system("pdflatex -interaction=nonstopmode knapsack.tex > /dev/null 2>&1");
    system("evince -f knapsack.pdf &");
}

// Recursivo para encontrar todas las soluciones optimas con las k que cumplan
void get_solutions(int i, int j, int *current, int n, int **solutions, int *solution_count, int *max_solutions) {
    
    if (j == 0) {
        // si ya no hay objetos, guarda la solucion actual que es optima
        int *solution = malloc(sizeof(int)*n);
        for (int x = 0; x < n; x++) solution[x] = current[x];
        solutions[*solution_count] = solution;
        (*solution_count)++;
        return;
    }

    int c = objects[j-1].cost;
    int v = objects[j-1].value;
    int quantity = objects[j-1].quantity;
    // define maximo de copias k que se pueden usar
    int max_copies = (quantity == INF ? i/c : (quantity < i/c ? quantity : i/c));
    int objective = table[i][j]; // valor objetivo para comparar

    // Probar todas las k que den el valor objetivo para ir mejorando
    for (int k = 0; k <= max_copies; k++) {
        int caseValue = table[i - k*c][j-1] + k*v;
        if (caseValue == objective) {
            current[j-1] = k;
            get_solutions(i - k*c, j-1, current, n, solutions, solution_count, max_solutions);
            current[j-1] = 0;
        }
    }
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    GtkBuilder *builder = gtk_builder_new_from_file("Knapsack/knapsack.glade");
    if (!builder) {
        g_printerr("No se pudo cargar Knapsack/knapsack.glade\n");
        return 1;
    }
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window_knapsack"));
    GtkWidget *btn_run = GTK_WIDGET(gtk_builder_get_object(builder, "btn_run"));
    GtkWidget *btn_close = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));
    // GtkWidget *btn_save = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save"));
    object_table = GTK_WIDGET(gtk_builder_get_object(builder, "object_table"));
    if (!object_table) {
        g_printerr("No se encontró el widget 'object_table' en el archivo Glade\n");
        return 1;
    }

    // Mostrar dialogo inicial
    if (!initial_dialog())
        return 0;

    // MUestra los objetos para editarlos
    edit_objects();

    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_run_clicked), NULL);
    g_signal_connect(btn_close, "clicked", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
