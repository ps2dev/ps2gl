/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef file_ops_h
#define file_ops_h

#ifndef PS2_LINUX
#   include "sifdev.h"
#   define O_WRONLY	SCE_WRONLY
#   define O_RDONLY	SCE_RDONLY
#   define O_CREAT	SCE_CREAT
#   undef  SEEK_END
#   define SEEK_END	SCE_SEEK_END
#   undef  SEEK_CUR
#   define SEEK_CUR	SCE_SEEK_CUR
#   undef  SEEK_SET
#   define SEEK_SET	SCE_SEEK_SET
#   define open		sceOpen
#   define lseek	sceLseek
#   define read		sceRead
#   define write	sceWrite
#   define close	sceClose
#   ifdef CDROM
#      define FILE_PREFIX	"cdrom0:"
#      define FILE_SEP		'\\'
#      define FILE_TERM		";1"
#   else
#      define FILE_PREFIX	"host0:"
#      define FILE_SEP		'/'
#      define FILE_TERM		""
#   endif
#else
#   include <sys/types.h>
#   include <unistd.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   define FILE_PREFIX	""
#endif

#endif // file_ops_h
