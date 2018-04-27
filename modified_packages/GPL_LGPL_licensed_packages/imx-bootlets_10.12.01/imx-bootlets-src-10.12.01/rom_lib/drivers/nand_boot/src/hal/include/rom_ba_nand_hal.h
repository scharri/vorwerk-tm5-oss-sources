#ifndef __ROM_BA_NAND_HAL_H
#define __ROM_BA_NAND_HAL_H

typedef enum {
    eBANandReset                        = 0xFF,
    eBANandLBAAbort                     = 0xCA,
    eBANandReadId                       = 0x90,
    eBANandReadParameterPage            = 0xEC,
    eBANandReadUniqueId                 = 0xED,
    eBANandGetFeatures                  = 0xEE,
    eBANandSetFeatures                  = 0xEF,
    eBANandReadStatus                   = 0x70,
    eBANandLBARead                      = 0xC0,
    eBANandLBAReadContinue              = 0xC8,
    eBANandLBAReadMetadata              = 0xC0,
    eBANandLBAReadMode                  = 0xC0,
    eBANandReadMode                     = 0x00,
    eBANandLBADeallocate                = 0xC3,
    eBANandLBAFlush                     = 0xC9,
    eBANandLBAWrite                     = 0xC1,
    eBANandLBAWriteContinue             = 0xC2
} BANAND_COMMAND_SET__CYCLE1;

typedef enum {
    eBANandLBAReadCycle2                = 0x30,
    eBANandLBAReadMetadataCycle2        = 0x31,
    eBANandLBADeallocateCycle2          = 0x10,
    eBANandLBAWriteCycle2               = 0x10,
    eBANandLBAWriteContinueCycle2       = 0x10
} BANAND_COMMAND_SET__CYCLE2;

#define BANAND_ROW_ADDRESS_BYTES (5)
#endif //__ROM_BA_NAND_HAL_H
