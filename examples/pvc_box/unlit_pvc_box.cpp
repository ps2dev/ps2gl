#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "ps2gl/renderermanager.h"
#include <ps2s/debug_macros.h>

void init_lights_and_color();
void display();
void cube_position_and_rotation();
void draw_rgb_cube();
static void colored_vertex(float r, float g, float b, float nx, float ny, float nz, float x, float y, float z);
void reshape(int width, int height);
void perspective(float fov, float aspect, float nearClip, float farClip);

static float cube_spin_angle = 0.0f;
static float cube_z = -6.0f, cube_forward_rotation = -18.0f;

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);// | GLUT_DEPTH);
    glutInitWindowSize(640, 448);
    glutCreateWindow("RGB Cube");
    init_lights_and_color();
    mDebugPrint("Renderer = %s\n", pglGetCurRendererName());
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMainLoop();
    return 0;
}

void init_lights_and_color()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //REQUIRED
    cube_spin_angle += 0.2f;
    draw_rgb_cube();
    glLoadIdentity();
    glutSwapBuffers();
}

void draw_rgb_cube()
{
    cube_position_and_rotation();
    glBegin(GL_TRIANGLES);
    {
        // +Z (front):
        colored_vertex(1, 0, 0, 0, 0, 1, 1, 1, 1);
        colored_vertex(0, 1, 0, 0, 0, 1, -1, 1, 1);
        colored_vertex(0, 0, 1, 0, 0, 1, -1, -1, 1);
        colored_vertex(1, 0, 0, 0, 0, 1, 1, 1, 1);
        colored_vertex(0, 0, 1, 0, 0, 1, -1, -1, 1);
        colored_vertex(0, 1, 0, 0, 0, 1, 1, -1, 1);

        // -Z (back):
        colored_vertex(1, 0, 0, 0, 0, -1, 1, -1, -1);
        colored_vertex(0, 1, 0, 0, 0, -1, -1, -1, -1);
        colored_vertex(0, 0, 1, 0, 0, -1, -1, 1, -1);
        colored_vertex(1, 0, 0, 0, 0, -1, 1, -1, -1);
        colored_vertex(0, 0, 1, 0, 0, -1, -1, 1, -1);
        colored_vertex(0, 1, 0, 0, 0, -1, 1, 1, -1);

        // +Y (top):
        colored_vertex(1, 0, 0, 0, 1, 0, 1, 1, -1);
        colored_vertex(0, 1, 0, 0, 1, 0, -1, 1, -1);
        colored_vertex(0, 0, 1, 0, 1, 0, -1, 1, 1);
        colored_vertex(1, 0, 0, 0, 1, 0, 1, 1, -1);
        colored_vertex(0, 0, 1, 0, 1, 0, -1, 1, 1);
        colored_vertex(0, 1, 0, 0, 1, 0, 1, 1, 1);

        // -Y (bottom)
        colored_vertex(1, 0, 0, 0, -1, 0, 1, -1, 1);
        colored_vertex(0, 1, 0, 0, -1, 0, -1, -1, 1);
        colored_vertex(0, 0, 1, 0, -1, 0, -1, -1, -1);
        colored_vertex(1, 0, 0, 0, -1, 0, 1, -1, 1);
        colored_vertex(0, 0, 1, 0, -1, 0, -1, -1, -1);
        colored_vertex(0, 1, 0, 0, -1, 0, 1, -1, -1);

        // -X (left)
        colored_vertex(1, 0, 0, -1, 0, 0, -1, 1, 1);
        colored_vertex(0, 1, 0, -1, 0, 0, -1, 1, -1);
        colored_vertex(0, 0, 1, -1, 0, 0, -1, -1, -1);
        colored_vertex(1, 0, 0, -1, 0, 0, -1, 1, 1);
        colored_vertex(0, 0, 1, -1, 0, 0, -1, -1, -1);
        colored_vertex(0, 1, 0, -1, 0, 0, -1, -1, 1);

        // +X (right)
        colored_vertex(1, 0, 0, 1, 0, 0, 1, 1, -1);
        colored_vertex(0, 1, 0, 1, 0, 0, 1, 1, 1);
        colored_vertex(0, 0, 1, 1, 0, 0, 1, -1, 1);
        colored_vertex(1, 0, 0, 1, 0, 0, 1, 1, -1);
        colored_vertex(0, 0, 1, 1, 0, 0, 1, -1, 1);
        colored_vertex(0, 1, 0, 1, 0, 0, 1, -1, -1);
    }
    glEnd();
}

void cube_position_and_rotation()
{
    glTranslatef(0.0f, 0.0f, cube_z);
    glRotatef(cube_forward_rotation, -1, 0, 0);
    glRotatef(cube_spin_angle, 0.0f, 1.0f, 0.0f);
}

static void colored_vertex(const float r, const float g, const float b, const float nx, const float ny, const float nz,
                           const float x, const float y, const float z)
{
    glColor3f(r, g, b);
    // glNormal3f(nx, ny, nz); //for sending in ignored normals when we want to
    glVertex3f(x, y, z);
}

void reshape(const int width, int height)
{
    if (height == 0)
        height = 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION); //REQUIRED (or else black screen)
    perspective(40.0f, (float)width / (float)height, 0.1f, 4000.0f);
    glMatrixMode(GL_MODELVIEW); //REQUIRED (or else black screen)
}

void perspective(float fov, const float aspect, const float nearClip, const float farClip)
{
    fov *= 3.141592654f / 180.0f;
    const float height = 2.0f * nearClip * tanf(fov / 2.0f);
    const float width = height * aspect;
    glFrustum(-width / 2.0f, width / 2.0f, -height / 2.0f, height / 2.0f, nearClip, farClip);
}