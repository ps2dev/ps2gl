// Copyright(C) 2001 Sony Computer Entertainment Inc. All Rights Reserved.
//
// "sjoy.c"
//
//
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <linux/types.h>
#include <linux/joystick.h>
#include "sjoy.h"

#define N_JOY	2

static char *g_devName[N_JOY] = {
	"/dev/js0",
	"/dev/js1",
};

static int g_fd[N_JOY] = {
	-1, -1,
};

static __u32 g_button[N_JOY];	// max 32 buttons per joystick
static __s16 g_axis[N_JOY][2];	// max 2 axis per joystick

//----------------------------------------------------------------------
int sjoy_open(void)
{
	int joy;
	
	sjoy_close();
	
	for (joy = 0; joy < N_JOY; joy++) {
		assert(g_fd[joy] == -1);
		g_fd[joy] = open(g_devName[joy], O_RDONLY | O_NONBLOCK);
		if (g_fd[joy] < 0) {
			fprintf(stderr, "can't open %s\n", g_devName[joy]);
			fprintf(stderr,
					"You don't have permission, or should load module for joysticks.\n"
					"How to load joystick module:\n"
					"    # modprobe ps2pad\n");
			return -1;
		}
	}
	
	return 0;
}

//----------------------------------------------------------------------
int sjoy_close(void)
{
	int joy;
	int fail = 0;
	
	for (joy = 0; joy < N_JOY; joy++) {
		if (g_fd[joy] >= 0) {
			fail |= close(g_fd[joy]);
		}
		g_fd[joy] = -1;
	}
	
	return fail ? -1 : 0;
}

//----------------------------------------------------------------------
void sjoy_poll(void)
{
	int joy;
	
	for (joy = 0; joy < N_JOY; joy++) {
		if (g_fd[joy] < 0) {
			continue;
		}
		for (; ;) {
			struct js_event e;
			int n = read(g_fd[joy], &e, sizeof(e));
			if (n != sizeof(e)) {
				break;
			}
			switch (e.type & ~JS_EVENT_INIT) {
			case JS_EVENT_BUTTON:
				g_button[joy] &= ~(1 << e.number);
				g_button[joy] |= (e.value << e.number);
				break;
				
			case JS_EVENT_AXIS:
				g_axis[joy][e.number] = e.value;
				break;
				
			default:
				assert(0);
				break;
			}
		}
	}
}

//----------------------------------------------------------------------
int sjoy_get_button(int joy)
{
	return g_button[joy];
}

//----------------------------------------------------------------------
int sjoy_get_axis(int joy, int axis)
{
	return g_axis[joy][axis];
}

//----------------------------------------------------------------------
int sjoy_get_ps2_button(int joy)
{
	int w = g_button[joy];
	int a0 = g_axis[joy][0];
	int a1 = g_axis[joy][1];
	int th = 0x4000;
	
	w |= (a0 < -th) ? SJOY_PS2_L_LEFT : 0;
	w |= (a1 > th) ? SJOY_PS2_L_DOWN : 0;
	w |= (a0 > th) ? SJOY_PS2_L_RIGHT : 0;
	w |= (a1 < -th) ? SJOY_PS2_L_UP : 0;
	
	return w;
}
