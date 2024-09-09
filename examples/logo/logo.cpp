/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "GL/gl.h"
#include "GL/glut.h"

#include "ps2glmesh.h"

/********************************************
 * for textures
 */

struct rtHeader {
    unsigned int ID; //1

    unsigned short VerHi;
    unsigned short VerLo;

    int Width;
    int Height;

    void* Texels;    // patched to texel data pointer
    int TexelFormat; // 2
    int TexelQSize;  // size of pixel data in Q words for DMA

    void* Clut;
    int ClutFormat;   // format of clut
    int ClutQSize;    // in Q words
    int nClutEntries; // size of pixel data in Q words for DMA
};

/********************************************
 * prototypes
 */

rtHeader* LoadRTexFile(char* fileName, unsigned int texName);
ps2glMeshHeader* LoadMesh(const char* fileName);
void DrawMesh(const void* header);

void init(void);
void init_lights(void);
void init_models(void);

void display(void);
void reshape(int w, int h);
void key(unsigned char k, int x, int y);
void special(int key, int x, int y);

void camera_xform(void);

/********************************************
 * static data
 */

float logo_alt = 10.5, logo_azi = 2.5;
float camera_z = 29.0f, camera_alt = 9.0f, camera_azi = 36.0f;
float shape_spin_angle = 0.0f;

bool view_xform_changed = true;

GLint ps2_list, gl_list, wet_list, circle_list, tri_list, square_list, x_list;

/********************************************
 * code
 */

int main(int argc, char** argv)
{
    int dummy_argc         = 1;
    char iop_module_path[] = "iop_module_path=host0:/usr/local/sce/iop/modules";
    char* dummy_argv[1];
    dummy_argv[0] = iop_module_path;

    glutInit(&dummy_argc, (char**)dummy_argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow(argv[0]);

    init();

    glutInitWindowPosition(0, 0);
    glutInitWindowSize(300, 300);
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutMainLoop();

    return 0;
}

void init(void)
{
    glClearColor(0.3f, 0.3f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glEnable(GL_RESCALE_NORMAL);

    init_lights();
    init_models();
}

void init_models(void)
{
    unsigned int tex_ids[3];
    ps2glMeshHeader* mesh;

    // glEnable( GL_TEXTURE_2D );

    glGenTextures(3, tex_ids);

    // ps2

    ps2_list = glGenLists(7);
    mesh     = LoadMesh("ps2.gl");
    LoadRTexFile("plywd_b.rtx", tex_ids[0]);
    glNewList(ps2_list, GL_COMPILE);
    {
        glPushMatrix();
        glRotatef(180, 0, 1, 0);
        glBindTexture(GL_TEXTURE_2D, tex_ids[0]);
        DrawMesh(mesh);
    }
    glEndList();

    // gl

    gl_list = ps2_list + 1;
    mesh    = LoadMesh("gl.gl");
    LoadRTexFile("plywd_y.rtx", tex_ids[1]);
    glNewList(gl_list, GL_COMPILE);
    {
        glBindTexture(GL_TEXTURE_2D, tex_ids[1]);
        DrawMesh(mesh);
    }
    glEndList();

    // wet paint

    wet_list = ps2_list + 2;
    mesh     = LoadMesh("note.gl");
    LoadRTexFile("wetpaint.rtx", tex_ids[2]);
    glNewList(wet_list, GL_COMPILE);
    {
        glBindTexture(GL_TEXTURE_2D, tex_ids[2]);
        DrawMesh(mesh);
        glPopMatrix();
    }
    glEndList();

    // circle

    circle_list = ps2_list + 3;
    mesh        = LoadMesh("cir.gl");
    glNewList(circle_list, GL_COMPILE);
    {
        DrawMesh(mesh);
    }
    glEndList();

    // square

    square_list = ps2_list + 4;
    mesh        = LoadMesh("sq.gl");
    glNewList(square_list, GL_COMPILE);
    {
        DrawMesh(mesh);
    }
    glEndList();

    // triangle

    tri_list = ps2_list + 5;
    mesh     = LoadMesh("tri.gl");
    glNewList(tri_list, GL_COMPILE);
    {
        DrawMesh(mesh);
    }
    glEndList();

    // x

    x_list = ps2_list + 6;
    mesh   = LoadMesh("x.gl");
    glNewList(x_list, GL_COMPILE);
    {
        DrawMesh(mesh);
    }
    glEndList();
}

void init_lights(void)
{
    // lighting
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };

    GLfloat l0_position[] = { 0, 1.0, 1.0, 0.0 };
    GLfloat l0_diffuse[]  = { 1.0f, 1.0f, 1.0f, 0 };

    GLfloat l1_position[] = { 0.0, -20.0, -80.0, 1.0 };
    // GLfloat l1_position[] = {0, -1, 1, 0.0};
    GLfloat l1_diffuse[] = { .6f, .6f, .6f, 0.0f };

    GLfloat black[] = { 0, 0, 0, 0 };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, l0_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, l0_position);

    glLightfv(GL_LIGHT1, GL_AMBIENT, black);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, l1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, black);
    glLightfv(GL_LIGHT1, GL_POSITION, l1_position);

    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.005f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0f);
}

void camera_xform(void)
{
    glTranslatef(0, 0, -camera_z);
    glRotatef(camera_azi, 0, 1, 0);
    glRotatef(camera_alt, -1, 0, 0);
}

void display(void)
{
    // materials

    static float black[]     = { 0, 0, 0, 0 };
    static float shininess[] = { 32.0f };

    static float ps2_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    static float ps2_specular[] = { 0.24f, 0.08f, 0.4f, 0 };

    static float gl_diffuse[]  = { 0.6f, 0.6f, 0.6f, 1.0f };
    static float gl_specular[] = { 0.2f, 0.2f, 0.2f, 1.0f };

    static float wet_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    static float wet_specular[] = { 0, 0, 0, 0 };

    static float shapes_diffuse[]  = { 0.07, 0.07, 0.15, 0.2f };
    static float shapes_specular[] = { 0, 0, 0, 0 };
    static float shapes_emission[] = { 0.0, 0.0, 0.0f, 0 };

    // this doesn't actually work..
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    camera_xform();

    // ps2gl logo

    glMaterialfv(GL_FRONT, GL_EMISSION, black);

    glEnable(GL_CULL_FACE);

    glPushMatrix();
    {
        glRotatef(-5, 0, 0, 1);
        glRotatef(logo_azi, 0, 1, 0);
        glRotatef(logo_alt, -1, 0, 0);

        glEnable(GL_TEXTURE_2D);

        // ps2

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ps2_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, ps2_specular);
        glCallList(ps2_list);

        // gl

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gl_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, gl_specular);
        glCallList(gl_list);

        // wet paint

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wet_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, wet_specular);
        glCallList(wet_list);
    }
    glPopMatrix();

    // background shapes

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    // circle

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, shapes_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, shapes_specular);
    glMaterialfv(GL_FRONT, GL_EMISSION, black);

    glPushMatrix();
    glRotatef(180, 0, 1, 0);
    glTranslatef(-7, 0.5f, 62);
    glRotatef(shape_spin_angle, 0, 1, 0);
    glScalef(1.2, 1.2, 1.2);
    glCallList(circle_list);
    glPopMatrix();

    // square

    glPushMatrix();
    glRotatef(180, 0, 1, 0);
    glTranslatef(-90, 1.5f, 54.5f);
    glRotatef(shape_spin_angle, 0, 1, 0);
    glScalef(1.2, 1.2, 1.2);
    glCallList(square_list);
    glPopMatrix();

    // x

    glPushMatrix();
    glRotatef(180, 0, 1, 0);
    glTranslatef(-38, 20, 81);
    glRotatef(shape_spin_angle, 0, 1, 0);
    glScalef(1.2, 1.2, 1.2);
    glCallList(x_list);
    glPopMatrix();

    // triangle

    glPushMatrix();
    glRotatef(180, 0, 1, 0);
    glTranslatef(-71, 26, 68);
    glRotatef(shape_spin_angle, 0, 1, 0);
    glScalef(1.2, 1.2, 1.2);
    glCallList(tri_list);
    glPopMatrix();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    shape_spin_angle += 0.15f;

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
        logo_alt -= 1.5f;
        break;
    case '2':
        logo_alt += 1.5f;
        break;
    case '4':
        logo_azi -= 1.5f;
        break;
    case '6':
        logo_azi += 1.5f;
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
        view_xform_changed = true;
        break;
    case GLUT_KEY_DOWN:
        camera_alt += 3;
        view_xform_changed = true;
        break;
    case GLUT_KEY_LEFT:
        camera_azi -= 3;
        view_xform_changed = true;
        break;
    case GLUT_KEY_RIGHT:
        camera_azi += 3;
        view_xform_changed = true;
        break;

    // left shoulder
    case GLUT_KEY_HOME:
        break;
    case GLUT_KEY_END:
        break;

    // right shoulder
    case GLUT_KEY_PAGE_UP:
        view_xform_changed = true;
        camera_z += 1;
        break;
    case GLUT_KEY_PAGE_DOWN:
        view_xform_changed = true;
        camera_z -= 1;
        break;
    }
}

/********************************************
 * mesh and textures
 */

void* ReadFile(char* name, unsigned int& size, int pad)
{
    int infile   = -1;
    void* buffer = 0;
    int success  = 0;

    size = 0;
    if (pad)
        pad = 1;

    infile = open(name, O_RDONLY);
    if (infile != -1) {
        size = lseek(infile, 0, SEEK_END);
        lseek(infile, 0, SEEK_SET);
        if (!size) {
            printf("file '%s' was null - not reading", name);
        } else {
            buffer = pglutAllocDmaMem(size + pad);

            if (!buffer) {
                printf("file '%s' failed to allocate", name);
            } else {
                if ((unsigned int)read(infile, buffer, size) != size) {
                    printf("file '%s' failed to read", name);
                } else {
                    if (pad) {
                        *(((unsigned char*)buffer) + size) = 0;
                    }
                    success = 1;
                }
            }
        }
    }

    if (infile != -1) {
        close(infile);
    }
    if (!success) {
        if (buffer)
            free(buffer);
        buffer = 0;
        printf("failed to read file '%s'\n", name);
    } else {
        printf("read %d bytes of file '%s'\n", size, name);
    }
    return buffer;
}

ps2glMeshHeader*
LoadMesh(const char* fileName)
{
    unsigned int size;
    return (ps2glMeshHeader*)ReadFile((char*)fileName, size, 0);
}

void DrawMesh(const void* header)
{
    int numStrips = ((int*)header)[0], numVertices = ((int*)header)[1];
    float* vertices  = (float*)header + 2 + numStrips;
    float* normals   = vertices + numVertices * 3;
    float* texCoords = normals + numVertices * 3;

    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glNormalPointer(GL_FLOAT, 0, normals);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

    int* curStripLength = (int*)header + 2;
    int curOffset       = 0;
    for (int i = 0; i < numStrips; i++, curStripLength++) {
        glDrawArrays(GL_TRIANGLE_STRIP, curOffset, *curStripLength);
        curOffset += *curStripLength;
    }

    printf("drawing %d vertices in %d strips (avg length = %f)\n",
        numVertices, numStrips, (float)numVertices / (float)numStrips);
}

#define RTEX_ID 'RTEX'
#define RTEX_VER_LO 1
#define RTEX_VER_HI 3

#define RTEX_FORMAT_NULL 0
#define RTEX_FORMAT_RGBA8 1
#define RTEX_FORMAT_INDEX8 2 // 256 colour palette

#include "ps2s/types.h" // for tU128

void reorderClut(unsigned int* clut)
{
    tU128 buffer, *entries_1, *entries_2;
    entries_1 = (tU128*)clut + 2;
    entries_2 = entries_1 + 2;
    unsigned int i;
    for (i = 0; i < 8; i++) {
        buffer     = *entries_1;
        *entries_1 = *entries_2;
        *entries_2 = buffer;

        entries_1++;
        entries_2++;

        buffer     = *entries_1;
        *entries_1 = *entries_2;
        *entries_2 = buffer;

        entries_1 += 7;
        entries_2 += 7;
    }
}

rtHeader*
LoadRTexFile(char* fileName, unsigned int texName)
{
    rtHeader* ramImage = NULL;

    unsigned int size;
    ramImage = (rtHeader*)ReadFile(fileName, size, 0);
    if (!ramImage) {
        printf("mTex::readfile - failed\n");
        exit(-1);
    }
    if (ramImage->VerHi != RTEX_VER_HI) {
        printf("mTex::Load_ram(). version HI wrong for texture (expected %d, got %d)\n", RTEX_VER_HI, ramImage->VerHi);
        exit(-1);
    }
    if (ramImage->VerLo != RTEX_VER_LO) {
        printf("mTex::Load_ram(). WARNING version LO wrong for texture - carry on anyway\n");
    }
    printf("mTex: loaded texture [%4d %4d] ''\n", ramImage->Width, ramImage->Height);

    if (ramImage->Texels) {
        ramImage->Texels = (void*)((unsigned int)ramImage + (unsigned int)ramImage->Texels);
        printf("found a %d quad words of texture\n", ramImage->TexelQSize);
    } else {
        printf("had no pixels!!\n");
    }

    if (ramImage->Clut) {
        ramImage->Clut = (void*)((unsigned int)ramImage + (unsigned int)ramImage->Clut);
        printf("found a %d entry palette\n", ramImage->nClutEntries);
        reorderClut((unsigned int*)ramImage->Clut);
    } else {
        ramImage->Clut = NULL;
    }

    glBindTexture(GL_TEXTURE_2D, texName);
    int format = (ramImage->Clut) ? GL_COLOR_INDEX : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, 3,
        ramImage->Width, ramImage->Height, 0,
        format, GL_UNSIGNED_BYTE, ramImage->Texels);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (ramImage->Clut) {
        glColorTable(GL_COLOR_TABLE, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
            ramImage->Clut);
    }

    return ramImage;
}
