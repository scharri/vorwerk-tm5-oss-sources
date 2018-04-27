////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_nand_hal
//! @{
//
// Copyright (c) 2006 SigmaTel, Inc.
//
//! \file rom_nand_hal_gpmi.h
//! \brief This file provides the GPMI function headers for the ROM NAND HAL.
//!
//!
////////////////////////////////////////////////////////////////////////////////
#ifndef ROM_NAND_HAL_GPMI_H
#define ROM_NAND_HAL_GPMI_H 1

#include "return_codes.h"
#include "rom_types.h"

//#define GPMI_PROBECFG_BUS  3
#define GPMI_PROBECFG_PADS 4
#define GPMI_PROBECFG  GPMI_PROBECFG_PADS


////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////


RtStatus_t rom_nand_hal_EnableGPMI(bool bUse16BitData, uint32_t u32ChipSelectReadyMask,
                                   //uint32_t efAltCEPinConfig,
                                   uint32_t efEnableIntPullups,
                                   bool bUse1_8V_Drive);

void rom_nand_hal_ClearEccCompleteFlag(void);

void rom_nand_hal_DisableGPMI(void);

void rom_nand_hal_ResetBCH(void);

void rom_nand_hal_EnableBCH(void); 

void rom_nand_hal_DisableBCH(void);

void rom_nand_hal_UpdateECCParams(NAND_ECC_Params_t *pNANDEccParams, uint32_t u32NumNandsInUse);

uint32_t rom_nand_hal_FindEccErrors(uint32_t u32NumberOfCorrections);

RtStatus_t rom_nand_hal_CheckECCDecodeCapability(uint32_t u32EccSize);

void rom_nand_hal_SetGPMITimeout(uint32_t u32Timeout);

#endif // NAND_HAL_GPMI_H
//@}
