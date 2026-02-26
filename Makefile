.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      := 3ds-weather
BUILD       := build
SOURCES     := source
DATA        := data
INCLUDES    := include

APP_TITLE       := 3DS Weather
APP_DESCRIPTION := Meteo per Nintendo 3DS
APP_AUTHOR      := Dzhmelyk135
ICON            := icon.png

ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS  := -g -Wall -O2 -mword-relocations \
           -fomit-frame-pointer -ffunction-sections \
           $(ARCH) $(INCLUDE) -D__3DS__

CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
ASFLAGS     := -g $(ARCH)
LDFLAGS      = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS        := -lctru -lm
LIBDIRS     := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)
export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES    := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD   := $(if $(CPPFILES),$(CXX),$(CC))
export OFILES_BIN   := $(addsuffix .o,$(BINFILES))
export OFILES_SRC   := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES       := $(OFILES_BIN) $(OFILES_SRC)
export INCLUDE      := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                       $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                       -I$(BUILD)
export LIBPATHS     := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON := $(TOPDIR)/icon.png

# ── Questa riga allega SMDH al .3dsx per Homebrew Launcher ──────────────
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh

.PHONY: $(BUILD) clean all cia

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf \
	        $(TARGET).cia $(TOPDIR)/banner.bnr

BANNERTOOL := $(TOPDIR)/bannertool

cia: all
	@echo "=== Generazione banner ==="
	$(BANNERTOOL) makebanner \
	    -i $(TOPDIR)/banner.png \
	    -a $(TOPDIR)/audio.wav \
	    -o $(TOPDIR)/banner.bnr
	@echo "=== Generazione SMDH ==="
	$(BANNERTOOL) makesmdh \
	    -s "3DS Weather" \
	    -l "App meteo per Nintendo 3DS" \
	    -p "Dzhmelyk135" \
	    -i $(TOPDIR)/icon.png \
	    -o $(TOPDIR)/$(TARGET).smdh
	@echo "=== Generazione CIA ==="
	makerom -f cia \
	        -target t \
	        -exefslogo \
	        -o $(TOPDIR)/$(TARGET).cia \
	        -elf $(TOPDIR)/$(TARGET).elf \
	        -rsf $(TOPDIR)/app.rsf \
	        -banner $(TOPDIR)/banner.bnr \
	        -icon $(TOPDIR)/$(TARGET).smdh
	@echo "=== CIA pronto: $(TARGET).cia ==="

else

DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).3dsx : $(OUTPUT).elf $(_3DSXDEPS)
$(OUTPUT).elf  : $(OFILES)
-include $(DEPENDS)

endif