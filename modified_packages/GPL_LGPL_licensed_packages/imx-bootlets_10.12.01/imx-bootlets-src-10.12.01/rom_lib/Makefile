LIBNAME		= romlib

SRCDIR		= .
NANDDIR		= $(SRCDIR)/drivers/nand_boot

INCLUDEDIRS	=   -I$(SRCDIR)/include \
		    -I$(SRCDIR)/include/registers \
		    -I$(SRCDIR)/drivers/include \
		    -I$(NANDDIR)/include \
		    -I$(NANDDIR)/src/include \
		    -I$(NANDDIR)/src/hal/include

CROSS_COMPILE ?= arm-linux-gnueabi-

AS	= $(CROSS_COMPILE)as
CC	= $(CROSS_COMPILE)gcc
LD	= $(CROSS_COMPILE)ld
CPP	= $(CROSS_COMPILE)cpp
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump

CFLAGS	= -g -Wall $(INCLUDEDIRS) -O -DTGT_MX28

# Generic code
CORE_OBJS = $(SRCDIR)/common/persistent_bit.o \
	    $(SRCDIR)/common/plugin_tools.o \
	    $(SRCDIR)/drivers/common/mbr.o \
	    $(SRCDIR)/drivers/common/configblock.o \
	    $(NANDDIR)/src/rom_nand_api.o \
	    $(NANDDIR)/src/rom_ba_nand_api.o \
	    $(NANDDIR)/src/rom_nand_hamming_code_ecc.o \
	    $(NANDDIR)/src/hal/rom_nand_hal.o \
	    $(NANDDIR)/src/hal/rom_ba_nand_hal.o \
	    $(NANDDIR)/src/hal/rom_nand_hal_dma.o \
	    $(NANDDIR)/src/hal/rom_nand_hal_gpmi.o


# Default goal
.PHONY: all build clean
all: $(LIBNAME)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# Make targets
#
$(LIBNAME): $(CORE_OBJS)
	ar rs $(LIBNAME).a $(CORE_OBJS)
#	ranlib lib$(LIBNAME).a

clean:
	@echo Cleaning...
	rm -f $(LIBNAME).a
	rm -f $(CORE_OBJS)

