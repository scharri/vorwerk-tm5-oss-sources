////////////////////////////////////////////////////////////////////////////////
//! \addtogroup include
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    cpu_support.h
//! \brief   Prototypes for ARM CPU support functions
//!
////////////////////////////////////////////////////////////////////////////////
#ifndef __CPU_SUPPORT_H_
#define __CPU_SUPPORT_H_

//!     Enable or disable the IRQ Interrupt.
//!
//!     Input parameter holds the enable/disable flag (1 = enable, 0 = disable)
//!
//!     Returns previous state of IRQ Interrupt.
uint32_t cpu_EnableIrqInterrupt( uint32_t flag ) ;


//!     Enable or disable the FIQ Interrupt.
//!
//!     Input parameter	holds the enable/disable flag (1 = enable, 0 = disable)
//!
//!     Returns previous state of FIQ Interrupt.
uint32_t cpu_EnableFiqInterrupt( uint32_t flag ) ;


//!   Invalidate Entire ARM926 Instruction and Data TLBs
void cpu_invalidate_TLBs( void ) ;


//!   Invalidate entire ARM926 Instruction TLB
void cpu_invalidate_ITLB( void ) ;


//!   Invalidate single ARM926 Instruction TLB entry
//!
//!   Input argument is the Virtual Address of entry to be invalidated
void cpu_invalidate_ITLB_entry( uint32_t address ) ;


//!   Invalidate entire ARM926 Instruction Cache
void cpu_invalidate_ICache( void ) ;


//!   Invalidate single ARM926 Instruction Cache entry
//!
//!   Input parameter is the Virtual Address to invalidate
//!
void cpu_invalidate_ICache_MVA( uint32_t address ) ;


//!   Returns the ARM926 CP15 Control Register content
//!
//!   Notes:
//!
//!   ARM DDI0198D recommends access to this register be RMW
//!
//!
//!   31                 19  18 17 16 15 14 13 12 11 10 9 8 7 6  3 2 1 0
//!   -------------------------------------------------------------------
//!   |                     |S |S |S |  |  |  |  |     | | | |    | | | |
//!   |      SBZ            |B |B |B |L |R |V |I | SBZ |R|S|B|SBO |C|A|M|
//!   |                     |O |Z |O |4 |R |  |  |     | | | |    | | | |
//!   -------------------------------------------------------------------
//!
//!   Bits 31:19           Reserved (SBZ)
//!   Bit18                Reserved (SBO)
//!   Bit17                Reserved (SBZ)
//!   Bit16                Reserved (SBO)
//!   Bit15       L4 bit   Determines if T bit is set when load instructions
//!                        change the PC. (0=set T bit, 1=do not set T bit(ARMv4 behaviour)
//!   Bit14       RR bit   Cache Replacement Strategy (0=random, 1=Round Robin)
//!   Bit13        V bit   Location of exception vectors (0=0x0,1=0xffff0000)
//!                        (reset value depends upon VINITHI pin)
//!   Bit12        I bit   I Cache Enable (0=disable,1=enable)
//!   11:10                Reserved (SBZ)
//!   Bit9         R bit   ROM Protection bit
//!   Bit8         S bit   System Protection bit
//!   Bit7         B bit   Endianess (0=little,1=big)
//!                        (reset value depends upon BIGENDINIT pin)
//!   Bits 6:3             Reserved (SBO)
//!   Bit2         C bit   D Cache Enable (0=disable,1=enable)
//!   Bit1         A bit   Data Alignment fault enable (0=disable,1=enable)
//!   Bit0         M bit   MMU enable (0=disable,1=enable)
//!
//!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cpu_get_CP15_control_reg_1( void ) ;


//!  Modifies ARM926 CP15 Control Register
//!  Input parameter is a Mask of CP15 reg1 bit positions
//!
//!   Notes:
//!
//!   NOTE: ARM DDI0198D recommends access to this register be RMW
//!
//!   IMPORTANT NOTE: This function will enforce the Reserved bits
//!                   to their required values (i.e. it will ignore
//!                   input bit values for these reserved bits)
//!
//!   31                 19  18 17 16 15 14 13 12 11 10 9 8 7 6  3 2 1 0
//!   -------------------------------------------------------------------
//!   |                     |S |S |S |  |  |  |  |     | | | |    | | | |
//!   |      SBZ            |B |B |B |L |R |V |I | SBZ |R|S|B|SBO |C|A|M|
//!   |                     |O |Z |O |4 |R |  |  |     | | | |    | | | |
//!   -------------------------------------------------------------------
//!
//!   Bits 31:19           Reserved (SBZ)
//!   Bit18                Reserved (SBO)
//!   Bit17                Reserved (SBZ)
//!   Bit16                Reserved (SBO)
//!   Bit15       L4 bit   Determines if T bit is set when load instructions
//!                        change the PC. (0=set T bit, 1=do not set T bit(ARMv4 behaviour)
//!   Bit14       RR bit   Cache Replacement Strategy (0=random, 1=Round Robin)
//!   Bit13        V bit   Location of exception vectors (0=0x0,1=0xffff0000)
//!                        (reset value depends upon VINITHI pin)
//!   Bit12        I bit   I Cache Enable (0=disable,1=enable)
//!   11:10                Reserved (SBZ)
//!   Bit9         R bit   ROM Protection bit
//!   Bit8         S bit   System Protection bit
//!   Bit7         B bit   Endianess (0=little,1=big)
//!                        (reset value depends upon BIGENDINIT pin)
//!   Bits 6:3             Reserved (SBO)
//!   Bit2         C bit   D Cache Enable (0=disable,1=enable)
//!   Bit1         A bit   Data Alignment fault enable (0=disable,1=enable)
//!   Bit0         M bit   MMU enable (0=disable,1=enable)
//!
//!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cpu_set_CP15_control_reg_1( uint32_t mask ) ;


//! Returns the ARM926 CPSR register
uint32_t cpu_get_CPSR( void ) ;


//!   Invalidate entire ARM926 data Cache
void cpu_invalidate_DCache(void);

//!   Invalidate entire ARM926 data Cache
void invalidate_dcache_by_line(uint32_t address, uint32_t length);

void cpu_mmu_enable(void);
void cpu_mmu_disable(void);
void cpu_mmu_setup(void);


/*  ARM926EJ-S 
----------------------------------------------------------------------------------------------------
31..........20|  19 .................12 | 11  10 | 9 | 8      5| 4  | 3 | 2 | 1  0 |
              |       should be zero    |   AP   |   | Domain  |    |   |   |      | 
sec_base_addr |        00000000         | 1 : 1  | 0 |         | 1  | C | B | 1  0 |
----------------------------------------------------------------------------------------------------
*/
/* C=0(Non cacheble), B=0(Non-bufferable), 
P=0, AP = 11(priveleged R/W, user R/W); Domain =0
*/
#define TT_ATTRIB_NON_CACHEABLE_NON_BUFFERABLE   0x00000C12

/* C=1(Cacheable), B=0(Non-bufferable), 
P=0, AP = 11(priveleged R/W, user R/W); Domain =0
*/
#define TT_ATTRIB_CACHEABLE_NON_BUFFERABLE       0x00000C1A

/* C=1(Cacheable), B=1(bufferable), 
P=0, AP = 11(priveleged R/W, user R/W); Domain =0
*/
#define TT_ATTRIB_CACHEABLE_BUFFERABLE           0x00000C1E
#endif //__CPU_SUPPORT_H_
////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
 
