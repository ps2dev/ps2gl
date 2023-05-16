EE_BIN       = box.elf
EE_CFLAGS   := -I$(PS2SDK)/ports/include -I../shared_code/ $(EE_CFLAGS)
EE_CXXFLAGS := -I$(PS2SDK)/ports/include -I../shared_code/ $(EE_CXXFLAGS)
EE_OBJS      = box.o ../shared_code/text_stuff.o
EE_LDFLAGS  += -L$(PS2SDK)/ports/lib
EE_LIBS      = -lps2glut -lps2gl -lps2stuff -lpad -ldma

ifeq ($(DEBUG), 1)
	EE_CFLAGS   += -D_DEBUG
	EE_CXXFLAGS += -D_DEBUG
endif

# Disabling warnings
WARNING_FLAGS = -Wno-strict-aliasing -Wno-conversion-null 

# VU0 code is broken so disable for now
EE_CFLAGS   += $(WARNING_FLAGS) -DNO_VU0_VECTORS -DNO_ASM
EE_CXXFLAGS += $(WARNING_FLAGS) -DNO_VU0_VECTORS -DNO_ASM

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run: $(EE_BIN)
	ps2client -h 192.168.1.10 execee host:$(EE_BIN)

reset:
	ps2client -h 192.168.1.10 reset

sim: $(EE_BIN)
	PCSX2 --elf=$(PWD)/$(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal_cpp
