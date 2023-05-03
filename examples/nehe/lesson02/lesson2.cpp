#include <GL/gl.h>   // The GL Header File
#include <GL/glut.h> // The GL Utility Toolkit (Glut) Header
#include <math.h>

//-----------------------------------------------------------------------------
void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    GLdouble xmin, xmax, ymin, ymax;

    ymax = zNear * tan(fovy * M_PI / 360.0f);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;

    glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

void init(GLvoid) // Create Some Everyday Functions
{

    glShadeModel(GL_SMOOTH);              // Enable Smooth Shading
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f); // Black Background
    glClearDepth(1.0f);                   // Depth Buffer Setup
    glEnable(GL_DEPTH_TEST);              // Enables Depth Testing
    glDepthFunc(GL_LEQUAL);               // The Type Of Depth Testing To Do
    //glEnable(GL_COLOR_MATERIAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void display(void)                                      // Create The Display Function
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear Screen And Depth Buffer
    glLoadIdentity();                                   // Reset The Current Modelview Matrix
    glTranslatef(-1.5f, 0.0f, -6.0f);                   // Move Left 1.5 Units And Into The Screen 6.0
    glBegin(GL_TRIANGLES);                              // Drawing Using Triangles
    glVertex3f(0.0f, 1.0f, 0.0f);                       // Top
    glVertex3f(-1.0f, -1.0f, 0.0f);                     // Bottom Left
    glVertex3f(1.0f, -1.0f, 0.0f);                      // Bottom Right
    glEnd();                                            // Finished Drawing The Triangle
    glTranslatef(3.0f, 0.0f, 0.0f);                     // Move Right 3 Units
    glBegin(GL_QUADS);                                  // Draw A Quad
    glVertex3f(-1.0f, 1.0f, 0.0f);                      // Top Left
    glVertex3f(1.0f, 1.0f, 0.0f);                       // Top Right
    glVertex3f(1.0f, -1.0f, 0.0f);                      // Bottom Right
    glVertex3f(-1.0f, -1.0f, 0.0f);                     // Bottom Left
    glEnd();


    glutSwapBuffers();
    // Swap The Buffers To Not Be Left With A Clear Screen
}

void reshape(int w, int h) // Create The Reshape Function (the viewport)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
    glLoadIdentity();            // Reset The Projection Matrix
    if (h == 0)                  // Calculate The Aspect Ratio Of The Window
        gluPerspective(80, (float)w, 1.0, 5000.0);
    else
        gluPerspective(80, (float)w / (float)h, 1.0, 5000.0);
    glMatrixMode(GL_MODELVIEW); // Select The Model View Matrix
    glLoadIdentity();           // Reset The Model View Matrix
}

int main(int argc, char **argv)                  // Create Main Function For Bringing It All Together
{
    glutInit(&argc, argv);                       // Erm Just Write It =)
    init();
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE); // Display Mode
    glutInitWindowSize(640, 448);                // If glutFullScreen wasn't called this is the window size
    glutCreateWindow("NeHe's OpenGL Framework"); // Window Title (argv[0] for current directory as title)
    glutDisplayFunc(display);                    // Matching Earlier Functions To Their Counterparts
    glutReshapeFunc(reshape);
    glutMainLoop();                              // Initialize The Main Loop

    return 0;
}
