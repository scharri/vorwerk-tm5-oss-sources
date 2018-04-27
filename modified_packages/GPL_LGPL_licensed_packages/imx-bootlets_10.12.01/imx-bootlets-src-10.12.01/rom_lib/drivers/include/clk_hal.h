////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_clk_hal
//! @{
//!
// Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file   clk_hal.h
//! \brief  Definitions for the the CLKCTRL API.
//!
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLK_HAL_H__
#define __CLK_HAL_H__

#if (defined(TGT_CHIP) || defined(TGT_SIM))

////////////////////////////////////////////////////////////////////////////////
//! \brief Query state of PLL
//! \fntype Function
//! This API will return current state of PLL circuit
//! \retval 0      PLL Off
//! \retval !0     PLL On
////////////////////////////////////////////////////////////////////////////////
extern bool CLK_Pll0_GetControl(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Controls PPL circuit
//! \fntype Function
//! This API will Enable/Disable PPL circuit
//! \param[in]   bEnable            1=On ;  0=Off
//! \retval SUCCESS                 Completed successfully
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Pll0_SetControl(bool bEnable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Configure IO0_REF clock to SSP_CLK
//! \fntype Function
//! This API will configure IO_REF divisor. Enables IO_GATE if value not 18.
//! Turns on PLL if not already ON
//! \param[in]   divisor            Divisor value (18..35)     0=Reset values
//! \retval SUCCESS                 Completed successfully
//! \retval ERROR_ROM               Invalid Range
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Io0Ref_SetClock(int divisor);
////////////////////////////////////////////////////////////////////////////////
//! \brief Configure IO1_REF clock to SSP_CLK
//! \fntype Function
//! This API will configure IO_REF divisor. Enables IO_GATE if value not 18.
//! Turns on PLL if not already ON
//! \param[in]   divisor            Divisor value (18..35)     0=Reset values
//! \retval SUCCESS                 Completed successfully
//! \retval ERROR_ROM               Invalid Range
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Io1Ref_SetClock(int divisor);

extern int  CLK_GpmiRef_SetClock(int divisor);
////////////////////////////////////////////////////////////////////////////////
//! \brief Configure SSP_CLK to SSP
//! \fntype Function
//! This API will configure IO_REF divisor. 
//! \param[in]   divisor            Divisor value (1..255)    0=Reset values
//! \retval SUCCESS                 Completed successfully
//! \retval ERROR_ROM               Invalid Range
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp0_SetClock(int divisor);

////////////////////////////////////////////////////////////////////////////////
//! \brief Selects SSP_CLK to be XTAL or IO_REF
//! \fntype Function
//! This API will select either XTAL or IO_REF to input of SSP_CLK
//! \param[in]   bSelectIoRef       1=Select IO_REF; 0=Select XTAL
//! \retval SUCCESS                 Completed successfully
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp0_SelectIoRef(bool bSelectIoRef);

////////////////////////////////////////////////////////////////////////////////
//! \brief Configure SSP_CLK to SSP
//! \fntype Function
//! This API will configure IO_REF divisor. 
//! \param[in]   divisor            Divisor value (1..255)    0=Reset values
//! \retval SUCCESS                 Completed successfully
//! \retval ERROR_ROM               Invalid Range
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp1_SetClock(int divisor);

////////////////////////////////////////////////////////////////////////////////
//! \brief Selects SSP_CLK to be XTAL or IO_REF
//! \fntype Function
//! This API will select either XTAL or IO_REF to input of SSP_CLK
//! \param[in]   bSelectIoRef       1=Select IO_REF; 0=Select XTAL
//! \retval SUCCESS                 Completed successfully
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp1_SelectIoRef(bool bSelectIoRef);

////////////////////////////////////////////////////////////////////////////////
//! \brief Configure SSP_CLK to SSP
//! \fntype Function
//! This API will configure IO_REF divisor. 
//! \param[in]   divisor            Divisor value (1..255)    0=Reset values
//! \retval SUCCESS                 Completed successfully
//! \retval ERROR_ROM               Invalid Range
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp2_SetClock(int divisor);

////////////////////////////////////////////////////////////////////////////////
//! \brief Selects SSP_CLK to be XTAL or IO_REF
//! \fntype Function
//! This API will select either XTAL or IO_REF to input of SSP_CLK
//! \param[in]   bSelectIoRef       1=Select IO_REF; 0=Select XTAL
//! \retval SUCCESS                 Completed successfully
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp2_SelectIoRef(bool bSelectIoRef);

////////////////////////////////////////////////////////////////////////////////
//! \brief Configure SSP_CLK to SSP
//! \fntype Function
//! This API will configure IO_REF divisor. 
//! \param[in]   divisor            Divisor value (1..255)    0=Reset values
//! \retval SUCCESS                 Completed successfully
//! \retval ERROR_ROM               Invalid Range
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp3_SetClock(int divisor);

////////////////////////////////////////////////////////////////////////////////
//! \brief Selects SSP_CLK to be XTAL or IO_REF
//! \fntype Function
//! This API will select either XTAL or IO_REF to input of SSP_CLK
//! \param[in]   bSelectIoRef       1=Select IO_REF; 0=Select XTAL
//! \retval SUCCESS                 Completed successfully
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp3_SelectIoRef(bool bSelectIoRef);

////////////////////////////////////////////////////////////////////////////////
//! \brief Configure SSP_CLK to SSP
//! \fntype Function
//! This API will configure IO_REF divisor. 
//! \param[in]   divisor            Divisor value (1..255)    0=Reset values
//! \retval SUCCESS                 Completed successfully
//! \retval ERROR_ROM               Invalid Range
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp_SetClock(int divisor, int ssp_channel);

////////////////////////////////////////////////////////////////////////////////
//! \brief Selects SSP_CLK to be XTAL or IO_REF
//! \fntype Function
//! This API will select either XTAL or IO_REF to input of SSP_CLK
//! \param[in]   bSelectIoRef       1=Select IO_REF; 0=Select XTAL
//! \retval SUCCESS                 Completed successfully
////////////////////////////////////////////////////////////////////////////////
extern int  CLK_Ssp_SelectIoRef(bool bSelectIoRef, int ssp_channel);

#endif // (defined(TGT_CHIP) || defined(TGT_SIM))

#endif //__CLK_HAL_H__

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
