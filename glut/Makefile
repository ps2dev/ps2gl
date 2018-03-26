EE_LIB = libps2glut.a

EE_LDFLAGS  += -L. -L$(PS2SDK)/ports/lib
EE_INCS     += -I./include -I$(PS2SDK)/ports/include
# VU0 code is broken so disable for now
EE_CFLAGS   += -DNO_VU0_VECTORS
EE_CXXFLAGS += -DNO_VU0_VECTORS

EE_OBJS = \
	glut_font_image.o \
	ps2/pads.o \
	ps2/ps2glut.o

all: $(EE_LIB)

install: all
	mkdir -p $(PS2SDK)/ports/include
	mkdir -p $(PS2SDK)/ports/lib
	cp -rf include/GL $(PS2SDK)/ports/include
	cp -f  $(EE_LIB) $(PS2SDK)/ports/lib

clean:
	rm -f $(EE_OBJS_LIB) $(EE_OBJS) $(EE_BIN) $(EE_LIB)

realclean: clean
	rm -f $(PS2SDK)/ports/lib/$(EE_LIB)

include $(PS2SDK)/Defs.make
include ../../Makefile.eeglobal
