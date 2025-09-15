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

// DP tables (dimensionadas dinámicamente según n y W)
int table[MAX_OBJECTS + 1][MAX_CAPACITY + 1]; // dp[i][w] = óptimo usando primeros i objetos con capacidad w
int copies[MAX_OBJECTS + 1][MAX_CAPACITY + 1]; // k elegido para dp[i][w]
int is_tie[MAX_OBJECTS + 1][MAX_CAPACITY + 1]; // 1 si hubo empate multiple para dp[i][w]

// Prototipos
void on_aceptar_clicked(GtkButton *button, gpointer dialog);
void knapsack(const char *filename, int n, int W, Object *objects);
void collect_solutions(int i, int w, int *curr, int n, int W, int **solutions, int *sol_count, int *sol_capacity);

// Dialogo para preguntar capacidad de la mochila y cantidad de objetos
int initial_dialog()
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Configuración de la mochila",
                                                    NULL,
                                                    GTK_DIALOG_MODAL,
                                                    "_Aceptar", GTK_RESPONSE_OK,
                                                    "_Cancelar", GTK_RESPONSE_CANCEL,
                                                    NULL);

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

    g_object_set_data(G_OBJECT(dialog), "spin_capacity", spin_capacity);
    g_object_set_data(G_OBJECT(dialog), "spin_objects", spin_objects);

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

    // Crear filas con entries y guardarlas en arrays globales
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

    // Ejecutar DP y generar TEX/PDF
    knapsack("knapsack.tex", object_count, backpack_capacity, objects);
}

// Escribir el archivo .tex 
void write_file(const char *filename, int n, int W, Object *objects, int sol_count, int **solutions)
{
    FILE *f = fopen(filename, "w");
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

    for (int i = 1; i <= n; i++) {
        fprintf(f, "\\subsection*{Hasta el objeto %d}\n", i);

        // tabla con columnas = objetos 1..i, filas = pesos
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

        // Filas = pesos
        for (int w = 0; w <= W; w++) {
            fprintf(f, "%d ", w);
            for (int j = 1; j <= i; j++) {
                int val = table[j][w];
                int k = copies[j][w];
                int tie = is_tie[j][w];

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

    // Mostrar soluciones óptimas ---
    fprintf(f, "\\section*{Solución(es) óptima(s)}\n");
    int bestZ = table[n][W];
    fprintf(f, "Valor óptimo $Z^* = %d$\\\\\n", bestZ);
    fprintf(f, "Todas las soluciones óptimas (vectores $x_1,\\dots,x_n$) se listan a continuación: \\\\ \n");

    if (sol_count == 0) {
        fprintf(f, "No se encontraron soluciones (algo anduvo mal en el DP).\\\\\n");
    } else {
        fprintf(f, "\\begin{itemize}\n");
        for (int s = 0; s < sol_count; s++) {
            fprintf(f, "\\item $(");
            for (int i = 0; i < n; i++) {
                if (i < n-1) fprintf(f, "%d, ", solutions[s][i]);
                else fprintf(f, "%d", solutions[s][i]);
            }
            fprintf(f, ")$\\\\\n");
        }
        fprintf(f, "\\end{itemize}\n");
    }
    fprintf(f, "\\end{document}\n");
    fclose(f);
}

// Resuelve knapsack y llama a generar el archivo
void knapsack(const char *filename, int n, int W, Object *objects)
{
    // --- Ejecutar DP y almacenar tablas auxiliares (chosen_k y tie_flag) ---
    // Inicializar dp[0][w] = 0
    for (int w = 0; w <= W; w++) {
        table[0][w] = 0;
        copies[0][w] = 0;
        is_tie[0][w] = 0;
    }

    // Para cada objeto i = 1..n
    for (int i = 1; i <= n; i++) {
        int c = objects[i-1].cost;
        int v = objects[i-1].value;
        int quantity = objects[i-1].quantity; // INF_QTY => unbounded

        for (int w = 0; w <= W; w++) {
            int best = INT_MIN;
            int best_k = 0;
            int ties = 0;

            int max_k;
            if (quantity == INF) max_k = w / c;
            else {
                max_k = quantity;
                if (max_k > w / c) max_k = w / c;
            }
            // probar todos los k posibles
            for (int k = 0; k <= max_k; k++) {
                int rem = w - k * c;
                int cand = table[i-1][rem] + k * v;
                if (cand > best) {
                    best = cand;
                    best_k = k;
                    ties = 0;
                } else if (cand == best) {
                    // si k distinto produce mismo best -> empate
                    if (k != best_k) ties = 1;
                }
            }
            if (best == INT_MIN) best = INT_MIN + 1; // por seguridad
            table[i][w] = best;
            copies[i][w] = best_k;
            is_tie[i][w] = ties;
        }
    }

    // --- Recolectar todas las soluciones óptimas por backtracking enumerando k's que producen dp[i][w] ---
    int max_solutions = 10000;
    int **solutions = malloc(sizeof(int*) * max_solutions);
    for (int i = 0; i < max_solutions; i++) solutions[i] = NULL;
    int sol_count = 0;

    int *curr = calloc(n, sizeof(int));
    collect_solutions(n, W, curr, n, W, solutions, &sol_count, &max_solutions);

    // --- Escribir archivo .tex usando la función separada ---
    write_file(filename, n, W, objects, sol_count, solutions);

    // Liberar soluciones
    free(curr);
    for (int s = 0; s < sol_count; s++) free(solutions[s]);
    free(solutions);

    // Compilar pdflatex y abrir PDF
    system("pdflatex -interaction=nonstopmode knapsack.tex > /dev/null 2>&1");
    system("pdflatex -interaction=nonstopmode knapsack.tex > /dev/null 2>&1");
    system("evince -f knapsack.pdf &");
}

// --- Recolectar soluciones: backtracking que toma todas las k que pueden producir dp[i][w] ---
// - i: índice actual (n .. 0)
// - w: capacidad actual
void collect_solutions(int i, int w, int *curr, int n, int W, int **solutions, int *sol_count, int *sol_capacity)
{
    if (i == 0) {
        // almacenamos curr copia
        if (*sol_count >= *sol_capacity) {
            int newcap = (*sol_capacity) * 2;
            if (newcap < 16) newcap = 16;
            solutions = realloc(solutions, sizeof(int*) * newcap);
        }
        int *sol = malloc(sizeof(int) * n);
        for (int j = 0; j < n; j++) sol[j] = curr[j];
        solutions[*sol_count] = sol;
        (*sol_count)++;
        return;
    }

    int idx = i;
    int c = objects[i-1].cost;
    int v = objects[i-1].value;
    int quantity = objects[i-1].quantity;

    int max_k;
    if (quantity == INF) max_k = w / c;
    else {
        max_k = quantity;
        if (max_k > w / c) max_k = w / c;
    }

    int target = table[i][w];

    // para cada k posible que cumpla target = dp[i-1][w - k*c] + k*v
    for (int k = 0; k <= max_k; k++) {
        int rem = w - k * c;
        int candidate = table[i-1][rem] + k * v;
        if (candidate == target) {
            curr[i-1] = k; // asignar cantidad para el objeto i
            collect_solutions(i-1, rem, curr, n, W, solutions, sol_count, sol_capacity);
            curr[i-1] = 0; // limpiar
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
