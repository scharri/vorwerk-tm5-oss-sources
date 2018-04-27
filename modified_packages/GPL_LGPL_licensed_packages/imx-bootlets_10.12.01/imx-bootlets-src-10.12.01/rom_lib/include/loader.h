////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_loader
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    loader.h
//! \brief   Boot loader definitions.
//!
////////////////////////////////////////////////////////////////////////////////
#ifndef _LOADER_H
#define _LOADER_H

#include "itf_rom.h"
#include "dcp.h"


////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! Boot image signature in 32-bit little-endian format "PMTS"
#define BOOT_SIGNATURE  0x504d5453

//! These define file header flags
#define FFLG_DISPLAY_PROGRESS   0x0001

//! These define section header flags
#define SFLG_SECTION_BOOTABLE   0x0001

//! These define boot command flags
#define CFLG_LAST_TAG   0x01

//! These define the boot command tags
#define ROM_NOP_CMD     0x00
#define ROM_TAG_CMD     0x01
#define ROM_LOAD_CMD    0x02
#define ROM_FILL_CMD    0x03
#define ROM_JUMP_CMD    0x04
#define ROM_CALL_CMD    0x05
#define ROM_MODE_CMD    0x06

#define ROM_HAB_FLAG 0x1

//! Boot command definition
typedef struct _boot_cmd
{
    uint8_t checksum;           //!< 8-bit checksum over command chunk
    uint8_t tag;                //!< command tag (identifier)
    uint16_t flags;             //!< command flags (modifier)
    uint32_t address;           //!< address argument
    uint32_t count;             //!< count argument
    uint32_t data;              //!< data argument
}
boot_cmd_t;


//! Definition for boot image file header chunk 1
typedef struct _boot_hdr1
{
    uint32_t hash;              //!< last 32-bits of SHA-1 hash
    uint32_t signature;         //!< must equal "STMP"
    uint8_t major;              //!< major file format version
    uint8_t minor;              //!< minor file format version
    uint16_t fileFlags;         //!< global file flags
    uint32_t fileChunks;        //!< total chunks in the file
}
boot_hdr1_t;


//! Definition for boot image file header chunk 2
typedef struct _boot_hdr2
{
    uint32_t bootOffset;        //!< chunk offset to the first boot section
    uint32_t bootSectID;        //!< section ID of the first boot section
    uint16_t keyCount;          //!< number of keys in the key dictionary
    uint16_t keyOffset;         //!< chunk offset to the key dictionary
    uint16_t hdrChunks;         //!< number of chunks in the header
    uint16_t sectCount;         //!< number of sections in the image
}
boot_hdr2_t;


// Provides forward reference to the loader context definition.
typedef struct _ldr_Context ldr_Context_t;


//! Function pointer definition for all loader state and action functions.
typedef int (*pLdrFnc_t)(ldr_Context_t *);


//! Jump command function pointer definition.
typedef int (*pJumpFnc_t)(uint32_t);


//! Call command function pointer definition.
typedef int (*pCallFnc_t)(uint32_t, uint32_t *);


//! Loader context definition.
struct _ldr_Context
{
    uint32_t bootMode;          //!< current boot mode
    rom_BootItf_t *pBootItf;    //!< pointer to boot driver interface
    pLdrFnc_t State;            //!< pointer to loader state function
    pLdrFnc_t Action;           //!< pointer to loader action function
    int getCnt;                 //!< number of chunks to get
    int gotCnt;                 //!< number of chunks received
    uint32_t fileChunks;        //!< chunks remaining in file
    uint32_t sectChunks;        //!< chunks remaining in section
    uint16_t fileFlags;         //!< file header flags
    uint16_t keyCount;          //!< number of keys in the key dictionary
    uint32_t objectID;          //!< ID of the current boot section or image
    uint32_t dcpStart;          //!< dcp start time in microseconds
    hw_dcp_packet1_t dcpPkt1;   //!< dcp packet 1 value to use for boot sections
    hw_dcp_packet_t *pDcp;      //!< pointer to current dcp work packet
    uint32_t crc32;             //!< crc calculated over load command payload
    chunk_t initVector;         //!< decryption initialization vector
    chunk_t scratchPad;         //!< chunk size scratch pad area
    boot_cmd_t bootCmd;         //!< current boot command
    bool bIsImageAuthenticated;    //!< state variable to track if first call cmd executed
};


//! Loader interface definition.
typedef struct _loader_Itf
{
    ldr_Context_t *pCtx;                    //!< pointer to loader context
    int (*StatePump)(void);                 //!< state pump function
    int (*StateInit)(ldr_Context_t *);      //!< initialization state function
    int (*StateMove)(ldr_Context_t *);      //!< move state function
    int (*StateAction)(ldr_Context_t *);    //!< action state function
    int (*DoHeader)(ldr_Context_t *);       //!< header action function
    int (*DoCommand)(ldr_Context_t *);      //!< command action function
}
loader_Itf_t;


////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////

extern ldr_Context_t g_ldrCtx;
extern hw_dcp_packet_t g_ldrDcp;
extern const loader_Itf_t rom_LoaderItf;


////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

void ldr_Entry(uint32_t mode);
int ldr_DoBoot(uint32_t mode);


#endif // _LOADER_H
//! @}
