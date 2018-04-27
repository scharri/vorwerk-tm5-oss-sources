////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_ssp_hal
//! @{
//!
// Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file   ssp_hal.h
//! \brief  Definitions for the the I2C HAL layer.
//!
////////////////////////////////////////////////////////////////////////////////
#if !defined(_ssp_hal_h_)
#define _ssp_hal_h_

#include "rom_types.h"      //! Type defs
#include "regsssp.h"        //! SSP Registers
#include "regsapbh.h"       //! DMA Bus

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! CNTL_IO commands
#define SSP_CNTL_CLOCK              0
#define SSP_CNTL_CLOCK_CFG          1
#define SSP_CNTL_PAD_DRIVE          2
#define SSP_CNTL_TIMEOUT            3
#define SSP_CNTL_BUS_WIDTH          4
#define SSP_CNTL_SD_STATUS          5
#define SSP_CNTL_CMD_PULLUPS        6
#define SSP_CNTL_DATA_PULLUPS       7
#define SSP_CNTL_DMA_START          8
#define SSP_CNTL_DMA_END            9
#define SSP_CNTL_DMA_WAIT           10
#define SSP_CNTL_DMA_STATUS         11
#define SSP_CNTL_CLOCK_FIND_INDEX   12
#define SSP_CNTL_DMA_ADD            13

//! SD Status values
#define SSP_DATA_RX_CRC             1
#define SSP_DATA_TX_CRC             2
#define SSP_DATA_TIMEOUT            3
#define SSP_DMA_OVR_UNDERFLOW       4
#define SSP_COMMAND_RESPONSE        5
#define SSP_COMMAND_RESP_TIMEOUT    6

//! DMA Status values
#define SSP_DMA_READY               0
#define SSP_DMA_RUNNING             1

//! INIT() and IO_CNTL() Constants
#define SSP_BUS_1_BIT               0
#define SSP_BUS_4_BIT               1
#define SSP_BUS_8_BIT               2

#define SSP_PULLUP_ENABLED          1
#define SSP_PULLUP_ENABLED          1

//! Index into SCK Table
#define SSP_CLOCK_INVALID           0
#define SSP_CLOCK_240000HZ        1
//#define SSP_CLOCK_SLOW              1
#define SSP_CLOCK_1MHZ              2
#define SSP_CLOCK_2MHZ              3
#define SSP_CLOCK_4MHZ              4
#define SSP_CLOCK_6MHZ              5
#define SSP_CLOCK_8MHZ              6
#define SSP_CLOCK_10MHZ             7
#define SSP_CLOCK_12MHZ             8
#define SSP_CLOCK_16MHZ             9
#define SSP_CLOCK_20MHZ             10
#define SSP_CLOCK_30MHZ             11
#define SSP_CLOCK_40MHZ             12
#define SSP_CLOCK_48MHZ             13
#define SSP_CLOCK_51MHZ             14       //! 51.4MHz actually
#define SSP_CLOCK_CUSTOM            15

#define SSP_DEFAULT_TIMEOUT         (1000*1000) // 1 sec
#define SSP_DEFAULT_WORD_LENGTH     BV_SSP_CTRL1_WORD_LENGTH__EIGHT_BITS
#define SSP_POLARITY_STEADY_LO      0
#define SSP_POLARITY_STEADY_HI      1
#define SSP_POLARITY_SD_RISING_EDGE 0
#define SSP_POLARITY_SD_FALLING_EDGE 1
#define SSP_PHASE_LO                0
#define SSP_PHASE_HI                1
#define SSP_RISING_CLOCK_TRANSITION  0
#define SSP_FALLING_CLOCK_TRANSITION 1

#define SSP_CHANNEL_COUNT           2
#define SSP_PIO_WORDS               9    //! 2 registers are added, Xfer_size, Block_size
#define SSP_DSCR_WORDS              (3 + SSP_PIO_WORDS)


typedef enum _ssp_Mode            //!< SSP Mode
{
    SSP_MODE_SPI = BV_SSP_CTRL1_SSP_MODE__SPI,
    SSP_MODE_SSI = BV_SSP_CTRL1_SSP_MODE__SSI,
    SSP_MODE_SD = BV_SSP_CTRL1_SSP_MODE__SD_MMC,
    SSP_MODE_MS = BV_SSP_CTRL1_SSP_MODE__MS
    //SSP_MODE_CE_ATA = BV_SSP_CTRL1_SSP_MODE__CE_ATA
} ssp_Mode_t;


typedef enum _ssp_Port            //!< 4 SSP Blocks, begin number from 0 to 3 
{  
    SSP_PORT_0,
    SSP_PORT_1,
    SSP_PORT_2,
    SSP_PORT_3
} ssp_Port_t;

typedef enum _ssp_Voltage         //!< Pin Voltage
{
    SSP_VOLT_1_8,
    SSP_VOLT_3_3
} ssp_Voltage_t;

typedef enum _ssp_Drive           //!< Pin Drive Current
{
    SSP_DRIVE_4MA,
    SSP_DRIVE_8MA,
    SSP_DRIVE_12MA
} ssp_Drive_t;

typedef enum _ssp_DLLCtrl
{
    DLL_CTRL_SET0,
    DLL_CTRL_SET1,
    DLL_CTRL_SET2,
    DLL_CTRL_SET3,
    DLL_CTRL_SET4,
    DLL_CTRL_SET5,
    DLL_CTRL_SET6,
    DLL_CTRL_SET7
}ssp_DLLCtrl_t;

////////////////////////////////////////
//! DMA stransfer
////////////////////////////////////////
typedef union _ssp_Pio_t ssp_Pio_t;
union _ssp_Pio_t
{                          
    uint32_t               Word[SSP_PIO_WORDS];
    struct                 
    {                      
        hw_ssp_ctrl0_t     Ctrl0;
        hw_ssp_cmd0_t      Cmd0;
        hw_ssp_cmd1_t      Cmd1;
        hw_ssp_xfer_size_t  Xfer_size;
        hw_ssp_block_size_t  Block_size;
        hw_ssp_compref_t   Compref;
        hw_ssp_compmask_t  Compmask;
        hw_ssp_timing_t    Timing;
        hw_ssp_ctrl1_t     Ctrl1;
    };
};

typedef union _ssp_Dscr_t ssp_Dscr_t;
union _ssp_Dscr_t
{
    uint32_t               Word[SSP_DSCR_WORDS];
    struct
    {
        ssp_Dscr_t*        pNxt;    //!< Pointer to the next DMA command to be executed.
        hw_apbh_chn_cmd_t  Cmd;     //!< Command information.
        void*              pBuf;    //!< Pointer to the buffer for this command.
        ssp_Pio_t          Pio;     //!< Hardware control
    };
};

////////////////////////////////////////
//! SPI-only parameters.
////////////////////////////////////////
typedef struct _ssp_SpiMode
{
    int Reserved;
}
ssp_SpiMode_t;

////////////////////////////////////////
//! SD-only parameters.
////////////////////////////////////////
typedef struct _ssp_SdMode
{
    int busWidth; 			    //!< Bus width 
                                //!< 	0=1-bit
                                //!<	1=4-bit
                                //!<	2=8-bit
    bool cmdPullups;		    //!< Enable Internal Pull-ups - CMD Line
                                //!<	0=Open    
                                //!<	1=Enable 10K Pullups
    bool dataPullups;		    //!< Enable Internal Pull-ups - Data Lines
                                //!<	0=Open    
                                //!<	1=Enable 47K Pullups
    int  dll_ctrl_set_index;
}
ssp_SdMode_t;

////////////////////////////////////////
//! Init structure for SSP.
////////////////////////////////////////
typedef struct _ssp_Cfg
{
    ssp_Port_t      ePort; 		    //!< physical port 
    ssp_Mode_t      eMode; 		    //!< device function 
    ssp_Voltage_t   eVoltage;		//!< driver voltage  
    ssp_Drive_t     eCurrent;		//!< driver current 
    int             SpeedIdx:4;		//!< SSP clock (Index to ClockConfig table)
    MemoryPool_t  * pMemPool;       //!< Available memory to use
    union {
	    ssp_SpiMode_t * pSpi;	    //!< pointer to SPI Parameters
	    ssp_SdMode_t  * pSd;	    //!< pointer to SD Parameters
    };
}
ssp_Cfg_t;

////////////////////////////////////////
//!  Pullup Lookup Table entry
////////////////////////////////////////
typedef struct _ssp_ClockConfig
{
    int    clkSel		:1;	        //!< Clock Select (0=io_ref 1=xtal_ref)	
    int    io_frac		:6;	        //!< IO FRAC 18-35 (io_frac+16)
    int    ssp_frac		:9;	        //!< SSP FRAC (1=default) 
    int    ssp_div		:8;	        //!< Divider: Must be an even value 2-254
    int    ssp_rate		:8;	        //!< Serial Clock Rate
}
ssp_ClockConfig_t;

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////
//extern ssp_BootItf_t * g_pSsp;                // Main Patch pointer
extern const ssp_BootItf_t rom_ssp_BootItf;     // ROM Patch table defaults
extern ssp_ClockConfig_t * g_pSspSckTable;      // Points to SCK Table

////////////////////////////////////////////////////////////////////////////////
// API Prototypes
////////////////////////////////////////////////////////////////////////////////
int SSPHAL_Init(ssp_Cfg_t * pConfig);
int SSPHAL_Shutdown(void);
int SSPHAL_Ctrl(int command, void * pParameter);

#endif // _ssp_hal_h_

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
