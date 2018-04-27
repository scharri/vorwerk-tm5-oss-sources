#include "rom_types.h"
#include "return_codes.h"
#include "configblock.h"

////////////////////////////////////////////////////////////////////////////////
// code
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! \brief      Returns start location and size of firmware by reading FW 
//!             Config Block
//!
//! \fntype     Non-Reentrant Function
//!
//! \param[in]  u32ChipNum:    Chip select
//! \param[in]  pBuf:    pointer to buffer having Config Block
//! \param[out]  pu32StartLoc:    pointer to return start location of 
//!              firmware block
//! \param[out]  pu32SectorCount:    pointer to return size of firmware in 
//!              sectors
//! \param[in]   u32SecondaryBoot, if set to 0 then return primarytag firmware
//!              else return secondarytag firmware.
//!
//! \retval     SUCCESS
//! \retval     ERROR_ROM_COMMON_DRIVER_INVALID_CONFIGBLOCK: FW Config Block is 
//!             not valid
//!                              
////////////////////////////////////////////////////////////////////////////////

RtStatus_t rom_ReadConfigBlock(uint32_t u32ChipNum, uint8_t *pBuf, uint32_t *pu32StartLoc, uint32_t *pu32SectorCount, uint32_t u32SecondaryBoot)
{
    ConfigBlock_t *pCB = (ConfigBlock_t *)pBuf;

    // Make sure we have a valid config block
    if (pCB->u32Signature == FIRMWARE_CONFIG_BLOCK_SIGNATURE)
    {
        int i;
        // Search for primary boot tag unless u32SecondaryBoot is set
        uint32_t u32BootTag = pCB->u32PrimaryBootTag; 
        
        if (u32SecondaryBoot)
        {
            u32BootTag = pCB->u32SecondaryBootTag; 
        }

        // Search drive info array 
        for (i=0; i<pCB->u32NumCopies; i++)
        {
            // Both chipNum and tag should match
            if (pCB->aDriveInfo[i].u32ChipNum == u32ChipNum &&
                pCB->aDriveInfo[i].u32Tag == u32BootTag)
            {
                // Found it!
                // return loc and size of firmware
                *pu32StartLoc = pCB->aDriveInfo[i].u32FirstSectorNumber;
                *pu32SectorCount = pCB->aDriveInfo[i].u32SectorCount;
                return SUCCESS;
            }
        }
    }
    // Failed to locate firmware drive in config block
    return ERROR_ROM_COMMON_DRIVER_INVALID_CONFIGBLOCK;
}
