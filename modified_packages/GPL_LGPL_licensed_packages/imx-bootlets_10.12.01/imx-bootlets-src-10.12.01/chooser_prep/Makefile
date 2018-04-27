CURRENTDIR	= .
SRCDIR		= $(CURRENTDIR)

INCLUDEDIR	= $(SRCDIR)/../mach-$(ARCH)/includes

# Linker script
BASE_ADDR	?= 0x00000000
BOOT_LAYOUT	= $(SRCDIR)/link.lds


CROSS_COMPILE ?= arm-linux-gnueabi-

AS	= $(CROSS_COMPILE)as
CC	= $(CROSS_COMPILE)gcc
LD	= $(CROSS_COMPILE)ld
CPP	= $(CROSS_COMPILE)cpp
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump

MEM_TYPE ?= MEM_DDR1

CFLAGS 	= -g -Wall -I$(INCLUDEDIR)/hab -I$(INCLUDEDIR)/registers -I$(INCLUDEDIR) -I./ -I../rom_lib/include -O -D$(MEM_TYPE)
LDFLAGS = -static -nostdlib -T $(BOOT_LAYOUT)

# Generic code
CORE_OBJS = chooser.o debug.o

all: clean chooser_prep


%.o: %.S
	$(CC) -c $(CFLAGS) -D__ASSEMBLY__ -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# Make targets
#
.PHONY: all build clean

chooser_prep: $(BOOT_LAYOUT) $(CORE_OBJS)
	$(MAKE) -C ../rom_lib
	$(LD) -o $@ $(CORE_OBJS) $(LDFLAGS) ../rom_lib/romlib.a /opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/lib/gcc/arm-fsl-linux-gnueabi/4.4.4/libgcc.a

clean:
	@echo Cleaning...
	@echo Files:
	@echo Build output:
	rm -rf *.o
	rm -f chooser_prep
