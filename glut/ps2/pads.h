/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef pads_h
#define pads_h

#include "eetypes.h" // needed for libpad.h
#include "libpad.h"

#include "ps2s/types.h"

/********************************************
 * types
 */


/********************************************
 * class def
 */

namespace Pads
{
      void Init( const char* module_path );
      void Read( void );

      static const unsigned int kSelect = 0,
	 kLeftStickButton = 1,
	 kRightStickButton = 2,
	 kStart = 3,
	 kLeftUp = 4,
	 kLeftRight = 5,
	 kLeftDown = 6,
	 kLeftLeft = 7,
	 kL2 = 8,
	 kR2 = 9,
	 kL1 = 10,
	 kR1 = 11,
	 kRightUp = 12,
	 kRightRight = 13,
	 kRightDown = 14,
	 kRightLeft = 15;
}

class CPad
{
   public:
      CPad( unsigned int port );

      bool Open( void );
      void Read( void );

      bool IsDown( unsigned int button );
      bool IsUp( unsigned int button );
      bool WasPushed( unsigned int button );
      bool WasReleased( unsigned int button );

      float RightStickX( void ) { return CurStatus.rightStick.xPos; }
      float RightStickY( void ) { return CurStatus.rightStick.yPos; }

      float LeftStickX( void ) { return CurStatus.leftStick.xPos; }
      float LeftStickY( void ) { return CurStatus.leftStick.yPos; }

   private:
      typedef struct {
	    tU16 select : 1;
	    tU16 i : 1;
	    tU16 j : 1;
	    tU16 start : 1;

	    tU16 leftUp : 1;
	    tU16 leftRight : 1;
	    tU16 leftDown : 1;
	    tU16 leftLeft : 1;

	    tU16 l2 : 1;
	    tU16 r2 : 1;
	    tU16 l1 : 1;
	    tU16 r1 : 1;

	    tU16 rightUp : 1;
	    tU16 rightRight : 1;
	    tU16 rightDown : 1;
	    tU16 rightLeft : 1;
      } tButtonsPressed;

      typedef struct {
	    tU8 xVal, yVal;
	    tU8 xCenter, yCenter;
	    float xPos, yPos;
	    bool isCentered;
      } tStickData;

      typedef struct {
	    tU8 success;
	    tU8 statLen;
	    tU16 buttons; /* 16 buttons */
	    tU8 r3h;
	    tU8 r3v;
	    tU8 l3h;
	    tU8 l3v;
	    tU8 kanAtsu[12]; // deal with this crap later
	    tU8 whoKnows[12]; // make the structure 32 bytes long
	    tStickData rightStick, leftStick;
      } tPadStatus;

      bool UpdateStick( tStickData* stickCur, tStickData* stickLast );

      tU128 		DmaBuffer[scePadDmaBufferMax] __attribute__ ((aligned(64)));
      tPadStatus	CurStatus __attribute__ ((aligned(16)));
      tPadStatus	LastStatus __attribute__ ((aligned(16)));
      unsigned int			uiPort;
      bool		bPadModeSet;


};

extern CPad Pad0;
extern CPad Pad1;

#endif // pads_h
