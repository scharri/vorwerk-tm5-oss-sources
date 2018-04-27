////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_ldfiles
//! @{
//
// Copyright (c) 2009 Freescale Semiconductor, Inc.
//
//! \file   catskills_rom.ld
//! \brief  catskills rom linker description file.
//! \note This file is conditionally built for chip and emulation platforms.
//! \note Chip register space will never be part of any linker description file.
//!
//! \see GHS build_arm.pdf and chip technical specification
//!
////////////////////////////////////////////////////////////////////////////////


CONSTANTS
{
  // **** OCRAM Constants ****

  RAM_ORIGIN          = 0     // OCRAM physical start address
  RAM_SZ              = 128K  //32K  // OCRAM is this big
  RAM_PATCH_ORIGIN    = 57K  // Make this (and/or RAM_PATCH_SZ) smaller to open up more OCRAM space for ROM use
  ROM_LOW_MEM_SZ      = 5K    // Exception vectors and assoc literal pool, Mfg test code
  RAM_L1PT_SIZE       = 16K
  RAM_L1PT_ORIGIN     = RAM_SZ - RAM_L1PT_SIZE
  RAM_HAB_MEM_SIZE    = 9K
  RAM_HAB_MEM_ORIGIN  = RAM_L1PT_ORIGIN - RAM_HAB_MEM_SIZE
  ROM_HAB_PERSIST_SIZE= 3K
  ROM_HAB_DMA_SIZE    = 256
  ROM_HAB_SCRATCH_SIZE= 6K-ROM_HAB_DMA_SIZE

  RAM_NOINIT_SZ       = 16    
  RAM_NOINIT_ORIGIN   = RAM_HAB_MEM_ORIGIN - RAM_NOINIT_SZ   // reserved OCRAM area begins here
  FIQ_STACK_SZ        = 64   // Stacks...
  ABORT_STACK_SZ      = 64
  UNDEF_STACK_SZ      =	64
  IRQ_STACK_SZ        = 512
  SUP_STACK_SZ        = 1K
  FREE_AREA_SZ        = 36K   // minimum size of .free_area section
  RAM_PATCH_SZ        = 4K    // This controls how much OCRAM is set aside for patch area
  RAM_DATA_SZ         = RAM_SZ - RAM_PATCH_ORIGIN - RAM_PATCH_SZ - RAM_L1PT_SIZE - RAM_NOINIT_SZ - RAM_HAB_MEM_SIZE 

  // **** ROM Constants	****
  ROM_ORIGIN           = 0xFFFE0000
  ROM_VECTOR_ORIGIN    = 0xFFFF0000
  ROM_END              = 0xFFFFFFFF
  ROM_RESERVED_ORIGIN  = 0xFFFFFFE4
  ROM_RESERVED_SZ      = 0x1C  // includes 8 bytes of spare
  ROM_HAB_REGION_SZ    = 64K
  ROM_SZ               = 128K

}


MEMORY
{
  // ROM @ 0xffff.0000 and vectors @ 0xffff.0000 (chip,simulation,and brazo rom in nor flash)
  // ROM @ 0x1080.0000 and vectors @ 0x0 (brazo with rom in ssram)

  // **** OCRAM ****
  RAM_PATCH_MEM      : ORIGIN = RAM_PATCH_ORIGIN,     LENGTH = RAM_PATCH_SZ
  RAM_DATA_MEM       : ORIGIN = .,                    LENGTH = RAM_DATA_SZ
  RAM_NOINIT_MEM     : ORIGIN = RAM_NOINIT_ORIGIN,    LENGTH = RAM_NOINIT_SZ
  RAM_HAB_MEM        : ORIGIN = RAM_HAB_MEM_ORIGIN,   LENGTH = RAM_HAB_MEM_SIZE
  RAM_PAGE_TABLE     : ORIGIN = RAM_L1PT_ORIGIN,      LENGTH = RAM_L1PT_SIZE 

  // **** ROM ****
  ROM_HAB_REGION     : ORIGIN = ROM_ORIGIN,           LENGTH = ROM_HAB_REGION_SZ
  ROM_LOW_MEM        : ORIGIN = ROM_VECTOR_ORIGIN,    LENGTH = ROM_LOW_MEM_SZ
  ROM_MEM            : ORIGIN = .,                    LENGTH = ROM_END - ROM_LOW_MEM_SZ - ROM_RESERVED_SZ - ROM_HAB_REGION_SZ
  ROM_RESERVED_MEM   : ORIGIN = ROM_RESERVED_ORIGIN,  LENGTH = ROM_RESERVED_SZ

}


//
// Program layout for starting in ROM, copying data to RAM,
// and continuing to execute out of ROM.
//

SECTIONS
{
    // *****************************
    // *** ROM RESIDENT SECTIONS ***
    // *****************************
    .text                                               ALIGN(4) : > ROM_HAB_REGION
    
    // vectors will be ROM resident at 0xFFFF.0000
    .vectors                                                     : > ROM_LOW_MEM
    // This section contains the Mfg Test code.
    .rom.text MAX_ENDADDRESS(ROM_VECTOR_ORIGIN+ROM_LOW_MEM_SZ)  ALIGN(4) : > .

    .hab.text                                           ALIGN(4) : {libhab_arm926_le.a(*(.text))}   > ROM_MEM
    .rodata                                             ALIGN(4) : > .

    // The ROM's R/W data copied to OCRAM by the startup code
    .copy.data  MAX_ENDADDRESS(ROM_RESERVED_ORIGIN) ROM(.data) ALIGN(4) : > .

    // ROM version stamp, CRC-32 of ROM, pointers to ROM and SDK function tables, pointer
	//  to coarse L2 table (mapping the ROM) used by SDK
    .rom.reserved           MAX_ENDADDRESS(ROM_END)              : > ROM_RESERVED_MEM

    // **********************
    // *** OCRAM SECTIONS ***
    // **********************
     .patch                        PAD(RAM_PATCH_SZ)             : > RAM_PATCH_MEM

    .sup.stack                    PAD(SUP_STACK_SZ)     ALIGN(4) : > RAM_DATA_MEM
    .irq.stack                    PAD(IRQ_STACK_SZ)     ALIGN(4) : > .
    .fiq.stack                    PAD(FIQ_STACK_SZ)     ALIGN(4) : > .
    .undef.stack                  PAD(UNDEF_STACK_SZ)   ALIGN(4) : > .
    .abort.stack                  PAD(ABORT_STACK_SZ)   ALIGN(4) : > .

    .bss                                                ALIGN(4) : > .
    .data                                               ALIGN(4) : > .
  
    // This free area is an "open hole" that ROM may carve up as needed.
	//
	// NOTE: A 'C' program can access this area like so:
	//     
	//   extern unsigned char _ghsbegin_free_area[];  - starting address
	//   extern unsigned int _ghssize_free_area[];    - length in bytes
	// 
    .free_area MAX_ENDADDRESS(RAM_NOINIT_ORIGIN) MIN_SIZE(FREE_AREA_SZ) : > .

    // This section holds unitialized data shared between startup .arm and startup .c files.
    // The variables defined in this section are NOT initialized as part of
    // normal .bss and .data initialization.  The intent is to initialize
    // variables in this section in .arm startup code and access them later from
    // "C" code.
    .global.noinit           MAX_ENDADDRESS(RAM_HAB_MEM_ORIGIN)  : > RAM_NOINIT_MEM
    .hab_memory                    PAD(RAM_HAB_MEM_SIZE) ALIGN(4) : > RAM_HAB_MEM
    .ram_pagetable                PAD(RAM_L1PT_SIZE)    ALIGN(4) : > RAM_PAGE_TABLE


// --------------------- LINKER SYMBOLS ---------------------

//
// These special symbols mark the bounds of RAM and ROM memory.
// They are used by the MULTI debugger.

   __ghs_romstart  = ROM_ORIGIN;
   __ghs_romend    = ROM_END;
   __ghs_ramstart  = RAM_ORIGIN;
   __ghs_ramend    = RAM_ORIGIN + RAM_SZ - 1;

  __linker_sym_free_area_sz = ADDR(.global.noinit) - ADDR(.free_area) ; 
  __linker_sym_stack_init_value  = 0xdeadbeef; // this is what gets written all over the stacks at init 
  __linker_sym_stack_size  = ENDADDR(.abort.stack) - ADDR(.sup.stack); // this much gets written with above

  __linker_sym_hab_persist_start = RAM_HAB_MEM_ORIGIN; 
  __linker_sym_hab_persist_end = RAM_HAB_MEM_ORIGIN + ROM_HAB_PERSIST_SIZE - 1; 
  __linker_sym_hab_dma_start = RAM_HAB_MEM_ORIGIN + ROM_HAB_PERSIST_SIZE; 
  __linker_sym_hab_dma_end = RAM_HAB_MEM_ORIGIN + ROM_HAB_PERSIST_SIZE + ROM_HAB_DMA_SIZE -1; 
  __linker_sym_hab_scratch_start = RAM_HAB_MEM_ORIGIN + ROM_HAB_PERSIST_SIZE + ROM_HAB_DMA_SIZE; 
  __linker_sym_hab_scratch_end = RAM_HAB_MEM_ORIGIN + ROM_HAB_PERSIST_SIZE + ROM_HAB_DMA_SIZE + ROM_HAB_SCRATCH_SIZE -1; 
}


////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

