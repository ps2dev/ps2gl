/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

/********************************************
 * includes
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "eetypes.h"
#include "sifdev.h"
#include "sifrpc.h"

#include "pads.h"
#include "ps2s/math.h"

/********************************************
 * constants
 */

#define kPad0	0
#define kPad1	1

#define kPort0	0
#define kSlot0	0

#define kPadModeStandard 0x4
#define kPadModeAnalog   0x7

#define kPadSetLockModeUnchanged 0
#define kPadSetLockModeLock 3
#define kPadSetLockModeUnlock 1

#define kStickMaxRadius 120
#define kStickDeadRadius 25

/********************************************
 * globals
 */

CPad Pad0( kPad0 );
CPad Pad1( kPad1 );

/********************************************
 * Pads
 */

void
Pads::Init( const char* module_path )
{
   char temp_buffer[256];
   int module_path_len = strlen(module_path);

   // open the pads.. this should be elsewhere..
   sceSifInitRpc(0);

   /* load sio2man.irx */
   strncpy( temp_buffer, module_path, 256 );
   if (temp_buffer[module_path_len-1] == '/')
      temp_buffer[module_path_len-1] = '\0';
   strncat( temp_buffer, "/sio2man.irx", 256-strlen(temp_buffer) );
   if (sceSifLoadModule(temp_buffer, 0, NULL) < 0)
   {
      printf("Can't load module sio2man\n");
      exit(0);
   }
   /* load padman.irx */
   strncpy( temp_buffer, module_path, 256 );
   if (temp_buffer[module_path_len-1] == '/')
      temp_buffer[module_path_len-1] = '\0';
   strncat( temp_buffer, "/padman.irx", 256-strlen(temp_buffer) );
   if (sceSifLoadModule(temp_buffer,0, NULL) < 0)
   {
      printf("Can't load module padman\n");
      exit(0);
   }

   scePadInit(0); // "must be zero"

   if ( ! Pad0.Open() ) {
      printf("Couldn't open Pad0.\n");
      exit(-1);
   }
}

void
Pads::Read( void )
{
   Pad0.Read();
}

/********************************************
 * CPad
 */

CPad::CPad( unsigned int port )
   : uiPort(port), bPadModeSet(false)
{
   memset( &CurStatus, 0, sizeof(tPadStatus) );
   memset( &LastStatus, 0, sizeof(tPadStatus) );
}

bool
CPad::Open( void )
{
   // slot is only for use with multitap
   return scePadPortOpen(uiPort, kSlot0, DmaBuffer);
}

void
CPad::Read( void )
{
   t32 padState = scePadGetState( kPort0, kSlot0 );
   if ( padState != scePadStateStable ) return;

   if ( !bPadModeSet ) {
      // who knows what the 1 parameter is..  a return val of 1 indicates that the request is
      // being processed
      if ( scePadSetMainMode(uiPort, kSlot0, 1, kPadSetLockModeUnlock) == 1 )
         bPadModeSet = true;
   }
   else {
      tPadStatus padStatus;
      scePadRead( uiPort, kSlot0, (tU8*)&padStatus );

      if ( padStatus.success == 0 ) { // 0 indicates success
	 LastStatus = CurStatus;
	 padStatus.rightStick = CurStatus.rightStick;
	 padStatus.leftStick = CurStatus.leftStick;
	 CurStatus = padStatus;

	 t32 id = scePadInfoMode( uiPort, kSlot0, InfoModeCurID, 0 );
	 if ( id == kPadModeStandard || id == kPadModeAnalog ) {
				// flip the sense of the bit field (1 = pressed)
	    CurStatus.buttons ^= 0xffff;
	 }

	 // sticks
	 if ( WasPushed( Pads::kRightStickButton ) ) {
	    CurStatus.leftStick.isCentered = false;
	    CurStatus.rightStick.isCentered = false;
	 }
	 CurStatus.leftStick.xVal = CurStatus.l3h;
	 CurStatus.leftStick.yVal = CurStatus.l3v;
	 CurStatus.rightStick.xVal = CurStatus.r3h;
	 CurStatus.rightStick.yVal = CurStatus.r3v;
	 UpdateStick( &CurStatus.leftStick, &LastStatus.leftStick );
	 UpdateStick( &CurStatus.rightStick, &LastStatus.rightStick );
      }
   }
}

bool
CPad::UpdateStick( tStickData* stickCur, tStickData* stickLast )
{
   t8 temp;
   bool isChanged = false;

   using namespace Math;

   if ( ! stickCur->isCentered ) {
      stickCur->xCenter = stickCur->xVal;
      stickCur->yCenter = stickCur->yVal;
      stickCur->xPos = 0.0f;
      stickCur->yPos = 0.0f;
      stickCur->isCentered = true;

      isChanged = true;
   }
   else {
      if ( !FuzzyEqualsi(stickCur->xVal, stickCur->xCenter, kStickDeadRadius) ) {
	 // stick is not inside the dead zone
         temp = ((stickCur->xVal > stickCur->xCenter) ? -kStickDeadRadius : kStickDeadRadius);
         stickCur->xPos = (float)(stickCur->xVal - stickCur->xCenter + temp) /
	    (float)kStickMaxRadius;
         isChanged = TRUE;
      }
      else {
	 // stick is inside the dead zone
	 stickCur->xPos = 0.0f;
	 // if it just entered the dead zone, send out one last event
	 if ( !FuzzyEqualsi(stickLast->xVal, stickCur->xCenter, kStickDeadRadius) )
	    isChanged = true;
      }
      if ( !FuzzyEqualsi(stickCur->yVal, stickCur->yCenter, kStickDeadRadius) ) {
	 // stick is not inside the dead zone
         temp = (stickCur->yVal > stickCur->yCenter) ? kStickDeadRadius : -kStickDeadRadius;
         stickCur->yPos = (float)(stickCur->yCenter - stickCur->yVal + temp) /
	    (float)kStickMaxRadius;
         isChanged = true;
      }
      else {
	 // stick is inside the dead zone
	 stickCur->yPos = 0.0f;
	 // if it just entered the dead zone, send out one last event
	 if ( !FuzzyEqualsi(stickLast->yVal, stickCur->yCenter, kStickDeadRadius) )
	    isChanged = true;
      }

      stickCur->xPos = Clamp( stickCur->xPos, -1.0f, 1.0f );
      stickCur->yPos = Clamp( stickCur->yPos, -1.0f, 1.0f );
   }

   return isChanged;
}

bool
CPad::IsDown( unsigned int button )
{
   return CurStatus.buttons & (1 << button);
}

bool
CPad::IsUp( unsigned int button )
{
   return ! (CurStatus.buttons & (1 << button));
}

bool
CPad::WasPushed( unsigned int button )
{
   return (CurStatus.buttons & (1 << button)) && ! (LastStatus.buttons & (1 << button));
}

bool
CPad::WasReleased( unsigned int button )
{
   return ! ((CurStatus.buttons & (1 << button)) && ! (LastStatus.buttons & (1 << button)));
}

