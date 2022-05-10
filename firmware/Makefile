prefix = arm-none-eabi

build_dir = build
src_dir = src

# project name & config 
target = oneffb
debug = 1
opt = -Og

# sources 
asm_sources +=
c_sources += src/main.c
c_sources += $(wildcard src/peripheral/src/*.c)
# add tinyusb
c_sources += $(wildcard src/tinyusb/src/*.c)
c_sources += $(wildcard src/tinyusb/src/device/*.c)
c_sources += $(wildcard src/tinyusb/src/class/*/*.c)
c_sources += $(wildcard src/tinyusb/src/portable/st/synopsys/*.c)
c_sources += $(wildcard src/tinyusb/src/common/*.c)

# defines
defs += -DUSE_STDPERIPH_DRIVER
defs += -DSTM32F411xE

# includes 
includes += -Isrc
includes += -Isrc/peripheral/inc

includes += -Isrc/tinyusb/src
includes += -Isrc/tinyusb/src/portable/st/synopsys
includes += -Isrc/tinyusb/src/common



# include platform support file
include src/platform/makefile