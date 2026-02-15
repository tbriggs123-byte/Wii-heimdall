# Wii Heimdall Complete Makefile
TARGET = wii-heimdall
BUILD = build
SOURCES = source
INCLUDES = include

# Toolchain
PREFIX = powerpc-eabi-
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip

# Paths
DEVKITPRO = /opt/devkitpro
DEVKITPPC = $(DEVKITPRO)/devkitPPC

# Libraries
LIBS = -lfat -lwiiuse -lbte -logc -lm

# Flags
CFLAGS = -g -O2 -Wall -Wno-strict-aliasing \
         -DGEKKO -mcpu=750 -meabi -mhard-float \
         -I$(DEVKITPRO)/libogc/include \
         -I$(DEVKITPRO)/libfat/include \
         -I$(INCLUDES)

LDFLAGS = -g $(MACHDEP) -Wl,-Map,$(TARGET).map

# Source files
C_SOURCES = \
    $(SOURCES)/main.c \
    $(SOURCES)/gui.c \
    $(SOURCES)/config.c \
    $(SOURCES)/heimdall.c \
    $(SOURCES)/usb.c \
    $(SOURCES)/pit.c \
    $(SOURCES)/flash.c \
    $(SOURCES)/fileio.c

# Object files
OBJS = $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c)))

# Default target
all: $(TARGET).dol

# Create DOL from ELF
$(TARGET).dol: $(TARGET).elf \
    $(OBJCOPY) -O binary $< $@ \
    @echo "Created $(TARGET).dol"

# Link ELF
$(TARGET).elf: $(OBJS) \
    $(CC) $(LDFLAGS) $^ $(LIBS) -o $@

# Compile C files
$(SOURCES).c: $(OBJS) \
    @mkdir -p $(BUILD) \
    $(CC) $(CFLAGS) -c $< -o $@
