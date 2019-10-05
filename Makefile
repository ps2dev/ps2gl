EE_LIB = libps2gl.a

EE_LDFLAGS  += -L. -L$(PS2SDK)/ports/lib
EE_INCS     += -I./include -I./vu1 -I$(PS2SDK)/ports/include
# VU0 code is broken so disable for now
EE_CFLAGS   += -D_DEBUG
EE_CXXFLAGS += -D_DEBUG
EE_CFLAGS   += -DNO_VU0_VECTORS
EE_CXXFLAGS += -DNO_VU0_VECTORS

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

EE_OBJS += \
	vu1/fast_nolights.vo \
	vu1/fast.vo \
	vu1/general_nospec_quad.vo \
	vu1/general_nospec_tri.vo \
	vu1/general_nospec.vo \
	vu1/general_pv_diff_quad.vo \
	vu1/general_pv_diff_tri.vo \
	vu1/general_pv_diff.vo \
	vu1/general_quad.vo \
	vu1/general_tri.vo \
	vu1/general.vo \
	vu1/indexed.vo \
	vu1/scei.vo

all: $(EE_LIB)

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

include $(PS2SDK)/Defs.make
include $(PS2SDK)/samples/Makefile.eeglobal

#%.vsm: %.pp.vcl
#	openvcl -o $@ $<

#%.pp.vcl: %.vcl
#	vclpp $< $@ -j

%.vo: %_vcl.vsm
	dvp-as -o $@ $<
