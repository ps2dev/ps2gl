EE_LIB = libps2gl.a

EE_LDFLAGS  += -L. -L$(PS2SDK)/ports/lib
EE_INCS     += -I./include -I./vu1 -I$(PS2SDK)/ports/include

ifeq ($(DEBUG), 1)
    EE_CFLAGS   += -D_DEBUG
    EE_CXXFLAGS += -D_DEBUG
endif

# Disabling warnings
WARNING_FLAGS = -Wno-strict-aliasing -Wno-conversion-null 

# VU0 code is broken so disable for now
EE_CFLAGS   += $(WARNING_FLAGS) -DNO_VU0_VECTORS -DNO_ASM
EE_CXXFLAGS += $(WARNING_FLAGS) -DNO_VU0_VECTORS -DNO_ASM

EE_OBJS = \
	src/base_renderer.o \
	src/clear.o \
	src/displaycontext.o \
	src/dlgmanager.o \
	src/dlist.o \
	src/drawcontext.o \
	src/gblock.o \
	src/glcontext.o \
	src/gmanager.o \
	src/gsmemory.o \
	src/immgmanager.o \
	src/indexed_renderer.o \
	src/inverse.o \
	src/lighting.o \
	src/linear_renderer.o \
	src/material.o \
	src/matrix.o \
	src/metrics.o \
	src/renderermanager.o \
	src/texture.o

RENDERERS = \
	fast_nolights \
	fast \
	general_nospec_quad \
	general_nospec_tri \
	general_nospec \
	general_pv_diff_quad \
	general_pv_diff_tri \
	general_pv_diff \
	general_quad \
	general_tri \
	general \
	indexed \
	scei \
	fast_no_lights_pvc_tri

EE_OBJS += $(addsuffix .vo, $(addprefix vu1/, $(RENDERERS)))

VSM_SOURCES = $(addsuffix _vcl.vsm, $(addprefix vu1/, $(RENDERERS)))

all: $(VSM_SOURCES) $(EE_LIB)

install: all
	mkdir -p $(PS2SDK)/ports/include
	mkdir -p $(PS2SDK)/ports/lib
	cp -rf include/GL    $(PS2SDK)/ports/include
	cp -rf include/ps2gl $(PS2SDK)/ports/include
	cp -f  $(EE_LIB) $(PS2SDK)/ports/lib

clean:
	rm -f $(EE_OBJS_LIB) $(EE_OBJS) $(EE_BIN) $(EE_LIB)

realclean: clean
	rm -rf $(PS2SDK)/ports/include/ps2gl
	rm -f  $(PS2SDK)/ports/lib/$(EE_LIB)
	rm -f  $(VSM_SOURCES)

include $(PS2SDK)/Defs.make
include $(PS2SDK)/samples/Makefile.eeglobal

## dvp-as origin in ps2dev toolchain: https://github.com/ps2dev/ps2toolchain/blob/master/scripts/001-dvp.sh
## Build .vo (VU object) from a compiled .vsm
%.vo: %_vcl.vsm
	dvp-as -o $@ $<

# VCL (Vector Command Language): https://ps2linux.no-ip.info/playstation2-linux.com/projects/vcl.html
# for documentation download the x86 or win32 tar above and read the VCL_User_Manual_E_v1.4_1.pdf
# more resources on vsm: http://lukasz.dk/files/vu-instruction-manual.pdf
%_vcl.vsm: %_pp4.vcl
	vcl -o$@ $<

# GCC / CPP flags (-E, -P, -imacros): https://gcc.gnu.org/onlinedocs/cpp/Invocation.html#Invocation
# -E = preprocess only, -P = strip #line, -imacros includes macros without writing #include
%indexed_pp4.vcl: %indexed_pp3.vcl
	cat $< | cc -E -P -imacros vu1/vu1_mem_indexed.h -o $@ -

# GCC / CPP flags (-E, -P, -imacros): https://gcc.gnu.org/onlinedocs/cpp/Invocation.html#Invocation
# -E = preprocess only, -P = strip #line, -imacros includes macros without writing #include
%_pp4.vcl: %_pp3.vcl
	cat $< | cc -E -P -imacros vu1/vu1_mem_linear.h -o $@ -

#TODO: remove this step? This could be covered simply from writing correct vcl code... unless intending to allow new and old syntax?
# you can standardize syntax by using ".syntax old" or ".syntax new" or by passing `-n` to VCL for "new" and writing sources
# accordingly, it might be better to allow for correcting towards that to avoid confusion...
%_pp3.vcl: %_pp2.vcl
	cat $< | sed 's/\[\([0-9]\)\]/_\1/g ; s/\[\([w-zW-Z]\)\]/\1/g' - > $@

# Expand assembly-style macros and .include with GASP
# -c ';' uses ';' as the comment char; -Ivu1 resolves local .include files.
# GASP (GNU assembler preprocessor) manpage: https://manpages.debian.org/unstable/binutils-m68hc1x/gasp.1.en.html
%_pp2.vcl: %_pp1.vcl
	gasp -c ';' -Ivu1 -o $@ $<

# this is in order to normalize sources for GASP by removing C preprocessor stuff (#include/#define),
# and then fix local .include paths so GASP can resolve them relative to the source dir.
# if the .vcl file ALREADY avoids #include/#define and only use .include/.macro etc
# you can wire %.vcl -> %_pp2.vcl directly and drop this rule
%_pp1.vcl: %.vcl
	cat $< | sed 's/#include[ 	]\+.\+// ; s/#define[ 	]\+.\+// ; s|\(\.include[ 	]\+\)"\([^/].\+\)"|\1"$(<D)/\2"|' - > $@
