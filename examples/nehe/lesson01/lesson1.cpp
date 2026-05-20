#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>

// Simple replacement for gluPerspective (GLU not available on PS2)
void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    GLdouble xmin, xmax, ymin, ymax;

    ymax = zNear * tan(fovy * M_PI / 360.0f);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;

    glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

// Global Variables
bool g_gamemode = false;
bool g_fullscreen = false;

// Our GL Specific Initializations
bool init(void)
{
    glShadeModel(GL_SMOOTH);                            // Enable Smooth Shading
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);               // Black Background
    glClearDepth(1.0f);                                 // Depth Buffer Setup
    glEnable(GL_DEPTH_TEST);                            // Enables Depth Testing
    glDepthFunc(GL_LEQUAL);                             // The Type Of Depth Testing To Do
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Nice perspective
    return true;
}

// Our Rendering Is Done Here
void render(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear Screen And Depth Buffer
    glLoadIdentity();                                   // Reset The Current Modelview Matrix

    // Swap The Buffers To Become Our Rendering Visible
    glutSwapBuffers();
}

// Our Reshaping Handler (Required Even In Fullscreen-Only Modes)
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);     // Select The Projection Matrix
    glLoadIdentity();                // Reset The Projection Matrix
    if (h == 0) h = 1;
    gluPerspective(80, (float)w/(float)h, 1.0, 5000.0);
    glMatrixMode(GL_MODELVIEW);      // Select The Modelview Matrix
    glLoadIdentity();                // Reset The Modelview Matrix
}

// Our Keyboard Handler (Normal Keys)
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 27:        // When Escape Is Pressed...
            exit(0);    // Exit The Program
        break;          // Ready For Next Case
        default:        // Now Wrap It Up
        break;
    }
}

// Our Keyboard Handler For Special Keys (Like Arrow Keys And Function Keys)
void special_keys(int a_keys, int x, int y)
{
    // PS2 GLUT does not support the original F1/fullscreen logic from NeHe.
    // Keep special keys no-op for compatibility with ps2glut's limited mapping.
    (void)a_keys; (void)x; (void)y;
}

int main(int argc, char** argv)
{
    // Note: original NeHe uses a Windows MessageBox to ask for gamemode.
    // For PS2 we default to windowed mode.
    glutInit(&argc, argv);                           // GLUT Initializtion
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);     // Display Mode (Rgb And Double Buffered)
    glutInitWindowSize(640, 448);                // If glutFullScreen wasn't called this is the window size
    glutCreateWindow("NeHe's OpenGL Framework - Lesson 01"); // Window Title
    init();                                          // Our Initialization
    glutDisplayFunc(render);                         // Register The Display Function
    glutReshapeFunc(reshape);                        // Register The Reshape Handler
    glutKeyboardFunc(keyboard);                      // Register The Keyboard Handler
    glutSpecialFunc(special_keys);                   // Register Special Keys Handler
    glutMainLoop();                                  // Go To GLUT Main Loop
    return 0;
}
