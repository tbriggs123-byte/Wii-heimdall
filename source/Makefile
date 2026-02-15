ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=/opt/devkitpro/devkitPPC")
endif

export PATH := $(DEVKITPPC)/bin:$(PATH)

# Wii Heimdall Complete Makefile
TARGET = wii-heimdall
BUILD = build
SOURCES = source
INCLUDES = include

# Toolchain - Updated to powerpc-eabi-gcc for Wii
PREFIX = powerpc-eabi-
CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip

# Libraries
LIBS = -lfat -lwiiuse -lbte -logc -lm

# Flags - Added -mrvl for Wii development
CFLAGS = -g -O2 -Wall -mrvl -mcpu=750 -meabi -mhard-float \
         -I$(DEVKITPRO)/libogc/include \
         -I$(DEVKITPRO)/libfat/include \
         -I$(INCLUDES)

LDFLAGS = -g -mrvl -mcpu=750 -Wl,-Map,$(TARGET).map

# Source files
C_SOURCES = $(wildcard $(SOURCES)/*.c)
# Object files (mapping source/*.c to build/*.o)
OBJS = $(patsubst $(SOURCES)/%.c, $(BUILD)/%.o, $(C_SOURCES))

# Default target
all: $(TARGET).dol

# Create DOL from ELF
$(TARGET).dol: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@echo "Created $(TARGET).dol"

# Link ELF
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

# Compile C files (Pattern rule)
$(BUILD)/%.o: $(SOURCES)/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).dol $(TARGET).map
