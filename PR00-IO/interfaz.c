#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <stdio.h>

  GtkWidget *window;
  GtkWidget *fixed1;
  GtkWidget *button1;
  GtkWidget *label1;
  GtkBuilder *builder;
  
int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);
  
  builder = gtk_builder_new_from_file ("interfaz.glade");
  
  window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  
  gtk_builder_connect_signals(builder, NULL);
  
  fixed1 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed1"));
  button1 = GTK_WIDGET(gtk_builder_get_object(builder, "button1"));
  label1 = GTK_WIDGET(gtk_builder_get_object(builder, "label1"));
  
  gtk_widget_show(window  );
  
  gtk_main();
  
  return EXIT_SUCCESS;
}

void on_button1_clicked(GtkButton *b){
  gtk_label_set_text (GTK_LABEL(label1), (const gchar*) "Clickeo el boton 1");
}

void on_button2_clicked(GtkButton *b){
  gtk_label_set_text (GTK_LABEL(label1), (const gchar*) "Clickeo el boton 2");
}

void on_button3_clicked(GtkButton *b){
  gtk_label_set_text (GTK_LABEL(label1), (const gchar*) "Clickeo el boton 3");
}

void on_button4_clicked(GtkButton *b){
  gtk_label_set_text (GTK_LABEL(label1), (const gchar*) "Clickeo el boton 4");
}

void on_button_salir_clicked(GtkButton *b) {
    gtk_main_quit();
}
