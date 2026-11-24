#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
# Unified Switch Makefile.
#
#   make nx       - build for Switch 1 (libnx / deko3d / uam)
#   make ounce    - build for Switch 2 (libounce / drako3d / uam2)
#   make          - same as 'make nx'
#
# A PLATFORM can also be selected directly:
#   make PLATFORM=ounce
#
# Each platform keeps its own object directory (build / build-ounce) so
# switching back and forth does not force a full rebuild. Both platforms
# produce $(TARGET).nro in this directory, so building one overwrites the
# other's output.
#
#   make clean         - remove both platforms' objects and output files
#   make clean-nx      - remove Switch 1 objects
#   make clean-ounce   - remove Switch 2 objects
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

PLATFORM ?= nx
ifeq ($(filter nx ounce,$(PLATFORM)),)
$(error "Unknown PLATFORM '$(PLATFORM)'. Use 'make nx' or 'make ounce' (or PLATFORM=nx|ounce)")
endif

TOPDIR ?= $(CURDIR)
ifeq ($(PLATFORM),ounce)
include $(DEVKITPRO)/libounce/switch2_rules
else
include $(DEVKITPRO)/libnx/switch_rules
endif

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# ROMFS is the directory containing data to be added to RomFS, relative to the Makefile
#---------------------------------------------------------------------------------
TARGET		:=	Mesen
ifeq ($(PLATFORM),nx)
BUILD		:=	build-nx
else
BUILD		:=	build
endif
DATA		:=	data
INCLUDES	:=	include source .. ../Core ../Utilities
ROMFS		:=	romfs

# Output folder for uam-compiled shaders inside romfs
OUT_SHADERS	:=	shaders

APP_TITLE	:=	MesenCE
APP_AUTHOR	:=	Mesen Contributors, qlaunch-ext
APP_VERSION	:=	2.2.1

MESEN_DIRS	:=	$(shell find $(TOPDIR)/../Core $(TOPDIR)/../Utilities $(TOPDIR)/../SevenZip $(TOPDIR)/../Lua $(TOPDIR)/source -type d 2>/dev/null)
SOURCES		:=	$(MESEN_DIRS)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
# warnings reduce performance of compilation for me so i disable them...
# feel free to re-enable them if you want
ifeq ($(PLATFORM),ounce)
ARCH	:=	$(SWITCH2_CPU) -mtp=soft -fPIE
else
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
endif

CFLAGS	:=	-g -w -Ofast -flto -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-strict-aliasing \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__ -DHAVE_LIBNX -DNDEBUG

CFLAGS	+=	-D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L

CXXFLAGS	:= $(CFLAGS) -std=c++17 -include $(TOPDIR)/source/SwitchCompat.h
CFLAGS	:= $(CFLAGS) -include $(TOPDIR)/source/SwitchCompat.h

ASFLAGS	:=	-g $(ARCH)
ifeq ($(PLATFORM),ounce)
LDFLAGS	=	-w -specs=$(DEVKITPRO)/libounce/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -ldrako3d -lnx -lm -lz

# Shader compiler for Switch 2
UAM	:=	uam2
else
LDFLAGS	=	-w -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -ldeko3d -lnx -lm -lz

# Shader compiler for Switch 1
UAM	:=	uam
endif

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(LIBNX)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(SOURCES) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
GLSLFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.glsl)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

# --- Switch port exclusions ------------------------------------------------
CFILES		:=	$(filter-out serial.c unix.c unixdgram.c unixstream.c,$(CFILES))
CPPFILES	:=	$(filter-out pch.cpp Socket.cpp,$(CPPFILES))
# ---------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

# Compiled deko3d shaders
ifneq ($(strip $(ROMFS)),)
	ROMFS_TARGETS :=
	ROMFS_FOLDERS :=
	ifneq ($(strip $(OUT_SHADERS)),)
		ROMFS_SHADERS := $(ROMFS)/$(OUT_SHADERS)
		ROMFS_TARGETS += $(patsubst %.glsl, $(ROMFS_SHADERS)/%.dksh, $(GLSLFILES))
		ROMFS_FOLDERS += $(ROMFS_SHADERS)
	endif

	export ROMFS_DEPS := $(foreach file,$(ROMFS_TARGETS),$(CURDIR)/$(file))
endif

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: all nx ounce _build clean clean-nx clean-ounce

#---------------------------------------------------------------------------------
# Default build is Switch 1. Use 'make ounce' for Switch 2.
all: nx

nx:
	@$(MAKE) --no-print-directory -f $(CURDIR)/Makefile PLATFORM=nx _build

ounce:
	@$(MAKE) --no-print-directory -f $(CURDIR)/Makefile PLATFORM=ounce _build

_build: $(ROMFS_TARGETS) | $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

ifneq ($(strip $(ROMFS_TARGETS)),)

$(ROMFS_TARGETS): | $(ROMFS_FOLDERS)

$(ROMFS_FOLDERS):
	@mkdir -p $@

$(ROMFS_SHADERS)/%_vsh.dksh: %_vsh.glsl
	@echo {vert} $(notdir $<)
	@$(UAM) -s vert -o $@ $<

$(ROMFS_SHADERS)/%_tcsh.dksh: %_tcsh.glsl
	@echo {tess_ctrl} $(notdir $<)
	@$(UAM) -s tess_ctrl -o $@ $<

$(ROMFS_SHADERS)/%_tesh.dksh: %_tesh.glsl
	@echo {tess_eval} $(notdir $<)
	@$(UAM) -s tess_eval -o $@ $<

$(ROMFS_SHADERS)/%_gsh.dksh: %_gsh.glsl
	@echo {geom} $(notdir $<)
	@$(UAM) -s geom -o $@ $<

$(ROMFS_SHADERS)/%_fsh.dksh: %_fsh.glsl
	@echo {frag} $(notdir $<)
	@$(UAM) -s frag -o $@ $<

$(ROMFS_SHADERS)/%.dksh: %.glsl
	@echo {comp} $(notdir $<)
	@$(UAM) -s comp -o $@ $<

endif

#---------------------------------------------------------------------------------
clean: clean-nx clean-ounce

clean-nx:
	@echo clean nx ...
	@rm -fr build $(TARGET).nro $(TARGET).nacp $(TARGET).elf $(TARGET).nsp $(TARGET).nso $(TARGET).npdm

clean-ounce:
	@echo clean ounce ...
	@rm -fr build-ounce $(TARGET).nro $(TARGET).nacp $(TARGET).elf $(TARGET).nsp $(TARGET).nso $(TARGET).npdm


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
ifeq ($(strip $(APP_JSON)),)

all	:	$(OUTPUT).nro

ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp $(ROMFS_DEPS)
else
$(OUTPUT).nro	:	$(OUTPUT).elf $(ROMFS_DEPS)
endif

else

all	:	$(OUTPUT).nsp

$(OUTPUT).nsp	:	$(OUTPUT).nso $(OUTPUT).npdm

$(OUTPUT).nso	:	$(OUTPUT).elf

endif

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
