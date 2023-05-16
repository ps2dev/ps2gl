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

#include "text_stuff.h"

/********************************************
 * prototypes
 */

void init(void);
void init_lights(void);
void set_light_positions();
void init_cylinder(void);
void draw_cylinder_points(void);
void draw_cylinder_lines(void);
void draw_cylinder_linestrips(void);
void draw_cylinder_tris(void);
void draw_cylinder_tristrip(void);
void draw_cylinder_fans(void);
void draw_cylinder_quads(void);
void draw_cylinder_quadstrips(void);

void display(void);
void reshape(int w, int h);
void key(unsigned char k, int x, int y);
void special(int key, int x, int y);

/********************************************
 * static data
 */

static float camera_z = 5.0f, camera_alt = 30, camera_azi = 0.0f;

static float *cylinder_points = NULL, *cylinder_normals = NULL;
static int cylinder_layers = 40, cylinder_slices = 40;

static char renderer_name[64];

static int render_ticks = 1, display_ticks = 1;
static CEETimer* timer1;

static bool wireframe = false, clipping = true;

static int cylinder_list = -1;

// lights

static int num_dir_lights = 0, num_pt_lights = 0;

typedef struct {
    float color[4];
    float position[4];
} light_t;

static light_t lights[8] = {
    { { 1, 0, 0, 0 }, { -1, 1, -1, 0 } },
    { { 0, 1, 0, 0 }, { -1, 1, 1, 0 } },
    { { 0, 0, 1, 0 }, { 1, 1, 1, 0 } },
    { { 1, 1, 0, 0 }, { 1, 1, -1, 0 } },

    { { 1, 0, 1, 0 }, { -1, -1, -1, 0 } },
    { { 1, .6f, 0, 0 }, { -1, -1, 1, 0 } },
    { { 0, .7f, 1, 0 }, { 1, -1, 1, 0 } },
    { { .4f, .4f, 1, 0 }, { 1, -1, -1, 0 } },
};

static int cur_prim = 4, num_prims = 8;
typedef void (*prim_draw_func_t)(void);
typedef struct {
    prim_draw_func_t draw_func;
    const char* name;
} prim_entry_t;

prim_entry_t prim_entries[8] = {
    { draw_cylinder_points, "GL_POINTS" },
    { draw_cylinder_lines, "GL_LINES" },
    { draw_cylinder_linestrips, "GL_LINE_STRIP" },
    { draw_cylinder_tris, "GL_TRIANGLES" },
    { draw_cylinder_tristrip, "GL_TRIANGLE_STRIP" },
    { draw_cylinder_fans, "GL_TRIANGLE_FAN" },
    { draw_cylinder_quads, "GL_QUADS" },
    { draw_cylinder_quadstrips, "GL_QUAD_STRIP" }
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
    init_cylinder();

    float cylinder_material[] = { .5f, .5f, .5f, .5f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cylinder_material);

    timer1 = new CEETimer(CEETimer::Timer1);
    timer1->SetResolution(CEETimer::BusClock_256th);
    pglSetRenderingFinishedCallback(rendering_finished);

    cylinder_list = glGenLists(1);
    glNewList(cylinder_list, GL_COMPILE);
    {
        (prim_entries[cur_prim].draw_func)();
    }
    glEndList();
}

void init_lights(void)
{
    GLfloat black[] = { 0, 0, 0, 0 };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    num_dir_lights = 1;
    num_pt_lights  = 0;

    for (int i = 0; i < 8; i++) {
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT, black);
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, lights[i].color);
        glLightfv(GL_LIGHT0 + i, GL_SPECULAR, black);
        glLightfv(GL_LIGHT0 + i, GL_POSITION, lights[i].position);

        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 0.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.5f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.0f);
    }
}

void set_light_positions()
{
    for (int i = 0; i < 8; i++)
        glLightfv(GL_LIGHT0 + i, GL_POSITION, lights[i].position);
}

void init_cylinder()
{
    glColor4f(1, 1, 1, 0.5f);

    if (cylinder_points != NULL) {
        free(cylinder_points);
        free(cylinder_normals);
    }

    cylinder_points  = (float*)malloc(cylinder_layers * cylinder_slices * 3 * sizeof(float));
    cylinder_normals = (float*)malloc(cylinder_layers * cylinder_slices * 3 * sizeof(float));

    float y_step   = 2.0f / (cylinder_layers - 1);
    float lat_step = 2.0f * Math::kPi / (cylinder_slices - 1);

    float *next_coord  = cylinder_points,
          *next_normal = cylinder_normals,
          current_y    = -1.0f;
    for (int layer = 0; layer < cylinder_layers; layer++) {
        for (int slice = 0; slice < cylinder_slices; slice++) {
            float cosine = cosf((float)slice * lat_step),
                  sine   = sinf((float)slice * lat_step);

            // vertex
            *next_coord++ = cosine;
            *next_coord++ = current_y;
            *next_coord++ = sine;

            // normal
            *next_normal++ = cosine;
            *next_normal++ = 0.0f;
            *next_normal++ = sine;
        }

        current_y += y_step;
    }
}

void draw_cylinder_points(void)
{
    glBegin(GL_POINTS);
    {
        for (int layer = 0; layer < (cylinder_layers - 1); layer++) {

            float *bottom_point  = cylinder_points + layer * cylinder_slices * 3,
                  *bottom_normal = cylinder_normals + layer * cylinder_slices * 3,
                  *top_point     = cylinder_points + (layer + 1) * cylinder_slices * 3,
                  *top_normal    = cylinder_normals + (layer + 1) * cylinder_slices * 3;

            for (int i = 0; i < cylinder_slices; i++) {
                glNormal3fv(top_normal);
                glVertex3fv(top_point);
                top_point += 3;
                top_normal += 3;

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);
                bottom_point += 3;
                bottom_normal += 3;
            }
        }
    }
    glEnd();
}

void draw_cylinder_lines(void)
{
    glBegin(GL_LINES);
    {

        for (int layer = 0; layer < cylinder_layers; layer++) {

            float *cur_point  = cylinder_points + layer * cylinder_slices * 3,
                  *cur_normal = cylinder_normals + layer * cylinder_slices * 3;

            for (int i = 0; i < cylinder_slices - 1; i++) {
                glNormal3fv(cur_normal);
                glVertex3fv(cur_point);
                cur_point += 3;
                cur_normal += 3;

                glNormal3fv(cur_normal);
                glVertex3fv(cur_point);
            }
        }
    }
    glEnd();
}

void draw_cylinder_linestrips(void)
{
    for (int layer = 0; layer < cylinder_layers; layer++) {

        float *cur_point  = cylinder_points + layer * cylinder_slices * 3,
              *cur_normal = cylinder_normals + layer * cylinder_slices * 3;

        glBegin(GL_LINE_STRIP);
        {
            for (int i = 0; i < cylinder_slices; i++) {
                glNormal3fv(cur_normal);
                glVertex3fv(cur_point);
                cur_point += 3;
                cur_normal += 3;
            }
        }
        glEnd();
    }
}

void draw_cylinder_tris(void)
{
    for (int layer = 0; layer < (cylinder_layers - 1); layer++) {

        float *bottom_point  = cylinder_points + layer * cylinder_slices * 3,
              *bottom_normal = cylinder_normals + layer * cylinder_slices * 3,
              *top_point     = cylinder_points + (layer + 1) * cylinder_slices * 3,
              *top_normal    = cylinder_normals + (layer + 1) * cylinder_slices * 3;

        glBegin(GL_TRIANGLES);
        {
            for (int i = 0; i < cylinder_slices * 2; i++) {
                if (i & 1) {
                    glNormal3fv(top_normal);
                    glVertex3fv(top_point);
                    top_point += 3;
                    top_normal += 3;

                    glNormal3fv(top_normal);
                    glVertex3fv(top_point);

                    glNormal3fv(bottom_normal);
                    glVertex3fv(bottom_point);
                } else {
                    glNormal3fv(top_normal);
                    glVertex3fv(top_point);

                    glNormal3fv(bottom_normal);
                    glVertex3fv(bottom_point);
                    bottom_point += 3;
                    bottom_normal += 3;

                    glNormal3fv(bottom_normal);
                    glVertex3fv(bottom_point);
                }
            }
        }
        glEnd();
    }
}

void draw_cylinder_tristrip(void)
{
    for (int layer = 0; layer < (cylinder_layers - 1); layer++) {

        float *bottom_point  = cylinder_points + layer * cylinder_slices * 3,
              *bottom_normal = cylinder_normals + layer * cylinder_slices * 3,
              *top_point     = cylinder_points + (layer + 1) * cylinder_slices * 3,
              *top_normal    = cylinder_normals + (layer + 1) * cylinder_slices * 3;

        glBegin(GL_TRIANGLE_STRIP);
        {
            for (int i = 0; i < cylinder_slices; i++) {
                glNormal3fv(top_normal);
                glVertex3fv(top_point);
                top_point += 3;
                top_normal += 3;

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);
                bottom_point += 3;
                bottom_normal += 3;
            }
        }
        glEnd();
    }
}

void draw_cylinder_fans(void)
{
    for (int layer = 0; layer < cylinder_layers - 2; layer += 2) {

        float *bottom_point  = cylinder_points + layer * cylinder_slices * 3,
              *bottom_normal = cylinder_normals + layer * cylinder_slices * 3,

              *mid_point  = cylinder_points + (layer + 1) * cylinder_slices * 3,
              *mid_normal = cylinder_normals + (layer + 1) * cylinder_slices * 3,

              *center_point  = mid_point + 3,
              *center_normal = mid_normal + 3,

              *top_point  = cylinder_points + (layer + 2) * cylinder_slices * 3,
              *top_normal = cylinder_normals + (layer + 2) * cylinder_slices * 3;

        for (int i = 0; i < cylinder_slices / 2; i++) {

            glBegin(GL_TRIANGLE_FAN);
            {
                glNormal3fv(center_normal);
                glVertex3fv(center_point);
                center_point += 3 * 2;
                center_normal += 3 * 2;

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);
                bottom_point += 3 * 2;
                bottom_normal += 3 * 2;

                glNormal3fv(mid_normal);
                glVertex3fv(mid_point);
                mid_point += 3 * 2;
                mid_normal += 3 * 2;

                glNormal3fv(top_normal);
                glVertex3fv(top_point);
                top_point += 3;
                top_normal += 3;

                glNormal3fv(top_normal);
                glVertex3fv(top_point);
                top_point += 3;
                top_normal += 3;

                glNormal3fv(top_normal);
                glVertex3fv(top_point);

                glNormal3fv(mid_normal);
                glVertex3fv(mid_point);

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);
                bottom_point -= 3;
                bottom_normal -= 3;

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);
                bottom_point -= 3;
                bottom_normal -= 3;

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);
                bottom_point += 3 * 2;
                bottom_normal += 3 * 2;
            }
            glEnd();
        }
    }
}

void draw_cylinder_quads(void)
{
    for (int layer = 0; layer < (cylinder_layers - 1); layer++) {

        float *bottom_point  = cylinder_points + layer * cylinder_slices * 3,
              *bottom_normal = cylinder_normals + layer * cylinder_slices * 3,
              *top_point     = cylinder_points + (layer + 1) * cylinder_slices * 3,
              *top_normal    = cylinder_normals + (layer + 1) * cylinder_slices * 3;

        glBegin(GL_QUADS);
        {
            for (int i = 0; i < cylinder_slices - 1; i++) {
                glNormal3fv(top_normal);
                glVertex3fv(top_point);
                top_point += 3;
                top_normal += 3;

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);
                bottom_point += 3;
                bottom_normal += 3;

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);

                glNormal3fv(top_normal);
                glVertex3fv(top_point);
            }
        }
        glEnd();
    }
}

void draw_cylinder_quadstrips(void)
{
    for (int layer = 0; layer < (cylinder_layers - 1); layer++) {

        float *bottom_point  = cylinder_points + layer * cylinder_slices * 3,
              *bottom_normal = cylinder_normals + layer * cylinder_slices * 3,
              *top_point     = cylinder_points + (layer + 1) * cylinder_slices * 3,
              *top_normal    = cylinder_normals + (layer + 1) * cylinder_slices * 3;

        glBegin(GL_QUAD_STRIP);
        {
            for (int i = 0; i < cylinder_slices; i++) {
                glNormal3fv(top_normal);
                glVertex3fv(top_point);
                top_point += 3;
                top_normal += 3;

                glNormal3fv(bottom_normal);
                glVertex3fv(bottom_point);
                bottom_point += 3;
                bottom_normal += 3;
            }
        }
        glEnd();
    }
}

void draw_text()
{
    glLoadIdentity();

    char buffer[1024];

    float frame_ticks  = 10000.0f; // bus is 150Mhz, timer is 1/256th
    float display_time = (float)display_ticks / frame_ticks * 100.0f;
    float render_time  = (float)render_ticks / frame_ticks * 100.0f;

    sprintf(buffer,
        "%d directional lights (up/down modifies)\n"
        "%d point lights       (right/left modifies)\n"
        "primitive is %s\n"
        "current renderer is \"%s\"\n"
        "\n"
        "(times not accurate -- see README)\n"
        "display() time: %3.1f%% (1/60 sec)\n"
        "render time:    %3.1f%% (1/60 sec)\n"
        "\n"
        "L1 toggles wireframe\n"
        "L2 toggles clipping\n"
        "R1/R2 changes prim type\n",
        num_dir_lights, num_pt_lights,
        prim_entries[cur_prim].name,
        renderer_name,
        display_time, render_time);

    tsDrawString(buffer);

    glDisable(GL_TEXTURE_2D);
}

//  #include "ps2s/utils.h"
//  #include "ps2s/core.h"
//  #include "../../vu1/vu1_mem_linear.h"

void display(void)
{
    timer1->Start();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    tsResetCursor();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // camera transform

    glTranslatef(0, 0, -camera_z);
    glRotatef(camera_azi, 0, 1, 0);
    glRotatef(camera_alt, 1, 0, 0);

    set_light_positions();

    // object space transforms

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    if (wireframe)
        glPolygonMode(GL_FRONT, GL_LINE);

    // (prim_entries[cur_prim].draw_func)();
    glCallList(cylinder_list);

    glPolygonMode(GL_FRONT, GL_FILL);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    glFlush();

    strncpy(renderer_name, pglGetCurRendererName(), 63);
    //     printf("renderer = %s\n", renderer_name );

    draw_text();

    //     float *material = (float*)Core::MemMappings::VU1Data + kMaterialEmission * 4;
    //     Utils::QwordFloatDump( material, 4 );
    //     tU32 *output = (tU32*)Core::MemMappings::VU1Data + kDoubleBufBase*4 + kOutputStart*4;
    //     Utils::QwordHexDump( output, 7 );
    //     printf("\n");

    display_ticks = timer1->GetTicks();
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
        camera_azi -= 2;
        break;
    case '6':
        camera_azi += 2;
        break;

    case 27: /* Escape */
        exit(0);
        break;
    default:
        return;
    }
}

void inc_dir_lights()
{
    if (num_dir_lights < 8) {
        num_dir_lights++;

        glEnable(GL_LIGHT0 + num_dir_lights - 1);

        lights[num_dir_lights - 1].position[3] = 0;
        glLightfv(GL_LIGHT0 + (num_dir_lights - 1),
            GL_POSITION,
            lights[num_dir_lights - 1].position);

        if (9 - num_pt_lights == num_dir_lights)
            num_pt_lights--;
    }
}

void dec_dir_lights()
{
    if (num_dir_lights > 0) {
        glDisable(GL_LIGHT0 + (num_dir_lights - 1));
        num_dir_lights--;
    }
}

void inc_pt_lights()
{
    if (num_pt_lights < 8) {
        num_pt_lights++;

        glEnable(GL_LIGHT0 + (8 - num_pt_lights));

        lights[8 - num_pt_lights].position[3] = 1;
        glLightfv(GL_LIGHT0 + (8 - num_pt_lights),
            GL_POSITION,
            lights[8 - num_pt_lights].position);

        if (9 - num_pt_lights == num_dir_lights)
            num_dir_lights--;
    }
}

void dec_pt_lights()
{
    if (num_pt_lights > 0) {
        glDisable(GL_LIGHT0 + (8 - num_pt_lights));
        num_pt_lights--;
    }
}

void special(int key, int x, int y)
{
    bool prim_changed = false;

    switch (key) {
    // arrow keys
    case GLUT_KEY_UP:
        inc_dir_lights();
        break;
    case GLUT_KEY_DOWN:
        dec_dir_lights();
        break;
    case GLUT_KEY_LEFT:
        dec_pt_lights();
        break;
    case GLUT_KEY_RIGHT:
        inc_pt_lights();
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
        cur_prim     = (cur_prim + 1) % num_prims;
        prim_changed = true;

        // camera_z -= 0.1f;
        break;
    case GLUT_KEY_PAGE_DOWN:
        cur_prim--;
        if (cur_prim < 0)
            cur_prim = num_prims - 1;
        prim_changed = true;

        // camera_z += 0.1f;
        break;
    }

    if (prim_changed) {
        glNewList(cylinder_list, GL_COMPILE);
        {
            (prim_entries[cur_prim].draw_func)();
        }
        glEndList();
    }
}
