#
# Bare-metal Dhrystone for Cortex-R52 (V4H / X5H).
#
# Shared toolchain, flags and the CR52 BSP come from ../Makefile.common;
# this file only lists the Dhrystone-specific objects and link steps.
#
# Build:    make
# Clean:    make clean
#

include ../Makefile.common

PORT_DIR = barebones

CFLAGS  = $(PORT_CFLAGS) -I$(PORT_DIR) -I$(BSP_DIR) -I. -D$(LSI) \
          -DFLAGS_STR="\"$(PORT_CFLAGS)\""
ASFLAGS = -I$(BSP_DIR) $(ASFLAGS_COMMON)

LFLAGS  = -T $(LDSCRIPT) -Map dhrystone.map \
          -L$(shell dirname $(shell $(CC) -print-libgcc-file-name)) -lgcc

# Shared BSP objects (built from ../bsp/cr52), then Dhrystone-specific objects.
# boot_mon.s provides the V4H reset/boot path; on X5H the boot is handled
# externally (mirrors CoreMark, which also omits boot_mon for X5H).
BSP_OBJS = $(addprefix $(BSP_DIR)/,$(BSP_CSRCS:.c=.o))
ifeq ($(LSI),V4H)
BSP_OBJS := $(BSP_DIR)/boot_mon.o $(BSP_OBJS)
endif

OBJS = \
	$(BSP_OBJS) \
	$(PORT_DIR)/dhry_portme.o \
	dhry_1.o \
	dhry_2.o

OUT = dhrystone.elf

.PHONY: all clean

all: $(OUT)

$(OUT): $(OBJS) $(LDSCRIPT)
	$(LD) $(OBJS) -o $(OUT) $(LFLAGS)
	$(OC) -O srec --srec-forceS3 $(OUT) dhrystone.mot
	$(OC) -O binary $(OUT) dhrystone.bin
	#$(OD) -h $(OUT)

# Dhrystone sources (top dir) and the port glue (barebones/).
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(PORT_DIR)/%.o: $(PORT_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Shared BSP objects.
$(BSP_DIR)/%.o: $(BSP_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BSP_DIR)/%.o: $(BSP_DIR)/%.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(OUT) dhrystone.map dhrystone.mot dhrystone.bin
