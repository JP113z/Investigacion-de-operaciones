#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAXT 50

/* ---------- Globals ---------- */
GtkBuilder *builder;
GtkWidget *grid_table;
GtkWidget *rev_entry[MAXT];
GtkWidget *man_entry[MAXT];
GtkWidget *gan_entry[MAXT];
GtkWidget *spin_costo_global = NULL;

gboolean loaded_from_file = FALSE;
int loaded_cost = 0;
int loaded_plazo = 0;
int loaded_vida = 0;
int loaded_rev[MAXT];
int loaded_man[MAXT];
int loaded_gan[MAXT];

int current_costo = 0;
int current_plazo = 0;
int current_vida = 0;

/* ---------- Struct ---------- */
typedef struct
{
    int costo_inicial;
    int plazo_proyecto;
    int vida_util;
} EquiposData;

/* ---------- Prototipos ---------- */
void on_aceptar_clicked(GtkButton *button, gpointer dialog);
void on_load_button_clicked_initial(GtkWidget *widget, gpointer dialog_parent);
void on_entry_insert_text_numeric(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data);
void build_table(EquiposData data);
void on_save_button_clicked_equipos(GtkWidget *widget, gpointer data);
void on_run_button_clicked(GtkWidget *widget, gpointer data);

int G_arr[MAXT + 1];
int R_arr[MAXT + 1];
gboolean tie_arr[MAXT + 1];   // indica si hubo empate en t
char tie_list[MAXT + 1][256]; // string con alternativas en caso de empate

/* ---------- Funciones auxiliares ---------- */
void on_aceptar_clicked(GtkButton *button, gpointer dialog)
{
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

/* Validación */
void on_entry_insert_text_numeric(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data)
{
    for (int i = 0; i < length; i++)
    {
        char c = text[i];
        if (!g_ascii_isdigit((gchar)c) && c != '-' && c != '+')
        {
            g_signal_stop_emission_by_name(editable, "insert-text");
            return;
        }
    }
}

/* ---------- Cargar archivo desde el diálogo inicial (botón Cargar) ---------- */
void on_load_button_clicked_initial(GtkWidget *widget, gpointer dialog_parent)
{
    GtkWidget *dialog;
    gchar *filename = NULL;

    dialog = gtk_file_chooser_dialog_new(
        "Cargar configuración",
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Abrir", GTK_RESPONSE_OK,
        "_Cancelar", GTK_RESPONSE_CANCEL,
        NULL);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }
    else
    {
        gtk_widget_destroy(dialog);
        return;
    }
    gtk_widget_destroy(dialog);

    FILE *f = fopen(filename, "r");
    if (!f)
    {
        g_free(filename);
        return;
    }

    char buffer[256];

    if (!fgets(buffer, sizeof(buffer), f))
    {
        fclose(f);
        g_free(filename);
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    int file_cost = atoi(buffer);

    if (!fgets(buffer, sizeof(buffer), f))
    {
        fclose(f);
        g_free(filename);
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    int file_plazo = atoi(buffer);

    if (!fgets(buffer, sizeof(buffer), f))
    {
        fclose(f);
        g_free(filename);
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    int file_vida = atoi(buffer);

    if (file_vida < 1 || file_vida > MAXT)
    {
        fclose(f);
        g_free(filename);
        return;
    }

    // leer triples reventa/mant/gan
    for (int i = 0; i < file_vida; i++)
    {
        // reventa
        if (!fgets(buffer, sizeof(buffer), f))
        {
            fclose(f);
            g_free(filename);
            return;
        }
        buffer[strcspn(buffer, "\r\n")] = 0;
        loaded_rev[i] = atoi(buffer);

        // mantenimiento
        if (!fgets(buffer, sizeof(buffer), f))
        {
            fclose(f);
            g_free(filename);
            return;
        }
        buffer[strcspn(buffer, "\r\n")] = 0;
        loaded_man[i] = atoi(buffer);

        // ganancia
        if (!fgets(buffer, sizeof(buffer), f))
        {
            fclose(f);
            g_free(filename);
            return;
        }
        buffer[strcspn(buffer, "\r\n")] = 0;
        loaded_gan[i] = atoi(buffer);
    }

    fclose(f);
    g_free(filename);

    loaded_from_file = TRUE;
    loaded_cost = file_cost;
    loaded_plazo = file_plazo;
    loaded_vida = file_vida;

    /* Indicar al diálogo que se seleccionó carga */
    gtk_dialog_response(GTK_DIALOG(dialog_parent), GTK_RESPONSE_APPLY);
}

/* ---------- Diálogo inicial (igual estilo nodes_dialog) ---------- */
EquiposData equipos_dialog()
{
    EquiposData data = {0, 0, 0};

    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Parámetros de equipos");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // Costo inicial
    GtkWidget *label_costo = gtk_label_new("Costo inicial del equipo:");
    gtk_box_pack_start(GTK_BOX(vbox), label_costo, FALSE, FALSE, 4);
    GtkWidget *spin_costo = gtk_spin_button_new_with_range(1, 1000000000, 100);
    gtk_box_pack_start(GTK_BOX(vbox), spin_costo, FALSE, FALSE, 4);

    // Plazo
    GtkWidget *label_plazo = gtk_label_new("Plazo del proyecto (1 a 30):");
    gtk_box_pack_start(GTK_BOX(vbox), label_plazo, FALSE, FALSE, 4);
    GtkWidget *spin_plazo = gtk_spin_button_new_with_range(1, 30, 1);
    gtk_box_pack_start(GTK_BOX(vbox), spin_plazo, FALSE, FALSE, 4);

    // Vida útil
    GtkWidget *label_vida = gtk_label_new("Vida útil del equipo (1 a 10):");
    gtk_box_pack_start(GTK_BOX(vbox), label_vida, FALSE, FALSE, 4);
    GtkWidget *spin_vida = gtk_spin_button_new_with_range(1, 10, 1);
    gtk_box_pack_start(GTK_BOX(vbox), spin_vida, FALSE, FALSE, 4);

    // Botones
    GtkWidget *btn_continuar = gtk_button_new_with_label("Continuar");
    gtk_box_pack_start(GTK_BOX(vbox), btn_continuar, FALSE, FALSE, 8);

    GtkWidget *btn_cargar = gtk_button_new_with_label("Cargar");
    gtk_box_pack_start(GTK_BOX(vbox), btn_cargar, FALSE, FALSE, 8);

    // conectar aceptar y cargar
    g_signal_connect(btn_continuar, "clicked", G_CALLBACK(on_aceptar_clicked), dialog);
    g_signal_connect(btn_cargar, "clicked", G_CALLBACK(on_load_button_clicked_initial), dialog);

    // Guardar spin widgets para luego obtener valores
    g_object_set_data(G_OBJECT(dialog), "spin_costo", spin_costo);
    g_object_set_data(G_OBJECT(dialog), "spin_plazo", spin_plazo);
    g_object_set_data(G_OBJECT(dialog), "spin_vida", spin_vida);

    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_ACCEPT)
    {
        GtkWidget *sc = g_object_get_data(G_OBJECT(dialog), "spin_costo");
        GtkWidget *sp = g_object_get_data(G_OBJECT(dialog), "spin_plazo");
        GtkWidget *sv = g_object_get_data(G_OBJECT(dialog), "spin_vida");

        data.costo_inicial = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sc));
        data.plazo_proyecto = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sp));
        data.vida_util = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sv));
    }
    else if (response == GTK_RESPONSE_APPLY)
    {
        // cargado desde archivo
        data.costo_inicial = loaded_cost;
        data.plazo_proyecto = loaded_plazo;
        data.vida_util = loaded_vida;
    }

    gtk_widget_destroy(dialog);
    return data;
}

/* ---------- Construir tabla dinámica dentro del grid_table del Glade ---------- */
void build_table(EquiposData data)
{
    // limpiar grid
    GList *children = gtk_container_get_children(GTK_CONTAINER(grid_table));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    // fila 0: Costo inicial (label + spin)
    GtkWidget *lbl_cost = gtk_label_new("Costo inicial del equipo:");
    gtk_grid_attach(GTK_GRID(grid_table), lbl_cost, 0, 0, 1, 1);

    spin_costo_global = gtk_spin_button_new_with_range(1, 1000000000, 100);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_costo_global), data.costo_inicial);
    gtk_grid_attach(GTK_GRID(grid_table), spin_costo_global, 1, 0, 3, 1);

    // fila 1: encabezados de la tabla
    GtkWidget *lbl_tiempo = gtk_label_new("Tiempo");
    GtkWidget *lbl_reventa = gtk_label_new("Reventa");
    GtkWidget *lbl_mant = gtk_label_new("Mantenimiento");
    GtkWidget *lbl_gan = gtk_label_new("Ganancia");
    gtk_grid_attach(GTK_GRID(grid_table), lbl_tiempo, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_table), lbl_reventa, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_table), lbl_mant, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_table), lbl_gan, 3, 1, 1, 1);

    // crear filas (tiempo 1..N) a partir de row = 2
    int N = data.vida_util;
    if (N > MAXT)
        N = MAXT;

    for (int i = 0; i < N; i++)
    {
        // Tiempo (no editable)
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", i + 1);
        GtkWidget *lbl_time = gtk_label_new(buf);
        gtk_grid_attach(GTK_GRID(grid_table), lbl_time, 0, i + 2, 1, 1);

        // Reventa (entry)
        GtkWidget *entry_rev = gtk_entry_new();
        if (loaded_from_file)
        {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%d", loaded_rev[i]);
            gtk_entry_set_text(GTK_ENTRY(entry_rev), tmp);
        }
        else
            gtk_entry_set_text(GTK_ENTRY(entry_rev), "0");
        g_signal_connect(entry_rev, "insert-text", G_CALLBACK(on_entry_insert_text_numeric), NULL);
        gtk_grid_attach(GTK_GRID(grid_table), entry_rev, 1, i + 2, 1, 1);
        rev_entry[i] = entry_rev;

        // Mantenimiento
        GtkWidget *entry_man = gtk_entry_new();
        if (loaded_from_file)
        {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%d", loaded_man[i]);
            gtk_entry_set_text(GTK_ENTRY(entry_man), tmp);
        }
        else
            gtk_entry_set_text(GTK_ENTRY(entry_man), "0");
        g_signal_connect(entry_man, "insert-text", G_CALLBACK(on_entry_insert_text_numeric), NULL);
        gtk_grid_attach(GTK_GRID(grid_table), entry_man, 2, i + 2, 1, 1);
        man_entry[i] = entry_man;

        // Ganancia
        GtkWidget *entry_gan = gtk_entry_new();
        if (loaded_from_file)
        {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%d", loaded_gan[i]);
            gtk_entry_set_text(GTK_ENTRY(entry_gan), tmp);
        }
        else
            gtk_entry_set_text(GTK_ENTRY(entry_gan), "0");
        g_signal_connect(entry_gan, "insert-text", G_CALLBACK(on_entry_insert_text_numeric), NULL);
        gtk_grid_attach(GTK_GRID(grid_table), entry_gan, 3, i + 2, 1, 1);
        gan_entry[i] = entry_gan;
    }

    // Si usamos valores cargados, ya los aplicamos — evitar reaplicar en próximas llamadas
    loaded_from_file = FALSE;

    current_costo = data.costo_inicial;
    current_plazo = data.plazo_proyecto;
    current_vida = data.vida_util;

    gtk_widget_show_all(grid_table);
}

/* ---------- Guardar la configuración a .txt ---------- */
void on_save_button_clicked_equipos(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    gchar *filename = NULL;

    dialog = gtk_file_chooser_dialog_new(
        "Guardar configuración",
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
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "equipos.txt");

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!g_str_has_suffix(filename, ".txt"))
        {
            char *with_ext = g_strconcat(filename, ".txt", NULL);
            g_free(filename);
            filename = with_ext;
        }
    }
    else
    {
        gtk_widget_destroy(dialog);
        return;
    }
    gtk_widget_destroy(dialog);

    FILE *f = fopen(filename, "w");
    if (!f)
    {
        g_free(filename);
        return;
    }

    // Obtener costo actual desde spin
    if (spin_costo_global)
        current_costo = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_costo_global));

    // guardar costo, plazo, vida
    fprintf(f, "%d\n", current_costo);
    fprintf(f, "%d\n", current_plazo);
    fprintf(f, "%d\n", current_vida);

    for (int i = 0; i < current_vida; i++)
    {
        const char *r = gtk_entry_get_text(GTK_ENTRY(rev_entry[i]));
        const char *m = gtk_entry_get_text(GTK_ENTRY(man_entry[i]));
        const char *g = gtk_entry_get_text(GTK_ENTRY(gan_entry[i]));
        fprintf(f, "%s\n", r ? r : "0");
        fprintf(f, "%s\n", m ? m : "0");
        fprintf(f, "%s\n", g ? g : "0");
    }

    fclose(f);
    g_free(filename);
}

/* ---------- Algoritmo Bellman recursivo ---------- */
// calcula el costo de cada segmento de t a x
int segment_cost(int dur, int C, int rev[], int man[], int gan[]) {
    long suma = 0;
    for (int a = 0; a < dur; a++) {
        suma += (long)man[a] - (long)gan[a];
    }
    // al final del tramo, vendo
    suma += C - rev[dur-1];
    return (int)suma;
}
// calcula el reemplazo óptimo y obtiene los G(t)
void compute_bellman(int plazo, int vida, int C, int rev[], int man[], int gan[]) {
    for (int i = 0; i <= plazo; i++) {
        G_arr[i] = INT_MAX / 4;
        R_arr[i] = -1;
        tie_arr[i] = FALSE;
        tie_list[i][0] = '\0';
    }
    G_arr[plazo] = 0;

    for (int t = plazo - 1; t >= 0; t--) {
        long best = LONG_MAX;
        int bestj = -1;
        tie_arr[t] = FALSE;
        tie_list[t][0] = '\0';

        for (int j = t+1; j <= plazo && j-t <= vida; j++) {
            int dur = j - t;
            long c = 0;
            for (int a = 0; a < dur; a++) {
                c += (long)man[a] - (long)gan[a];
            }
            // restamos la reventa
            c += (long)C - (long)rev[dur-1];

            long total = c + (long)G_arr[j];

            if (total < best) {
                best = total;
                bestj = j;
                // guardamos la mejor opcion
                tie_list[t][0] = '\0';
                snprintf(tie_list[t], sizeof(tie_list[t]), "%d", bestj);
                tie_arr[t] = FALSE;
            } else if (total == best) {
                // añade los empates si existen
                tie_arr[t] = TRUE;
                if (tie_list[t][0] == '\0') {
                    snprintf(tie_list[t], sizeof(tie_list[t]), "%d", bestj);
                }
                char addbuf[32];
                snprintf(addbuf, sizeof(addbuf), ",%d", j);
                strncat(tie_list[t], addbuf, sizeof(tie_list[t]) - strlen(tie_list[t]) - 1);
            }
        }

        if (best == LONG_MAX) {
            G_arr[t] = INT_MAX / 4;
            R_arr[t] = -1;
        } else {
            G_arr[t] = (int)best;
            R_arr[t] = bestj;
        }
    }
}

// obtiene todos los planes optimos posibles con backtracking
void get_posible_plans(int t, int plazo, const char *path, FILE *f) {
    if (t == plazo) {
        fprintf(f, "  \\item %s\n", path);
        return;
    }

    if (tie_list[t][0] == '\0') {
        if (R_arr[t] <= 0) return;
        char newpath[512];
        snprintf(newpath, sizeof(newpath), "%s -> %d", path, R_arr[t]);
        get_posible_plans(R_arr[t], plazo, newpath, f);
        return;
    }

    // Hacemos una copia de tie_list[t] y la recorremos con un puntero
    char buf[256];
    strncpy(buf, tie_list[t], sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    char *p = buf;
    while (*p) {
        // buscar siguiente coma
        char *comma = strchr(p, ',');
        if (comma) *comma = '\0';
        int next = atoi(p);
        char newpath[512];
        snprintf(newpath, sizeof(newpath), "%s - %d", path, next);
        get_posible_plans(next, plazo, newpath, f);
        if (!comma) break;
        p = comma + 1;
    }
}

/* ---------- Generar LaTeX y compilar ---------- */
void write_tex_equipos(const char *fname, int plazo, int vida, int C, int rev[], int man[], int gan[])
{
    FILE *f = fopen(fname, "w");
    if (!f)
    {
        perror("fopen tex");
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
    fprintf(f, "{\\LARGE \\textbf{Proyecto 3: Reemplazo de equipos}}\\\\[2cm]\n");
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

    // --- Algoritmo usado ---
    fprintf(f, "\\section*{Algoritmo utilizado}\n");

    // --- Datos del problema ---
    fprintf(f, "\\section*{Problema}\n");
    fprintf(f, "\\begin{itemize}\n");
    fprintf(f, "\\item \\textbf{Costo inicial del equipo:} %d\n", C);
    fprintf(f, "\\item \\textbf{Plazo del proyecto:} %d períodos\n", plazo);
    fprintf(f, "\\item \\textbf{Vida útil del equipo:} %d períodos\n", vida);
    fprintf(f, "\\end{itemize}\n\n");

    fprintf(f, "\\textbf{Datos iniciales de Reventa, Mantenimiento y ganancia:} \n");
    fprintf(f, "\\begin{table}[h!]\n");
    fprintf(f, "\\centering\n");
    fprintf(f, "\\begin{tabular}{|c|c|c|c|}\n");
    fprintf(f, "\\hline\n");
    fprintf(f, "\\textbf{Año de vida} & \\textbf{Reventa} & \\textbf{Mantenimiento} & \\textbf{Ganancia} \\\\\n");
    fprintf(f, "\\hline\n");
    
    for (int i = 0; i < vida; i++) {
        fprintf(f, "%d & %d & %d & %d \\\\\n", i+1, rev[i], man[i], gan[i]);
        fprintf(f, "\\hline\n");
    }
    
    fprintf(f, "\\end{tabular}\n");
    fprintf(f, "\\end{table}\n\n");

    fprintf(f, "\\subsection*{Costos de cada periodo $C_{t,x}$}\n");
    fprintf(f, "\\begin{longtable}{|c|" );
    for (int j = 1; j <= plazo; j++) fprintf(f, "c|");
    fprintf(f, "}\n\\hline\n");

    fprintf(f, "$t \\to x$ ");
    for (int j = 1; j <= plazo; j++) fprintf(f, " & %d", j);
    fprintf(f, " \\\\\\hline\\hline\n");

    for (int t = 0; t < plazo; t++) {
        fprintf(f, "%d", t);
        for (int j = 1; j <= plazo; j++) {
            if (j <= t || j - t > vida) {
                fprintf(f, " & -");
            } else {
                int dur = j - t;
                int c = segment_cost(dur, C, rev, man, gan);
                fprintf(f, " & %d", c);
            }
        }
        fprintf(f, " \\\\\\hline\n");
    }
    fprintf(f, "\\end{longtable}\n");


     // --- Tabla de trabajo ---
    fprintf(f, "\\section*{Tabla de trabajo}\n");
    fprintf(f, "\\begin{tabular}{ccc}\\toprule\n");
    fprintf(f, "t & G(t) & Próximo \\\\\\midrule\n");
    for (int t = 0; t <= plazo; t++) {
        const char *nextstr;
        static char tmpbuf[64];
        if (tie_list[t][0] != '\0') {
            nextstr = tie_list[t];
        } else if (R_arr[t] != -1) {
            snprintf(tmpbuf, sizeof(tmpbuf), "%d", R_arr[t]);
            nextstr = tmpbuf;
        } else {
            nextstr = "-";
        }
        fprintf(f, "%d & %d & %s \\\\\n", t, G_arr[t], nextstr);
    }
    fprintf(f, "\\bottomrule\\end{tabular}\n");

    // --- Soluciones óptimas ---
    fprintf(f, "\\section*{Solución óptima}\n");
    fprintf(f, "Costo mínimo total: \\textbf{%d} \\\\\n", G_arr[0]);

    fprintf(f, "\\subsection*{Planes óptimos}\n");
    fprintf(f, "\\begin{itemize}\n");
    char start[32];
    snprintf(start, sizeof(start), "0");
    get_posible_plans(0, plazo, start, f);
    fprintf(f, "\\end{itemize}\n");
    // --- Referencias ---
    fprintf(f, "\\section*{Referencias}\n");
    fprintf(f, "\\begin{thebibliography}{9}\n");
    fprintf(f, "\\end{thebibliography}\n");

    fprintf(f, "\\end{document}\n");
    fclose(f);
}

/* ---------- Botón Ejecutar: recolecta entradas, corre Bellman, genera LaTeX y abre PDF ---------- */
void on_run_button_clicked(GtkWidget *widget, gpointer data)
{
    if (!spin_costo_global)
        return;

    // obtener datos actuales
    current_costo = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_costo_global));
    int N = current_vida;
    if (N <= 0 || N > MAXT)
        return;

    int rev[MAXT], man[MAXT], gan[MAXT];
    for (int i = 0; i < N; i++)
    {
        const char *r = gtk_entry_get_text(GTK_ENTRY(rev_entry[i]));
        const char *m = gtk_entry_get_text(GTK_ENTRY(man_entry[i]));
        const char *g = gtk_entry_get_text(GTK_ENTRY(gan_entry[i]));
        rev[i] = r ? atoi(r) : 0;
        man[i] = m ? atoi(m) : 0;
        gan[i] = g ? atoi(g) : 0;
    }

    // calcular Bellman
    compute_bellman(current_plazo, current_vida, current_costo, rev, man, gan);

    // Generar LaTeX
    const char *texname = "equipos_output.tex";
    write_tex_equipos(texname, current_plazo, current_vida, current_costo, rev, man, gan);

    // Compilar con pdflatex
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pdflatex -interaction=nonstopmode %s > /dev/null 2>&1", texname);
    system(cmd);
    system(cmd);

    // Abrir PDF en evince (presentación)
    snprintf(cmd, sizeof(cmd), "evince --presentation equipos_output.pdf &");
    system(cmd);
}

/* ---------- MAIN ---------- */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // CSS para headers
    const char *css_data =
        ".header-entry {"
        " background: #b9f6ca;"
        " color: #222;"
        " font-weight: bold;"
        "}";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);

    // Cargar Glade
    builder = gtk_builder_new_from_file("Equipos/equipos.glade");
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window_equipo"));
    grid_table = GTK_WIDGET(gtk_builder_get_object(builder, "grid_table"));
    GtkWidget *btn_run = GTK_WIDGET(gtk_builder_get_object(builder, "btn_run"));
    GtkWidget *btn_close = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));
    GtkWidget *btn_save = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save"));

    // Conectar guardar al handler
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_button_clicked_equipos), NULL);

    // Preguntar parámetros antes de mostrar la ventana principal
    EquiposData datos = equipos_dialog();
    if (datos.vida_util <= 0)
        return 0;

    // Construir tabla dinámica dentro del grid del glade
    build_table(datos);

    // Conectar botones principales
    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_run_button_clicked), NULL);
    g_signal_connect(btn_close, "clicked", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}