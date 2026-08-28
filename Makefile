# Makefile - eX1 / eX1turbo / eX1turboZ (Sharp X1 family) SDL2 port for macOS / POSIX
#
# Builds the Common Source Code Project X1-family cores against a native
# SDL2 OSD layer (src/sdl/).
#
#   make            # all three: build/ex1, build/ex1turbo, build/ex1turboz
#   make x1turbo    # just one variant (x1 | x1turbo | x1turboz)
#   make clean      # remove objects + binaries (ROMs etc. in build/ are kept)

CXX      ?= clang++
SRC      := src
SDLDIR   := $(SRC)/sdl

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

# _USE_SDL selects the OSD; _RGB888 -> 32bpp scrntype; machine define is per-variant
DEFS     := -D_USE_SDL -D_RGB888
INCLUDES := -I$(SRC) -I$(SDLDIR)/compat $(SDL_CFLAGS)

CXXFLAGS := -std=c++14 -O2 -Wno-multichar -Wno-switch -Wno-format \
            -fno-strict-aliasing $(DEFS) $(INCLUDES)
LDFLAGS  := $(SDL_LIBS)

BUILD := build

# ---- variants (machine macro per vc++2017/x1*.vcxproj) ----
VARIANTS := x1 x1turbo x1turboz
MDEF_x1       := -D_X1
MDEF_x1turbo  := -D_X1TURBO
MDEF_x1turboz := -D_X1TURBOZ

# ---- portable core (from vc++2017/x1turbo.vcxproj, minus win32 OSD) ----
# x1turbo/x1turboz share the same list; plain x1 drops z80dma.cpp (turbo-only DMA)
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
	$(SRC)/vm/z80sio.cpp

EXTRA_SRCS_x1       :=
EXTRA_SRCS_x1turbo  := $(SRC)/vm/z80dma.cpp
EXTRA_SRCS_x1turboz := $(SRC)/vm/z80dma.cpp

# ---- SDL OSD layer (machine-dependent: includes x1.h feature macros) ----
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

# ---- Dear ImGui (vendored; machine-independent, objects shared) ----
IMGUI_SRCS := \
	$(SDLDIR)/imgui/imgui.cpp \
	$(SDLDIR)/imgui/imgui_draw.cpp \
	$(SDLDIR)/imgui/imgui_tables.cpp \
	$(SDLDIR)/imgui/imgui_widgets.cpp \
	$(SDLDIR)/imgui/imgui_impl_sdl2.cpp \
	$(SDLDIR)/imgui/imgui_impl_sdlrenderer2.cpp

IMGUI_OBJS := $(patsubst $(SRC)/%.cpp,$(BUILD)/imgui-obj/%.o,$(IMGUI_SRCS))

.PHONY: all clean $(VARIANTS)
all: $(VARIANTS)

# one rule set per variant: objects under build/<variant>/, binary build/e<variant>
define VARIANT_template
OBJS_$(1) := $$(patsubst $$(SRC)/%.cpp,$$(BUILD)/$(1)/%.o,$$(CORE_SRCS) $$(EXTRA_SRCS_$(1)) $$(OSD_SRCS)) $$(IMGUI_OBJS)

$(1): $$(BUILD)/e$(1)

$$(BUILD)/e$(1): $$(OBJS_$(1))
	$$(CXX) $$^ -o $$@ $$(LDFLAGS)
	@echo "==> built $$@"

$$(BUILD)/$(1)/%.o: $$(SRC)/%.cpp
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) $$(MDEF_$(1)) -c $$< -o $$@
endef
$(foreach v,$(VARIANTS),$(eval $(call VARIANT_template,$(v))))

$(BUILD)/imgui-obj/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# clean removes only what make produced; ROMs/disks living in build/ survive
clean:
	rm -rf $(addprefix $(BUILD)/,$(VARIANTS) imgui-obj) $(addprefix $(BUILD)/e,$(VARIANTS))
