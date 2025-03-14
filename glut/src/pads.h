/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef pads_h
#define pads_h

#include "libpad.h"

#include "ps2s/types.h"

#define scePadDmaBufferMax (256 / 16)

/********************************************
 * types
 */

/********************************************
 * class def
 */

namespace Pads {
void Init(void);
void Read(void);

static const unsigned int kSelect           = 0,
                          kLeftStickButton  = 1,
                          kRightStickButton = 2,
                          kStart            = 3,
                          kLeftUp           = 4,
                          kLeftRight        = 5,
                          kLeftDown         = 6,
                          kLeftLeft         = 7,
                          kL2               = 8,
                          kR2               = 9,
                          kL1               = 10,
                          kR1               = 11,
                          kRightUp          = 12,
                          kRightRight       = 13,
                          kRightDown        = 14,
                          kRightLeft        = 15;
}

class CPad {
public:
    CPad(unsigned int port);

    bool Open(void);
    void Read(void);

    bool IsDown(unsigned int button);
    bool IsUp(unsigned int button);
    bool WasPushed(unsigned int button);
    bool WasReleased(unsigned int button);

    float RightStickX(void) { return CurStatus.rightStick.xPos; }
    float RightStickY(void) { return CurStatus.rightStick.yPos; }

    float LeftStickX(void) { return CurStatus.leftStick.xPos; }
    float LeftStickY(void) { return CurStatus.leftStick.yPos; }

private:
    typedef struct {
        uint16_t select : 1;
        uint16_t i : 1;
        uint16_t j : 1;
        uint16_t start : 1;

        uint16_t leftUp : 1;
        uint16_t leftRight : 1;
        uint16_t leftDown : 1;
        uint16_t leftLeft : 1;

        uint16_t l2 : 1;
        uint16_t r2 : 1;
        uint16_t l1 : 1;
        uint16_t r1 : 1;

        uint16_t rightUp : 1;
        uint16_t rightRight : 1;
        uint16_t rightDown : 1;
        uint16_t rightLeft : 1;
    } tButtonsPressed;

    typedef struct {
        uint8_t xVal, yVal;
        uint8_t xCenter, yCenter;
        float xPos, yPos;
        bool isCentered;
    } tStickData;

    typedef struct {
        uint8_t success;
        uint8_t statLen;
        uint16_t buttons; /* 16 buttons */
        uint8_t r3h;
        uint8_t r3v;
        uint8_t l3h;
        uint8_t l3v;
        uint8_t kanAtsu[12];  // deal with this crap later
        uint8_t whoKnows[12]; // make the structure 32 bytes long
        tStickData rightStick, leftStick;
    } tPadStatus;

    bool IsDown(tPadStatus status, unsigned int button);
    bool IsUp(tPadStatus status, unsigned int button);
    bool UpdateStick(tStickData* stickCur, tStickData* stickLast);

    uint128_t DmaBuffer[scePadDmaBufferMax] __attribute__((aligned(64)));
    tPadStatus CurStatus __attribute__((aligned(16)));
    tPadStatus LastStatus __attribute__((aligned(16)));
    unsigned int uiPort;
    bool bPadModeSet;
};

extern CPad Pad0;
extern CPad Pad1;

#endif // pads_h
