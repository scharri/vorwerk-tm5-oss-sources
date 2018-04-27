////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    persistent_bit.c
//! \brief   load the persistent bits into shadow registers and save pb off.
//!
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//!   Includes and external references
////////////////////////////////////////////////////////////////////////////////
#include "rom_types.h"
#include "persistent_bit.h"
#include "debug.h"

#include <regsdigctl.h>
#include <rom_utils.h>

#define MAX_PERSISTENT_WAIT_USEC         3100   // uSeconds to wait for valid persistent data.

////////////////////////////////////////////////////////////////////////////////
//! Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! \brief Load ROM eFuse persistent into shadow RAM memory for ROM usage
//!
//! \fntype Function
//!
//! This function will initialize the persistent bit shadow registers.  Even
//! though this happens automatically upon startup, upon exiting this function,
//! these registers should be ready to read..
//!
//! \param[in]  pu32PersistentWord - which word to read.
//! \param[out]  pu32Result - where to store the result.
//!
//! \return Status of the operation.
//!
//! \post The Persistent bit shadow registers have been loaded.
//!
//!
//! \internal
//! 
////////////////////////////////////////////////////////////////////////////////
RtStatus_t load_persistent_registers(volatile reg32_t * pu32PersistentWord, 
                                     uint32_t * pu32Result)
{
    int i;
    volatile uint32_t u32PerBitStaleRegs=1, u32StartTime;
    
    DBG_PRINTF("load rom persistent bit shadows\n");

    // Ensure the RTC is out of reset.
    HW_RTC_CTRL_CLR(BM_RTC_CTRL_SFTRST | BM_RTC_CTRL_CLKGATE);
    
    for (i=0; ((i<2) && u32PerBitStaleRegs); i++)
    {    
        u32StartTime = ROM_GET_TIME_USEC();
        DBG_PRINTF("Start Time %d\n", u32StartTime);
        DBG_PRINTF("Persistent Status register = 0x%X\n", HW_RTC_STAT_RD());
        // Update data from RTC persistent memory.  Wait a maximum of 3100msec.
        do        
        {
            u32PerBitStaleRegs = (((HW_RTC_STAT_RD() & BM_RTC_STAT_STALE_REGS) >>
                                    BP_RTC_STAT_STALE_REGS) & HW_RTC_ROM_USE_MASK); 
        }
        while(u32PerBitStaleRegs && 
              (!ROM_TIME_USEC_EXPIRED(u32StartTime, MAX_PERSISTENT_WAIT_USEC)));

        // Check to see if we timed out.
        if (u32PerBitStaleRegs)
        {   // if so try again after forcing the update.
            DBG_PRINTF("End -> Force Time %d\n", ROM_GET_TIME_USEC());
            DBG_PRINTF("Persistent Status register = 0x%X\n", HW_RTC_STAT_RD());
            HW_RTC_CTRL_SET(BM_RTC_CTRL_SFTRST);

            // At 24Mhz, it takes no more than 4 clocks (160 ns) Maximum for 
            // the part to reset, reading the rtc register twice should
            // be sufficient to get 4 clks delay.
            HW_RTC_CTRL_RD();
            HW_RTC_CTRL_RD();
           
            HW_RTC_CTRL_CLR(BM_RTC_CTRL_SFTRST | BM_RTC_CTRL_CLKGATE);
            // Force an update.
            //BW_RTC_CTRL_FORCE_UPDATE(1);
        }
        else    // if we didn't time out, exit.           
            break;
    }

    // Determine return code.
    if (u32PerBitStaleRegs)
    {
        return ERROR_ROM_STARTUP_UNABLE_TO_LOAD_PERSISTENT_WORD;
    } else
    {
        *pu32Result = *pu32PersistentWord;
        DBG_PRINTF("Persistent Register = 0x%X\n", *pu32PersistentWord);
        DBG_PRINTF("End Time %d\n", ROM_GET_TIME_USEC());
        return SUCCESS;
    }

}

////////////////////////////////////////////////////////////////////////////////
//! \brief Set ROM persistent data in shadow memory for ROM usage
//!
//! \fntype Function
//!
//! This function will store a value in the persistent bit shadow registers.
//! To ensure we don't lose the current value, we need to wait until any copies-
//! in-progress are finished.
//! To only set a bit, use the HW_RTC_PERSISTENTx_SET register.
//! To only clear a bit, use the HW_RTC_PERSISTENTx_CLR register.
//!
//! \param[in]  pu32PersistentWord Which persistent word to write to (1-5).
//! \param[in]  u32Data Data to be written out.
//!
//! \return Status of the operation.
//!
//! \post The persistent value has been transferred to the analog side.
//!
//!
//! \internal Note that Persistent0 should not be used.  Saved for analog.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t write_persistent_word(volatile reg32_t * pu32PersistentWord, 
                                 uint32_t u32Data)
{
    int i;
    volatile uint32_t u32PerBitNewRegs, u32StartTime;
    
    DBG_PRINTF("Write Persistent Reg \n");

    // Ensure the RTC is out of reset.
    HW_RTC_CTRL_CLR(BM_RTC_CTRL_SFTRST | BM_RTC_CTRL_CLKGATE);
    
    // Run through this loop a couple of times.
    for (i=0; i<2; i++)
    {    
        // Grab the current time.
        u32StartTime = ROM_GET_TIME_USEC();
        u32PerBitNewRegs = 1;

        // Ensure data in ROM shadow memory is fresh.  Wait a maximum 
        // of MAX_PERSISTENT_WAIT_USEC (3.1msec)
        // We must check BOTH the New and Stale bits!!
        while(u32PerBitNewRegs && 
              (!ROM_TIME_USEC_EXPIRED(u32StartTime, MAX_PERSISTENT_WAIT_USEC))) 
        {        
            u32PerBitNewRegs = ((HW_RTC_STAT_RD() & 
                                (BM_RTC_STAT_NEW_REGS | BM_RTC_STAT_STALE_REGS)));
          
        }

        //DBG_PRINTF("RTC STAT register 0x%X\n", HW_RTC_STAT_RD());

        // Check to see if we timed out.
        if (u32PerBitNewRegs)
        {   // if so try again after forcing the update.
            DBG_PRINTF("RTC STAT register 0x%X\n", HW_RTC_STAT_RD());
            HW_RTC_CTRL_SET(BM_RTC_CTRL_SFTRST);
            
            // At 24Mhz, it takes no more than 4 clocks (160 ns) Maximum for 
            // the part to reset, reading the rtc register twice should
            // be sufficient to get 4 clks delay.
            HW_RTC_CTRL_RD();
            HW_RTC_CTRL_RD();
            
            HW_RTC_CTRL_CLR(BM_RTC_CTRL_SFTRST | BM_RTC_CTRL_CLKGATE);
            DBG_PRINTF("Timeout Force update\n");
            // Force an update.
            //BW_RTC_CTRL_FORCE_UPDATE(1);
            continue;
        } else
        {           
            // Store the data.
            *pu32PersistentWord = u32Data;            
        }
        // Grab the current time.
        u32StartTime = ROM_GET_TIME_USEC();

        // Force the loop below to run at least 1 time.
        u32PerBitNewRegs = 1;

        // Copy data into ROM shadow memory.  Wait a maximum of
        // MAX_PERSISTENT_WAIT_USEC (3.1msec)
        while(u32PerBitNewRegs && 
              (!ROM_TIME_USEC_EXPIRED(u32StartTime, MAX_PERSISTENT_WAIT_USEC))) 
        {        
            u32PerBitNewRegs = (HW_RTC_STAT_RD() & BM_RTC_STAT_NEW_REGS); 
          
        }

        //DBG_PRINTF("RTC STAT register 0x%X\n", HW_RTC_STAT_RD());
        //DBG_PRINTF("End Time %d\n", ROM_GET_TIME_USEC());
        // Force an update - state machine implementation errata requires this.
        //BW_RTC_CTRL_FORCE_UPDATE(1);
        //load_persistent_rom_shadows();
        // Check to see if we timed out.  Break out of loop if not.
        if (!u32PerBitNewRegs ) 
        {
            return SUCCESS;
        }
    }

    return ERROR_ROM_STARTUP_UNABLE_TO_SET_PERSISTENT_WORD;

}

// Example unit test snippet.
/*
    {
        uint32_t u32Data;
        RtStatus_t retStatus;
        initialize_RTC();
        DBG_PRINTF("Persistent1 register = 0x%X\n", HW_RTC_PERSISTENT1_RD());
        DBG_PRINTF("Persistent1 load register start time = %d\n",HW_DIGCTL_MICROSECONDS_RD());
        retStatus = load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32Data);
        DBG_PRINTF("Persistent1 load register end time = %d\n",HW_DIGCTL_MICROSECONDS_RD());
        DBG_PRINTF("Persistent1 register from routine = 0x%X ; 0x%X\n", u32Data, retStatus);
        DBG_PRINTF("Setting 0xAA55AA55 in Persistent2 register\n");
        DBG_PRINTF("Persistent1 set register start time = %d\n",HW_DIGCTL_MICROSECONDS_RD());
        retStatus = write_persistent_word(HW_RTC_ROM_PERSISTENT_BITS1_SET, 0xAA55AA55);
        DBG_PRINTF("Persistent1 set register end time = %d\n",HW_DIGCTL_MICROSECONDS_RD());
        DBG_PRINTF("Persistent1 register just read = 0x%X ; 0x%X\n", HW_RTC_PERSISTENT1_RD(), retStatus);
        DBG_PRINTF("Persistent1 load register start time = %d\n",HW_DIGCTL_MICROSECONDS_RD());
        retStatus = load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32Data);
        DBG_PRINTF("Persistent1 register from routine = 0x%X ; 0x%X\n", u32Data, retStatus);
        DBG_PRINTF("Persistent1 load register end time = %d\n",HW_DIGCTL_MICROSECONDS_RD());
        DBG_PRINTF("Clearing 0x0000AA55 in Persistent1 register\n");
        retStatus = write_persistent_word(HW_RTC_ROM_PERSISTENT_BITS1_CLR, 0x0000AA55);
        DBG_PRINTF("Persistent1 register just read = 0x%X ; 0x%X\n", HW_RTC_PERSISTENT1_RD(), retStatus);
        DBG_PRINTF("Persistent1 load register start time = %d\n",HW_DIGCTL_MICROSECONDS_RD());
        retStatus = load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32Data);
        DBG_PRINTF("Persistent1 load register end time = %d\n",HW_DIGCTL_MICROSECONDS_RD());
        DBG_PRINTF("Persistent1 register from routine = 0x%X ; 0x%X\n", u32Data, retStatus);
        DBG_PRINTF("Writing 0xFF00AA00 to Persistent1 register\n");
        retStatus = write_persistent_word(HW_RTC_ROM_PERSISTENT_BITS1, 0xFF00AA00);
        DBG_PRINTF("Persistent1 register just read = 0x%X ; 0x%X\n", HW_RTC_PERSISTENT1_RD(), retStatus);
        retStatus = load_persistent_registers(HW_RTC_ROM_PERSISTENT_BITS1, &u32Data);
        DBG_PRINTF("Persistent1 register from routine = 0x%X ; 0x%X\n", u32Data, retStatus);
    }
*/
// eof persistent_bit.c
//! @}
