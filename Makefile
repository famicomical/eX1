# Makefile - eX1 (Sharp X1turbo) SDL2 port for macOS / POSIX
#
# Builds the Common Source Code Project X1turbo core against a native
# SDL2 OSD layer (src/sdl/).

CXX      ?= clang++
SRC      := src
SDLDIR   := $(SRC)/sdl

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

# _X1TURBO selects the machine; _USE_SDL selects the OSD; _RGB888 -> 32bpp scrntype
DEFS     := -D_USE_SDL -D_X1TURBO -D_RGB888
INCLUDES := -I$(SRC) -I$(SDLDIR)/compat $(SDL_CFLAGS)

CXXFLAGS := -std=c++14 -O2 -Wno-multichar -Wno-switch -Wno-format \
            -fno-strict-aliasing $(DEFS) $(INCLUDES)
LDFLAGS  := $(SDL_LIBS)

BUILD := build
BIN   := $(BUILD)/ex1turbo

# ---- portable core (from vc++2017/x1turbo.vcxproj, minus win32 OSD) ----
CORE_SRCS := \
	$(SRC)/common.cpp \
	$(SRC)/config.cpp \
	$(SRC)/debugger.cpp \
	$(SRC)/emu.cpp \
	$(SRC)/fifo.cpp \
	$(SRC)/fileio.cpp \
	$(SRC)/vm/ay_3_891x.cpp \
	$(SRC)/vm/datarec.cpp \
	$(SRC)/vm/disk.cpp \
	$(SRC)/vm/event.cpp \
	$(SRC)/vm/fmgen/fmgen.cpp \
	$(SRC)/vm/fmgen/fmtimer.cpp \
	$(SRC)/vm/fmgen/opm.cpp \
	$(SRC)/vm/fmgen/opna.cpp \
	$(SRC)/vm/fmgen/psg.cpp \
	$(SRC)/vm/harddisk.cpp \
	$(SRC)/vm/hd46505.cpp \
	$(SRC)/vm/i8255.cpp \
	$(SRC)/vm/io.cpp \
	$(SRC)/vm/mb8877.cpp \
	$(SRC)/vm/mcs48.cpp \
	$(SRC)/vm/midi.cpp \
	$(SRC)/vm/mz1p17.cpp \
	$(SRC)/vm/noise.cpp \
	$(SRC)/vm/pcm8bit.cpp \
	$(SRC)/vm/prnfile.cpp \
	$(SRC)/vm/scsi_dev.cpp \
	$(SRC)/vm/scsi_hdd.cpp \
	$(SRC)/vm/scsi_host.cpp \
	$(SRC)/vm/upd1990a.cpp \
	$(SRC)/vm/x1/cz8rb.cpp \
	$(SRC)/vm/x1/display.cpp \
	$(SRC)/vm/x1/emm.cpp \
	$(SRC)/vm/x1/floppy.cpp \
	$(SRC)/vm/x1/iobus.cpp \
	$(SRC)/vm/x1/joystick.cpp \
	$(SRC)/vm/x1/keyboard.cpp \
	$(SRC)/vm/x1/memory.cpp \
	$(SRC)/vm/x1/mouse.cpp \
	$(SRC)/vm/x1/psub.cpp \
	$(SRC)/vm/x1/sasi.cpp \
	$(SRC)/vm/x1/sub.cpp \
	$(SRC)/vm/x1/x1.cpp \
	$(SRC)/vm/ym2151.cpp \
	$(SRC)/vm/z80.cpp \
	$(SRC)/vm/z80ctc.cpp \
	$(SRC)/vm/z80dma.cpp \
	$(SRC)/vm/z80sio.cpp

# ---- SDL OSD layer ----
OSD_SRCS := \
	$(SDLDIR)/osd.cpp \
	$(SDLDIR)/osd_screen.cpp \
	$(SDLDIR)/osd_sound.cpp \
	$(SDLDIR)/osd_input.cpp \
	$(SDLDIR)/osd_console.cpp \
	$(SDLDIR)/osd_midi.cpp \
	$(SDLDIR)/osd_printer.cpp \
	$(SDLDIR)/gui.cpp \
	$(SDLDIR)/main.cpp

# ---- Dear ImGui (vendored) ----
IMGUI_SRCS := \
	$(SDLDIR)/imgui/imgui.cpp \
	$(SDLDIR)/imgui/imgui_draw.cpp \
	$(SDLDIR)/imgui/imgui_tables.cpp \
	$(SDLDIR)/imgui/imgui_widgets.cpp \
	$(SDLDIR)/imgui/imgui_impl_sdl2.cpp \
	$(SDLDIR)/imgui/imgui_impl_sdlrenderer2.cpp

SRCS := $(CORE_SRCS) $(OSD_SRCS) $(IMGUI_SRCS)
OBJS := $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean
all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "==> built $@"

$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
