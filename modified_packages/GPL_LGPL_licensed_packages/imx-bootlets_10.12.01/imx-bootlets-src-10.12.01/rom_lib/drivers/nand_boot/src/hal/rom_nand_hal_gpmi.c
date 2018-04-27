////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_hal
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_hal_gpmi.c
//! \brief This file provides the GPMI functions for the ROM NAND HAL.
//!
//!
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "rom_types.h"
#include "regs.h"
#include "regspinctrl.h"
#include "regsgpmi.h"
#include "regsclkctrl.h"
#include "rom_nand_hal_api.h"
#include "rom_nand_internal.h"
#include "rom_nand_hal_gpmi.h"
#include "efuse.h"
#include <itf_rom.h>

#include "regsbch.h"


#include "debug.h"

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Definitions
//
// These values are used to initialize the GPMI NAND pins.
////////////////////////////////////////////////////////////////////////////////

// Generate bitmask for use with HW_PINCTRL_MUXSELn registers
// This was copied from ata_hal.h but should be globally available
// in regspinctrl.h
#define BM_PINCTRL_MUXSEL_NAND(msb, lsb) \
    (uint32_t)(((4 << (2*((msb)-(lsb)))) - 1) << (2*((lsb)&0xF)))

///////////////////////////////////////////////////////////////////////////////
//     GPMI Timing
//////////////////////////////////////////////////////////////////////////////
//! Set the default GPMI clock for 24MHz which is ~42nsec.
#define GPMI_DEFAULT_CLK_PERIOD     (42)        // 1/24 = 41.6ns rounded to 42ns.

#define FLASH_TSU_MIN       (120)        //!< tCLS, tCS, tALS
//#define FLASH_THOLD_MIN     (10)       //!< tCLH, tCH, tALH, tDH
#define FLASH_TDS_MIN       (100)        //!< tWP, tDS
#define FLASH_TDH_MIN       (80)         //!< tWH, tREH, tDH
//#define FLASH_TDS_TDH_MIN   (50)        // tRC, tWC

#define DSAMPLE_TIME        (10)         //!< Use 10ns as safe DSAMPLE_TIME.

// Should only need 10msec (Program/Erase), and reads shouldn't be anywhere near this long..
#define DEFAULT_FLASH_BUSY_TIMEOUT  (58)    //!< (10msec / 41.6ns) / 4096 = 58
#define FLASH_BUSY_TIMEOUT          10000000  //!< Busy Timeout time in nsec. (10msec)
#define FLASH_BUSY_TIMEOUT_DIV_4096 ((FLASH_BUSY_TIMEOUT + 4095) / 4096)

// Calculate max search cycles based upon a 200MHz clock - 5ns.
#define BUSY_TIMEOUT_SEARCH_CYCLES  (FLASH_BUSY_TIMEOUT_DIV_4096/5)
// Use a 100nsec time at 200 MHz
#define NORMAL_CLOCKS_SEARCH_CYCLES  (100/5)

#define CALC_CLK(Time,GpmiPeriod)         ((Time / GpmiPeriod) + 1) //!< calculation macro.

// This calculation uses rounding
#define ROUND_CLK(Time,GpmiPeriod)      ((Time + (GpmiPeriod>>2)) / (GpmiPeriod>>1)) //!< Rounded Clock calculation.

// 24MHz = 41.7ns
// This register has a divisor of 4096.  ((FLASH_BUSY_TIMEOUT / GPMI_Period) / 4096)
#define NAND_GPMI_TIMING1(GpmiPeriod) \
   (BF_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT((CALC_CLK(FLASH_BUSY_TIMEOUT,GpmiPeriod)/4096)))

// 24MHz / (TDS+TDH) => 6MHz NAND strobe, TAS=0, TDS=45, TDH=30 nsec
//! Use the worst case conditions for all supported NANDs by default.
#define NAND_GPMI_TIMING0(AddSetup, DataSetup, DataHold) \
           (BF_GPMI_TIMING0_ADDRESS_SETUP(AddSetup) | \
            BF_GPMI_TIMING0_DATA_HOLD(DataHold) | \
            BF_GPMI_TIMING0_DATA_SETUP(DataSetup))

//! Default structure that should be more than safe for initial reads.
const NAND_Timing_t    zFailsafeTimings =
{
    FLASH_TDS_MIN,          //!< Data Setup (ns)
    FLASH_TDH_MIN,          //!< Data Hold (ns)    
    FLASH_TSU_MIN,          //!< Address Setup (ns)
    DSAMPLE_TIME            //!< DSAMPLE_TIME (ns)
};

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! \brief Enable the GPMI.
//!
//! This function configures the GPMI block using the HW registers based upon
//! the desired chip and the number of bits.
//!
//! \param[in]  bUse16BitData 0 for 8 bit, 1 for 16 bit NAND support.
//! \param[in]  u32NumberOfNANDs Number of chips to configure.
//! \param[in]  efAltCEPinConfig, use the Alternate Chip Enables.
//! \param[in]  efEnableIntPullups, internal pullups implemented on pins.
//!
//! \return SUCCESS
//! \return ERROR_ROM_NAND_DRIVER_NO_GPMI_PRESENT
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_EnableGPMI(bool bUse16BitData, 
                                   uint32_t u32NumberOfNANDs,
                                   //uint32_t efAltCEPinConfig,
                                   uint32_t efEnableIntPullups,
                                   bool bUse1_8V_Drive)
{
    // Can't boot from NAND if GPMI block is not present
    if (!(HW_GPMI_STAT_RD() & BM_GPMI_STAT_PRESENT))
        return ERROR_ROM_NAND_DRIVER_NO_GPMI_PRESENT;

    // CLKGATE = 0 and DIV = 1 (we're assuming a 24MHz XTAL for this).
    // HW_CLKCTRL_GPMICLKCTRL_WR(0x01);
    // Clock dividers are now set globally for PLL bypass in startup / setup_default_clocks()
    // The divider may also be changed by drivers (like USB) that turn on the PLL
    // HW_CLKCTRL_GPMICLKCTRL_CLR(BM_CLKCTRL_GPMICLKCTRL_CLKGATE); // ungate

    // Ungate GPMICLK. Because the gate is upstream of the divider, special
    // care must be taken to make sure the divider is set correctly. Any
    // change to HW_CLKCTRL_GPMICLKCTRL.B.DIV while the clock is gated is
    // saved to the register, but *NOT* transferred to the actual divider.
    // Clearing HW_CLKCTRL_GPMICLKCTRL.B.WAIT_PLL_LOCK serves two purposes.
    // First, it forces the divider to update because it writes the control
    // register while the clock is not gated. Second, it makes sure the update
    // completes immediately by removing the PLL locked qualifier.
    HW_CLKCTRL_GPMI.B.CLKGATE = 0;

    // Bring GPMI out of soft reset and release clock gate.
    // SoftReset needs to be set before ClockGate - can't be the same
    // instruction.

    // preparing soft reset and clock gate.
    HW_GPMI_CTRL0_CLR( BM_GPMI_CTRL0_SFTRST | BM_GPMI_CTRL0_CLKGATE);

    // Only soft reset if GPMI hasn't been enabled.
    HW_GPMI_CTRL0_SET( BM_GPMI_CTRL0_SFTRST );

    // At 24Mhz, it takes no more than 4 clocks (160 ns) Maximum for 
    // the part to reset, reading the register twice should
    // be sufficient to get 4 clks delay.
    // HW_GPMI_CTRL0_RD();
    // HW_GPMI_CTRL0_RD();
    // waiting for confirmation of soft reset
    while (!HW_GPMI_CTRL0.B.CLKGATE)
    {
        // busy wait
    }

    // Now bring out of reset and disable Clk gate.
    HW_GPMI_CTRL0_CLR( BM_GPMI_CTRL0_SFTRST | BM_GPMI_CTRL0_CLKGATE);

    // Use the failsafe timings and default 24MHz clock
    g_pNand->HalUpdateGPMITiming((NAND_Timing_t *)&zFailsafeTimings, 0);

    // Configure all of the pads that will be used for GPMI.
    g_pNand->HalGpmiConfigurePinmux(bUse16BitData, u32NumberOfNANDs, 
                                 efEnableIntPullups,  bUse1_8V_Drive);

    // Put GPMI in NAND mode, keep DEVICE reset enabled, and make certain
    // polarity is active high
    HW_GPMI_CTRL1_WR(
        BF_GPMI_CTRL1_DEV_RESET(BV_GPMI_CTRL1_DEV_RESET__DISABLED) |
        BF_GPMI_CTRL1_ATA_IRQRDY_POLARITY(BV_GPMI_CTRL1_ATA_IRQRDY_POLARITY__ACTIVEHIGH) |
        BW_GPMI_CTRL1_GPMI_MODE(BV_GPMI_CTRL1_GPMI_MODE__NAND));


    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Find NAND Timing.
//!
//! This function determines the NAND Timing parameter which is given in
//! units of GPMI cycles.  The GPMI period is used to determine how many
//! cycles fit into the NAND parameter. 
//!
//! \param[in]  u32NandTime_ns structure with Address Setup, Data Setup and Hold.
//! \param[in]  u32GpmiPeriod_ns GPMI Clock Period in nsec.
//!
//! \return Number of GPMI cycles required for this time.
//!
//! \internal
//! To view function details, see rom_nand_hal_gpmi.c.
////////////////////////////////////////////////////////////////////////////////
int rom_nand_hal_FindGpmiCycles(uint32_t u32NandTime_ns, 
                                uint32_t u32GpmiPeriod_ns,
                                uint32_t u32MaxSearchTimes)
{
    int i, iCycleTime = u32GpmiPeriod_ns;

    // Assume a maximum of 15 tests
    for (i=1;i<u32MaxSearchTimes;i++)
    {
        if (iCycleTime > u32NandTime_ns)
        {
            break;
        } else
        {
            iCycleTime += u32GpmiPeriod_ns;
        }
    }
    return i;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Setup the NAND clocks
//!
//! This function sets the GPMI NAND timing based upon the NAND timings that
//! are passed in.  This module assumes a GPMI_CLK of 24MHz if the GpmiPeriod
//! parameter is zero (41nsec period).  If the GPMI clock period is non-zero
//! it is used in the calculation of the new register values.
//!
//! \param[in]  pNANDTiming Structure with Address Setup, Data Setup and Hold.
//! \param[in]  u32GpmiPeriod_ns GPMI Clock Period in nsec.
//!
//! \return void
//!
//! \internal
//! To view function details, see rom_nand_hal_gpmi.c.
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_GpmiSetNandTiming(void * pNewNANDTiming, uint32_t u32GpmiPeriod_ns)
{
    NAND_Timing_t * pNANDTiming = (NAND_Timing_t *) pNewNANDTiming;
     
    
    // CLKGATE = 0 and DIV = 1 (we're assuming a 24MHz XTAL for this).
    // HW_CLKCTRL_GPMICLKCTRL_WR(0x01);
    // Clock dividers are now set globally for PLL bypass in startup / setup_default_clocks()
    // The divider may also be changed by drivers (like USB) that turn on the PLL
    // HW_CLKCTRL_GPMICLKCTRL_CLR(BM_CLKCTRL_GPMICLKCTRL_CLKGATE); // ungate

    // Ungate GPMICLK. Because the gate is upstream of the divider, special
    // care must be taken to make sure the divider is set correctly. Any
    // change to HW_CLKCTRL_GPMICLKCTRL.B.DIV while the clock is gated is
    // saved to the register, but *NOT* transferred to the actual divider.
    // Clearing HW_CLKCTRL_GPMICLKCTRL.B.WAIT_PLL_LOCK serves two purposes.
    // First, it forces the divider to update because it writes the control
    // register while the clock is not gated. Second, it makes sure the update
    // completes immediately by removing the PLL locked qualifier.
    HW_CLKCTRL_GPMI.B.CLKGATE = 0;

    // If u32GpmiPeriod is passed in as 0, we'll use the default 41nsec 
    // for a 24MHz clock.
    if (u32GpmiPeriod_ns == 0) u32GpmiPeriod_ns = GPMI_DEFAULT_CLK_PERIOD;

    // Set all NAND timing parameters
    // Setup pin timing parameters: ADRESS_SETUP, DATA_SETUP, and DATA_HOLD.
    // (Note that these are in units of GPMICLK cycles.)
    {
        uint32_t u32AddressSetup ;
        uint32_t u32DataSetup ;
        uint32_t u32DataHold ;
        uint32_t u32DataSampleTime ;
        uint32_t u32BusyTimeout ;

        u32AddressSetup = rom_nand_hal_FindGpmiCycles(
                   pNANDTiming->m_u8AddressSetup, u32GpmiPeriod_ns, 
                   NORMAL_CLOCKS_SEARCH_CYCLES);
        u32DataSetup = rom_nand_hal_FindGpmiCycles(
                   pNANDTiming->m_u8DataSetup, u32GpmiPeriod_ns, 
                   NORMAL_CLOCKS_SEARCH_CYCLES);
        u32DataHold = rom_nand_hal_FindGpmiCycles(
                   pNANDTiming->m_u8DataHold, u32GpmiPeriod_ns, 
                   NORMAL_CLOCKS_SEARCH_CYCLES);

        // DSAMPLE is calculated in 1/2 GPMI clock units, so use shifts to compensate.
        // This one should not round up so I subtract the cycle back off.
        u32DataSampleTime = rom_nand_hal_FindGpmiCycles(
        (pNANDTiming->m_u8DSAMPLE_TIME + (u32GpmiPeriod_ns >> 2)), (u32GpmiPeriod_ns>>1),
        NORMAL_CLOCKS_SEARCH_CYCLES) - 1;

        HW_GPMI_TIMING0_WR(NAND_GPMI_TIMING0(u32AddressSetup, u32DataSetup, u32DataHold));

        // Set DSAMPLE_TIME value
        //BW_GPMI_CTRL1_DSAMPLE_TIME(ROUND_CLK(pNANDTiming->m_u8DSAMPLE_TIME, u32GpmiPeriod));
        //BW_GPMI_CTRL1_DSAMPLE_TIME(u32DataSampleTime);

        u32BusyTimeout = rom_nand_hal_FindGpmiCycles(FLASH_BUSY_TIMEOUT_DIV_4096, 
                                                     u32GpmiPeriod_ns, 
                                                     BUSY_TIMEOUT_SEARCH_CYCLES);

        // Number of cycles / 4096.
        HW_GPMI_TIMING1_WR( BF_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT(u32BusyTimeout));
    }
    

    

    
    
#if (BRAZO_ROM_FLASH && !TGT_SIM)
    // Turn on peripheral power.
    BW_BRAZOIOCSR_PWRUP3_3V(1);
    BW_BRAZOIOCSR_PWRUP5V(1);
#endif

#if BRAZO_ROM_FLASH
    // If we are running on brazo, setup probe config for GPMI debugging
    // and provide an initial trigger for the logic analyzer.
    BW_BRAZOIOCSR_PROBECFG(GPMI_PROBECFG);
#endif

    DBG_PRINTF("Set New GPMI NAND Timing.  GPMI_PERIOD = %dns\n", u32GpmiPeriod_ns);
    DBG_PRINTF("GPMI_AS = %dns, GPMI_DH = %dns, GPMI_DS = %dns, GPMI_DSAMPLE = %dns.\n", 
               pNANDTiming->m_u8AddressSetup,
               pNANDTiming->m_u8DataHold,
               pNANDTiming->m_u8DataSetup,
               pNANDTiming->m_u8DSAMPLE_TIME);
    DBG_PRINTF("HW_GPMI_TIMING0 Register = 0x%X.\n", HW_GPMI_TIMING0_RD());
    DBG_PRINTF("HW_GPMI_TIMING1 Register = 0x%X.\n", HW_GPMI_TIMING1_RD());
    //DBG_PRINTF("GPMI_DSAMPLE_TIME = %d half-clocks.\n", 
    //          ((HW_GPMI_CTRL1_RD() & BM_GPMI_CTRL1_DSAMPLE_TIME)>>BP_GPMI_CTRL1_DSAMPLE_TIME));

}

////////////////////////////////////////////////////////////////////////////////
//! \brief Setup the Pinmux and Pad pins for the NAND.
//!
//! This function configures the pads and pinmux to support the NAND.
//!
//! \param[in]  bUse16BitData 0 for 8 bit, 1 for 16 bit NAND support.
//! \param[in]  u32NumberOfNANDs Which chip select needs to be used.
//! \param[in]  efAltCEPinConfig, use the Alternate Chip Enables.
//! \param[in]  efEnableIntPullups, enable internal pullups.
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_ConfigurePinmux(bool bUse16BitData, 
                                  uint32_t u32NumberOfNANDs,
                                  //uint32_t efAltCEPinConfig,
                                  uint32_t efEnableIntPullups,
                                  bool bUse1_8V_Drive)
{
    //uint32_t u32AltCE3 = ((efAltCEPinConfig & BM_OCOTP_ROM1_USE_ALTERNATE_GPMI_CE3) >> BP_OCOTP_ROM1_USE_ALTERNATE_GPMI_CE3);
    //uint32_t u32AltRDY3 = ((efAltCEPinConfig & BM_OCOTP_ROM1_USE_ALTERNATE_GPMI_RDY3) >> BP_OCOTP_ROM1_USE_ALTERNATE_GPMI_RDY3);

    // Wake up PINCTRL for GPMI use (i.e. bring out of reset and clkgate).
    HW_PINCTRL_CTRL_CLR( BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE);

    // By default, all NAND pins are GPIO's after reset
    // Connect NAND signals by configuring 2-bit pinmux value for each pin
    // Most NAND pins are the default peripheral = 0b00.
    // Startup already checked PINCTRL present bits
    // -----------------------------------------------------------------
    // Always power up lower 8 bits of NAND Data bus.

 //Add 1.8 V device support boot media
    if (bUse1_8V_Drive)
    {
        DBG_PRINTF("Configure as 1.8V\n");
        // Configure all GPMI lower Data Line pins as 1.8V
        HW_PINCTRL_DRIVE0_CLR(BF_PINCTRL_DRIVE0_BANK0_PIN07_V(1) | 
                              BF_PINCTRL_DRIVE0_BANK0_PIN06_V(1) |
                              BF_PINCTRL_DRIVE0_BANK0_PIN05_V(1) | 
                              BF_PINCTRL_DRIVE0_BANK0_PIN04_V(1) |
                              BF_PINCTRL_DRIVE0_BANK0_PIN03_V(1) | 
                              BF_PINCTRL_DRIVE0_BANK0_PIN02_V(1) |
                              BF_PINCTRL_DRIVE0_BANK0_PIN01_V(1) | 
                              BF_PINCTRL_DRIVE0_BANK0_PIN00_V(1) );

        // Configure all GPMI Control Required pins as 1.8V
        HW_PINCTRL_DRIVE3_CLR(BF_PINCTRL_DRIVE3_BANK0_PIN28_V(1) |  //GPMI_WRN
                              BF_PINCTRL_DRIVE3_BANK0_PIN27_V(1) |  //GPMI_RESETN 
                              BF_PINCTRL_DRIVE3_BANK0_PIN26_V(1) |  //GPMI_RDN 
                              BF_PINCTRL_DRIVE3_BANK0_PIN25_V(1) |  //GPMI_CLE 
                              BF_PINCTRL_DRIVE3_BANK0_PIN24_V(1) ); //GPMI_ALE 
  
        HW_PINCTRL_DRIVE2_CLR(BF_PINCTRL_DRIVE2_BANK0_PIN20_V(1) ); //GPMI_RDY0 
                                                            


    } 
    else
    {
        DBG_PRINTF("Configure as 3.3V\n");
        // Configure all GPMI lower Data Line pins as 3.3V
        HW_PINCTRL_DRIVE0_SET(BF_PINCTRL_DRIVE0_BANK0_PIN07_V(1) | 
                              BF_PINCTRL_DRIVE0_BANK0_PIN06_V(1) |
                              BF_PINCTRL_DRIVE0_BANK0_PIN05_V(1) | 
                              BF_PINCTRL_DRIVE0_BANK0_PIN04_V(1) |
                              BF_PINCTRL_DRIVE0_BANK0_PIN03_V(1) | 
                              BF_PINCTRL_DRIVE0_BANK0_PIN02_V(1) |
                              BF_PINCTRL_DRIVE0_BANK0_PIN01_V(1) | 
                              BF_PINCTRL_DRIVE0_BANK0_PIN00_V(1) );

         // Configure all GPMI Control Required pins as 3.3V
        HW_PINCTRL_DRIVE3_SET(BF_PINCTRL_DRIVE3_BANK0_PIN28_V(1) |  //GPMI_WRN
                              BF_PINCTRL_DRIVE3_BANK0_PIN27_V(1) |  //GPMI_RESETN 
                              BF_PINCTRL_DRIVE3_BANK0_PIN26_V(1) |  //GPMI_RDN 
                              BF_PINCTRL_DRIVE3_BANK0_PIN25_V(1) |  //GPMI_CLE 
                              BF_PINCTRL_DRIVE3_BANK0_PIN24_V(1) ); //GPMI_ALE 

        HW_PINCTRL_DRIVE2_SET(BF_PINCTRL_DRIVE2_BANK0_PIN20_V(1));  //GPMI_RDY0 	  
    }
							




	HW_PINCTRL_MUXSEL0_CLR(BM_PINCTRL_MUXSEL_NAND(7, 0));   // BANK0[7:0] = 0
    // If 16 bit NAND, power up upper 8 bits of NAND Data bus.



    // Select the ALE, CLE, WRN, and RDN pins of NAND.
    HW_PINCTRL_MUXSEL1_CLR (
                            //BF_PINCTRL_MUXSEL1_BANK0_PIN27(0x3) |    // GPMI_RDY3 
                            //BF_PINCTRL_MUXSEL1_BANK0_PIN26(0x3) |   // GPMI_RDY2
                            //BF_PINCTRL_MUXSEL1_BANK0_PIN25(0x3) |   // GPMI_RDY1
                            BF_PINCTRL_MUXSEL1_BANK0_PIN20(0x3) |     // GPMI_RDY0
                            //BF_PINCTRL_MUXSEL1_BANK0_PIN19(0x3) |   // GPMI_CE3N 
                            //BF_PINCTRL_MUXSEL1_BANK0_PIN18(0x3) |   // GPMI_CE2N 
                            //BF_PINCTRL_MUXSEL1_BANK0_PIN17(0x3) |   // GPMI_CE1N
                            BF_PINCTRL_MUXSEL1_BANK0_PIN16(0x3) );    // GPMI_CE0N



    HW_PINCTRL_MUXSEL1_CLR (BF_PINCTRL_MUXSEL1_BANK0_PIN25(0x3) |    // GPMI_WRN  
                            BF_PINCTRL_MUXSEL1_BANK0_PIN28(0x3) |   // GPMI_RESETN 
                            BF_PINCTRL_MUXSEL1_BANK0_PIN24(0x3) |   // GPMI_RDN 
                            BF_PINCTRL_MUXSEL1_BANK0_PIN27(0x3) |   // GPMI_CLE
                            BF_PINCTRL_MUXSEL1_BANK0_PIN26(0x3) );  // GPMI_ALE 
							
    // Set the pin drive for the RDN and WRN to 8mA.
    BW_PINCTRL_DRIVE3_BANK0_PIN25_MA(0x1); 
    BW_PINCTRL_DRIVE3_BANK0_PIN24_MA(0x1); // http://jira/browse/ROM-31

    if (bUse1_8V_Drive)
         HW_PINCTRL_DRIVE2_CLR(BF_PINCTRL_DRIVE2_BANK0_PIN16_V(1));  //GPMI_CE0
    else
         HW_PINCTRL_DRIVE2_SET(BF_PINCTRL_DRIVE2_BANK0_PIN16_V(1));  //GPMI_CE0

    if (u32NumberOfNANDs > 1)
    {
        // Power up Ready Busy for NAND1
        HW_PINCTRL_MUXSEL1_CLR(BF_PINCTRL_MUXSEL1_BANK0_PIN21(0x3)); //GPMI_RDY1
        // Power up CE1 by setting bit field to b00.
        HW_PINCTRL_MUXSEL1_CLR(BF_PINCTRL_MUXSEL1_BANK0_PIN17(0x3)); //GPMI_CE1

        if (bUse1_8V_Drive)
        	{
	     HW_PINCTRL_DRIVE2_CLR(BF_PINCTRL_DRIVE2_BANK0_PIN17_V(1));  //GPMI_CE1
            HW_PINCTRL_DRIVE2_CLR(BF_PINCTRL_DRIVE2_BANK0_PIN21_V(1)); //ready1
        	}
	else
		{
            HW_PINCTRL_DRIVE2_SET(BF_PINCTRL_DRIVE2_BANK0_PIN17_V(1));  //GPMI_CE1
            HW_PINCTRL_DRIVE2_SET(BF_PINCTRL_DRIVE2_BANK0_PIN21_V(1)); //ready1
		}
    }
    // CE2 and CE3 aren't relevant for ROM but are for SDK.



    if ((u32NumberOfNANDs > 2)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        // primary, set bits to b00
        HW_PINCTRL_MUXSEL1_CLR(BF_PINCTRL_MUXSEL1_BANK0_PIN18(0x3)); //GPMI_CE2
       
        // primary, set bits to b00
        HW_PINCTRL_MUXSEL1_CLR(BF_PINCTRL_MUXSEL1_BANK0_PIN22(0x3)); //GPMI_RDY2

        if (bUse1_8V_Drive)
        {
            HW_PINCTRL_DRIVE2_CLR(BF_PINCTRL_DRIVE2_BANK0_PIN18_V(1));  //GPMI_CE2
            HW_PINCTRL_DRIVE2_CLR(BF_PINCTRL_DRIVE2_BANK0_PIN22_V(1));//ready2
        }
        else
        	{
            HW_PINCTRL_DRIVE2_SET(BF_PINCTRL_DRIVE2_BANK0_PIN18_V(1));  //GPMI_CE2
            HW_PINCTRL_DRIVE2_SET(BF_PINCTRL_DRIVE2_BANK0_PIN22_V(1)); 

		}
    	}
    if ((u32NumberOfNANDs > 3)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        
        // set b0
        HW_PINCTRL_MUXSEL1_CLR(BF_PINCTRL_MUXSEL1_BANK0_PIN19(0x3)); //GPMI_CE3
       
        // set b00
        HW_PINCTRL_MUXSEL1_CLR(BF_PINCTRL_MUXSEL1_BANK0_PIN23(0x3)); //GPMI_RDY3
            
        if (bUse1_8V_Drive)
        	{
	     HW_PINCTRL_DRIVE2_CLR(BF_PINCTRL_DRIVE2_BANK0_PIN19_V(1));  //GPMI_CE3
            HW_PINCTRL_DRIVE2_CLR(BF_PINCTRL_DRIVE2_BANK0_PIN23_V(1)); 
        	}
	else
		{
            HW_PINCTRL_DRIVE2_SET(BF_PINCTRL_DRIVE2_BANK0_PIN19_V(1));  //GPMI_CE3
            HW_PINCTRL_DRIVE2_SET(BF_PINCTRL_DRIVE2_BANK0_PIN23_V(1)); 
	        }
    	}
    if ((u32NumberOfNANDs > 4)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {      
        // set b01
        HW_PINCTRL_MUXSEL8_CLR(BF_PINCTRL_MUXSEL8_BANK4_PIN00(0x3)); //GPMI_CE4
        HW_PINCTRL_MUXSEL8_SET(BF_PINCTRL_MUXSEL8_BANK4_PIN00(0x1));

        // set b01
        HW_PINCTRL_MUXSEL8_CLR(BF_PINCTRL_MUXSEL8_BANK4_PIN04(0x3)); //GPMI_RDY4
        HW_PINCTRL_MUXSEL8_SET(BF_PINCTRL_MUXSEL8_BANK4_PIN04(0x1));


        if (bUse1_8V_Drive)
        {
            HW_PINCTRL_DRIVE16_CLR(BF_PINCTRL_DRIVE16_BANK4_PIN00_V(1));  //GPMI_CE4
            HW_PINCTRL_DRIVE16_CLR(BF_PINCTRL_DRIVE16_BANK4_PIN04_V(1));  //GPMI_RDY4
        }		
		else
	    {
            HW_PINCTRL_DRIVE16_SET(BF_PINCTRL_DRIVE16_BANK4_PIN00_V(1));  //GPMI_CE4
            HW_PINCTRL_DRIVE16_SET(BF_PINCTRL_DRIVE16_BANK4_PIN04_V(1));

	    }

    }	
    if ((u32NumberOfNANDs > 5) &&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        // set b01
        HW_PINCTRL_MUXSEL8_CLR(BF_PINCTRL_MUXSEL8_BANK4_PIN01(0x3)); //GPMI_CE5  
        HW_PINCTRL_MUXSEL8_SET(BF_PINCTRL_MUXSEL8_BANK4_PIN01(0x1));
        // set b01
        HW_PINCTRL_MUXSEL8_CLR(BF_PINCTRL_MUXSEL8_BANK4_PIN06(0x3)); //GPMI_RDY5
        HW_PINCTRL_MUXSEL8_SET(BF_PINCTRL_MUXSEL8_BANK4_PIN06(0x1));

        if (bUse1_8V_Drive)
	    {
            HW_PINCTRL_DRIVE16_CLR(BF_PINCTRL_DRIVE16_BANK4_PIN01_V(1));  //GPMI_CE5
            HW_PINCTRL_DRIVE16_CLR(BF_PINCTRL_DRIVE16_BANK4_PIN06_V(1));  //GPMI_RDY5
	    }		
        else
	    {   
	     HW_PINCTRL_DRIVE16_SET(BF_PINCTRL_DRIVE16_BANK4_PIN01_V(1));  //GPMI_CE5
            HW_PINCTRL_DRIVE16_SET(BF_PINCTRL_DRIVE16_BANK4_PIN06_V(1));
	    }
    }


    if ((u32NumberOfNANDs > 6)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        // set b01
        HW_PINCTRL_MUXSEL8_CLR(BF_PINCTRL_MUXSEL8_BANK4_PIN02(0x3)); //GPMI_CE6
        HW_PINCTRL_MUXSEL8_SET(BF_PINCTRL_MUXSEL8_BANK4_PIN02(0x1));

        // set b01
        HW_PINCTRL_MUXSEL8_CLR(BF_PINCTRL_MUXSEL8_BANK4_PIN07(0x3)); //GPMI_RDY6
        HW_PINCTRL_MUXSEL8_SET(BF_PINCTRL_MUXSEL8_BANK4_PIN07(0x1));

        if (bUse1_8V_Drive)
	    {   
            HW_PINCTRL_DRIVE16_CLR(BF_PINCTRL_DRIVE16_BANK4_PIN02_V(1));  //GPMI_CE6
            HW_PINCTRL_DRIVE16_CLR(BF_PINCTRL_DRIVE16_BANK4_PIN07_V(1));  //GPMI_RDY6
	    }		
        else
	    {   
	     HW_PINCTRL_DRIVE16_SET(BF_PINCTRL_DRIVE16_BANK4_PIN02_V(1));  //GPMI_CE6
            HW_PINCTRL_DRIVE16_SET(BF_PINCTRL_DRIVE16_BANK4_PIN07_V(1));
        }

    }

    if ((u32NumberOfNANDs > 7)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        // set b01
        HW_PINCTRL_MUXSEL8_CLR(BF_PINCTRL_MUXSEL8_BANK4_PIN03(0x3)); //GPMI_CE7
        HW_PINCTRL_MUXSEL8_SET(BF_PINCTRL_MUXSEL8_BANK4_PIN03(0x1));

        // set b01
        HW_PINCTRL_MUXSEL8_CLR(BF_PINCTRL_MUXSEL8_BANK4_PIN08(0x3)); //GPMI_RDY7
        HW_PINCTRL_MUXSEL8_SET(BF_PINCTRL_MUXSEL8_BANK4_PIN08(0x1));

        if (bUse1_8V_Drive)
	    {   
            HW_PINCTRL_DRIVE16_CLR(BF_PINCTRL_DRIVE16_BANK4_PIN03_V(1));  //GPMI_CE7
            HW_PINCTRL_DRIVE17_CLR(BF_PINCTRL_DRIVE17_BANK4_PIN08_V(1));  //GPMI_RDY7
	    }		
        else
	    {   
	     HW_PINCTRL_DRIVE16_SET(BF_PINCTRL_DRIVE16_BANK4_PIN03_V(1));  //GPMI_CE7
            HW_PINCTRL_DRIVE17_SET(BF_PINCTRL_DRIVE17_BANK4_PIN08_V(1));
	    }
    }


// enables internal pullups
    if(efEnableIntPullups & BM_OCOTP_ROM1_ENABLE_NAND0_CE_RDY_PULLUP)
    {
        HW_PINCTRL_PULL0_SET(BF_PINCTRL_PULL0_BANK0_PIN16(1)); //GPMI_CE0
        HW_PINCTRL_PULL0_SET(BF_PINCTRL_PULL0_BANK0_PIN20(1)); //GPMI_RDY0
    }
    if(efEnableIntPullups & BM_OCOTP_ROM1_ENABLE_NAND1_CE_RDY_PULLUP)
    {
        HW_PINCTRL_PULL0_SET(BF_PINCTRL_PULL0_BANK0_PIN17(1)); //GPMI_CE1
        HW_PINCTRL_PULL0_SET(BF_PINCTRL_PULL0_BANK0_PIN21(1)); //GPMI_RDY1
    }
    if((efEnableIntPullups & BM_OCOTP_ROM1_ENABLE_NAND2_CE_RDY_PULLUP)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        HW_PINCTRL_PULL0_SET(BF_PINCTRL_PULL0_BANK0_PIN18(1)); //GPMI_CE2
        HW_PINCTRL_PULL0_SET(BF_PINCTRL_PULL0_BANK0_PIN22(1)); //GPMI_RDY2
    }
    if((efEnableIntPullups & BM_OCOTP_ROM1_ENABLE_NAND3_CE_RDY_PULLUP)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        HW_PINCTRL_PULL0_SET(BF_PINCTRL_PULL0_BANK0_PIN19(1)); //GPMI_CE3 
        HW_PINCTRL_PULL0_SET(BF_PINCTRL_PULL0_BANK0_PIN23(1)); //GPMI_RDY3 
    }


    if((efEnableIntPullups & BM_OCOTP_ROM1_ENABLE_NAND4_CE_RDY_PULLUP)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        HW_PINCTRL_PULL4_SET(BF_PINCTRL_PULL4_BANK4_PIN00(1)); //GPMI_CE4 
        HW_PINCTRL_PULL4_SET(BF_PINCTRL_PULL4_BANK4_PIN04(1)); //GPMI_RDY4 
    }

    if((efEnableIntPullups & BM_OCOTP_ROM1_ENABLE_NAND5_CE_RDY_PULLUP)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        HW_PINCTRL_PULL4_SET(BF_PINCTRL_PULL4_BANK4_PIN01(1)); //GPMI_CE5 
        HW_PINCTRL_PULL4_SET(BF_PINCTRL_PULL4_BANK4_PIN06(1)); //GPMI_RDY5 
    }

	if((efEnableIntPullups & BM_OCOTP_ROM1_ENABLE_NAND6_CE_RDY_PULLUP)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        HW_PINCTRL_PULL4_SET(BF_PINCTRL_PULL4_BANK4_PIN02(1)); //GPMI_CE6
        HW_PINCTRL_PULL4_SET(BF_PINCTRL_PULL4_BANK4_PIN07(1)); //GPMI_RDY6 
    }

	if((efEnableIntPullups & BM_OCOTP_ROM1_ENABLE_NAND7_CE_RDY_PULLUP)&&((HW_OCOTP_HWCAPn_RD(3) & BM_OCOTP_HWCAP3_PACKAGE_TYPE) != BV_OCOTP_HWCAP3_PACKAGE_TYPE__QFP208))
    {
        HW_PINCTRL_PULL4_SET(BF_PINCTRL_PULL4_BANK4_PIN03(1)); //GPMI_CE7
        HW_PINCTRL_PULL4_SET(BF_PINCTRL_PULL4_BANK4_PIN08(1)); //GPMI_RDY7 
    }


//#ifdef VERBOSE_PRINTF    
    DBG_PRINTF("PinCtrl Mux0 0x%X\n", HW_PINCTRL_MUXSEL0_RD());
    DBG_PRINTF("PinCtrl Mux1 0x%X\n", HW_PINCTRL_MUXSEL1_RD());
    DBG_PRINTF("PinCtrl Mux3 0x%X\n", HW_PINCTRL_MUXSEL3_RD());
    DBG_PRINTF("PinCtrl Mux4 0x%X\n", HW_PINCTRL_MUXSEL4_RD());
    DBG_PRINTF("PinCtrl Mux5 0x%X\n", HW_PINCTRL_MUXSEL5_RD());
    DBG_PRINTF("PinCtrl Drive0 0x%X\n", HW_PINCTRL_DRIVE0_RD());
    DBG_PRINTF("PinCtrl Drive1 0x%X\n", HW_PINCTRL_DRIVE1_RD());
    DBG_PRINTF("PinCtrl Drive2 0x%X\n", HW_PINCTRL_DRIVE2_RD());
    DBG_PRINTF("PinCtrl Drive3 0x%X\n", HW_PINCTRL_DRIVE3_RD());
    DBG_PRINTF("PinCtrl Pull0 0x%X\n", HW_PINCTRL_PULL0_RD());
    DBG_PRINTF("PinCtrl Pull1 0x%X\n", HW_PINCTRL_PULL1_RD());
    DBG_PRINTF("PinCtrl Pull2 0x%X\n", HW_PINCTRL_PULL2_RD());
    DBG_PRINTF("PinCtrl Pull3 0x%X\n", HW_PINCTRL_PULL3_RD());
//#endif

}


////////////////////////////////////////////////////////////////////////////////
//! \brief Disable the GPMI.
//!
//! This function deconfigures the GPMI block using the boolean flag used to
//! track whether the block has been configured.
//!
//! \param[in]  void
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
// 
void rom_nand_hal_DisableGPMI()
{
#if (BRAZO_ROM_FLASH && !TGT_SIM)
    // Turn off peripheral power.
    BW_BRAZOIOCSR_PWRUP5V(0);
    BW_BRAZOIOCSR_PWRUP3_3V(0);
#endif

    // Gate clocks to GPMI.
    BW_GPMI_CTRL0_CLKGATE(1);

}

///////////////////////////////////////////////////////////////////////////////
// Set or reset the Write Protect Pin (Renesas Reset).
// To view function documentation, see rom_nand_hal_gpmi.h.
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_SetClearWriteProtect(uint32_t bDisableEnable)
{
    if (bDisableEnable)
    {
        HW_GPMI_CTRL1_SET(BF_GPMI_CTRL1_DEV_RESET(bDisableEnable));
    } else
    {
        HW_GPMI_CTRL1_CLR(BF_GPMI_CTRL1_DEV_RESET(bDisableEnable));
    }
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Reset the BCH block.
//!
//! This function resets the BCH block in preparation for ECC operation.
//!
//! \param[in]  none
//!
//! \return none.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_ResetBCH(void)
{

    // Bring out of reset and disable Clk gate.
    // Soft Reset the BCH block
    BW_BCH_CTRL_SFTRST(BV_BCH_CTRL_SFTRST__RESET);
    BW_BCH_CTRL_SFTRST(BV_BCH_CTRL_SFTRST__RUN);
    // Now bring out of reset and disable Clk gate.
    BW_BCH_CTRL_CLKGATE(BV_BCH_CTRL_CLKGATE__NO_CLKS);
    BW_BCH_CTRL_CLKGATE(BV_BCH_CTRL_CLKGATE__RUN);

    // set complete irq enable bit.
    //HW_BCH_CTRL_SET( BM_BCH_CTRL_COMPLETE_IRQ_EN);  
    // Set the AHBM soft reset.
    //BW_BCH_CTRL_AHBM_SFTRST(BV_BCH_CTRL_AHBM_SFTRST__RESET);
    //BW_BCH_CTRL_AHBM_SFTRST(BV_BCH_CTRL_AHBM_SFTRST__RUN);
    // Set the AHBM soft reset.
    //BW_ECC8_CTRL_AHBM_SFTRST(BV_ECC8_CTRL_AHBM_SFTRST__RESET);
    //BW_ECC8_CTRL_AHBM_SFTRST(BV_ECC8_CTRL_AHBM_SFTRST__RUN);

    //BW_BCH_CTRL_CLKGATE(BV_BCH_CTRL_CLKGATE__RUN);
//#ifdef VERBOSE_PRINTF
    DBG_PRINTF("ECC CTRL = 0x%X\n", HW_BCH_CTRL_RD());
//#endif
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Configure BCH ECC registers and set the ecc.
//!
//! This function sets the type of ecc to use and configures BCH ECC registers
//!
//! \param[in]  pNANDEccParams
//!
//! \param[in]  u32NumNandsInUse
//!
//! \return none.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_UpdateECCParams(NAND_ECC_Params_t *pNANDEccParams, uint32_t u32NumNandsInUse)
{
    // set the BCH mode on/off.
    BW_GPMI_CTRL1_BCH_MODE(1);

    BW_BCH_MODE_ERASE_THRESHOLD(pNANDEccParams->m_u32EraseThreshold);
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Enable the BCH block.
//!
//! This function enables the BCH block.
//!
//! \param[in]  none
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_EnableBCH(void)
{
    // Remove the clock gate.
    HW_BCH_CTRL_CLR( BM_BCH_CTRL_CLKGATE);
    // Remove Soft Reset from the BCH block
    HW_BCH_CTRL_CLR( BM_BCH_CTRL_SFTRST );
    // Remove Soft Reset from the BCH block
    //HW_BCH_CTRL_CLR( BM_BCH_CTRL_AHBM_SFTRST );
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Disable the BCH block.
//!
//! This function disables the BCH block.
//!
//! \param[in]  none
//!
//! \return none.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_DisableBCH(void)
{
    // Clear complete irq enable bit.
//    HW_BCH_CTRL_CLR( BM_BCH_CTRL_COMPLETE_IRQ_EN);  
    // Gate the BCH block
    HW_BCH_CTRL_SET( BM_BCH_CTRL_CLKGATE );
}
////////////////////////////////////////////////////////////////////////////////
//! \brief Find the number of ECC corrections in each payload.
//!
//! This function returns the maximum number of error found in a payload.  Each
//! payload is tested and the maximum corrections are returned.
//!
//! \param[in]  u32NumberOfCorrections Size of the ECC (max corrections)
//!
//! \return Maximum Number of ECC corrections.
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
uint32_t rom_nand_hal_FindEccErrors(uint32_t u32NumberOfCorrections)
{
    uint32_t u32EccErrors = 0;
    uint32_t uEccStatus;

    // Spin until ECC Complete Interrupt triggers.
    while(true)
    {
        if(HW_BCH_CTRL_RD() & BM_BCH_CTRL_COMPLETE_IRQ)
        {
            DBG_PRINTF("BCH completed interrupt\n");
            break;
        }
        if(HW_BCH_CTRL_RD() & BM_BCH_CTRL_BM_ERROR_IRQ)
        {
            DBG_PRINTF("BCH: AHB Bus interface Error\n");
            break;
        }
    }

    // Now read the ECC status.
    uEccStatus = HW_BCH_STATUS0_RD();
    DBG_PRINTF("BCH ECC Status0 = 0x%X\n", uEccStatus);

    if( uEccStatus != SUCCESS )
    {
        //:dig later u32EccErrors = -1; //at least 1 error is corrected
        u32EccErrors = uEccStatus;
    }

    // moved this to rom_nand_hal_CheckECCStatus
    // Clear the ECC Complete IRQ.
    //BW_BCH_CTRL_COMPLETE_IRQ(0);

    return u32EccErrors;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Check the ECC decode capability
//!
//! This function checks the ECC to determine if the correct ECC block is
//! available.
//!
//! \param[in]  u32EccSize  4 or 8 depending upon number of ECC corrections.
//!
//! \return SUCCESS
//! \return ERROR_ROM_NAND_DRIVER_NO_ECC_PRESENT
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
RtStatus_t rom_nand_hal_CheckECCDecodeCapability(uint32_t u32EccSize)
{
    // only 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20 are valid BCH ecc sizes
    if((u32EccSize > 20) || (u32EccSize % 2))
    {
        return ERROR_ROM_NAND_DRIVER_NO_ECC_PRESENT;
    }

    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Set Maximum GPMI DMA timeout.
//!
//! This function sets the maximum DMA timeout value.  If u32Timeout is 0, use
//! default value of 12msec.
//!
//! \param[in]  u32Timeout  (# of GPMI cycles / 4096)
//!
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_SetGPMITimeout(uint32_t u32Timeout)
{

    // If zero, use default timeout.
    if (!u32Timeout)
    {
        u32Timeout = DEFAULT_FLASH_BUSY_TIMEOUT;
    }

    HW_GPMI_TIMING1_WR(BF_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT(u32Timeout));

#ifdef VERBOSE_PRINTF
    DBG_PRINTF("GPMI Timeout set to 0x%X\n", 
              (HW_GPMI_TIMING1_RD() >> BP_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT));
#endif
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Clear the ECC Complete IRQ flag.
//!
//! This function clears the ECC Complete flag.  This should be done before each
//! transaction that uses the ECC.
//!
//!
//! \return void
//!
//! \internal
////////////////////////////////////////////////////////////////////////////////
void rom_nand_hal_ClearEccCompleteFlag(void)
{
    HW_BCH_CTRL_CLR(BM_BCH_CTRL_COMPLETE_IRQ);
}

//! @}
