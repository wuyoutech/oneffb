# sources
asm_sources += src/platform/startup/gcc/startup_stm32f411xe.s

c_sources += src/platform/system_stm32f4xx.c
#remove fmc.c because STM32F407 not support it
c_sources += $(wildcard src/platform/std_periph/src/*.c)

# includes 
includes += -Isrc/platform
includes += -Isrc/platform/cmsis
includes += -Isrc/platform/std_periph/inc

# link script 
ld_scripts = src/platform/link_script/STM32F411CEUx_FLASH.ld

# compilers
cc = $(prefix)-gcc
as = $(prefix)-gcc -x assembler-with-cpp
cp = $(prefix)-objcopy
sz = $(prefix)-size

# build flags
# if transplant to other platform, add hard float ABI support here 
mcu = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
build_flags = $(mcu) $(defs) $(includes) $(opt) -Wall -fdata-sections -ffunction-sections
# add debug pattern
ifeq ($(debug), 1)
build_flags += -g -gdwarf-2
endif

# generate dependency information 
build_flags += -MMD -MP -MF"$(@:%.o=%.d)"

# libraries
libs = -lc -lm -lnosys
libdir =
ld_flags = $(mcu) -specs=nano.specs -T$(ld_scripts) $(libdir) $(libs) -Wl,-Map=$(build_dir)/$(target).map,--cref -Wl,--gc-sections

# list of objects 
objects += $(patsubst $(src_dir)/%.c,$(build_dir)/%.o,$(c_sources))
objects += $(patsubst $(src_dir)/%.s,$(build_dir)/%.o,$(asm_sources))

# list of folders
objdirs = $(sort $(dir $(objects)))

# include dependencies
-include $(wildcard $(build_dir)/*.d)

# action
all: $(target).elf
hex: $(target).hex
bin: $(target).bin
clean:
	$(info clean build result $<)
	@-rm -rf $(build_dir)/*
	@-rm -rf $(target).hex $(target).bin $(target).elf
flash: $(target).elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c halt -c "flash write_image erase $(target).elf" -c reset -c shutdown

	
# dependency
$(objdirs):
	@mkdir -p $@
$(target).elf: $(objects) 
	$(info linking $@)
	@$(cc) $(objects) $(ld_flags) -o $@
	@$(sz) $@
$(build_dir)/%.o: $(src_dir)/%.s | $(objdirs)
	@$(as) -c $(build_flags) $< -o $@
	$(info compiling $<)
$(build_dir)/%.o: $(src_dir)/%.c | $(objdirs)
	@$(cc) -c $(build_flags) -Wa,-a,-ad,-alms=$(build_dir)/$(notdir $(<:.c=.lst)) $< -o $@
	$(info compiling $<)
%.hex: %.elf
	@$(cp) -O ihex $< $@
	$(info converting hex file $<)
%.bin: %.elf
	@$(cp) -O binary -S $< $@
	$(info converting bin file $<)