/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <linux/ps2/gs.h>
#include <sys/ioctl.h>
// for allocating memory with the ps2stuff kernel module
#include <sys/mman.h>

#include "ps2gs.h"
#include "ps2dma.h"
#include "ps2vpu.h"
#include "ps2vpufile.h"


#include "sjoy.h"

#include "GL/glut.h"
#include "GL/ps2gl.h"

#include "ps2s/timer.h"
#include "ps2s/gs.h"
#include "ps2s/packet.h"
#include "ps2s/displayenv.h"

#include "ps2gl/debug.h"

// #include "pads.h"

struct ps2_vpu_struct {
	int fd;
	int vpu;
	unsigned int mapsize;
	void *text;
	unsigned int text_size;
	void *data;
	unsigned int data_size;
};

/********************************************
 * some function pointer types
 */

typedef void (* tFunctionPtr_ii)	(int, int);
typedef void (* tFunctionPtr_ucii)	(unsigned char, int, int);
typedef void (* tFunctionPtr_iii)	(int, int, int);
typedef void (* tFunctionPtr)		(void);
typedef void (* tFunctionPtr_i)		(int);

/********************************************
 * local functions
 */

static void initGsMemory();
static bool doPads( int frameCount );
static void parse_cl( int *argcp, char **argv );

/********************************************
 * local data
 */

tFunctionPtr DisplayFunc = NULL;
tFunctionPtr_ii ReshapeFunc = NULL;
tFunctionPtr_ucii KeyboardFunc = NULL;
tFunctionPtr_i VisibilityFunc = NULL;
tFunctionPtr IdleFunc = NULL;
tFunctionPtr_iii SpecialFunc = NULL;

static CEETimer *Timer2, *Timer3;
//  static char default_module_path[] = "host0:/usr/local/sce/iop/modules";
//  static char *module_path = '\0';

int g_inter;
int g_out_mode;
int g_ff_mode;
int g_resolution;
int g_refresh_rate;
int g_psm;
int g_zpsm;
int g_zbits;

int g_fd_gs;
ps2_vpu *g_vpu0, *g_vpu1;

typedef enum { eNtsc, eVesa0 } screen_mode_t;
screen_mode_t screen_mode = eVesa0;

int release(void)
{
   if (g_vpu1) {
      ps2_vpu_close(g_vpu1);
      g_vpu1 = NULL;
   }

   if (g_vpu0) {
      ps2_vpu_close(g_vpu0);
      g_vpu0 = NULL;
   }

   if (g_fd_gs >= 0) {
      ps2_gs_close();
      g_fd_gs = -1;
   }

   return PS2_GS_VC_REL_SUCCESS;
}

int acquire(void)
{
   g_fd_gs = ps2_gs_open(-1);
   g_vpu0 = ps2_vpu_open(0);
   g_vpu1 = ps2_vpu_open(1);

   if (g_fd_gs < 0 || g_vpu0 == NULL || g_vpu1 == NULL) {
      release();
      return PS2_GS_VC_ACQ_FAILURE;
   }

   ps2_vpu_reset(g_vpu1);
   ps2_vpu_reset(g_vpu0);

   ps2_gs_reset(0, g_inter, g_out_mode, g_ff_mode, g_resolution,
		g_refresh_rate);

   return PS2_GS_VC_ACQ_SUCCESS;
}

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

// it doesn't get much lamer than this...
extern "C" int setcrtmode(int argc, char **argv, int gs_fd);

#   include <unistd.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <errno.h>
#include <string.h>

int Ps2stuffDeviceFd = -1;
bool WaitForVsync = true;

/**
 * Initialize the ps2glut library, also the ps2gl library and gs memory
 * if not already initialized by the app.
 *
 * @param argcp a pointer to the number of elements in argv
 * @param argvp command line args.  glut will look for "ntsc" or "vesa0".
 */
void glutInit(int *argcp, char **argv)
{
   parse_cl( argcp, argv );

   if ( screen_mode == eVesa0 ) {
      printf("vesa0 mode\n");
      g_inter = PS2_GS_NOINTERLACE;
      g_out_mode = PS2_GS_VESA;
      g_ff_mode = PS2_GS_FRAME;
      g_resolution = PS2_GS_640x480;
   }
   else if ( screen_mode == eNtsc ) {
      printf("ntsc1 mode\n");
      g_inter = PS2_GS_NOINTERLACE;
      g_out_mode = PS2_GS_NTSC;
      g_ff_mode = PS2_GS_FRAME;
      g_resolution = PS2_GS_640x480;
   }

   g_refresh_rate = PS2_GS_60Hz;
   g_psm = PS2_GS_PSMCT32;
   g_zpsm = PS2_GS_PSMZ32;
   g_zbits = 32;

   ps2_gs_vc_graphicsmode();

   g_fd_gs = ps2_gs_open(-1);
   g_vpu0 = ps2_vpu_open(0); // must be opened for vpu1...
   g_vpu1 = ps2_vpu_open(1);

   Ps2stuffDeviceFd = open( "/dev/ps2stuff", O_RDWR );
   if ( Ps2stuffDeviceFd < 0 ) {
     fprintf( stderr, "Couldn't open /dev/ps2stuff: %s\n", strerror(errno) );
     exit(-1);
   }

   ioctl( Ps2stuffDeviceFd, PS2STUFF_IOCTMEMRESET );

   // these are static, so they should be ok to call before
   // pglInit..
   CDmaPacket::InitFileDescriptors( g_fd_gs, g_vpu0->fd, g_vpu1->fd, Ps2stuffDeviceFd );
   GS::CDisplayEnv::InitGsFd( g_fd_gs );

   ps2_gs_reset(0, g_inter, g_out_mode, g_ff_mode, g_resolution,
		g_refresh_rate);

   ps2_gs_start_display(1);

   ps2_gs_vc_enablevcswitch(acquire, release);

   ps2_gs_sync_v(0);

   // does the ps2gl library need to be initialized?

   if ( ! pglHasLibraryBeenInitted() ) {
      mWarn( "ps2gl library has not been initialized by the user; using default values." );
      int immBufferVertexSize = 8 * 1024;
      pglInit( immBufferVertexSize, 1000 );
   }

   // does gs memory need to be initialized?

   if ( ! pglHasGsMemBeenInitted() ) {
      mWarn("GS memory has not been allocated by the user; using default values.");
      initGsMemory();
   }

   // init the timing system

   Timer2 = new CEETimer( CEETimer::Timer2 );
//     Timer2->SetResolution( CEETimer::BusClock_256th );
//     Timer3 = new CEETimer( CEETimer::Timer3 );
//     Timer3->SetResolution( CEETimer::BusClock_16th );
   mInitTimers( Timer2, CEETimer::BusClock_256th );

   // open the pads
   sjoy_open();
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
void glutMainLoop( void )
{
   mErrorIf( DisplayFunc == NULL, "ps2glut:  No display function!" );

   if ( ReshapeFunc )
      ReshapeFunc( 640, 480 );

   if ( VisibilityFunc )
      VisibilityFunc( GLUT_VISIBLE );

   int frameCount = 0;

   while(1) {
      mUpdateTimers();

      mStartTimer("frame time");

      // ps2_gs_vc_lock();

      bool printTimes = doPads( frameCount );

      if ( DisplayFunc ) {
	 mStartTimer("DisplayFunc()");
	 pglBeginGeometry();
	 DisplayFunc();
	 pglEndGeometry();
	 mStopTimer("DisplayFunc()");
      }

      if ( IdleFunc ) {
	 mStartTimer("IdleFunc()");
	 IdleFunc();
	 mStopTimer("IdleFunc()");
      }

      mStartTimer("wait for vu1");
      pglFinishRenderingGeometry( PGL_DONT_FORCE_IMMEDIATE_STOP );
      mStopTimer("wait for vu1");

      mStopTimer("frame time");

      if ( printTimes ) {
	 mDisplayTimers();
	 printTimes = false;
      }

      if ( WaitForVsync )
	 pglWaitForVSync();

      pglSwapBuffers();

      pglRenderGeometry();

      frameCount++;

      // ps2_gs_vc_unlock();
   }
}

/** @} */ // glut_api

static void
parse_cl( int *argcp, char **argv )
{
   for ( int i = 0; i < *argcp; i++ ) {
      bool found = false;

      if ( strstr(argv[i], "-ntsc1") == argv[i] ) {
	 screen_mode = eNtsc;
	 found = true;
      }
      else if ( strstr(argv[i], "-vesa0") == argv[i] ) {
	 screen_mode = eVesa0;
	 found = true;
      }

      if ( found ) argv[i] = NULL;
   }

   // fix up argv

   int numArgs = *argcp;
   for ( int i = 0; i < numArgs; i++ ) {
      if ( argv[i] == NULL ) {
	 // move everything after left one
	 for ( int j = i; j < numArgs - 1; j++ )
	    argv[j] = argv[j+1];
	 *argcp -= 1;
      }
   }
}

static bool
doPads( int frameCount )
{
   static int paddata = 0;

   sjoy_poll();
   paddata = sjoy_get_ps2_button(0);

   bool printTimes = ( paddata & SJOY_PS2_START );

   if ( SpecialFunc ) {
      if ( (frameCount % 15) == 0 ) {
	 if ( paddata & SJOY_PS2_L_UP ) {
	    SpecialFunc( GLUT_KEY_UP, 0, 0 );
	 }
	 if ( paddata & SJOY_PS2_L_DOWN ) {
	    SpecialFunc( GLUT_KEY_DOWN, 0, 0 );
	 }
	 if ( paddata & SJOY_PS2_L_RIGHT ) {
	    SpecialFunc( GLUT_KEY_RIGHT, 0, 0 );
	 }
	 if ( paddata & SJOY_PS2_L_LEFT ) {
	    SpecialFunc( GLUT_KEY_LEFT, 0, 0 );
	 }

	 if ( paddata & SJOY_PS2_L1 ) {
	    SpecialFunc( GLUT_KEY_HOME, 0, 0 );
	 }
	 if ( paddata & SJOY_PS2_L2 ) {
	    SpecialFunc( GLUT_KEY_END, 0, 0 );
	 }

	 if ( paddata & SJOY_PS2_R1 ) {
	    WaitForVsync = ! WaitForVsync;
	    SpecialFunc( GLUT_KEY_PAGE_UP, 0, 0 );
	 }
	 if ( paddata & SJOY_PS2_R2 ) {
	    SpecialFunc( GLUT_KEY_PAGE_DOWN, 0, 0 );
	 }
      }
   }

   if ( KeyboardFunc ) {
      if ( paddata & SJOY_PS2_R_UP ) {
	 KeyboardFunc( '8', 0, 0 );
      }
      if ( paddata & SJOY_PS2_R_DOWN ) {
	 KeyboardFunc( '2', 0, 0 );
      }
      if ( paddata & SJOY_PS2_R_LEFT ) {
	 KeyboardFunc( '4', 0, 0 );
      }
      if ( paddata & SJOY_PS2_R_RIGHT ) {
	 KeyboardFunc( '6', 0, 0 );
      }
   }

   return printTimes;
}


void glutInitDisplayMode(unsigned int mode)
{
   mNotImplemented( );
}

void glutInitWindowPosition( int x, int y )
{
   mNotImplemented( );
}

void glutInitWindowSize( int x, int y )
{
   mNotImplemented( );
}

int glutCreateWindow(const char *title)
{
   mNotImplemented( );

   return 1;
}

void glutPostRedisplay(void)
{
   // mNotImplemented( );
}

void glutSwapBuffers( void )
{
}

int glutGet(GLenum type)
{
   mNotImplemented( );
   return 0;
}

void*
pglutAllocDmaMem( unsigned int num_bytes )
{
   // use the ps2stuff kernel module to allocate
   return mmap(0, num_bytes,
	       PROT_READ | PROT_WRITE,
	       MAP_SHARED, Ps2stuffDeviceFd,
	       0 );
}

void
pglutFreeDmaMem( void *mem )
{
   // use the ps2stuff kernel module to allocate
   munmap( mem, 0 );
}

/********************************************
 * local function definitions
 */

static void
initGsMemory()
{
   // frame and depth buffer
   pgl_slot_handle_t frame_slot_0, frame_slot_1, depth_slot;
   frame_slot_0 = pglAddGsMemSlot( 0, 150, GS::kPsm32 );
   frame_slot_1 = pglAddGsMemSlot( 150, 150, GS::kPsm32 );
   depth_slot = pglAddGsMemSlot( 300, 150, GS::kPsmz24 );
   // lock these slots so that they aren't allocated by the memory manager
   pglLockGsMemSlot( frame_slot_0 );
   pglLockGsMemSlot( frame_slot_1 );
   pglLockGsMemSlot( depth_slot );

   // ntsc or vesa?

   // default to vesa
   int fb_height = 480, interlace = PGL_NONINTERLACED;

   if ( screen_mode == eNtsc ) {
      fb_height = 224;
      interlace = PGL_INTERLACED;
   }

   // create gs memory area objects to use for frame and depth buffers
   pgl_area_handle_t frame_area_0, frame_area_1, depth_area;
   frame_area_0 = pglCreateGsMemArea( 640, fb_height, GS::kPsm24 );
   frame_area_1 = pglCreateGsMemArea( 640, fb_height, GS::kPsm24 );
   depth_area = pglCreateGsMemArea( 640, fb_height, GS::kPsmz24 );
   // bind the areas to the slots we created above
   pglBindGsMemAreaToSlot( frame_area_0, frame_slot_0 );
   pglBindGsMemAreaToSlot( frame_area_1, frame_slot_1 );
   pglBindGsMemAreaToSlot( depth_area, depth_slot );

   // draw to the new areas...
   pglSetDrawBuffers( interlace, frame_area_0, frame_area_1, depth_area );
   // ...and display from them
   pglSetDisplayBuffers( interlace, frame_area_0, frame_area_1 );

   // 32 bit

   // 64x32
   pglAddGsMemSlot( 450, 1, GS::kPsm32 );
   pglAddGsMemSlot( 451, 1, GS::kPsm32 );
   // 64x64
   pglAddGsMemSlot( 452, 2, GS::kPsm32 );
   pglAddGsMemSlot( 454, 2, GS::kPsm32 );
   // 128x128
   pglAddGsMemSlot( 456, 8, GS::kPsm32 );
   pglAddGsMemSlot( 464, 8, GS::kPsm32 );
   pglAddGsMemSlot( 472, 8, GS::kPsm32 );
   // 256x256
   pglAddGsMemSlot( 480, 32, GS::kPsm32 );

   pglPrintGsMemAllocation();
}
