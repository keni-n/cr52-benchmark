# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Original Author: Shay Gal-on

#File : core_portme.mak

# Shared toolchain, flags, target-SoC (LSI) and CR52 BSP location/sources.
# CoreMark is built from coremark/, so the BSP is at ../bsp/cr52.
include ../Makefile.common

# Flag : OUTFLAG
#	Use this flag to define how to to get an executable (e.g -o)
OUTFLAG= -o
# Flag : CFLAGS
#	Compiler options. PORT_CFLAGS / CC / LD / AS come from ../Makefile.common.
#	Add more from the command line using XCFLAGS="other flags".
FLAGS_STR = "$(PORT_CFLAGS) $(XCFLAGS) $(XLFLAGS) $(LFLAGS_END)"
CFLAGS = $(PORT_CFLAGS) -I$(PORT_DIR) -I$(BSP_DIR) -I. -DFLAGS_STR=\"$(FLAGS_STR)\"
ifeq ($(LSI),V4H)
CFLAGS += -DV4H
endif
ifeq ($(LSI),X5H)
CFLAGS += -DX5H
endif
#Flag : LFLAGS_END
#	Define any libraries needed for linking or other flags that should come at the end of the link line (e.g. linker scripts).
#	Note : On certain platforms, the default clock_gettime implementation is supported but requires linking of librt.
SEPARATE_COMPILE=1
# Flag : SEPARATE_COMPILE
# You must also define below how to create an object file, and how to link.
OBJOUT 	= -o
# Linker script (memory map) is provided per-SoC by the BSP via $(LDSCRIPT).
LFLAGS  = -T $(LDSCRIPT) -Map coremark.map
ASFLAGS = -I$(PORT_DIR) -I$(BSP_DIR) $(ASFLAGS_COMMON)
#OFLAG 	= -o
COUT 	= -c

#LFLAGS_END = -L$(subst libc.a, ,$(shell $(CC) -print-file-name=libc.a 2> /dev/null)) -lc
LFLAGS_END += -L$(subst libgcc.a, ,$(shell $(CC) -print-libgcc-file-name 2> /dev/null)) -lgcc
# Flag : PORT_SRCS
# 	Port specific source files. core_portme.c is CoreMark-specific (lives in
#	$(PORT_DIR)); the console/PMU/CPU-bring-up sources are the shared BSP.
PORT_SRCS = $(PORT_DIR)/core_portme.c $(addprefix $(BSP_DIR)/,$(BSP_CSRCS))
vpath %.c $(PORT_DIR) $(BSP_DIR)
vpath %.s $(PORT_DIR) $(BSP_DIR)
vpath %.h $(PORT_DIR) $(BSP_DIR)
vpath %.mak $(PORT_DIR)

# Flag : LOAD
#	For a simple port, we assume self hosted compile and run, no load needed.

# Flag : RUN
#	For a simple port, we assume self hosted compile and run, simple invocation of the executable

LOAD = echo "Please set LOAD to the process of loading the executable to the flash"
RUN = echo "Please set LOAD to the process of running the executable (e.g. via jtag, or board reset)"

OEXT = .o
EXE = .elf

ifeq ($(LSI),V4H)
PORT_OBJS = $(PORT_DIR)/boot_mon$(OEXT)
endif
ifeq ($(LSI),X5H)
PORT_OBJS =
endif
# core_portme + shared BSP objects (compiled from $(BSP_DIR) via vpath).
PORT_OBJS += $(PORT_DIR)/core_portme$(OEXT) $(addprefix $(PORT_DIR)/,$(BSP_CSRCS:.c=$(OEXT)))
PORT_CLEAN = $(PORT_DIR)/*$(OEXT) coremark.map $(basename $(OUTFILE)).bin $(basename $(OUTFILE)).mot

$(OPATH)$(PORT_DIR)/%$(OEXT) : %.c
	$(CC) $(CFLAGS) $(XCFLAGS) $(COUT) $< $(OBJOUT) $@

$(OPATH)%$(OEXT) : %.c
	$(CC) $(CFLAGS) $(XCFLAGS) $(COUT) $< $(OBJOUT) $@

$(OPATH)$(PORT_DIR)/%$(OEXT) : %.s
	$(AS) $(ASFLAGS) $< $(OBJOUT) $@

# Target : port_pre% and port_post%
# port_postbuild emits the SREC (.mot) and raw binary (.bin) images next to
# the .elf (same as the Dhrystone build). The other hooks are no-ops.

.PHONY : port_prebuild port_postbuild port_prerun port_postrun port_preload port_postload
port_postbuild :
	$(OC) -O srec --srec-forceS3 $(OUTFILE) $(basename $(OUTFILE)).mot
	$(OC) -O binary $(OUTFILE) $(basename $(OUTFILE)).bin

port_prebuild port_prerun port_postrun port_preload port_postload :

# FLAG : OPATH
# Path to the output folder. Default - current folder.
OPATH = ./
MKDIR = mkdir -p
