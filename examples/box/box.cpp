/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "GL/gl.h"
#include "GL/glut.h"

#include "text_stuff.h"

/********************************************
 * prototypes
 */

void init(void);
void init_lights(void);
void set_light_positions();

void draw_cube(void);

void display(void);
void reshape(int w, int h);
void key(unsigned char k, int x, int y);
void special(int key, int x, int y);

/********************************************
 * static data
 */

static float camera_z = 5.0f, camera_alt = 0.0f, camera_azi = 0.0f;
static float spin_angle = 0.0f;

/********************************************
 * code
 */

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    // need to set up the gl context with glutInit before calling init()
    init();

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);

    glutMainLoop();

    return 0;
}

void init(void)
{
    // load the font
    tsLoadFont();

    glClearColor(0.3f, 0.3f, 0.4f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);

    init_lights();
}

static GLfloat l0_position[] = { -1, 0.9f, 0.9f, 0 };
static GLfloat l1_position[] = { -2, -1, 1, 0.0 };

void init_lights(void)
{
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };

    GLfloat l0_diffuse[] = { 1.0f, 1.0f, 1.0f, 0 };

    GLfloat l1_diffuse[] = { .6f, .2f, .2f, 0.0f };

    GLfloat black[] = { 0, 0, 0, 0 };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    // a directional light

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, l0_diffuse);

    // a point light

    glLightfv(GL_LIGHT1, GL_AMBIENT, black);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, l1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, black);

    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.005f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0f);
}

void set_light_positions()
{
    glLightfv(GL_LIGHT0, GL_POSITION, l0_position);
    glLightfv(GL_LIGHT1, GL_POSITION, l1_position);
}

void draw_cube(void)
{
    static float points[8][3] = {
        { -1, 1, -1 },
        { -1, 1, 1 },
        { 1, 1, 1 },
        { 1, 1, -1 },

        { -1, -1, -1 },
        { -1, -1, 1 },
        { 1, -1, 1 },
        { 1, -1, -1 }
    };

    static float material_diff_amb[] = { 0.5f, 0.5f, 0.5f, 0 };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material_diff_amb);

    glBegin(GL_QUADS);
    {
        // top
        glNormal3f(0, 1, 0);
        glVertex3fv(points[0]);
        glVertex3fv(points[1]);
        glVertex3fv(points[2]);
        glVertex3fv(points[3]);

        // bottom
        glNormal3f(0, -1, 0);
        glVertex3fv(points[7]);
        glVertex3fv(points[6]);
        glVertex3fv(points[5]);
        glVertex3fv(points[4]);

        // left
        glNormal3f(-1, 0, 0);
        glVertex3fv(points[0]);
        glVertex3fv(points[4]);
        glVertex3fv(points[5]);
        glVertex3fv(points[1]);

        // right
        glNormal3f(1, 0, 0);
        glVertex3fv(points[2]);
        glVertex3fv(points[6]);
        glVertex3fv(points[7]);
        glVertex3fv(points[3]);

        // front
        glNormal3f(0, 0, 1);
        glVertex3fv(points[1]);
        glVertex3fv(points[5]);
        glVertex3fv(points[6]);
        glVertex3fv(points[2]);

        // back
        glNormal3f(0, 0, -1);
        glVertex3fv(points[3]);
        glVertex3fv(points[7]);
        glVertex3fv(points[4]);
        glVertex3fv(points[0]);
    }
    glEnd();
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    tsResetCursor();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // camera transform

    glTranslatef(0, 0, -camera_z);
    glRotatef(camera_azi, 0, 1, 0);
    glRotatef(camera_alt, -1, 0, 0);

    set_light_positions();

    // object space transforms

    glRotatef(spin_angle, 0, 1, 0);
    spin_angle += 0.15f;

    draw_cube();

    glLoadIdentity();

    tsDrawString("Wow! A cube!\n");
    tsDrawString("Left D-Pad rotates the camera\n");
    tsDrawString("R1 and R2 move in and out\n");
    glDisable(GL_TEXTURE_2D);

    glFlush();

    glutPostRedisplay();
    glutSwapBuffers();
}

void perspective(float fov, float aspect, float nearClip, float farClip)
{
    float w, h;
    fov *= 3.141592654f / 180.0f;
    h = 2.0f * nearClip * (float)tanf(fov / 2.0f);
    w = h * aspect;

    glFrustum(-w / 2.0f, w / 2.0f, -h / 2.0f, h / 2.0f, nearClip, farClip);
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    perspective(40.0f, (float)w / (float)h, 1.0f, 4000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void key(unsigned char k, int x, int y)
{
    switch (k) {
    case '8':
        break;
    case '2':
        break;
    case '4':
        break;
    case '6':
        break;

    case 27: /* Escape */
        exit(0);
        break;
    default:
        return;
    }
}

void special(int key, int x, int y)
{
    switch (key) {
    // arrow keys
    case GLUT_KEY_UP:
        camera_alt -= 3;
        break;
    case GLUT_KEY_DOWN:
        camera_alt += 3;
        break;
    case GLUT_KEY_LEFT:
        camera_azi -= 3;
        break;
    case GLUT_KEY_RIGHT:
        camera_azi += 3;
        break;

    // left shoulder
    case GLUT_KEY_HOME:
        break;
    case GLUT_KEY_END:
        break;

    // right shoulder
    case GLUT_KEY_PAGE_UP:
        camera_z -= 0.1f;
        break;
    case GLUT_KEY_PAGE_DOWN:
        camera_z += 0.1f;
        break;
    }
}
