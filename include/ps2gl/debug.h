/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_debug_h
#define ps2gl_debug_h

#ifdef DEBUG_MODULE_NAME
#undef DEBUG_MODULE_NAME
#endif
#define DEBUG_MODULE_NAME PS2GL

#include "ps2s/debug_macros.h"

#define mNotImplemented(_msg, _args...) mWarn("(in %s) not implemented" _msg, __FUNCTION__, ##_args)

//#define GL_FUNC_DEBUG printf
#define GL_FUNC_DEBUG(msg, ...)

#endif // ps2gl_debug_h
