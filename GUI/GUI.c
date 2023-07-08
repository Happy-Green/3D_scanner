#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

// Zmienne globalne przechowujące aktualne kąty obrotu
float angleX = 0.0;
float angleY = 0.0;
float X = 0.0;
float Y = 0.0;
float Z = 0.0;
float zoomFactor = 10;

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

// Funkcja obsługująca ruch myszą
void motion(int x, int y)
{
    // Obliczenie kątów obrotu na podstawie ruchu myszy
    angleX += (float)(x - angleX) * 0.1;
    angleY += (float)(y - angleY) * 0.1;

    // Ponowne wyświetlenie wykresu z nowymi kątami obrotu
    glutPostRedisplay();
}

// Funkcja obsługująca ruch myszą
void mouse(int button, int state, int x, int y)
{
    // Jeśli lewy przycisk myszy jest wciśnięty
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        // Zapisanie pozycji myszy
        angleX = x;
        angleY = y;
    }
    motion(x, y);
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'w':
        Y += 0.1;
        break;
    case 's':
        Y -= 0.1;
        break;
    case 'd':
        X += 0.1;
        break;
    case 'a':
        X -= 0.1;
        break;
    case '+': // klawisz plusa zwiększa skalę
        zoomFactor += 1;
        break;
    case '-': // klawisz minusa zmniejsza skalę
        zoomFactor -= 1;
        break;
    case 'q':
    case 'Q':
        exit(0);
        break;
    }
    glutPostRedisplay();
}

void display(void)
{
    // Wyczyszczenie ekranu
    glClear(GL_COLOR_BUFFER_BIT);

    // Ustawienie trybu wyświetlania 3D
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Przesunięcie kamery zgodnie z kątami obrotu
    glTranslatef(X, Y, -5.0);
    glRotatef(angleY, 1.0, 0.0, 0.0);
    glRotatef(angleX, 0.0, 1.0, 0.0);

    // Rysowanie wykresu typu scatter
    glPointSize(5.0);
    glBegin(GL_POINTS);
    struct Point3D points[MAX_POINTS];
    int num_points = read_points("test.txt", points, MAX_POINTS);
    if (num_points < 0)
    {
        printf("Error reading file\n");
        exit(-1);
    }

    for (int i = 0; i < num_points; i++)
    {
        glColor3f(1.0, 1.0, 1.0);
        glVertex3f(points[i].x * zoomFactor, points[i].y * zoomFactor, points[i].z * zoomFactor);
    }
    glEnd();

    // Wyświetlenie wykresu
    glutSwapBuffers();
}

int main(int argc, char **argv)
{
    // Inicjalizacja biblioteki GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Scatter Plot");

    // Rejestracja funkcji wyświetlania
    glutDisplayFunc(display);

    // Rejestracja funkcji obsługującej ruch myszą
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(motion);

    /// Ustawienie koloru tła
    glClearColor(0.0, 0.0, 0.0, 0.0);

    // Uruchomienie głównej pętli programu
    glutMainLoop();

    return 0;
}
