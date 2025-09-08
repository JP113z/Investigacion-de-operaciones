
// Requiere: pdflatex, evince
// sudo apt install texlive-latex-base texlive-latex-extra texlive-fonts-recommended
// sudo apt install evince

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXN 8
#define INF 1000000000

GtkWidget *grid_table;
GtkWidget *name_row[MAXN], *name_col[MAXN];
GtkWidget *entry_matrix[MAXN][MAXN];
int current_n = 0;
gboolean updating = FALSE;
gboolean loaded_from_file = FALSE;

// prototipos
int ask_nodes_dialog();
void build_table();
int parse_val(const char *txt);
void run_floyd(GtkWidget *btn, gpointer data);
void on_aceptar_clicked(GtkButton *button, gpointer dialog);
void on_header_changed(GtkEditable *editable, gpointer index_ptr);

// Memoria para snapshots
int **alloc_int_matrix(int n)
{
    int **m = malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++)
        m[i] = malloc(n * sizeof(int));
    return m;
}
void free_int_matrix(int **m, int n)
{
    for (int i = 0; i < n; i++)
        free(m[i]);
    free(m);
}

// Cambiar filas y columnas
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

void on_entry_insert_text(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data)
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
    {
        g_signal_stop_emission_by_name(editable, "insert-text");
        return;
    }
}

// crear el grid en interfaz
void build_table()
{
    // limpiar grid
    GList *children = gtk_container_get_children(GTK_CONTAINER(grid_table));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    // crear encabezados de columnas
    for (int j = 0; j < current_n; j++)
    {
        GtkWidget *entry = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid_table), entry, j + 1, 0, 1, 1);
        name_col[j] = entry;
    }

    // crear filas + matriz
    for (int i = 0; i < current_n; i++)
    {
        GtkWidget *entry = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid_table), entry, 0, i + 1, 1, 1);
        name_row[i] = entry;

        // conectar señales de sincronización
        g_signal_connect(entry, "changed", G_CALLBACK(on_header_changed), GINT_TO_POINTER(i));
        g_signal_connect(name_col[i], "changed", G_CALLBACK(on_header_changed), GINT_TO_POINTER(i));

        for (int j = 0; j < current_n; j++)
        {
            GtkWidget *e = gtk_entry_new();
            gtk_grid_attach(GTK_GRID(grid_table), e, j + 1, i + 1, 1, 1);
            entry_matrix[i][j] = e;
            g_signal_connect(e, "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
        }
    }

    gtk_widget_show_all(grid_table);
}

void init_defaults()
{
    // nombres por defecto A, B, C...
    for (int j = 0; j < current_n; j++)
    {
        char buf[2] = {'A' + j, '\0'};
        gtk_entry_set_text(GTK_ENTRY(name_col[j]), buf);
        gtk_entry_set_text(GTK_ENTRY(name_row[j]), buf);
    }

    // valores de la matriz
    for (int i = 0; i < current_n; i++)
    {
        for (int j = 0; j < current_n; j++)
        {
            if (i == j)
            {
                gtk_entry_set_text(GTK_ENTRY(entry_matrix[i][j]), "0");
                gtk_editable_set_editable(GTK_EDITABLE(entry_matrix[i][j]), FALSE);
            }
            else
            {
                gtk_entry_set_text(GTK_ENTRY(entry_matrix[i][j]), "INF");
                gtk_editable_set_editable(GTK_EDITABLE(entry_matrix[i][j]), TRUE);
            }
        }
    }
}

// parser de inf o numero
int parse_val(const char *txt)
{
    if (!txt)
        return INF;
    if (strcasecmp(txt, "*") == 0 || strlen(txt) == 0)
        return INF;
    // permitir espacios
    char *end;
    long v = strtol(txt, &end, 10);
    if (end == txt)
        return INF;
    return (int)v;
}

// Dialog para preguntar nodos iniciales o cargar archivo
void on_aceptar_clicked(GtkButton *button, gpointer dialog)
{
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

void on_load_button_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog_parent = GTK_WIDGET(data);

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

    // leer numero de nodos
    if (!fgets(buffer, sizeof(buffer), f))
    {
        fclose(f);
        g_free(filename);
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    int file_n = atoi(buffer);
    if (file_n < 1 || file_n > MAXN)
    {
        fclose(f);
        g_free(filename);
        return;
    }

    current_n = file_n;

    // reconstruir la tabla
    build_table();

    // leer nombres
    for (int i = 0; i < current_n; i++)
    {
        if (!fgets(buffer, sizeof(buffer), f))
        {
            fclose(f);
            g_free(filename);
            return;
        }
        buffer[strcspn(buffer, "\r\n")] = 0;
        gtk_entry_set_text(GTK_ENTRY(name_row[i]), buffer);
        gtk_entry_set_text(GTK_ENTRY(name_col[i]), buffer);
    }

    for (int i = 0; i < current_n; i++)
    {
        for (int j = 0; j < current_n; j++)
        {
            if (!fgets(buffer, sizeof(buffer), f))
            {
                fclose(f);
                g_free(filename);
                return;
            }
            buffer[strcspn(buffer, "\r\n")] = 0;
            gtk_entry_set_text(GTK_ENTRY(entry_matrix[i][j]), buffer);
        }
    }

    fclose(f);
    g_free(filename);

    loaded_from_file = TRUE;

    gtk_dialog_response(GTK_DIALOG(dialog_parent), GTK_RESPONSE_APPLY);
}

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

    GtkWidget *btn_load = gtk_button_new_with_label("Load");
    gtk_box_pack_start(GTK_BOX(vbox), btn_load, FALSE, FALSE, 8);
    gtk_widget_set_halign(btn_load, GTK_ALIGN_CENTER);

    g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_button_clicked), dialog);

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
    else if (response == GTK_RESPONSE_APPLY)
    {
        val = current_n;
    }
    gtk_widget_destroy(dialog);
    return val;
}

void on_save_button_clicked(GtkWidget *widget, gpointer data)
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

    // Confirmar si existe un archivo con el mismo nombre
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    // Filtro para archivos .txt
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_filter_set_name(filter, "Archivos de texto (*.txt)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    // Nombre sugerido por defecto
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "config.txt");

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

    // guardar número de nodos
    fprintf(f, "%d\n", current_n);

    // guardar nombres
    for (int i = 0; i < current_n; i++)
    {
        fprintf(f, "%s\n", gtk_entry_get_text(GTK_ENTRY(name_row[i])));
    }

    // guardar matriz
    for (int i = 0; i < current_n; i++)
    {
        for (int j = 0; j < current_n; j++)
        {
            fprintf(f, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_matrix[i][j])));
        }
    }

    fclose(f);
    g_free(filename);
}

// -------------------------------  Crea la ruta optima entre cada nodo ----------------------------------------------------------------
char *build_route_string(int i, int j, int **P, char names[][32], int n)
{

    typedef struct
    {
        int a, b;
    } Seg;
    Seg segs[512];
    int sstart = 0, send = 0;
    segs[send++] = (Seg){i, j};

    int nodes[1024];
    int nodes_len = 0;

    while (sstart < send)
    {
        Seg cur = segs[sstart++];
        int a = cur.a, b = cur.b;
        int k = P[a][b];
        if (k == -1)
        { // si el camino es directo
            // agrega el primer nodo
            if (nodes_len == 0 || nodes[nodes_len - 1] != a)
                nodes[nodes_len++] = a;
            // agrega el segundo nodo
            if (nodes_len == 0 || nodes[nodes_len - 1] != b)
                nodes[nodes_len++] = b;
        }
        else
        {
            // si hay parada entre los nodos divide en dos para agregar cada segmento
            // cuidar si n es mayor a 8 puede fallar
            for (int t = send + 1; t > sstart; t--)
                segs[t] = segs[t - 1];
            segs[sstart] = (Seg){a, k};
            segs[sstart + 1] = (Seg){k, b};
            send += 1;
        }
    }

    // construye el string de resultado
    char *res = malloc(1024);
    res[0] = '\0';
    for (int t = 0; t < nodes_len; t++)
    {
        strcat(res, names[nodes[t]]);
        if (t < nodes_len - 1)
            strcat(res, " -- ");
    }
    // Asegura que el destino final esté en la ruta
    if (nodes_len == 0 || nodes[nodes_len - 1] != j)
    {
        if (nodes_len > 0)
            strcat(res, " -- ");
        strcat(res, names[j]);
    }

    return res;
}

// --------------------------------------- Escritura en latex ------------------------------------------------
void write_tex(const char *fname, int n, int ***snapD, int ***snapP, char names[][32], int steps)
{
    FILE *f = fopen(fname, "w");
    if (!f)
    {
        perror("fopen tex");
        return;
    }

    fprintf(f, "%% Generado por floyd.c\n");
    fprintf(f, "\\documentclass[a4paper,11pt]{article}\n");
    fprintf(f, "\\usepackage[utf8]{inputenc}\n");
    fprintf(f, "\\usepackage{tikz}\n");
    fprintf(f, "\\usepackage{array}\n");
    fprintf(f, "\\usepackage[table]{xcolor}\n");
    fprintf(f, "\\usepackage{longtable}\n");
    fprintf(f, "\\usepackage{geometry}\n");
    fprintf(f, "\\geometry{margin=1.5cm}\n");
    fprintf(f, "\\begin{document}\n");

    // Portada
    fprintf(f, "\\begin{center}\n{\\LARGE \\textbf{Proyecto 1: Algoritmo de Floyd}}\\\\[2cm]\n");
    fprintf(f, "{\\large Investigacion de Operaciones\\\\[2cm]\n");
    fprintf(f, "{\\large Integrantes: }\\\\[1cm]\n");
    fprintf(f, "{\\large Jose Pablo Fernandez Jimenez - 2023117752}\\\\[1cm]\n");
    fprintf(f, "{\\large Diego Duran Rodriguez - 2022437509}\\\\[2cm]\n");
    fprintf(f, "{\\large Segundo semestre 2025\\\\[1cm]\n");
    fprintf(f, "\\end{center}\n\\newpage\n");

    // Descripcion alogirtmo
    fprintf(f, "\\section*{Algoritmo de Floyd}\n");
    fprintf(f, "Descripcion\n\n");
    fprintf(f, "\\bigskip\n");

    // Dibujo del grafo con tikz (nodos en círculo)
    fprintf(f, "\\section*{Problema}\n");
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.2,\n");
    fprintf(f, "  every node/.style={circle, draw, fill=blue!10, minimum size=10mm},\n");
    fprintf(f, "  every edge/.style={draw, thick}]\n");

    double radius = 4.0; // Separacion de los nodos
    for (int i = 0; i < n; i++)
    {
        double ang = 2.0 * 3.141592653589793 * i / n;
        double x = radius * cos(ang);
        double y = radius * sin(ang);
        fprintf(f, "\\node (N%d) at (%.3f, %.3f) {%s};\n", i, x, y, names[i]);
    }

    // aristas con pesos desde D(0)
    int **D0 = snapD[0];
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            if (i == j)
                continue;
            int val = D0[i][j];
            if (val >= INF)
                continue;
            const char *bend = ((i + j) % 2 == 0) ? "bend left=15" : "bend right=15"; // para que no se encimen las aristas
            fprintf(f, "\\draw[->, thick, >=latex, red] (N%d) to[%s] node[midway, above, draw=none, fill=none, rectangle, font=\\small\\sffamily] {%d} (N%d);\n", i, bend, val, j);
        }
    fprintf(f, "\\end{tikzpicture}\n\\end{center}\n\\newpage\n");

    // Tablas iniciales
    fprintf(f, "\\section*{Tablas iniciales}\n");
    fprintf(f, "\\subsection*{D(0)}\n");
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tabular}{c|");
    for (int j = 0; j < n; j++)
        fprintf(f, "c");
    fprintf(f, "}\n");
    fprintf(f, " & ");
    for (int j = 0; j < n; j++)
    {
        fprintf(f, "%s", names[j]);
        if (j < n - 1)
            fprintf(f, " & ");
    }
    fprintf(f, " \\\\ \\hline\n");
    for (int i = 0; i < n; i++)
    {
        fprintf(f, "%s & ", names[i]);
        for (int j = 0; j < n; j++)
        {
            int val = snapD[0][i][j];
            if (val >= INF)
                fprintf(f, "$\\infty$");
            else
                fprintf(f, "%d", val);
            if (j < n - 1)
                fprintf(f, " & ");
        }
        fprintf(f, " \\\\\n");
    }
    fprintf(f, "\\end{tabular}\n\\end{center}\n");

    // P(0)
    fprintf(f, "\\subsection*{P(0)}\n");
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tabular}{c|");
    for (int j = 0; j < n; j++)
        fprintf(f, "c");
    fprintf(f, "}\n");
    fprintf(f, " & ");
    for (int j = 0; j < n; j++)
    {
        fprintf(f, "%s", names[j]);
        if (j < n - 1)
            fprintf(f, " & ");
    }
    fprintf(f, " \\\\ \\hline\n");
    for (int i = 0; i < n; i++)
    {
        fprintf(f, "%s & ", names[i]);
        for (int j = 0; j < n; j++)
        {
            int v = snapP[0][i][j];
            if (v == -1)
                fprintf(f, "0");
            else
                fprintf(f, "%s", names[v]);
            if (j < n - 1)
                fprintf(f, " & ");
        }
        fprintf(f, " \\\\\n");
    }
    fprintf(f, "\\end{tabular}\n\\end{center}\n\\newpage\n");

    // Tablas intermedias (D(k), P(k)), marcar cambios
    fprintf(f, "\\section*{Tablas intermedias}\n");
    for (int k = 1; k < steps; k++)
    {
        fprintf(f, "\\section*{Calculo de D(%d)}\n", k);
        fprintf(f, "\\subsection*{D(%d)}\n", k);
        fprintf(f, "\\begin{center}\n");
        fprintf(f, "\\begin{tabular}{c|");
        for (int j = 0; j < n; j++)
            fprintf(f, "c");
        fprintf(f, "}\n");
        fprintf(f, " & ");
        for (int j = 0; j < n; j++)
        {
            fprintf(f, "%s", names[j]);
            if (j < n - 1)
                fprintf(f, " & ");
        }
        fprintf(f, " \\\\ \\hline\n");
        for (int i = 0; i < n; i++)
        {
            fprintf(f, "%s & ", names[i]);
            for (int j = 0; j < n; j++)
            {
                int prev = snapD[k - 1][i][j];
                int cur = snapD[k][i][j];
                if (cur != prev)
                { // revisa si hay un cambio en D para escribirlo en tabla P
                    if (cur >= INF)
                        fprintf(f, "\\textcolor{orange}{$\\infty$}");
                    else
                        fprintf(f, "\\textcolor{orange}{%d}", cur);
                }
                else
                {
                    if (cur >= INF)
                        fprintf(f, "$\\infty$");
                    else
                        fprintf(f, "%d", cur);
                }
                if (j < n - 1)
                    fprintf(f, " & ");
            }
            fprintf(f, " \\\\\n");
        }
        fprintf(f, "\\end{tabular}\n\\end{center}\n");

        fprintf(f, "\\subsection*{P(%d)}\n", k);
        fprintf(f, "\\begin{center}\n");
        fprintf(f, "\\begin{tabular}{c|");
        for (int j = 0; j < n; j++)
            fprintf(f, "c");
        fprintf(f, "}\n");
        fprintf(f, " & ");
        for (int j = 0; j < n; j++)
        {
            fprintf(f, "%s", names[j]);
            if (j < n - 1)
                fprintf(f, " & ");
        }
        fprintf(f, " \\\\ \\hline\n");
        for (int i = 0; i < n; i++)
        {
            fprintf(f, "%s & ", names[i]);
            for (int j = 0; j < n; j++)
            {
                int v = snapP[k][i][j];
                if (v == -1)
                    fprintf(f, "-");
                else
                    fprintf(f, "%s", names[v]);
                if (j < n - 1)
                    fprintf(f, " & ");
            }
            fprintf(f, " \\\\\n");
        }
        fprintf(f, "\\end{tabular}\n\\end{center}\n\\newpage\n");
    }

    // Distancias y rutas óptimas
    fprintf(f, "\\section*{Distancias y rutas \\'optimas}\n");
    fprintf(f, "\\begin{longtable}{c c c p{7cm}}\n");
    fprintf(f, "\\hline\nOrigen & Destino & Distancia & Ruta \\\\\\hline\n\\endhead\n");

    int **Dfinal = snapD[steps - 1];
    int **Pfinal = snapP[steps - 1];
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            if (i == j)
                continue;
            fprintf(f, "%s & %s & ", names[i], names[j]);
            int d = Dfinal[i][j];
            if (d >= INF)
            {
                fprintf(f, "$\\infty$ & (no existe ruta) \\\\\n");
            }
            else
            {
                // reconstruye la ruta
                char *route = build_route_string(i, j, Pfinal, (char (*)[32])names, n);
                fprintf(f, "%d & %s \\\\\n", d, route);
                free(route);
            }
        }
    fprintf(f, "\\hline\n\\end{longtable}\n");

    fprintf(f, "\\end{document}\n");
    fclose(f);
}

/* ---------- run_floyd: guarda snapshots, escribe tex, compila y abre PDF ---------- */
void run_floyd(GtkWidget *btn, gpointer data)
{
    int n = current_n;
    int **D = alloc_int_matrix(n);
    int **P = alloc_int_matrix(n);

    // obtener nombres
    char names[MAXN][32];
    for (int i = 0; i < n; i++)
    {
        const char *t = gtk_entry_get_text(GTK_ENTRY(name_row[i]));
        if (!t || strlen(t) == 0)
            snprintf(names[i], sizeof(names[i]), "%c", 'A' + i);
        else
        {
            strncpy(names[i], t, sizeof(names[i]) - 1);
            names[i][31] = 0;
        }
    }

    // leer D(0), inicializar P
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            const char *txt = gtk_entry_get_text(GTK_ENTRY(entry_matrix[i][j]));
            D[i][j] = parse_val(txt);
            P[i][j] = -1;
        }

    // snapshots: steps = n+1 (D0 .. Dn)
    int steps = n + 1;
    int ***snapD = malloc(steps * sizeof(int **));
    int ***snapP = malloc(steps * sizeof(int **));

    // almacenar D(0) y P(0)
    snapD[0] = alloc_int_matrix(n);
    snapP[0] = alloc_int_matrix(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            snapD[0][i][j] = D[i][j];
            snapP[0][i][j] = P[i][j];
        }

    // Imprimir D(0) y P(0) por consola
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
    printf("\nP(0):\n");
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (P[i][j] == -1)
                printf(" - ");
            else
                printf("%3d ", P[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    // Floyd con snapshots
    for (int k = 0; k < n; k++)
    {
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                long dik = D[i][k];
                long dkj = D[k][j];
                if (dik >= INF || dkj >= INF)
                    continue;
                if (D[i][j] > dik + dkj)
                {
                    D[i][j] = D[i][k] + D[k][j];
                    P[i][j] = k;
                }
            }
        }
        // almacenar snapshot después de usar k
        snapD[k + 1] = alloc_int_matrix(n);
        snapP[k + 1] = alloc_int_matrix(n);
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
            {
                snapD[k + 1][i][j] = D[i][j];
                snapP[k + 1][i][j] = P[i][j];
            }

        // imprimir por consola también
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
        printf("\nP(%d):\n", k + 1);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                if (P[i][j] == -1)
                    printf(" - ");
                else
                    printf("%3d ", P[i][j]);
            }
            printf("\n");
        }
        printf("\n");
    }

    // Generar LaTeX
    const char *texname = "floyd_output.tex";
    write_tex(texname, n, snapD, snapP, names, steps);

    // Compilar con pdflatex (dos veces)
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pdflatex -interaction=nonstopmode %s > /dev/null 2>&1", texname);
    system(cmd);
    system(cmd);

    // Abrir PDF con evince en modo presentación (lanzar en background)
    snprintf(cmd, sizeof(cmd), "evince --presentation floyd_output.pdf &");
    system(cmd);

    // Liberar memoria de snapshots
    for (int s = 0; s < steps; s++)
    {
        free_int_matrix(snapD[s], n);
        free_int_matrix(snapP[s], n);
    }
    free(snapD);
    free(snapP);
    free_int_matrix(D, n);
    free_int_matrix(P, n);
}

/* ---------- main ---------- */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // cargar interfaz
    GtkBuilder *builder = gtk_builder_new_from_file("PR01/floyd.glade");
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window_floyd"));
    grid_table = GTK_WIDGET(gtk_builder_get_object(builder, "grid_table"));
    GtkWidget *btn_run = GTK_WIDGET(gtk_builder_get_object(builder, "btn_run"));
    GtkWidget *btn_close = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));
    GtkWidget *btn_save = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save"));
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_button_clicked), NULL);

    // preguntar antes de mostrar ventana principal
    current_n = ask_nodes_dialog();
    if (current_n <= 0)
        return 0;

    if (!loaded_from_file)
    {
        build_table();
        init_defaults();
    }
    g_signal_connect(btn_run, "clicked", G_CALLBACK(run_floyd), NULL);
    g_signal_connect(btn_close, "clicked", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
