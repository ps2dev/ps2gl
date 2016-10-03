// Copyright(C) 2001 Sony Computer Entertainment Inc. All Rights Reserved.
//
// "sjoy.h"
//
//
//----------------------------------------------------------------------
// defines
//----------------------------------------------------------------------
#define SJOY_PS2_R_LEFT		(1)
#define SJOY_PS2_R_DOWN		(1 << 1)
#define SJOY_PS2_R_UP		(1 << 2)
#define SJOY_PS2_R_RIGHT	(1 << 3)
#define SJOY_PS2_L1			(1 << 4)
#define SJOY_PS2_R1			(1 << 5)
#define SJOY_PS2_L2			(1 << 6)
#define SJOY_PS2_R2			(1 << 7)
#define SJOY_PS2_SELECT		(1 << 8)
#define SJOY_PS2_START		(1 << 9)
#define SJOY_PS2_L_LEFT		(1 << 10)
#define SJOY_PS2_L_DOWN		(1 << 11)
#define SJOY_PS2_L_UP		(1 << 12)
#define SJOY_PS2_L_RIGHT	(1 << 13)

//           ___                              ___
//          [___] L2 (btn 6)                 [___] R2 (btn 7)
//          [___] L1 (btn 4)                 [___] R1 (btn 5)
//
//     (axis 1-)         (btn 8) (btn 9)         (btn 2)
//            ^ LUp       Select Start         ^ RUp
//            |           [__]   [__>          | 
//   LLeft <-- --> LRight             RLeft <-- --> RRight
//   (axis 0-)|    (axis 0+)          (btn 0)  |    (btn3)
//            V LDown                          V RDown
//              (axis 1+)                      (btn 1)
//

//----------------------------------------------------------------------
// prototypes
//----------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

int sjoy_open(void);
int sjoy_close(void);
void sjoy_poll(void);
int sjoy_get_button(int joy);
int sjoy_get_axis(int joy, int axis);
int sjoy_get_ps2_button(int joy);

#ifdef __cplusplus
}
#endif
