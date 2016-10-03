##########################################################################
### Copyright (c) 1999, 2000 Sony Computer Entertainment America Inc.
###	All rights reserved.
### 
### Boilerplate Makefile by Bret Mogilefsky (mogul@playstation.sony.com)
### 	and Tyler Daniel (tyler_daniel@playstation.sony.com)
###
###	Use this makefile as a template for new projects!
### 
### General Features:
###
###	Just specify SRCS and go!
###	Automatic and minimal (fast!) dependency generation (for vu microcode as well)
###	Allows keeping source and headers from src and include dirs, or elsewhere.
###	Builds in a subdirectory.
###	Allows additional defines, include dirs, and lib dirs without 
###		specifying -D, -I, and -L
###   Easy to specify parallel builds (debug, optimized, release, etc)
###   Easy to add flags on a per-file, per-build, or per-file-build basis
###	Can specify parent projects to make first (libraries)
###	Builds libraries
###	Slices, dices, feeds your cat, calls your mum.
###
### VU microcode features:
###
###	Generates depencies for microcode (for .include and #include)
###	Uses a preprocessing script to manage registers (configurable)
###	Runs the c preprocessor over microcode - you can use #define and #include
###		freely (and share #defines with c/c++)
###	Support for vcl
###
### Useful targets:
###
###	run		Run the executable.
###	xrun		Run the executable under a new xterminal.
###	clean		Remove everything we can rebuild.
###	tags		Generate source-browsing tags for Emacs.
###
### Using builds:
###
### 	To specify a particular build include the name of the build anywhere on
### 	the command line:
### 		make xrun optimized,
###   	make clean optimized, etc.
###
### 	Included builds (add your own!):
### 		debug
###		optimized	(default)
###		release
###
###	For more info see the "Build Options" section below
##########################################################################


##########################################################################
### Target
##########################################################################


# The name of the binary file we want to generate.  Also handles libraries! (.a)
TARGET		= libps2glut.a


##########################################################################
### Files and Paths - this is probably the only section you'll need to change
##########################################################################


# The source files for the project.
# get all cpp source files
SRCS		+= $(wildcard *.cpp)
SRCS		+= $(foreach DIR,$(SRCDIRS),$(subst $(DIR)/,,$(wildcard $(DIR)/*.cpp)))
# get all c source files
SRCS		+= $(wildcard *.c)
SRCS		+= $(foreach DIR,$(SRCDIRS),$(subst $(DIR)/,,$(wildcard $(DIR)/*.c)))

# Additional objects to link. Only add things that aren't built from SRCS!
OBJS		= 

# Additional libs to link with. (sce libs are listed in the section below)
LIBS		= 

# Additional locations for header files
INCDIRS		= include ../include/ ../../ps2stuff/include

# Additional locations for library files
LIBDIRS		= 

# Additional locations for source files
SRCDIRS		= 

# Object files and the target will be placed in this directory with an
# underscore and the buildname appended (e.g., for the "debug" build: objs_debug/)
OBJDIRBASE	= objs

# Dependency files will be placed in this directory with an underscore and
# the buildname appended (e.g., for the "debug" build: deps_debug/)
DEPDIRBASE	= deps

# If this project depends other projects (a ps2 rendering library for example) that should
# be built with make before making this one, list the directories here.
MAKEPARENTS	= ../

# Where to find PSX2 development stuff.
SCEDIR		= /usr/local/sce
PS2DEVDIR	= /usr/local/ps2

# Where to find the ps2stuff project
PS2STUFF	= ../../ps2stuff

##########################################################################
### Common Options (shared across builds)
##########################################################################

# Additional preprocessor definitions
DEFINES		= 

# Compiler optimization options
OPTFLAGS	= -fno-rtti -fno-exceptions -G 0

# Compiler debug options

# enable all warnings
DEBUGFLAGS	= -Wall
# output assembly listings with c/c++ code
DEBUGFLAGS	+= -Wa,-alh
# This is not recommended as it generates slower code, but let's leave it
# as the default so that "*(u_long128*)&someVar" behaves as you expect.
# It would be better to remove this and not do the above (try templates).
DEBUGFLAGS	+= -fno-strict-aliasing
# for multithreading to work properly?
DEBUGFLAGS	+= -fno-common

# Command-line arguments to be passed to the target when we run it
RUNARGS		= 


##########################################################################
### Build Options - applied per-build
##########################################################################


# use ps2stuff's build configuration
include $(PS2STUFF)/Makefile.builds

# we want to use source from 'ps2/' for ps2 (native) builds and source
# from 'linux/' for linux builds..

debug_SRCDIRS			+= ps2
optimized_SRCDIRS		+= ps2
release_SRCDIRS			+= ps2
cdrom_SRCDIRS			+= ps2
perf_SRCDIRS			+= ps2
debug_no_vu0_SRCDIRS		+= ps2
optimized_no_vu0_SRCDIRS	+= ps2
release_no_vu0_SRCDIRS		+= ps2

# linux

linux_SRCDIRS			+= linux
linux_debug_SRCDIRS		+= linux
linux_release_SRCDIRS		+= linux
cross_linux_SRCDIRS		+= linux


##########################################################################
### Per-file Options
##########################################################################


# Additional defines and include dirs can be specified on a per-file basis
# by prefixing with the stem of the filename.  For example, if I wanted special flags
# for building mucilage.cpp, I could add any of the following 
# mucilage_INCDIRS	= someincdirs 
# mucilage_LIBDIRS	= somelibdirs
# mucilage_DEFINES	= somedefs
# mucilage_OPTFLAGS	= someoptflags
# mucilage_DEBUGFLAGS	= somedebugflags


##########################################################################
### Per-file, per-build Options
##########################################################################


# Similar to above.. To apply special flags for building mucilage.cpp for
# the debug build, I could add any of the following
# mucilage_debug_INCDIRS	= someincdirs 
# mucilage_debug_LIBDIRS	= somelibdirs
# mucilage_debug_DEFINES	= somedefs
# mucilage_debug_OPTFLAGS	= someoptflags
# mucilage_debug_DEBUGFLAGS	= somedebugflags


##########################################################################
### Makefile operation
##########################################################################


# Set this to 1 to print status messages (like 'Compiling somefile.cpp...')
PRINT_MSGS		= 1

# Set this to 1 to print the exact command lines used to build files
PRINT_CMDS		= 0


##########################################################################
### include the makefile that does all the work
##########################################################################

include $(PS2STUFF)/Makefile.work

ifeq ($(GCC_MAJOR),3)
DEBUGFLAGS		+= -Wno-deprecated
endif
