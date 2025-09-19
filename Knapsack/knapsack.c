#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define MAX_OBJECTS 10
#define MAX_CAPACITY 20
#define INF -1
gboolean loaded_from_file = FALSE;

GtkWidget *object_table;
int backpack_capacity = 0;
int object_count = 0;

GtkWidget *name_entries[MAX_OBJECTS];
GtkWidget *cost_entries[MAX_OBJECTS];
GtkWidget *value_entries[MAX_OBJECTS];
GtkWidget *quantity_entries[MAX_OBJECTS];

typedef struct
{
    char name[64];
    int cost;
    int value;
    int quantity;
} Object;
Object objects[MAX_OBJECTS];

int table[MAX_CAPACITY + 1][MAX_OBJECTS + 1];  // Table[i][j] = capacidad i con j objetos
int copies[MAX_CAPACITY + 1][MAX_OBJECTS + 1]; // k usado
int is_tie[MAX_CAPACITY + 1][MAX_OBJECTS + 1]; // empate

// Prototipos
void on_aceptar_clicked(GtkButton *button, gpointer dialog);
void knapsack(const char *filename, int n, int W, Object *objects);
void get_solutions(int i, int j, int *current, int n, int **solutions, int *solution_count, int *sol_capacity);
void edit_objects();
void on_load_knapsack_clicked(GtkWidget *widget, gpointer data);
void on_run_clicked(GtkButton *button, gpointer user_data);
void on_save_knapsack_clicked(GtkWidget *widget, gpointer data);

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
    g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_knapsack_clicked), dialog);
    gtk_box_pack_end(GTK_BOX(vbox), btn_load, FALSE, FALSE, 8);
    gtk_widget_set_halign(btn_load, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(btn_load, "Carga un archivo .txt con un ejercicio previamente guardado.");

    g_object_set_data(G_OBJECT(dialog), "spin_capacity", spin_capacity);
    g_object_set_data(G_OBJECT(dialog), "spin_objects", spin_objects);

    // Señales
    g_signal_connect(btn_aceptar, "clicked", G_CALLBACK(on_aceptar_clicked), dialog);
    // g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_button_clicked), dialog);

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
    for (int i = 0; i < length; i++)
    {
        char c = text[i];
        if (!g_ascii_isdigit(c) && c != '*')
        {
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
    for (int i = 0; i < length; i++)
    {
        if (!g_ascii_isdigit(text[i]))
        {
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
    const char *headers[] = {"Nombre", "Costo", "Valor", "Cantidad (*)"};
    for (int j = 0; j < 4; j++)
    {
        GtkWidget *lbl = gtk_label_new(headers[j]);
        gtk_grid_attach(GTK_GRID(object_table), lbl, j, 0, 1, 1);
    }

    // Crear filas
    for (int i = 0; i < object_count; i++)
    {
        // Nombre
        GtkWidget *entry_name = gtk_entry_new();
        char default_name[32];
        snprintf(default_name, sizeof(default_name), "Obj %d", i + 1);
        gtk_entry_set_text(GTK_ENTRY(entry_name), default_name);
        gtk_grid_attach(GTK_GRID(object_table), entry_name, 0, i + 1, 1, 1);
        name_entries[i] = entry_name;

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
    for (int i = 0; i < object_count; i++)
    {
        const gchar *name = gtk_entry_get_text(GTK_ENTRY(name_entries[i]));
        const gchar *cost = gtk_entry_get_text(GTK_ENTRY(cost_entries[i]));
        const gchar *value = gtk_entry_get_text(GTK_ENTRY(value_entries[i]));
        const gchar *quantity = gtk_entry_get_text(GTK_ENTRY(quantity_entries[i]));

        if (!name || strlen(name) == 0)
        {
            snprintf(objects[i].name, sizeof(objects[i].name), "Obj %d", i + 1);
        }
        else
        {
            strncpy(objects[i].name, name, sizeof(objects[i].name) - 1);
            objects[i].name[sizeof(objects[i].name) - 1] = '\0';
        }

        if (!cost || strlen(cost) == 0)
        {
            g_printerr("Debe ingresar el costo para objeto %d\n", i + 1);
            return;
        }
        if (!value || strlen(value) == 0)
        {
            g_printerr("Debe ingresar el valor para objeto %d\n", i + 1);
            return;
        }
        if (!quantity || strlen(quantity) == 0)
        {
            g_printerr("Debe ingresar la cantidad para objeto %d\n", i + 1);
            return;
        }

        objects[i].cost = atoi(cost);
        objects[i].value = atoi(value);
        if (strcmp(quantity, "*") == 0)
            objects[i].quantity = INF;
        else
            objects[i].quantity = atoi(quantity);
        if (objects[i].cost <= 0)
        {
            g_printerr("Costo debe ser positivo en objeto %d\n", i + 1);
            return;
        }
        if (objects[i].value < 0)
        {
            g_printerr("Valor negativo en objeto %d\n", i + 1);
            return;
        }
    }

    // Ejecutar knapsack
    knapsack("knapsack.tex", object_count, backpack_capacity, objects);
}

// Escribir el archivo .tex (modo sobrescritura)
void write_file(const char *filename, int n, int W, Object *objects, int solution_count, int **solutions)
{
    FILE *f = fopen(filename, "w");
    if (!f)
    {
        g_printerr("No pude crear %s\n", filename);
        return;
    }

    fprintf(f, "%% Archivo generado automáticamente por knapsack.c\n");
    fprintf(f, "\\documentclass[12pt]{article}\n");
    fprintf(f, "\\usepackage[utf8]{inputenc}\n");
    fprintf(f, "\\usepackage{graphicx}\n");
    fprintf(f, "\\usepackage{array,booktabs}\n");
    fprintf(f, "\\usepackage[table]{xcolor}\n");
    fprintf(f, "\\usepackage{longtable}\n");
    fprintf(f, "\\usepackage{geometry}\n");
    fprintf(f, "\\usepackage{pdflscape}\n");
    fprintf(f, "\\geometry{margin=0.8in}\n");
    fprintf(f, "\\begin{document}\n");

    // --- Portada ---
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "{\\large Instituto Tecnológico de Costa Rica\\\\[1cm]\n");
    fprintf(f, "\\includegraphics[width=0.4\\textwidth]{TEC.png}\\\\[2cm]\n");
    fprintf(f, "{\\LARGE \\textbf{Proyecto 2: Problema de la Mochila}}\\\\[2cm]\n");
    fprintf(f, "{\\large Investigación de Operaciones\\\\[2cm]\n");
    fprintf(f, "{\\large Profesor: }\\\\[1cm]\n");
    fprintf(f, "{\\large Francisco Jose Torres Roja}\\\\[2cm]\n");
    fprintf(f, "{\\large Integrantes: }\\\\[1cm]\n");
    fprintf(f, "{\\large Jose Pablo Fernandez Jimenez - 2023117752}\\\\[1cm]\n");
    fprintf(f, "{\\large Diego Durán Rodríguez - 2022437509}\\\\[2cm]\n");
    fprintf(f, "{\\large Segundo semestre 2025\\\\[1cm]\n");
    fprintf(f, "\\end{center}\n\\newpage\n");

    // --- Descripción del algoritmo ---
    fprintf(f, "\\section*{Algoritmo de la Mochila}\n");
    fprintf(f,
            "El problema de la mochila es un problema clásico de optimización combinatoria en el que se busca "
            "seleccionar un subconjunto de objetos de un conjunto dado, cada uno con un peso y un valor, "
            "de manera que el valor total de los objetos seleccionados se maximice sin exceder la capacidad de la ``mochila''. "
            "Este problema tiene varias versiones según las restricciones sobre la cantidad de veces que un objeto puede ser incluido:\n\n"
            "\\textbf{0/1 Knapsack (Mochila 0/1):} En esta variante, cada objeto puede ser incluido como máximo una vez. "
            "La decisión de incluir o no un objeto es binaria (0 o 1).\n\n"
            "\\textbf{Bounded Knapsack (Mochila acotada):} Aquí, cada objeto puede ser incluido hasta un número limitado de veces.\n\n"
            "\\textbf{Unbounded Knapsack (Mochila ilimitada):} En esta versión, no hay límite en la cantidad de veces que un objeto puede ser incluido (cada objeto puede ser incluido infinitas veces).\n\n"
            "\\bigskip\n");

    // --- Algoritmo usado ---
    fprintf(f, "\\section*{Algoritmo utilizado}\n");
    fprintf(f,
            "Para resolver el problema de la mochila implementamos un algoritmo de programación dinámica basado en la ecuación de Bellman. "
            "Este enfoque permite calcular de manera eficiente el valor óptimo de la mochila para cada capacidad y conjunto de objetos posibles.\n\n"
            "El procedimiento general es el siguiente:\n"
            "\\begin{enumerate}\n"
            "  \\item Inicializar una tabla donde cada fila representa una capacidad de la mochila y cada columna representa los objetos disponibles.\n"
            "  \\item Para cada capacidad $i$ y cada objeto $j$, se calcula el mejor valor posible considerando todas las cantidades $k$ de ese objeto que se pueden incluir, "
            "respetando las restricciones de cantidad (0/1, acotada o ilimitada).\n"
            "  \\item Se almacena el mejor valor encontrado en la tabla, junto con el número de copias utilizadas y si existe un empate entre distintas elecciones.\n"
            "  \\item Una vez completada la tabla, se realiza un backtracking para reconstruir todas las soluciones óptimas posibles.\n"
            "\\end{enumerate}\n\n"
            "\\bigskip\n");

    // --- Datos del problema ---
    fprintf(f, "\\section*{Problema}\n");
    fprintf(f, "Capacidad de la mochila: $W = %d$\\\\\n", W);
    fprintf(f, "Número de objetos: $n = %d$\\\\\n\n", n);

    // --- Formulación matemática ---
    fprintf(f, "\\textbf{Formulación matemática:}\\\\\n");

    // Función objetivo
    fprintf(f, "\\begin{align*}\n");
    fprintf(f, "\\textbf{Maximizar Z = }\\\\\n");
    for (int idx = 0; idx < n; idx++)
    {
        fprintf(f, "%d\\,x_{\\text{%s}}", objects[idx].value, objects[idx].name);
        if (idx < n - 1)
        {
            fprintf(f, " + ");
            if ((idx + 1) % 3 == 0 && idx < n - 1)
                fprintf(f, " \\\\\n&\\quad ");
        }
    }
    fprintf(f, "\n\\end{align*}\n");
    fprintf(f, "\\bigskip\\bigskip\n");

    fprintf(f, "\\textbf{}\\\\\n");
    fprintf(f, "\\begin{align*}\n");
    fprintf(f, "\\textbf{Sujeto a:}\\\\\n");
    for (int idx = 0; idx < n; idx++)
    {
        fprintf(f, "%d\\,x_{\\text{%s}}", objects[idx].cost, objects[idx].name);
        if (idx < n - 1)
        {
            fprintf(f, " + ");
            if ((idx + 1) % 3 == 0 && idx < n - 1)
                fprintf(f, " \\\\\n&\\quad ");
        }
    }
    fprintf(f, " &\\leq %d\n", W);
    fprintf(f, "\\end{align*}\n");
    fprintf(f, "\\bigskip\\bigskip\n");

    // Restricciones de cantidades
    fprintf(f, "\\textbf{}\\\\\n");
    fprintf(f, "\\begin{align*}\n");
    for (int idx = 0; idx < n; idx++)
    {
        if (objects[idx].quantity == INF)
            fprintf(f, "x_{\\text{%s}} &\\in \\mathbb{Z}_{\\geq 0}\\\\\n", objects[idx].name);
        else
            fprintf(f, "0 \\leq x_{\\text{%s}} &\\leq %d,\\; x_{\\text{%s}} \\in \\mathbb{Z}\\\\\n",
                    objects[idx].name, objects[idx].quantity, objects[idx].name);
    }
    fprintf(f, "\\end{align*}\n");

    // --- Tabla de trabajo ---
    fprintf(f, "\\section*{Tabla de trabajo}\n");
    fprintf(f, "Color: {\\color{green}verde} = se toma el objeto, {\\color{red}rojo} = no se toma, {\\color{yellow}amarillo} = empate.\\\\\n");

    for (int obj_idx = 1; obj_idx <= n; obj_idx++)
    {
        fprintf(f, "\\begin{landscape}\n");
        fprintf(f, "\\subsection*{Objeto %s}\n", objects[obj_idx - 1].name);
        fprintf(f, "\\begin{longtable}{c");
        for (int col = 1; col <= obj_idx; col++)
            fprintf(f, "c");
        fprintf(f, "}\n");
        fprintf(f, "\\toprule\nPeso ");
        for (int col = 1; col <= obj_idx; col++)
            fprintf(f, "& %s ", objects[col - 1].name);
        fprintf(f, "\\\\\n\\midrule\n");

        for (int w = 0; w <= W; w++)
        {
            fprintf(f, "%d ", w);
            for (int col = 1; col <= obj_idx; col++)
            {
                int val = table[w][col];
                int k = copies[w][col];
                int tie = is_tie[w][col];
                const char *color = tie ? "yellow!50" : (k > 0 ? "green!40" : "red!20");
                fprintf(f, "& \\cellcolor{%s}%d~(k=%d) ", color, val, k);
            }
            fprintf(f, "\\\\\n");
        }
        fprintf(f, "\\bottomrule\n\\end{longtable}\n");
        fprintf(f, "\\end{landscape}\n");
    }

    // --- Soluciones óptimas ---
    fprintf(f, "\\section*{Soluciones óptimas}\n");
    int bestZ = table[W][n];
    fprintf(f, "El valor óptimo es de $Z = %d$ con las siguientes cantidades de cada objeto:\\\\\n", bestZ);

    if (solution_count == 0)
    {
        fprintf(f, "No se encontraron soluciones.\\\\\n");
    }
    else
    {
        fprintf(f, "\\begin{itemize}\n");
        for (int s = 0; s < solution_count; s++)
        {
            fprintf(f, "\\item ");
            for (int i = 0; i < n; i++)
            {
                int val = solutions[s][i];
                if (val != 0)
                    fprintf(f, "x_{\\text{%s}} = \\textcolor{green}{%d}", objects[i].name, val);
                else
                    fprintf(f, "x_{\\text{%s}} = %d", objects[i].name, val);

                if (i < n - 1)
                    fprintf(f, ", ");
            }
            fprintf(f, "\\\\\n");
        }
        fprintf(f, "\\end{itemize}\n");
    }
    fprintf(f, "\\renewcommand{\\refname}{Referencias}\n");
    fprintf(f, "\\begin{thebibliography}{9}\n");
    fprintf(f, "\\bibitem{Wikipedia} colaboradores de Wikipedia. (2024, 23 febrero). Problema de la mochila. Wikipedia, la Enciclopedia Libre. \\\\ \\url{https://es.wikipedia.org/wiki/Problema_de_la_mochila}\n");
    fprintf(f, "\\end{thebibliography}\n");

    fprintf(f, "\\end{document}\n");
    fclose(f);
}

// Resuelve knapsack y llama a generar el archivo
void knapsack(const char *filename, int n, int capacity, Object *objects)
{
    // Inicializar fila capacidad=0
    for (int j = 0; j <= n; j++)
    {
        table[0][j] = 0;
        copies[0][j] = 0;
        is_tie[0][j] = 0;
    }

    // Para cada capacidad i y objeto j aplica la ecuacion de Bellman
    for (int i = 1; i <= capacity; i++)
    {
        table[i][0] = 0; // 0 objetos = 0 valor
        for (int j = 1; j <= n; j++)
        {
            int c = objects[j - 1].cost;
            int v = objects[j - 1].value;
            int quantity = objects[j - 1].quantity;
            int best_case = INT_MIN; // mejor valor, se utiliza INT_MIN como un minimo imposible para probar
            int best_copies = 0;
            int tie = 0;
            int max_copies;

            // Calcular maximo de copias k que se pueden usar
            if (quantity == INF)
                max_copies = i / c;
            else
            {
                max_copies = quantity;
                if (max_copies > i / c)
                    max_copies = i / c;
            }

            // Probar todas las casos posibles de k
            for (int k = 0; k <= max_copies; k++)
            {
                int caseValue = table[i - k * c][j - 1] + k * v;
                if (caseValue > best_case)
                {
                    best_case = caseValue;
                    best_copies = k;
                    tie = 0;
                }
                else if (caseValue == best_case && k != best_copies)
                { // si hay empate
                    tie = 1;
                }
            }

            //
            if (best_case == INT_MIN)
                best_case = INT_MIN + 1;
            table[i][j] = best_case;
            copies[i][j] = best_copies;
            is_tie[i][j] = tie;
        }
    }

    // Soluciones optimas con backtracking
    int max_solutions = 10000;
    int **solutions = malloc(sizeof(int *) * max_solutions);
    for (int i = 0; i < max_solutions; i++)
        solutions[i] = NULL;
    int solution_count = 0;
    int *current = calloc(n, sizeof(int));
    get_solutions(capacity, n, current, n, solutions, &solution_count, &max_solutions);

    // Escribir archivo con las tablas y soluciones
    write_file(filename, n, capacity, objects, solution_count, solutions);
    // Liberar
    free(current);
    for (int s = 0; s < solution_count; s++)
        free(solutions[s]);
    free(solutions);

    // Compilar pdflatex y abrir PDF
    system("pdflatex -interaction=nonstopmode knapsack.tex > /dev/null 2>&1");
    system("pdflatex -interaction=nonstopmode knapsack.tex > /dev/null 2>&1");
    system("evince -f knapsack.pdf &");
}

// Recursivo para encontrar todas las soluciones optimas con las k que cumplan
void get_solutions(int i, int j, int *current, int n, int **solutions, int *solution_count, int *max_solutions)
{

    if (j == 0)
    {
        // si ya no hay objetos, guarda la solucion actual que es optima
        int *solution = malloc(sizeof(int) * n);
        for (int x = 0; x < n; x++)
            solution[x] = current[x];
        solutions[*solution_count] = solution;
        (*solution_count)++;
        return;
    }

    int c = objects[j - 1].cost;
    int v = objects[j - 1].value;
    int quantity = objects[j - 1].quantity;
    // define maximo de copias k que se pueden usar
    int max_copies = (quantity == INF ? i / c : (quantity < i / c ? quantity : i / c));
    int objective = table[i][j]; // valor objetivo para comparar

    // Probar todas las k que den el valor objetivo para ir mejorando
    for (int k = 0; k <= max_copies; k++)
    {
        int caseValue = table[i - k * c][j - 1] + k * v;
        if (caseValue == objective)
        {
            current[j - 1] = k;
            get_solutions(i - k * c, j - 1, current, n, solutions, solution_count, max_solutions);
            current[j - 1] = 0;
        }
    }
}

void on_save_knapsack_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    gchar *filename = NULL;

    dialog = gtk_file_chooser_dialog_new(
        "Guardar problema de la mochila",
        NULL,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Guardar", GTK_RESPONSE_ACCEPT,
        "_Cancelar", GTK_RESPONSE_CANCEL,
        NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_filter_set_name(filter, "Archivos de texto (*.txt)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "mochila.txt");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (!g_str_has_suffix(filename, ".txt"))
        {
            char *with_ext = g_strconcat(filename, ".txt", NULL);
            g_free(filename);
            filename = with_ext;
        }

        FILE *f = fopen(filename, "w");
        if (!f)
        {
            g_free(filename);
            return;
        }

        // Guardar capacidad y número de objetos
        fprintf(f, "%d\n", backpack_capacity);
        fprintf(f, "%d\n", object_count);

        // Guardar cada objeto
        for (int i = 0; i < object_count; i++)
        {
            fprintf(f, "%s\n", gtk_entry_get_text(GTK_ENTRY(name_entries[i])));
            fprintf(f, "%s\n", gtk_entry_get_text(GTK_ENTRY(cost_entries[i])));
            fprintf(f, "%s\n", gtk_entry_get_text(GTK_ENTRY(value_entries[i])));
            fprintf(f, "%s\n", gtk_entry_get_text(GTK_ENTRY(quantity_entries[i])));
        }

        fclose(f);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void on_load_knapsack_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog_parent = GTK_WIDGET(data); // el diálogo inicial
    GtkWidget *file_dialog;
    gchar *filename = NULL;

    file_dialog = gtk_file_chooser_dialog_new(
        "Cargar problema de la mochila",
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Abrir", GTK_RESPONSE_OK,
        "_Cancelar", GTK_RESPONSE_CANCEL,
        NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_filter_set_name(filter, "Archivos de texto (*.txt)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_dialog), filter);

    gint response = gtk_dialog_run(GTK_DIALOG(file_dialog));
    if (response == GTK_RESPONSE_OK)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_dialog));
    else
    {
        gtk_widget_destroy(file_dialog);
        return;
    }
    gtk_widget_destroy(file_dialog);

    FILE *f = fopen(filename, "r");
    if (!f)
    {
        g_free(filename);
        return;
    }

    char buffer[256];

    // Capacidad
    if (!fgets(buffer, sizeof(buffer), f))
    {
        fclose(f);
        g_free(filename);
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    int file_capacity = atoi(buffer);
    if (file_capacity < 0)
        file_capacity = 0;
    if (file_capacity > MAX_CAPACITY)
        file_capacity = MAX_CAPACITY;
    backpack_capacity = file_capacity;

    // Cantidad de objetos
    if (!fgets(buffer, sizeof(buffer), f))
    {
        fclose(f);
        g_free(filename);
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    int file_n = atoi(buffer);
    if (file_n < 1)
        file_n = 1;
    if (file_n > MAX_OBJECTS)
        file_n = MAX_OBJECTS;
    object_count = file_n;

    // actualizamos los spinbuttons del diálogo inicial (si hay)
    if (dialog_parent)
    {
        GtkWidget *spin_capacity = g_object_get_data(G_OBJECT(dialog_parent), "spin_capacity");
        GtkWidget *spin_objects = g_object_get_data(G_OBJECT(dialog_parent), "spin_objects");
        if (spin_capacity)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_capacity), backpack_capacity);
        if (spin_objects)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_objects), object_count);
    }

    // --- leer el resto del archivo en buffers temporales ---
    char temp_names[MAX_OBJECTS][64];
    for (int i = 0; i < object_count; i++)
        temp_names[i][0] = '\0';

    int temp_costs[MAX_OBJECTS] = {0};
    int temp_values[MAX_OBJECTS] = {0};
    char temp_quantities[MAX_OBJECTS][32];
    for (int i = 0; i < object_count; i++)
        temp_quantities[i][0] = '\0';

    for (int i = 0; i < object_count; i++)
    {
        // nombre
        if (!fgets(buffer, sizeof(buffer), f))
            break;
        buffer[strcspn(buffer, "\r\n")] = 0;
        strncpy(temp_names[i], buffer, sizeof(temp_names[i]) - 1);
        temp_names[i][sizeof(temp_names[i]) - 1] = '\0';

        // costo
        if (!fgets(buffer, sizeof(buffer), f))
            break;
        buffer[strcspn(buffer, "\r\n")] = 0;
        temp_costs[i] = atoi(buffer);

        // valor
        if (!fgets(buffer, sizeof(buffer), f))
            break;
        buffer[strcspn(buffer, "\r\n")] = 0;
        temp_values[i] = atoi(buffer);

        // cantidad
        if (!fgets(buffer, sizeof(buffer), f))
            break;
        buffer[strcspn(buffer, "\r\n")] = 0;
        strncpy(temp_quantities[i], buffer, sizeof(temp_quantities[i]) - 1);
        temp_quantities[i][sizeof(temp_quantities[i]) - 1] = '\0';
    }
    // --- fin lectura ---

    fclose(f);
    g_free(filename);

    // Reconstruir la tabla con el nuevo object_count
    edit_objects();

    // Llenar las entradas con los datos leídos
    for (int i = 0; i < object_count; i++)
    {
        char buf[32];

        snprintf(buf, sizeof(buf), "%d", temp_costs[i]);
        if (cost_entries[i])
            gtk_entry_set_text(GTK_ENTRY(cost_entries[i]), buf);

        snprintf(buf, sizeof(buf), "%d", temp_values[i]);
        if (value_entries[i])
            gtk_entry_set_text(GTK_ENTRY(value_entries[i]), buf);

        if (quantity_entries[i])
            gtk_entry_set_text(GTK_ENTRY(quantity_entries[i]), temp_quantities[i]);
        if (name_entries[i])
            gtk_entry_set_text(GTK_ENTRY(name_entries[i]), temp_names[i]);
    }

    // Señalamos que ya cargamos desde archivo para que main no vuelva a recrear la tabla
    loaded_from_file = TRUE;

    if (dialog_parent)
    {
        gtk_dialog_response(GTK_DIALOG(dialog_parent), GTK_RESPONSE_OK);
    }
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    GtkBuilder *builder = gtk_builder_new_from_file("Knapsack/knapsack.glade");
    if (!builder)
    {
        g_printerr("No se pudo cargar Knapsack/knapsack.glade\n");
        return 1;
    }
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window_knapsack"));
    GtkWidget *btn_run = GTK_WIDGET(gtk_builder_get_object(builder, "btn_run"));
    GtkWidget *btn_close = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));
    GtkWidget *btn_save = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save"));
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_knapsack_clicked), NULL);
    object_table = GTK_WIDGET(gtk_builder_get_object(builder, "object_table"));
    if (!object_table)
    {
        g_printerr("No se encontró el widget 'object_table' en el archivo Glade\n");
        return 1;
    }

    // Mostrar dialogo inicial
    if (!initial_dialog())
        return 0;

    // Muestra los objetos para editarlos
    if (!loaded_from_file)
        edit_objects();

    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_run_clicked), NULL);
    g_signal_connect(btn_close, "clicked", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
