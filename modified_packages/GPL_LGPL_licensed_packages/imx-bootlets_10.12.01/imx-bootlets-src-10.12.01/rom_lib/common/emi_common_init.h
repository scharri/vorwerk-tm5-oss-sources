#ifndef EMI_COMMON_INIT_H
#define EMI_COMMON_INIT_H

////////////////////////////////////////////////////////////////////////////////
//
// Filename: emi_common_init.h
//
// Description: Common Types, Constants, Macros, Functions specific to EMI
//              Initialization.
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
//#include "test_util.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef void (*dram_init_fcn)(void);

typedef enum
{
    EMI_CLK_6MHz = 6,
    EMI_CLK_24MHz = 24,
    EMI_CLK_48MHz = 48,
    EMI_CLK_60MHz = 60,
    EMI_CLK_80MHz = 80,
    EMI_CLK_96MHz = 96,
    EMI_CLK_120MHz = 120,
    EMI_CLK_133MHz = 133,
    EMI_CLK_144MHz = 144,
    EMI_CLK_150MHz = 150,
    EMI_CLK_166MHz = 166,
    EMI_CLK_200MHz = 200
} TEmiClkFreq;

typedef enum
{
    EMI_CLK_DELAY_0_TAPS,
    EMI_CLK_DELAY_1_TAPS,
    EMI_CLK_DELAY_2_TAPS,
    EMI_CLK_DELAY_3_TAPS,
    EMI_CLK_DELAY_4_TAPS,
    EMI_CLK_DELAY_5_TAPS,
    EMI_CLK_DELAY_6_TAPS,
    EMI_CLK_DELAY_7_TAPS,
    EMI_CLK_DELAY_8_TAPS,
    EMI_CLK_DELAY_9_TAPS,
    EMI_CLK_DELAY_10_TAPS,
    EMI_CLK_DELAY_11_TAPS,
    EMI_CLK_DELAY_12_TAPS,
    EMI_CLK_DELAY_13_TAPS,
    EMI_CLK_DELAY_14_TAPS,
    EMI_CLK_DELAY_15_TAPS,
    EMI_CLK_DELAY_16_TAPS,
    EMI_CLK_DELAY_17_TAPS,
    EMI_CLK_DELAY_18_TAPS,
    EMI_CLK_DELAY_19_TAPS,
    EMI_CLK_DELAY_20_TAPS,
    EMI_CLK_DELAY_21_TAPS,
    EMI_CLK_DELAY_22_TAPS,
    EMI_CLK_DELAY_23_TAPS,
    EMI_CLK_DELAY_24_TAPS,
    EMI_CLK_DELAY_25_TAPS,
    EMI_CLK_DELAY_26_TAPS,
    EMI_CLK_DELAY_27_TAPS,
    EMI_CLK_DELAY_28_TAPS,
    EMI_CLK_DELAY_29_TAPS,
    EMI_CLK_DELAY_30_TAPS,
    EMI_CLK_DELAY_31_TAPS
} TEmiClkDelay;

typedef enum
{
    PIN_VOLTAGE_1pt5V = 0,
    PIN_VOLTAGE_1pt8V = 1,
} TPinVoltage;


typedef enum  
{
    PIN_DRIVE_5mA  = 0,
    PIN_DRIVE_10mA  = 1,
    PIN_DRIVE_20mA = 2
} TPinDrive;



typedef enum
{
    EMI_DEV_MOBILE_DDR,
    EMI_DEV_DDR2,
    EMI_DEV_LVDDR2
} TEmiDevice;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define DCC_RESYNC_NO  0
#define DCC_RESYNC_YES 1

// for power control setting
#define POWER_VDDD_MIN                    800
#define POWER_VDDD_MAX                   1575
#define POWER_VDDD_INCR                    25

#define POWER_VDDM_MIN                   1100
#define POWER_VDDM_MAX                   1750
#define POWER_VDDM_INCR                    25

#define POWER_VDDIO_MIN                  2800
#define POWER_VDDIO_MAX                  3600
#define POWER_VDDIO_INCR                   50

#define POWER_VDDA_MIN                   1500
#define POWER_VDDA_MAX                   2275
#define POWER_VDDA_INCR                    25

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
void CLKCTRL_TurnOnPll(void);
void CLKCTRL_ChgEmiClk(TEmiClkFreq emi_freq, uint32_t dcc_resync_enable);

void DIGCTL_KillTimeUsec(uint32_t microseconds);

int  EMI_ChgFreq(TEmiClkFreq emiclk_freq, dram_init_fcn new_dram_reg_settings, uint32_t verbose);

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
        TEmiDevice device);

void EMI_StartDramController(void);

void PINCTRL_TurnOnEmiPins(
        TPinVoltage pin_voltage,
        TPinDrive pin_drive_addr,
        TPinDrive pin_drive_ctrl,
        TPinDrive pin_drive_clk,
        TPinDrive pin_drive_data_slice_0,
        TPinDrive pin_drive_data_slice_1);

void PINCTRL_DisableEmiPadKeepers(void);

void LRADC_EnableBatteryMeasurement(void);

#endif

