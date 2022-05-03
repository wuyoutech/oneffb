prefix = arm-none-eabi

build_dir = build
src_dir = src

# project name & config 
target = template
debug = 1
opt = -Og

# sources 
asm_sources +=
c_sources += src/main.c
c_sources += $(wildcard src/peripheral/src/*.c)
c_sources += $(wildcard src/usb/src/*.c)
c_sources += $(wildcard src/usb/STM32_USB_Device_Library/Core/src/*.c)

# defines
defs += -DUSE_STDPERIPH_DRIVER
defs += -DSTM32F40_41xxx

# includes 
includes += -Isrc
includes += -Isrc/peripheral/inc

includes += -Isrc/usb/inc
includes += -Isrc/usb/STM32_USB_OTG_Driver/inc
includes += -Isrc/usb/STM32_USB_Device_Library/Core/inc



# include platform support file
include src/platform/makefile