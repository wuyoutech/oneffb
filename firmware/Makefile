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

# defines
defs += -DUSE_STDPERIPH_DRIVER
defs += -DSTM32F40_41xxx

# includes 
includes += -Isrc
includes += -Isrc/peripheral

# include platform support file
include src/platform/makefile