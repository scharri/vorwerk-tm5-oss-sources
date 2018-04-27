////////////////////////////////////////////////////////////////////////////////
//
// Filename: emi_common.c
//
// Description: Common Types, Constants, Macros, Functions specific to EMI
//              test cases.
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) SigmaTel, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary & Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may compromise trade secrets of SigmaTel, Inc.
// or its associates, and any unauthorized use thereof is prohibited.
//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "regsclkctrl.h"
#include "regsdigctl.h"
//#include "regsemi.h"
#include "regspinctrl.h"
#include "regssimmemsel.h"
#include "regsdram.h"

#include "rom_types.h"
#include "debug.h"

//#include "test_util.h"
#include "emi_common_init.h"
#include "regsclkctrl.h"
#include "regsdigctl.h"
#include "regsdram.h"
#include "regslradc.h"
#include "regspinctrl.h"
#include "regspower.h"
#include "regssimmemsel.h"
#include "regsusbphy.h"

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define EMI_START_ADDR      (0x40000000)

// Function Specification ******************************************************
//!
//! \brief Change EMI_CLK Frequency
//!
//! This function programs the EMI_CLK to the specified frequency by selecting
//! the appropriate clock source (XTAL or PLL) and divider settings.
//!
//! \param[in] emi_freq - EMI_CLK Frequency
//!
// End Function Specification **************************************************

void CLKCTRL_ChgEmiClk(TEmiClkFreq emi_freq, uint32_t dcc_resync_enable)
{
    int use_xtal_src = 1;

    uint32_t new_xtal_int_div = 1;

    uint32_t cur_pll_int_div = (HW_CLKCTRL_EMI_RD() & BM_CLKCTRL_EMI_DIV_EMI) >>
                          BP_CLKCTRL_EMI_DIV_EMI;
    uint32_t new_pll_int_div = 4;

    uint32_t cur_pll_frac_div = (HW_CLKCTRL_FRAC0_RD() & BM_CLKCTRL_FRAC0_EMIFRAC) >>
                           BP_CLKCTRL_FRAC0_EMIFRAC;
    uint32_t new_pll_frac_div = 18;

    uint32_t timeout_start;

    /////////////////////////////////////
    // Information about emi_clk
    // Use PLL 
    //   1. format : emi_clk = (480*18/pll_frac_div)  / pll_int_div 
    //   pll_frac_div = HW_CLKCTRL_FRAC0.EMIFRAC (range: 18 ~ 35)
    //   pll_int_div = HW_CLKCTRL_EMI.DIV_EMI (range: 1 ~ 64) 
    //   
    //   2. Max - Min (4MHz ~ 480MHz)
    //   Max freq : emi_clk(max) = 480MHz (frac_div = int_div = 1)
    //   Min freq : emi_clk(min) = 4MHz  (frac_div=35, int_div=64;)
    //   Note:
    //    When only take frac divider, the min freq is 247MHz, the max is 480Mhz
    //    When only take int divider , the min freq is 135MHz, the max is 480Mhz
    //  
    //Use XTL:
    //   1. format: emi_clk = 24 / xtal_int_div
    //   xtal_int_div = HW_CLKCTRL_EMI.DIV_XTAL (range : 1 ~32)
    //   
    //   2. Max - Min (750KHz ~ 24MHz)
    //   Max freq : emi_clk(max) = 24MHz (xtal_int_div = 1)
    //   Min freq : emi_clk(min) = 750KHz  (xtal_int_div =32)
    //                
    ///////////////////////////////////////
    
    switch (emi_freq)
    {
        case EMI_CLK_6MHz:
            use_xtal_src = 1;
            new_xtal_int_div = 4;
            break;

        case EMI_CLK_24MHz:
            use_xtal_src = 1;
            new_xtal_int_div = 1;
            break;

        case EMI_CLK_48MHz:   //Use PLL
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 10;
            break;

        case EMI_CLK_60MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 8;
            break;

        case EMI_CLK_80MHz:     
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 6;
            break;

        case EMI_CLK_96MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 5;
            break;

        case EMI_CLK_120MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 4;
            break;

        case EMI_CLK_133MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 22;
            new_pll_int_div = 3;
            break;
        case EMI_CLK_150MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 29;   
            new_pll_int_div = 2;     
            break;
        case EMI_CLK_166MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 26;
            new_pll_int_div = 2;
            break;	    
        case EMI_CLK_200MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 22;
            new_pll_int_div = 2;
            break;
    }

    if (use_xtal_src)
    {
        // Write the XTAL EMI clock divider
        HW_CLKCTRL_EMI_WR( BF_CLKCTRL_EMI_CLKGATE(0)
	                 | BF_CLKCTRL_EMI_DIV_XTAL(new_xtal_int_div) 
		         | BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div) );

        // Wait until the BUSY_REF_XTAL clears or a timeout of 100 usec occurs
	// HW_DIGCTL_MICROSECONDS: This register maintains a 32-bit counter that increments at a 1/1000_000 second rate.The 32-bit value wraps in less than two hours.
        timeout_start = HW_DIGCTL_MICROSECONDS_RD();       
        while ( HW_CLKCTRL_EMI_RD() & BM_CLKCTRL_EMI_BUSY_REF_XTAL ) { 
            if ( ((uint32_t)HW_DIGCTL_MICROSECONDS_RD() - timeout_start) >= 100) {
                break;
            }
        }
        // Set the PLL EMI-bypass bit so that the XTAL clock takes over
        HW_CLKCTRL_CLKSEQ_SET(BM_CLKCTRL_CLKSEQ_BYPASS_EMI);
    }
    else
    {
        // The fractional divider and integer divider must be written in such
        // an order to guarantee that when going from a lower frequency to a
        // higher frequency that any intermediate frequencies do not exceed
        // the final frequency. For this reason, we must make sure to check
        // the current divider values with the new divider values and write
        // them in the correct order.
        if (new_pll_frac_div > cur_pll_frac_div) {

            // Read the PLL fractional divider
            uint32_t frac_val = (HW_CLKCTRL_FRAC0_RD() & ~BM_CLKCTRL_FRAC0_EMIFRAC) |
                BF_CLKCTRL_FRAC0_EMIFRAC(new_pll_frac_div);

            // Write the PLL fractional divider
            HW_CLKCTRL_FRAC0_WR(frac_val);
            cur_pll_frac_div = new_pll_frac_div;
        }

        if (new_pll_int_div > cur_pll_int_div) {
            // Wait until BUSY_REF_EMI clears or a timeout of 100 usec occurs
            // Write the PLL EMI clock divider
            HW_CLKCTRL_EMI_WR(BF_CLKCTRL_EMI_CLKGATE(0) 
		           | BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div)
			   | BF_CLKCTRL_EMI_DIV_XTAL(new_xtal_int_div) );

            cur_pll_int_div = new_pll_int_div;

            timeout_start = HW_DIGCTL_MICROSECONDS_RD();
            while (HW_CLKCTRL_EMI_RD() & BM_CLKCTRL_EMI_BUSY_REF_EMI ) {
                if ( ((uint32_t)HW_DIGCTL_MICROSECONDS_RD() - timeout_start) >= 100) {
                    break;
                }
            }
        }

        // process the case: "new_pll_frac_div < = cur_pll_frac_div"
        if (new_pll_frac_div != cur_pll_frac_div) {
            // Read the PLL fractional divider
            uint32_t frac_val = (HW_CLKCTRL_FRAC0_RD() & ~BM_CLKCTRL_FRAC0_EMIFRAC) |
                BF_CLKCTRL_FRAC0_EMIFRAC(new_pll_frac_div);
            // Write the PLL fractional divider
            HW_CLKCTRL_FRAC0_WR(frac_val);
        }
        // process the case: "new_pll_int_div < = cur_pll_int_div"
        if (new_pll_int_div != cur_pll_int_div) {
            // Write the PLL EMI clock divider
            HW_CLKCTRL_EMI_WR(
                    BF_CLKCTRL_EMI_CLKGATE(0) |
                    BF_CLKCTRL_EMI_DIV_XTAL(new_xtal_int_div) |
                    BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div));

            // Wait until BUSY_REF_EMI clears or a timeout of 100 usec occurs
            timeout_start = HW_DIGCTL_MICROSECONDS_RD();

            while (HW_CLKCTRL_EMI_RD() &BM_CLKCTRL_EMI_BUSY_REF_EMI ) {
                if ( ((uint32_t)HW_DIGCTL_MICROSECONDS_RD() - timeout_start) >= 100) {
                    break;
                }
            }
        }
        // Clear the PLL EMI-bypass bit so that the PLL clock takes over
        HW_CLKCTRL_CLKSEQ_CLR(BM_CLKCTRL_CLKSEQ_BYPASS_EMI);
    }
    
}

// Function Specification ******************************************************
//!
//! \brief Turn On PLL to specified Frequency
//!
//! This function powers up the PLL to 480MHz.
//!
// End Function Specification **************************************************

void CLKCTRL_TurnOnPll()
{
    // Power up PLL to specified frequency
    HW_CLKCTRL_PLL0CTRL0_WR(BF_CLKCTRL_PLL0CTRL0_POWER(1));
    // Wait 10 usec (see T-spec)
    DIGCTL_KillTimeUsec(10);
    // Wait for the PLL to lock.
    while (!HW_CLKCTRL_PLL0CTRL1.B.LOCK)
    {
        // busy wait
    }
    // Turn on fractional divider (at default 18 setting = (18)/18 = 1 divider)
    HW_CLKCTRL_FRAC0_CLR(BM_CLKCTRL_FRAC0_CLKGATEEMI | BM_CLKCTRL_FRAC0_CLKGATECPU);
}

// Function Specification ******************************************************
//!
//! \brief Kill the specified Number of Microseconds
//!
//! This function waits until the specified number of microseconds has elapsed.
//!
//! \param[in] microseconds - Number of microseconds to wait before returning.
//!
// End Function Specification **************************************************

void DIGCTL_KillTimeUsec(uint32_t microseconds)
{
    uint32_t timeout_start = HW_DIGCTL_MICROSECONDS_RD();

    // Loop waiting until specified time has expired
    while ( ((uint32_t)HW_DIGCTL_MICROSECONDS_RD() - timeout_start) < microseconds)
    {}
}

//* Function Specification *****************************************************
//!
//! \brief Initialize Pin Mux to enable all EMI pins
//!
//! This function sets each of the Pin Mux select registers to enable all the
//! EMI associated pins. In addition, it also set the voltage level and drive
//! strength as specified for each of the EMI pins. This routine only sets/clears
//! the bits necessary for those pins associated with EMI. No other Pin Mux
//! settings are changed.
//!
//! \param[in] pin_voltage    - Pin voltage assigned to each EMI pin.
//! \param[in] pin_drive_addr - Pin drive strength (mA) assigned to EMI address pins.
//! \param[in] pin_drive_ctrl - Pin drive strength (mA) assigned to EMI control pins.
//! \param[in] pin_drive_clk  - Pin drive strength (mA) assigned to EMI clock pins.
//! \param[in] pin_drive_data_slice_0 - Pin drive strength (mA) assigned to EMI data pins.
//! \param[in] pin_drive_data_slice_1 - Pin drive strength (mA) assigned to EMI data pins.
//
//!
//******************************************************************************
void PINCTRL_TurnOnEmiPins(
        TPinVoltage pin_voltage,
        TPinDrive pin_drive_addr,
        TPinDrive pin_drive_ctrl,
        TPinDrive pin_drive_clk,
        TPinDrive pin_drive_data_slice_0,
        TPinDrive pin_drive_data_slice_1)
{  

///////////////////////////////////////////////////////////////
   // must clear the HW_PINCTRL_CTRL_CLR to enable pinmux block clock --APB clock
   HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_CLKGATE | BM_PINCTRL_CTRL_SFTRST);
    //-------------------------------------------------------------------------    
    // First, set the voltage and drive strength of these pins as specified.
    // Second, set the pinmux value to 0x0 to enable the EMI connection.
    //-------------------------------------------------------------------------   
    /////////////// Set drive strength ///////////////
    HW_PINCTRL_EMI_DS_CTRL_SET(
            BF_PINCTRL_EMI_DS_CTRL_ADDRESS_MA(pin_drive_addr) |
            BF_PINCTRL_EMI_DS_CTRL_CONTROL_MA(pin_drive_ctrl) |
	    BF_PINCTRL_EMI_DS_CTRL_DUALPAD_MA(pin_drive_clk) |
            BF_PINCTRL_EMI_DS_CTRL_SLICE0_MA(pin_drive_data_slice_0) |
	    BF_PINCTRL_EMI_DS_CTRL_SLICE1_MA(pin_drive_data_slice_1));

    /////////////// Set the pinmux for EMI ///////////////

   // Configure Bank-6  EMI_D15 ~ EMI_D00 as EMI pins
    HW_PINCTRL_MUXSEL10_CLR(
	    BM_PINCTRL_MUXSEL10_BANK5_PIN15 | 
            BM_PINCTRL_MUXSEL10_BANK5_PIN14 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN13 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN12 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN11 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN10 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN09 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN08 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN07 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN06 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN05 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN04 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN03 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN02 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN01 |
            BM_PINCTRL_MUXSEL10_BANK5_PIN00 );

    // EMI_DDR_OPEN ,EMI_DQS1,EMI_DQS0,EMI_CLK,EMI_DDR_OPEN_FB,EMI_DQM1,EMI_ODT1,EMI_DQM0,EMI_ODT0
    HW_PINCTRL_MUXSEL11_CLR(
	   BM_PINCTRL_MUXSEL11_BANK5_PIN26 |
	   BM_PINCTRL_MUXSEL11_BANK5_PIN23 |
	   BM_PINCTRL_MUXSEL11_BANK5_PIN22 |
	   BM_PINCTRL_MUXSEL11_BANK5_PIN21 |
	   BM_PINCTRL_MUXSEL11_BANK5_PIN20 |
	   BM_PINCTRL_MUXSEL11_BANK5_PIN19 |
	   BM_PINCTRL_MUXSEL11_BANK5_PIN18 |
	   BM_PINCTRL_MUXSEL11_BANK5_PIN17 |
	   BM_PINCTRL_MUXSEL11_BANK5_PIN16 );

    // EMI_A14 ~ EMI_A00
    HW_PINCTRL_MUXSEL12_CLR(
	  BM_PINCTRL_MUXSEL12_BANK6_PIN14 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN13 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN12 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN11 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN10 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN09 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN08 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN07 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN06 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN05 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN04 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN03 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN02 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN01 |
	  BM_PINCTRL_MUXSEL12_BANK6_PIN00 );

    // EMI_CKE,EMI_CE1N,EMI_CE0N,EMI_WEN,EMI_RASN,EMI_CASN,EMI_BA2,EMI_BA1,EMI_BA0
        HW_PINCTRL_MUXSEL13_CLR(
          BM_PINCTRL_MUXSEL13_BANK6_PIN24 |
          BM_PINCTRL_MUXSEL13_BANK6_PIN23 |
          BM_PINCTRL_MUXSEL13_BANK6_PIN22 |
          BM_PINCTRL_MUXSEL13_BANK6_PIN21 |
          BM_PINCTRL_MUXSEL13_BANK6_PIN20 |
          BM_PINCTRL_MUXSEL13_BANK6_PIN19 |
          BM_PINCTRL_MUXSEL13_BANK6_PIN18 |
          BM_PINCTRL_MUXSEL13_BANK6_PIN17 |
          BM_PINCTRL_MUXSEL13_BANK6_PIN16 );
    
 }


//* Function Specification *****************************************************
//!
//! \brief Disable Bus Keepers on EMI Pins
//!
//! This function disables the internal bus keepers on the EMI pins. This is only
//! necessary when connecting a Mobile DDR device to the DRAM controller.
//!
//******************************************************************************
void PINCTRL_DisableEmiPadKeepers(void)
{  
    // Enable the Pinmux by clearing the Soft Reset and Clock Gate
    HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE);

    // Disable the internal bus-keeper pins associated with EMI.
    HW_PINCTRL_PULL5_SET(
        BM_PINCTRL_PULL5_BANK5_PIN26 |
        BM_PINCTRL_PULL5_BANK5_PIN23 |
        BM_PINCTRL_PULL5_BANK5_PIN22 |
        BM_PINCTRL_PULL5_BANK5_PIN21 |
        BM_PINCTRL_PULL5_BANK5_PIN20 |
        BM_PINCTRL_PULL5_BANK5_PIN19 |
        BM_PINCTRL_PULL5_BANK5_PIN18 |
        BM_PINCTRL_PULL5_BANK5_PIN17 |
        BM_PINCTRL_PULL5_BANK5_PIN16 |
        BM_PINCTRL_PULL5_BANK5_PIN07 |
        BM_PINCTRL_PULL5_BANK5_PIN06 |
        BM_PINCTRL_PULL5_BANK5_PIN05 |
        BM_PINCTRL_PULL5_BANK5_PIN04 |
        BM_PINCTRL_PULL5_BANK5_PIN03 |
        BM_PINCTRL_PULL5_BANK5_PIN02 |
        BM_PINCTRL_PULL5_BANK5_PIN01 |
        BM_PINCTRL_PULL5_BANK5_PIN00  );

    HW_PINCTRL_PULL6_SET(
        BM_PINCTRL_PULL6_BANK6_PIN24 |
        BM_PINCTRL_PULL6_BANK6_PIN23 |
        BM_PINCTRL_PULL6_BANK6_PIN22 |
        BM_PINCTRL_PULL6_BANK6_PIN21 |
        BM_PINCTRL_PULL6_BANK6_PIN20 |
        BM_PINCTRL_PULL6_BANK6_PIN19 |
        BM_PINCTRL_PULL6_BANK6_PIN18 |
        BM_PINCTRL_PULL6_BANK6_PIN17 |
        BM_PINCTRL_PULL6_BANK6_PIN16 |
        BM_PINCTRL_PULL6_BANK6_PIN14 |
        BM_PINCTRL_PULL6_BANK6_PIN13 |
        BM_PINCTRL_PULL6_BANK6_PIN12 |
        BM_PINCTRL_PULL6_BANK6_PIN11 |
        BM_PINCTRL_PULL6_BANK6_PIN10 |
        BM_PINCTRL_PULL6_BANK6_PIN08 |
        BM_PINCTRL_PULL6_BANK6_PIN09 |
        BM_PINCTRL_PULL6_BANK6_PIN07 |
        BM_PINCTRL_PULL6_BANK6_PIN06 |
        BM_PINCTRL_PULL6_BANK6_PIN05 |
        BM_PINCTRL_PULL6_BANK6_PIN04 |
        BM_PINCTRL_PULL6_BANK6_PIN03 |
        BM_PINCTRL_PULL6_BANK6_PIN02 |
        BM_PINCTRL_PULL6_BANK6_PIN01 |
        BM_PINCTRL_PULL6_BANK6_PIN00 );

      
} 

void Init_mobile_ddr_mt46h32m16lf_5_150MHz(void)
{
 volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

    DRAM_REG[0] =    0x00000000;  //00000000000000000000000000000000 user_def_reg_0(RW) 
    DRAM_REG[16] =    0x00000000; //0000000_0 write_modereg(WR) 0000000_0 power_down(RW) 000000000000000_0 start(RW) 
    DRAM_REG[21] =    0x00000000; //00000_000 cke_delay(RW) 00000000 dll_lock(RD) 0000000_0 dlllockreg(RD) 0000000_0 dll_bypass_mode(RW) 
    DRAM_REG[26] =    0x00010101; //000000000000000_1 priority_en(RW) 0000000_1 addr_cmp_en(RW) 0000000_1 placement_en(RW) 
    DRAM_REG[27] =    0x01010101; //0000000_1 swap_port_rw_same_en(RW) 0000000_1 swap_en(RW) 0000000_1 bank_split_en(RW) 0000000_1 rw_same_en(RW) 
    DRAM_REG[28] =    0x000f0f01; //00000_000 q_fullness(RW) 0000_1111 age_count(RW) 0000_1111 command_age_count(RW) 0000000_1 active_aging(RW) 
    DRAM_REG[29] =    0x0f02020a; //0000_1111 cs_map(RW) 00000_010 column_size(RW) 00000_010 addr_pins(RW) 0000_1010 aprebit(RW) 
    DRAM_REG[30] =    0x00000000; //0000000000000_000 max_cs_reg(RD) 0000_0000 max_row_reg(RD) 0000_0000 max_col_reg(RD) 
    DRAM_REG[31] =    0x00000101; //000000000000000_0 eight_bank_mode(RW) 0000000_1 drive_dq_dqs(RW) 0000000_1 dqs_n_en(RW) 
    DRAM_REG[32] =    0x00000100; //00000000000000000000000_1 reduc(RW) 0000000_0 reg_dimm_enable(RW) 
    DRAM_REG[33] =    0x00000100; //00000000000000000000000_1 concurrentap(RW) 0000000_0 ap(RW) 
    DRAM_REG[34] =    0x01000000; //0000000_1 writeinterp(RW) 0000000_0 intrptwritea(RW) 0000000_0 intrptreada(RW) 0000000_0 intrptapburst(RW) 
    DRAM_REG[35] =    0x00000002; //000000000000000_0 pwrup_srefresh_exit(RW) 0000000_0 no_cmd_init(RW) 0000_0010 initaref(RW) 
    DRAM_REG[36] =    0x01010000; //0000000_1 tref_enable(RW) 0000000_1 tras_lockout(RW) 000000000000000_0 fast_write(RW) 
    DRAM_REG[37] =    0x07060301; //0000_0111 caslat_lin_gate(RW) 0000_0110 caslat_lin(RW) 00000_011 caslat(RW) 0000_0001 wrlat(RW) 
    DRAM_REG[38] =    0x06000001; //000_00110 tdal(RW) 0000000000000000 tcpd(RW) 00000_001 tcke(RW) 
    DRAM_REG[39] =    0x0a000000; //00_001010 tfaw(RW) 000000000000000000000000 tdll(RW) 
    DRAM_REG[40] =    0x02000020; // shorten tinit for simulation    
    DRAM_REG[41] =    0x00020309; //0000000000000010 tpdex(RW) 00000011 trcd_int(RW) 00_001001 trc(RW) 
    DRAM_REG[42] =    0x0028f506; //000000000010100011110101 tras_max(RW) 00000110 tras_min(RW) 
    DRAM_REG[43] =    0x030f048b; //0000_0011 trp(RW) 00001111 trfc(RW) 00_00010010001011 tref(RW) 
    DRAM_REG[44] =    0x03030002; //0000_0011 twtr(RW) 000_00011 twr_int(RW) 00000_000 trtp(RW) 00000_010 trrd(RW) 
    DRAM_REG[45] =    0x00150012; //0000000000010101 txsr(RW) 0000000000010010 txsnr(RW) 
    DRAM_REG[48] =    0x00011900; //0_0000000 axi0_current_bdw(RD) 0000000_1 axi0_bdw_ovflow(RW) 0_0100001 axi0_bdw(RW) 000000_00 axi0_fifo_type_reg(RW) 
    DRAM_REG[49] =    0xffff0303;
    DRAM_REG[52] =    0x00011900; //0_0000000 axi2_current_bdw(RD) 0000000_1 axi2_bdw_ovflow(RW) 0_0100001 axi2_bdw(RW) 000000_00 axi2_fifo_type_reg(RW)     
    DRAM_REG[53] =    0xffff0303;
    DRAM_REG[54] =    0x00011900; //0_0000000 axi3_current_bdw(RD) 0000000_1 axi3_bdw_ovflow(RW) 0_0100001 axi3_bdw(RW) 000000_00 axi3_fifo_type_reg(RW) 
    DRAM_REG[55] =    0xffff0303;
    DRAM_REG[56] =    0x00000003; //00000000000000000000000000000_011 arb_cmd_q_threshold(RW) 
    DRAM_REG[58] =    0x00000000; //00000_00000000000 int_status(RD) 00000_00000000000 int_mask(RW) 
    DRAM_REG[66] =    0x0000048b; //000000000000_0000 tdfi_ctrlupd_min(RD) 00_00010010001011 tdfi_ctrlupd_max(RW) 
    DRAM_REG[67] =    0x01000f02; //0000_0001 tdfi_dram_clk_enable(RW) 00000_000 tdfi_dram_clk_disable(RW) 0000_0000 dram_clk_disable(RW) 0000_0010 tdfi_ctrl_delay(RW) 
    DRAM_REG[68] =    0x048b048b; //00_00010010001011 tdfi_phyupd_type0(RW) 00_00010010001011 tdfi_phyupd_resp(RW) 
    DRAM_REG[69] =    0x00000200; //00000000000000000000_0010 tdfi_phy_wrlat_base(RW) 0000_0000 tdfi_phy_wrlat(RD) 
    DRAM_REG[70] =    0x00020007; //000000000000_0010 tdfi_rddata_en_base(RW) 0000_0000 tdfi_rddata_en(RD) 0000_0111 tdfi_phy_rdlat(RW) 
    DRAM_REG[71] =    0xf3004a27; //11110100000000000100101000100111 phy_ctrl_reg_0_0(RW) 
    DRAM_REG[72] =    0xf3004a27; //11110100000000000100101000100111 phy_ctrl_reg_0_1(RW) 
    DRAM_REG[75] =    0x07400310; //00000111010000000000001100010000 phy_ctrl_reg_1_0(RW) 
    DRAM_REG[76] =    0x07400310; //00000111010000000000001100010000 phy_ctrl_reg_1_1(RW) 
    DRAM_REG[79] =    0x00800004; //00000000100000000000000000000101 phy_ctrl_reg_2(RW) 
    DRAM_REG[82] =    0x01000000; //0000000_1 odt_alt_en(RW) 000000000000000000000000
    DRAM_REG[83] =    0x01020408; //0000_0001 odt_rd_map_cs3(RW) 0000_0010 odt_rd_map_cs2(RW) 0000_0100 odt_rd_map_cs1(RW) 0000_1000 odt_rd_map_cs0(RW) 
    DRAM_REG[84] =    0x08040201; //0000_1000 odt_wr_map_cs3(RW) 0000_0100 odt_wr_map_cs2(RW) 0000_0010 odt_wr_map_cs1(RW) 0000_0001 odt_wr_map_cs0(RW) 
    DRAM_REG[85] =    0x000f1133; //00000000000011110001000100110011 pad_ctrl_reg_0(RW)    
    DRAM_REG[87] =    0x00089f30; //00000000000010001001111100110000 dll_ctrl_reg_0_0(RW) 
    DRAM_REG[88] =    0x00089f30; //00000000000010001001111100110000 dll_ctrl_reg_0_1(RW) 
    DRAM_REG[91] =    0x00081f0d; //00000000000010000001111100001101 dll_ctrl_reg_1_0(RW) 
    DRAM_REG[92] =    0x00081f0d; //00000000000010000001111100001101 dll_ctrl_reg_1_1(RW)  
    DRAM_REG[162] =   0x00000000; //00000_000 w2r_samecs_dly(RW) 00000_000 w2r_diffcs_dly(RW) 0000000_000000000
    DRAM_REG[163] =   0x00010301; //00000000 dll_rst_adj_dly(RW) 0000_0001 wrlat_adj(RW) 0000_0011 rdlat_adj(RW) 0000_0001 dram_class(RW) 
    DRAM_REG[164] =   0x00000002; //00000000000000_0000000000 int_ack(WR) 00000010 tmod(RW) 
    DRAM_REG[171] =   0x01010000; //0000000_1 axi5_bdw_ovflow(RW) 0000000_1 axi4_bdw_ovflow(RW) 0000000000000000 dll_rst_delay(RW) 
    DRAM_REG[172] =   0x01000100; //0000000_1 resync_dll_per_aref_en(RW) 0000000_0 resync_dll(WR) 0000000_1 concurrentap_wr_only(RW) 0000000_0 cke_status(RD) 
    DRAM_REG[173] =   0x00000000; //00000_011 axi4_w_priority(RW) 00000_011 axi4_r_priority(RW) 000000_00 axi5_fifo_type_reg(RW) 000000_00 axi4_fifo_type_reg(RW) 
    DRAM_REG[174] =   0x00020303; //00000_000 r2r_samecs_dly(RW) 00000_010 r2r_diffcs_dly(RW) 00000_011 axi5_w_priority(RW) 00000_011 axi5_r_priority(RW) 
    DRAM_REG[175] =   0x01010202; //00000_001 w2w_diffcs_dly(RW) 00000_001 tbst_int_interval(RW) 00000_010 r2w_samecs_dly(RW) 00000_010 r2w_diffcs_dly(RW) 
    DRAM_REG[176] =   0x00000000; //0000_0000 add_odt_clk_sametype_diffcs(RW) 0000_0000 add_odt_clk_difftype_samecs(RW) 0000_0000 add_odt_clk_difftype_diffcs(RW) 00000_000 w2w_samecs_dly(RW) 
    DRAM_REG[177] =   0x01030101; //000_00001 tccd(RW) 0000_0011 trp_ab(RW) 0000_0001 cksrx(RW) 0000_0001 cksre(RW) 
    DRAM_REG[178] =   0x01001901; //0_0100001 axi5_bdw(RW) 0_0000000 axi4_current_bdw(RD) 0_0100001 axi4_bdw(RW) 000_00001 tckesr(RW) 
    DRAM_REG[181] =   0x00320032; //0_000000000110010 mr0_data_1(RW) 0_000000000110010 mr0_data_0(RW) 
    DRAM_REG[183] =   0x00000000; //0_000000000000000 mr1_data_1(RW) 0_000000000000000 mr1_data_0(RW) 
    DRAM_REG[189] =   0xffffffff;
}

//* Function Specification *****************************************************
//!
//! \brief Initialize/Start EMI DRAM Controller
//!
//! This function sets up and starts the EMI DRAM Controller using the specified
//! simulated memory device, clock frequency and dram register init function.
//!
//! \param[in] simmemsel      - Simulated device to connect with (device/chip-sel)
//! \param[in] emi_clk_freq   - EMI Clock frequency to begin with.
//! \param[in] pin_voltage    - Pin voltage assigned to each EMI pin.
//! \param[in] pin_drive_addr - Pin drive strength (mA) assigned to EMI address pins.
//! \param[in] pin_drive_data - Pin drive strength (mA) assigned to EMI data pins.
//! \param[in] pin_drive_ce   - Pin drive strength (mA) assigned to EMI chip select pins.
//! \param[in] pin_drive_clk  - Pin drive strength (mA) assigned to EMI clock pins.
//! \param[in] pin_drive_ctrl - Pin drive strength (mA) assigned to EMI control pins.
//! \param[in] init_dram_regs - Function pointer to DRAM Register Init routine.
//! \param[in] device         - Device type (SDRAM, MOBILE_SDRAM, MOBILE_DDR)
//!
//******************************************************************************

void EMI_StartDramController1(
        uint32_t simmemsel_cfg,
        uint32_t simmemsel_ce_cfg,
        TEmiClkFreq emi_clk_freq,
        TEmiClkDelay emi_clk_delay,
        TPinVoltage pin_voltage,
        TPinDrive pin_drive_addr,
        TPinDrive pin_drive_ctrl,
        TPinDrive pin_drive_clk,
        TPinDrive pin_drive_data_slice_0,
        TPinDrive pin_drive_data_slice_1,
        uint32_t emi_controller_changes,
        uint32_t emi_controller_mask,
        TEmiDevice device)
{
    uint32_t tmp_ctrl;  uint32_t i,temp;

    // Turn on the PLL (480MHz)
    CLKCTRL_TurnOnPll();

    // Set up the EMI clock
    CLKCTRL_ChgEmiClk(emi_clk_freq, DCC_RESYNC_NO);

    // Set up the pinmux for the EMI
    PINCTRL_TurnOnEmiPins(pin_voltage, pin_drive_addr, pin_drive_ctrl,
            pin_drive_clk, pin_drive_data_slice_0, pin_drive_data_slice_1);

    // Set keeper
    if (device == EMI_DEV_MOBILE_DDR) {
        PINCTRL_DisableEmiPadKeepers();
    } 
  
    // Program up the simmemsel to use the specified device
    HW_SIMMEMSEL_CFG_WR(simmemsel_cfg); 
    HW_SIMMEMSEL_CE_CFG_WR(simmemsel_ce_cfg); 

    ////======= initialize EMI ========/////
     //enable EMI_CLK output     
    HW_DRAM_CTL00_SET(0x00000001);

    //Initialize EMI...
     HW_DRAM_CTL16_CLR(0x00000001); //clear "start"
     Init_mobile_ddr_mt46h32m16lf_5_150MHz(); // Write the Databahn SDRAM setup register values
     HW_DRAM_CTL17.B.SREFRESH = 0;
     HW_DRAM_CTL16_SET(0x00000001);  //set "start"

     temp = HW_DRAM_CTL58_RD();  //Wait for EMI initization completed
     while ( (temp & 0x00100000) != 0x00100000 ){
     temp = HW_DRAM_CTL58_RD();
     }

}

void EMI_StartDramController(void)
{
     EMI_StartDramController1(
            0x00030204,                                     // HW_SIMMEMSEL_CFG_WR
            0x00000000,                                      // HW_SIMMEMSEL_CE_CFG_WR
            EMI_CLK_150MHz,                          // emi_clk_freq
            EMI_CLK_DELAY_0_TAPS,                 // emi_clk_delay
            PIN_VOLTAGE_1pt8V,                    // pin_voltage
            PIN_DRIVE_5mA,                           // pin_drive_addr
            PIN_DRIVE_5mA,                           // pin_drive_ctrl
            PIN_DRIVE_10mA,                        // pin_drive_clk
            PIN_DRIVE_10mA,                        // pin_drive_data_slice0
            PIN_DRIVE_10mA,                        // pin_drive_data_slice1
            0,                                                        // emi_controller_changes
            0,                                                       // emi_controller_mask
            EMI_DEV_MOBILE_DDR);                        // device-type

}

