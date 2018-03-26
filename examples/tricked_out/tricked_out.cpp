/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GL/gl.h"
#include "GL/glut.h"
#include "GL/ps2gl.h"

#include "ps2s/eetimer.h"
#include "ps2s/math.h"

#include "file_ops.h"
#include "text_stuff.h"

#include "billboard_renderer.h"

/********************************************
 * prototypes
 */

void init(void);
void init_lights(void);
void init_billboards();
void load_bb_texture();
void set_light_positions();

void draw_cube();
void draw_text();
void draw_billboards();

void display(void);
void reshape(int w, int h);
void key(unsigned char k, int x, int y);
void special(int key, int x, int y);

/********************************************
 * static data
 */

static float camera_z = 10.0f, camera_alt = 30, camera_azi = 0.0f;

static char renderer_name[64];

static int render_ticks = 1, display_ticks = 1;
static CEETimer* timer1;

static bool wireframe = false, clipping = true;

// billboards

static float* billboard_points = NULL;
static int NumBillboards       = 20;
static unsigned int bb_tex_id  = 0;

// lights

static int num_dir_lights = 0, num_pt_lights = 0;

typedef struct {
    float color[4];
    float position[4];
} light_t;

static light_t lights[8] = {
    { { 1, 1, 1, 0 }, { -1, 1, -1, 0 } },
    { { 0, 1, 0, 0 }, { -1, 1, 1, 0 } },
    { { 0, 0, 1, 0 }, { 1, 1, 1, 0 } },
    { { 1, 1, 0, 0 }, { 1, 1, -1, 0 } },

    { { 1, 0, 1, 0 }, { -1, -1, -1, 0 } },
    { { 1, .6f, 0, 0 }, { -1, -1, 1, 0 } },
    { { 0, .7f, 1, 0 }, { 1, -1, 1, 0 } },
    { { .4f, .4f, 1, 0 }, { 1, -1, -1, 0 } },
};

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

void rendering_finished(void)
{
    render_ticks = timer1->GetTicks();
}

void init(void)
{
    // load the font
    tsLoadFont();

    glClearColor(0.3f, 0.3f, 0.4f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);

    init_lights();

    float material[] = { .5f, .5f, .5f, .5f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material);

// how nice of them to invert the set of reserved timers when building
// linux...
#ifdef PS2_LINUX
    timer1 = new CEETimer(CEETimer::Timer3);
#else
    timer1 = new CEETimer(CEETimer::Timer1);
#endif
    timer1->SetResolution(CEETimer::BusClock_256th);
    pglSetRenderingFinishedCallback(rendering_finished);

    // add our billboard prim type
    CBillboardRenderer::Register();

    init_billboards();
}

void init_lights(void)
{
    GLfloat black[] = { 0, 0, 0, 0 };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

    num_dir_lights = 3;
    num_pt_lights  = 0;

    for (int i = 0; i < 8; i++) {
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT, black);
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, lights[i].color);
        glLightfv(GL_LIGHT0 + i, GL_SPECULAR, black);

        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 0.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.0f);
    }
}

void init_billboards()
{
    // let's randomly generate "NumBillboards" billboards on the surface
    // of a sphere..

    billboard_points = (float*)pglutAllocDmaMem(NumBillboards * 4 * sizeof(float));

    srand(12341234);

    float* next_pt = billboard_points;
    float radius   = 3;
    for (int i = 0; i < NumBillboards; i++) {
        float altitude = rand() % 360;
        float azimuth  = rand() % 360;

        // let's have sizes range between 0.5 and 1.5
        float size = (float)rand() / (float)RAND_MAX;
        size += 0.5f;

        next_pt[0] = radius * sinf(azimuth) * cosf(altitude);
        next_pt[1] = radius * sinf(azimuth) * sinf(altitude);
        next_pt[2] = radius * cosf(azimuth);
        next_pt[3] = size;

        next_pt += 4;
    }

    load_bb_texture();
}

void load_bb_texture()
{
    // the texture file is just an rgba image with no header info
    int tex_fd = open(FILE_PREFIX "car.bin", O_RDONLY);
    assert(tex_fd != -1);

    int image_size = 128 * 128 * 4; // 128x128 32-bit image
    void* texels   = pglutAllocDmaMem(image_size);
    int bytes_read = read(tex_fd, texels, image_size);
    assert(bytes_read == image_size);

    // make the gl texture

    glGenTextures(1, &bb_tex_id);
    glBindTexture(GL_TEXTURE_2D, bb_tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, 3,
        128, 128, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, texels);
}

void set_light_positions()
{
    for (int i = 0; i < 8; i++)
        glLightfv(GL_LIGHT0 + i, GL_POSITION, lights[i].position);
}

void display(void)
{
    timer1->Start();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    tsResetCursor();

    // camera transform

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(0, 0, -camera_z);
    glRotatef(camera_azi, 0, 1, 0);
    glRotatef(camera_alt, 1, 0, 0);

    set_light_positions();

    if (wireframe)
        glPolygonMode(GL_FRONT, GL_LINE);

    draw_cube();

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    draw_billboards();

    glFlush();

    glPolygonMode(GL_FRONT, GL_FILL);
    glDisable(GL_ALPHA_TEST);

    strncpy(renderer_name, pglGetCurRendererName(), 64);

    draw_text();

    display_ticks = timer1->GetTicks();
}

/********************************************
 * draw functions
 */

void draw_text()
{
    glLoadIdentity();

    char buffer[1024];

    float frame_ticks  = 10000.0f; // bus is 150Mhz, timer is 1/256th
    float display_time = (float)display_ticks / frame_ticks * 100.0f;
    float render_time  = (float)render_ticks / frame_ticks * 100.0f;

    sprintf(buffer,
        "%d directional lights\n"
        "%d point lights\n"
        "current renderer is \"%s\"\n"
        "\n"
        "display() time: %3.1f%% (1/60 sec)\n"
        "render time:    %3.1f%% (1/60 sec)\n"
        "\n"
        "L1 toggles wireframe\n"
        "L2 toggles clipping\n"
        "R1/R2 moves camera in/out\n",
        num_dir_lights, num_pt_lights, renderer_name,
        display_time, render_time);

    tsDrawString(buffer);

    glDisable(GL_TEXTURE_2D);
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

    static float material_diff_amb[] = { 0.5f, 0.5f, 0.5f, 1 };
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

void draw_billboards()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bb_tex_id);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // the color of the billboards
    glColor3f(1, 1, 1);

    // we can now draw billboards in Begin/End pairs..

    glBegin(kBillboardPrimType);
    {
        glVertex4f(3, 0, 0, 0.5f);
        glVertex4f(-3, 0, 0, 1);
    }
    glEnd();

    // ..or, more efficiently, in DrawArrays

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(4, GL_FLOAT, 0, billboard_points);
    glDrawArrays(kBillboardPrimType, 0, NumBillboards);

    glDisable(GL_TEXTURE_2D);
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

/********************************************
 * key/pad handlers
 */

void key(unsigned char k, int x, int y)
{
    switch (k) {
    case '8':
        camera_alt += 3;
        break;
    case '2':
        camera_alt -= 3;
        break;
    case '4':
        camera_azi += 3;
        break;
    case '6':
        camera_azi -= 3;
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
        break;
    case GLUT_KEY_DOWN:
        break;
    case GLUT_KEY_LEFT:
        break;
    case GLUT_KEY_RIGHT:
        break;

    // left shoulder
    case GLUT_KEY_HOME:
        wireframe = !wireframe;
        break;
    case GLUT_KEY_END:
        clipping = !clipping;
        if (clipping)
            pglEnable(PGL_CLIPPING);
        else
            pglDisable(PGL_CLIPPING);
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
