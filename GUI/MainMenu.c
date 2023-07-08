
#include <GL/gl.h>
#include <GL/glu.h>
#include <gtk-3.0/gtk/gtk.h>

static GtkWidget *glarea;
// Zmienne globalne przechowujące aktualne kąty obrotu
float angleX = 0.0;
float angleY = 0.0;
float X = 0.0;
float Y = 0.0;
float Z = 0.0;
float zoomFactor = 1;

#define MAX_POINTS 100000

struct Point3D
{
    float x, y, z;
};

int read_points(const char *filename, struct Point3D *points, int max_points)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    int i = 0;
    while (i < max_points)
    {
        if (fscanf(fp, "%f %f %f", &points[i].x, &points[i].y, &points[i].z) != 3)
        {
            break;
        }
        i++;
    }

    fclose(fp);
    return i;
}

static gboolean render(GtkGLArea *area, GdkGLContext *context)
{
    gtk_gl_area_make_current(area);

    glClearColor(1, 1, 1, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1, 0, 0);
    glBegin(GL_TRIANGLES);
    glVertex3f(0.0, 1.0, 0.0);
    glVertex3f(-1.0, -1.0, 0.0);
    glVertex3f(1.0, -1.0, 0.0);
    glEnd();

    return TRUE;
}

static void on_start_scan_button_clicked(GtkWidget *widget, gpointer data)
{
    // TUTAJ DODAJ KOD OBSŁUGI KLIKNIECIA PRZYCISKU "START SCAN"
}

static void on_export_button_clicked(GtkWidget *widget, gpointer data)
{
    // TUTAJ DODAJ KOD OBSŁUGI KLIKNIECIA PRZYCISKU "START SCAN"
}

static void on_exit_button_clicked(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Skaner - KD AB - SWIS23L");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    glarea = gtk_gl_area_new();
    gtk_widget_set_hexpand(glarea, TRUE);
    gtk_widget_set_vexpand(glarea, TRUE);
    g_signal_connect(glarea, "render", G_CALLBACK(render), NULL);
    gtk_grid_attach(GTK_GRID(grid), glarea, 0, 0, 1, 1);

    GtkWidget *button_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(grid), button_grid);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(box, 200, 200);
    gtk_grid_attach(GTK_GRID(button_grid), box, 0, 0, 1, 1);

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file("obrazek.jpg", NULL);
    GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 200, 200, GDK_INTERP_BILINEAR);
    GtkWidget *image = gtk_image_new_from_pixbuf(scaled_pixbuf);
    gtk_box_pack_start(GTK_BOX(box), image, TRUE, TRUE, 0);

    GtkWidget *start_scan_button = gtk_button_new_with_label("Start Scan");
    gtk_widget_set_size_request(start_scan_button, 200, 100); // ustawienie maksymalnej wysokości przycisku
    g_signal_connect(start_scan_button, "clicked", G_CALLBACK(on_start_scan_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(button_grid), start_scan_button, 0, 1, 1, 1);

    GtkWidget *exit_button = gtk_button_new_with_label("Exit");
    gtk_widget_set_size_request(exit_button, 100, 100);
    g_signal_connect(exit_button, "clicked", G_CALLBACK(on_exit_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(button_grid), exit_button, 0, 3, 1, 1);

    GtkWidget *export_button = gtk_button_new_with_label("Exportuj do pliku typu: xyz");
    gtk_widget_set_size_request(export_button, 100, 100);
    g_signal_connect(export_button, "clicked", G_CALLBACK(on_export_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(button_grid), export_button, 0, 2, 1, 1);

    GtkWidget *label = gtk_label_new("Instrukcje\n\r \
      W   - Przesun do gory kamere\n\r \
      S   - Przesun w dol kamere\n\r \
      A   - Przesun w prawo kamere\n\r \
      D   - Przesun w lewo kamere\n\r \
      +   - Powieksz skan\n\r \
      -   - Pomniejsz skan\n\r \
     Mysz - Rotacja obiektu\n");
    gtk_grid_attach(GTK_GRID(button_grid), label, 0, 4, 2, 1);

    
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
