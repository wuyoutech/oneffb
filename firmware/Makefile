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
c_sources += src/usb_descriptors.c
c_sources += $(wildcard src/peripheral/src/*.c)
# add tinyusb
c_sources += $(wildcard src/tinyusb/src/*.c)
c_sources += $(wildcard src/tinyusb/src/device/*.c)
c_sources += $(wildcard src/tinyusb/src/class/*/*.c)
c_sources += $(wildcard src/tinyusb/src/portable/synopsys/dwc2/*.c)
c_sources += $(wildcard src/tinyusb/src/common/*.c)

# add lwip
include src/lwip/src/Filelists.mk
LWIPDIR := src/lwip/src
c_sources += $(COREFILES)
c_sources += $(CORE4FILES)
c_sources += $(NETIFFILES)
c_sources += $(HTTPFILES)

# add network server
c_sources += $(wildcard src/networking/*.c)


# defines
defs += -DUSE_STDPERIPH_DRIVER
defs += -DSTM32F411xE

# includes 
includes += -Isrc
includes += -Isrc/peripheral/inc

includes += -Isrc/tinyusb/src
includes += -Isrc/tinyusb/src/portable/st/synopsys
includes += -Isrc/tinyusb/src/common
includes += -Isrc/networking
includes += -Isrc/lwip/src/include



# include platform support file
include src/platform/makefile