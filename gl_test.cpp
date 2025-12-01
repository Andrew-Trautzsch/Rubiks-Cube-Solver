#include <GL/freeglut.h>

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(-0.5f, -0.5f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f( 0.5f, -0.5f);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f( 0.0f,  0.5f);
    glEnd();

    glutSwapBuffers();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("OpenGL Test");

    glClearColor(0.f, 0.f, 0.f, 1.f);

    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
