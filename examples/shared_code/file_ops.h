/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef file_ops_h
#define file_ops_h

#include "stdio.h"
#ifdef CDROM
#define FILE_PREFIX "cdrom0:"
#define FILE_SEP '\\'
#define FILE_TERM ";1"
#else
#define FILE_PREFIX "host0:"
#define FILE_SEP '/'
#define FILE_TERM ""
#endif

#endif // file_ops_h
