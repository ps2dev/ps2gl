EE_LIB = libps2gl.a

EE_LDFLAGS  += -L. -L$(PS2SDK)/ports/lib
EE_INCS     += -I./include -I./vu1 -I$(PS2SDK)/ports/include
# VU0 code is broken so disable for now
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

#EE_OBJS += \
#	vu1/general_tri.vsm

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
	rm -rf $(PS2SDK)/ports/include/GL
	rm -rf $(PS2SDK)/ports/include/ps2gl
	rm -f  $(PS2SDK)/ports/lib/$(EE_LIB)
	
include $(PS2SDK)/Defs.make
include ../ps2sdk-ports/Makefile.eeglobal

#%.vsm: %.pp.vcl
#	openvcl -o $@ $<

#%.pp.vcl: %.vcl
#	vclpp $< $@ -j
