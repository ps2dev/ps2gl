/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdio.h>
#include <stdlib.h>

#include "graph.h"

#include "GL/glut.h"
#include "GL/ps2gl.h"

#include "ps2s/core.h"
#include "ps2s/displayenv.h"
#include "ps2s/drawenv.h"
#include "ps2s/gs.h"
#include "ps2s/timer.h"

#include "ps2gl/debug.h"
#include "ps2gl/displaycontext.h"
#include "ps2gl/drawcontext.h"
#include "ps2gl/glcontext.h"

#include "pads.h"

/********************************************
 * some function pointer types
 */

typedef void (*tFunctionPtr_ii)(int, int);
typedef void (*tFunctionPtr_ucii)(unsigned char, int, int);
typedef void (*tFunctionPtr_iii)(int, int, int);
typedef void (*tFunctionPtr)(void);
typedef void (*tFunctionPtr_i)(int);

/********************************************
 * function prototypes
 */

static void normal_main_loop();
static void initGsMemory();
static void do_keys();

/********************************************
 * local data
 */

tFunctionPtr DisplayFunc       = NULL;
tFunctionPtr_ii ReshapeFunc    = NULL;
tFunctionPtr_ucii KeyboardFunc = NULL;
tFunctionPtr_i VisibilityFunc  = NULL;
tFunctionPtr IdleFunc          = NULL;
tFunctionPtr_iii SpecialFunc   = NULL;

static CEETimer* Timer0;

static bool printTimes = false;

/********************************************
 * macros
 */

/********************************************
 * glut implementation (loosely speaking..)
 */

/**
 * @defgroup glut_api ps2glut API
 *
 * As the name implies, this is a [very incomplete] glut implementation for the ps2.
 * In general it does three things:  initializes the ps2gl library, provides minimal
 * pad support through the "keyboard" and "special" callback functions, and does
 * a simple double-buffered display loop.
 *
 * ps2glut was written to help test ps2gl against the many existing
 * glut samples/demos, but might be helpful in writing quick prototypes.
 * Please note that it is <b>not intended for game development</b>.
 *
 * ps2glut will also do some rough timing of the callback functions (using
 * timer0).  Press the 'start' button to display the timings on stdout.
 *
 * @{
 */

/**
 * Initialize the ps2glut library, also the ps2gl library and gs memory
 * if not already initialized by the app.  (Remember that the app must
 * reset the machine and graphics mode before calling pglInit!)
 *
 * @param argcp a pointer to the number of elements in argv
 * @param argvp command line args.
 */
void glutInit(int* argcp, char** argv)
{
    Pads::Init();

    // does the ps2gl library need to be initialized?

    if (!pglHasLibraryBeenInitted()) {
        // reset the machine
        //      sceDevVif0Reset();
        //      sceDevVu0Reset();
        //      sceDmaReset(1);
        //      sceGsResetPath();

        // Reset the GIF. OSDSYS leaves PATH3 busy, that ends up having
        // our PATH1/2 transfers ignored by the GIF.
        *GIF::Registers::ctrl = 1;

        //      sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FRAME);

        mWarn("ps2gl library has not been initialized by the user; using default values.");
        int immBufferVertexSize = 64 * 1024;
        pglInit(immBufferVertexSize, 1000);
    }

    // does gs memory need to be initialized?

    if (!pglHasGsMemBeenInitted()) {
        mWarn("GS memory has not been allocated by the user; using default values.");
        initGsMemory();
    }
}

/**
 * Set the display function callback.  The callback will be called once per frame.
 */
void glutDisplayFunc(void (*func)(void))
{
    DisplayFunc = func;
}

/**
 * Set the reshape function callback.  This will be called once before entering
 * the main loop.  (At the moment ps2glut is fixed to set up a full-screen display --
 * 640x448, interlaced.)
 */
void glutReshapeFunc(void (*func)(int width, int height))
{
    ReshapeFunc = func;
}

/**
 * Set the keyboard function callback.  The square, triangle, circle, and x buttons
 * are mapped to the numbers 4, 8, 6, and 2, respectively.  The callback is
 * called once per frame while for each button that is held down.
 */
void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y))
{
    KeyboardFunc = func;
}

/**
 * Set the visibility function callback.  The callback will be called once
 * before the main loop with the argument 'GLUT_VISIBLE.'  For
 * compatibility.
 */
void glutVisibilityFunc(void (*func)(int state))
{
    VisibilityFunc = func;
}

/**
 * Set the idle function callback.  The callback will be called once per frame,
 * after the display callback.
 */
void glutIdleFunc(void (*func)(void))
{
    IdleFunc = func;
}

/**
 * Set the special function callback.  The left dpad buttons will be mapped
 * to the arrow keys (GLUT_KEY_UP/DOWN/LEFT/RIGHT) and called intermittently,
 * similar to a pc keyboard.
 */
void glutSpecialFunc(void (*func)(int key, int x, int y))
{
    SpecialFunc = func;
}

/**
 * Enter the main loop.  Since this is the ps2 and not a pc, this function will
 * not return.
 */
void glutMainLoop(void)
{
    mErrorIf(DisplayFunc == NULL, "ps2glut:  No display function!");

    if (VisibilityFunc)
        VisibilityFunc(GLUT_VISIBLE);

    normal_main_loop();
}

/** @} */ // glut_api

void glutInitDisplayMode(unsigned int mode)
{
    mNotImplemented();
}

void glutInitWindowPosition(int x, int y)
{
    mNotImplemented();
}

void glutInitWindowSize(int x, int y)
{
    mNotImplemented();
}

int glutCreateWindow(const char* title)
{
    mNotImplemented();

    return 1;
}

void glutPostRedisplay(void)
{
    // mNotImplemented( );
}

void glutSwapBuffers(void)
{
}

int glutGet(GLenum type)
{
    mNotImplemented();
    return 0;
}

void* pglutAllocDmaMem(unsigned int num_bytes)
{
    // don't really need memalign, but it's clearer..
    return memalign(16, num_bytes);
}

void pglutFreeDmaMem(void* mem)
{
    free(mem);
}

/********************************************
 * local function definitions
 */

void normal_main_loop()
{
    bool firstTime = true;

    // init the timing system

    Timer0 = new CEETimer(CEETimer::Timer0);
    mInitTimers(Timer0, CEETimer::BusClock_256th);

    if (ReshapeFunc)
        ReshapeFunc(640, 448);

    while (1) {
        mUpdateTimers();

        mStartTimer("frame time");

        Pads::Read();

        if (Pad0.WasPushed(Pads::kStart))
            printTimes = true;

        do_keys();

        if (DisplayFunc) {
            mStartTimer("DisplayFunc()");
            pglBeginGeometry();
            DisplayFunc();
            pglEndGeometry();
            mStopTimer("DisplayFunc()");
        }

        if (IdleFunc) {
            mStartTimer("IdleFunc()");
            IdleFunc();
            mStopTimer("IdleFunc()");
        }

        mStartTimer("wait for vu1");
        if (!firstTime)
            pglFinishRenderingGeometry(PGL_DONT_FORCE_IMMEDIATE_STOP);
        else
            firstTime = false;
        mStopTimer("wait for vu1");

        mStopTimer("frame time");

        if (printTimes) {
            mDisplayTimers();
            printTimes = false;

            // pglPrintGsMemAllocation();
        }

        pglWaitForVSync();
        pglSwapBuffers();
        pglRenderGeometry();
    }
}

void do_keys()
{
    static int frameCount = 0;

    if (SpecialFunc) {
        if (Pad0.WasPushed(Pads::kLeftUp)) {
            SpecialFunc(GLUT_KEY_UP, 0, 0);
            frameCount = 0;
        }
        if (Pad0.WasPushed(Pads::kLeftDown)) {
            SpecialFunc(GLUT_KEY_DOWN, 0, 0);
            frameCount = 0;
        }
        if (Pad0.WasPushed(Pads::kLeftRight)) {
            SpecialFunc(GLUT_KEY_RIGHT, 0, 0);
            frameCount = 0;
        }
        if (Pad0.WasPushed(Pads::kLeftLeft)) {
            SpecialFunc(GLUT_KEY_LEFT, 0, 0);
            frameCount = 0;
        }

        if (Pad0.WasPushed(Pads::kL1)) {
            SpecialFunc(GLUT_KEY_HOME, 0, 0);
            frameCount = 0;
        }
        if (Pad0.WasPushed(Pads::kL2)) {
            SpecialFunc(GLUT_KEY_END, 0, 0);
            frameCount = 0;
        }

        if (Pad0.WasPushed(Pads::kR1)) {
            SpecialFunc(GLUT_KEY_PAGE_UP, 0, 0);
            frameCount = 0;
        }
        if (Pad0.WasPushed(Pads::kR2)) {
            SpecialFunc(GLUT_KEY_PAGE_DOWN, 0, 0);
            frameCount = 0;
        }
    }

    if (KeyboardFunc) {
        if (Pad0.IsDown(Pads::kRightUp)) {
            KeyboardFunc('8', 0, 0);
            frameCount = 0;
        }
        if (Pad0.IsDown(Pads::kRightDown)) {
            KeyboardFunc('2', 0, 0);
            frameCount = 0;
        }
        if (Pad0.IsDown(Pads::kRightLeft)) {
            KeyboardFunc('4', 0, 0);
            frameCount = 0;
        }
        if (Pad0.IsDown(Pads::kRightRight)) {
            KeyboardFunc('6', 0, 0);
            frameCount = 0;
        }
    }

    if (frameCount > 40 && frameCount % 3 == 0) {

        if (SpecialFunc) {
            if (Pad0.IsDown(Pads::kLeftUp)) {
                SpecialFunc(GLUT_KEY_UP, 0, 0);
            }
            if (Pad0.IsDown(Pads::kLeftDown)) {
                SpecialFunc(GLUT_KEY_DOWN, 0, 0);
            }
            if (Pad0.IsDown(Pads::kLeftRight)) {
                SpecialFunc(GLUT_KEY_RIGHT, 0, 0);
            }
            if (Pad0.IsDown(Pads::kLeftLeft)) {
                SpecialFunc(GLUT_KEY_LEFT, 0, 0);
            }

            if (Pad0.IsDown(Pads::kL1)) {
                SpecialFunc(GLUT_KEY_HOME, 0, 0);
            }
            if (Pad0.IsDown(Pads::kL2)) {
                SpecialFunc(GLUT_KEY_END, 0, 0);
            }

            if (Pad0.IsDown(Pads::kR1)) {
                SpecialFunc(GLUT_KEY_PAGE_UP, 0, 0);
            }
            if (Pad0.IsDown(Pads::kR2)) {
                SpecialFunc(GLUT_KEY_PAGE_DOWN, 0, 0);
            }
        }

        if (KeyboardFunc) {
            if (Pad0.IsDown(Pads::kRightUp)) {
                KeyboardFunc('8', 0, 0);
            }
            if (Pad0.IsDown(Pads::kRightDown)) {
                KeyboardFunc('2', 0, 0);
            }
            if (Pad0.IsDown(Pads::kRightLeft)) {
                KeyboardFunc('4', 0, 0);
            }
            if (Pad0.IsDown(Pads::kRightRight)) {
                KeyboardFunc('6', 0, 0);
            }
        }
    }

    frameCount++;
}

static void
initGsMemory()
{
    // frame and depth buffer
    pgl_slot_handle_t frame_slot_0, frame_slot_1, depth_slot;
    frame_slot_0 = pglAddGsMemSlot(0, 70, GS::kPsm32);
    frame_slot_1 = pglAddGsMemSlot(70, 70, GS::kPsm32);
    depth_slot   = pglAddGsMemSlot(140, 70, GS::kPsmz24);
    // lock these slots so that they aren't allocated by the memory manager
    pglLockGsMemSlot(frame_slot_0);
    pglLockGsMemSlot(frame_slot_1);
    pglLockGsMemSlot(depth_slot);

    // create gs memory area objects to use for frame and depth buffers
    pgl_area_handle_t frame_area_0, frame_area_1, depth_area;
    frame_area_0 = pglCreateGsMemArea(640, 224, GS::kPsm24);
    frame_area_1 = pglCreateGsMemArea(640, 224, GS::kPsm24);
    depth_area   = pglCreateGsMemArea(640, 224, GS::kPsmz24);
    // bind the areas to the slots we created above
    pglBindGsMemAreaToSlot(frame_area_0, frame_slot_0);
    pglBindGsMemAreaToSlot(frame_area_1, frame_slot_1);
    pglBindGsMemAreaToSlot(depth_area, depth_slot);

    // draw to the new areas...
    pglSetDrawBuffers(PGL_INTERLACED, frame_area_0, frame_area_1, depth_area);
    // ...and display from them
    pglSetDisplayBuffers(PGL_INTERLACED, frame_area_0, frame_area_1);

    // 32 bit

    // a slot for fonts (probably)
    pglAddGsMemSlot(210, 2, GS::kPsm8);

    // 64x32
    pglAddGsMemSlot(212, 1, GS::kPsm32);
    pglAddGsMemSlot(213, 1, GS::kPsm32);
    // 64x64
    pglAddGsMemSlot(214, 2, GS::kPsm32);
    pglAddGsMemSlot(216, 2, GS::kPsm32);
    pglAddGsMemSlot(218, 2, GS::kPsm32);
    pglAddGsMemSlot(220, 2, GS::kPsm32);
    // 128x128
    pglAddGsMemSlot(222, 8, GS::kPsm32);
    pglAddGsMemSlot(230, 8, GS::kPsm32);
    // 256x256
    pglAddGsMemSlot(238, 32, GS::kPsm32);
    pglAddGsMemSlot(270, 32, GS::kPsm32);
    // 512x256
    pglAddGsMemSlot(302, 64, GS::kPsm32);
    pglAddGsMemSlot(366, 64, GS::kPsm32);

    pglPrintGsMemAllocation();
}
