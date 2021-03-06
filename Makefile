# 
# Test random seed generation using LSI/HSI jitter
#

# Project  Basename
PROJECT = ltpnl

# Firmware Version
VERSION = 20201205

# Build Objects

# main() [required]
OBJECTS = src/main.o

# Extra build objects
OBJECTS += src/well512.o
OBJECTS += src/flash.o
OBJECTS += src/display.o
OBJECTS += src/mca.o

# system_init(), reset and fault handers [required]
OBJECTS += src/system.o

# Cortex-M4 vector table [required]
OBJECTS += src/vtable.o

# Link script
LINKSCRIPT = include/stm32f303xe_ram.ld

# ---

# Target binary
TARGET = $(PROJECT).elf

# Flash memory loader
LOADELF = loader.elf
LOADSRC = src/loader.s
LOADSCRIPT = include/loader.ld

# Listing files
TARGETLIST = $(TARGET:.elf=.lst)

# Compiler
CC = arm-none-eabi-gcc

# Include debugging symbols
DEBUG = -g

# Set language standard and bare metal environment (note: ARM CMSIS is C99)
DIALECT = -std=c99 -pedantic -ffreestanding

# General code optimisation level
OPTIMISE = -Os
# Mark functions and data so linker can omit dead sections
OPTIMISE += -ffunction-sections -fdata-sections
# Place uninitialized global variables in the data section of the object file
OPTIMISE += -fno-common
# Perform link-time optimisation (see note re ld bug) [prod]
OPTIMISE += -flto

# Treat all warnings as errors
WARN = -Werror
# Warn on questionable constructs
WARN += -Wall
# Extra warning flags not enabled by -Wall
WARN += -Wextra
# Warn whenever a local variable or type declaration shadows another ...
WARN += -Wshadow
# Warn if an undefined identifier is evaluated in an #if directive
WARN += -Wundef
# Warn for implicit conversions that may alter a value [annoying]
WARN += -Wconversion

# Arm cpu type (implies endian and arch)
ARMOPTS = -mcpu=cortex-m4
# Generate thumb code
ARMOPTS += -mthumb

# Define chip type for STM headers
CPPFLAGS = -DSTM32F303xE
# Add include path for headers
CPPFLAGS += -Iinclude
# Lock GPIO ports in system_init() (requires power on reset to clear)
CPPFLAGS += -DLOCK_GPIO
# Enable IWDG in user interface handler
CPPFLAGS += -DUSE_IWDG

# Store firmware version in binary
CPPFLAGS += -DSYSTEMVERSION=$(VERSION)
FIRMWARE = $(PROJECT)_$(VERSION).bin
BINSIGNATURE = $(FIRMWARE).sign

# Extra libraries
LDLIBS = 

# Use provided memory script when linking [required]
LDFLAGS = -T$(LINKSCRIPT)
# Don't include stdlib when linking
LDFLAGS += -nostdlib
# Omit dead sections [prod] (note: this may cause problems with LTO)
#LDFLAGS += -Wl,--gc-sections

# Combined Compiler flags
CFLAGS = $(DIALECT) $(DEBUG) $(OPTIMISE) $(WARN) $(ARMOPTS)
LCFLAGS = CFLAGS

# Debugger
DB = gdb-multiarch
DBSCRIPT = -ex 'target remote :3333'
DBSCRIPT += -ex 'monitor reset halt'
DBSCRIPT += -ex 'set $$msp = 0x20010000'
DBSCRIPT += -ex 'load'

# On-chip debugger
OCD = openocd
OCDFLAGS = -d0 -f ./include/openocd-stlinkv2.cfg
#OCDFLAGS = -d0 -f ./include/openocd-nucleo.cfg

# Flash re-programming script
OCDWRITE = -c init
OCDWRITE += -c "reset halt" 
OCDWRITE += -c "flash write_image erase $(FIRMWARE) 0x08000000"
OCDWRITE += -c "verify_image $(FIRMWARE) 0x08000000"
OCDWRITE += -c shutdown

# Flash erase script
OCDERASE = -c init
OCDERASE += -c "reset halt" 
OCDERASE += -c "stm32f1x mass_erase 0"
OCDERASE += -c shutdown

# binutils
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
NM = arm-none-eabi-nm
NMFLAGS = -n -r
DISFLAGS = -d -S
OBJDUMP = arm-none-eabi-objdump
GPG = gpg2
GPGFLAGS = -b -u $(GPGKEY)

# Default target is $(TARGET)
.PHONY: elf
elf: $(TARGET)

$(OBJECTS): Makefile

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $(TARGET) $(OBJECTS)

$(LOADELF): $(LOADSRC) $(LOADSCRIPT)
	$(CC) $(CFLAGS) -T$(LOADSCRIPT) -nostdlib -o $(LOADELF) $(LOADSRC)

$(FIRMWARE): $(LOADELF) $(TARGET)
	$(OBJCOPY) --pad-to=0x08000800 --gap-fill=0xFF -O binary $(LOADELF) .loader.bin
	$(OBJCOPY) --pad-to=0x10004000 --gap-fill=0xFF -O binary $(TARGET) .target.bin
	cat .loader.bin .target.bin > $(FIRMWARE)
	-rm .loader.bin .target.bin

$(BINSIGNATURE): $(FIRMWARE)
	$(GPG) $(GPGFLAGS) --output $(BINSIGNATURE) $(FIRMWARE)

# Override compilation recipe for assembly files
%.o: %.s
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Add recipe for listing files
%.lst: %.o
	$(OBJDUMP) $(DISFLAGS) $< > $@
%.lst: %.elf
	$(OBJDUMP) $(DISFLAGS) $< > $@

.PHONY: size
size: $(TARGET)
	$(SIZE) $(TARGET)

.PHONY: nm
nm: $(TARGET)
	$(NM) $(NMFLAGS) $(TARGET)

.PHONY: list
list: $(TARGETLIST)

.PHONY: erase
erase:
	$(OCD) $(OCDFLAGS) $(OCDERASE)

.PHONY: upload
upload: $(FIRMWARE)
	$(OCD) $(OCDFLAGS) $(OCDWRITE)

.PHONY: ocd
ocd:
	$(OCD) $(OCDFLAGS)
	
.PHONY: debug
debug: $(TARGET)
	$(DB) $(DBSCRIPT) $(TARGET)

.PHONY: bin
bin: $(FIRMWARE)

.PHONY: sign
sign: $(BINSIGNATURE)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(LOADELF) $(LOADOBJ) $(FIRMWARE) $(BINSIGNATURE) $(OBJECTS) $(LISTFILES) $(TARGETLIST)

.PHONY: help
help:
	@echo
	@echo Targets:
	@echo " elf [default]	build all objects, link and write $(TARGET)"
	@echo " bin		create flash loader binary $(FIRMWARE)"
	@echo " sign		sign $(FIRMWARE) using gpg"
	@echo " size		list $(TARGET) section sizes"
	@echo " nm		list all defined symbols in $(TARGET)"
	@echo " list		create text listing for $(TARGET)"
	@echo " ocd		launch openocd on target in foreground"
	@echo " debug		debug $(TARGET) on target"
	@echo " erase		bulk erase flash on target"
	@echo " upload		write $(FIRMWARE) to flash and verify"
	@echo " clean		remove all intermediate files and logs"
	@echo
