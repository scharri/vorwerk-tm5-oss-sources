////////////////////////////////////////////////////////////////////////////////
//! \addtogroup rom_loader
//! @{
//!
//  Copyright (c) 2006 SigmaTel, Inc.
//!
//! \file    dcp.h
//! \brief   Exports an API for using the Encore Data Co-Processor (DCP) block.
//!
////////////////////////////////////////////////////////////////////////////////
#ifndef _DCP_H
#define _DCP_H

#include "registers\regsdcp.h"


////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! DCP work packet 1 value used to calculate CBC-MAC over the image header
#define DCP_PKT1_HEADER \
   (BM_DCP_PACKET1_DECR_SEMAPHORE | \
    BM_DCP_PACKET1_ENABLE_CIPHER | \
    BM_DCP_PACKET1_CIPHER_ENCRYPT | \
    BM_DCP_PACKET1_CIPHER_INIT | \
    BM_DCP_PACKET1_OTP_KEY)

//! DCP work packet 1 value used to decrypt DEK in key dictionary
#define DCP_PKT1_DICTIONARY \
   (BM_DCP_PACKET1_DECR_SEMAPHORE | \
    BM_DCP_PACKET1_ENABLE_CIPHER | \
    BM_DCP_PACKET1_CIPHER_INIT | \
    BM_DCP_PACKET1_OTP_KEY)

//! DCP work packet 1 value used to decrypt image section data
#define DCP_PKT1_CIPHERTEXT \
   (BM_DCP_PACKET1_DECR_SEMAPHORE | \
    BM_DCP_PACKET1_ENABLE_CIPHER | \
    BM_DCP_PACKET1_CIPHER_INIT)

//! DCP work packet 1 value used for plaintext image data
#define DCP_PKT1_PLAINTEXT \
   (BM_DCP_PACKET1_DECR_SEMAPHORE | \
    BM_DCP_PACKET1_ENABLE_MEMCOPY)

//! DCP work packet 1 bit mask used to start CRC32 hash
#define BM_DCP_PKT1_CRC_INIT \
   (BM_DCP_PACKET1_ENABLE_HASH | \
    BM_DCP_PACKET1_HASH_INIT | \
    BM_DCP_PACKET1_HASH_OUTPUT)

//! DCP work packet 1 bit mask used to terminate CRC32 hash
#define BM_DCP_PKT1_CRC_TERM \
   (BM_DCP_PACKET1_ENABLE_HASH | \
    BM_DCP_PACKET1_HASH_TERM | \
    BM_DCP_PACKET1_HASH_OUTPUT)

//! DCP work packet 1 bit mask of fields to clear after each dcp transaction
#define BM_DCP_PKT1_AUTOCLEAR \
   (BM_DCP_PACKET1_CIPHER_INIT | \
    BM_DCP_PACKET1_HASH_INIT | \
    BM_DCP_PACKET1_HASH_TERM)

//! DCP work packet 2 value used for entire image
#define DCP_PKT2_VALUE \
   (BV_FLD(DCP_PACKET2, CIPHER_SELECT, AES128) | \
    BV_FLD(DCP_PACKET2, CIPHER_MODE, CBC) | \
    BV_FLD(DCP_PACKET2, HASH_SELECT, CRC32))

//! DCP channel status bit mask of error fields checked by the rom. No need to
//! test for hash mismatch since this function is not used.
#define BM_DCP_CHnSTAT_ERRORS \
   (BM_DCP_CHnSTAT_ERROR_SETUP | \
    BM_DCP_CHnSTAT_ERROR_PACKET | \
    BM_DCP_CHnSTAT_ERROR_SRC | \
    BM_DCP_CHnSTAT_ERROR_DST)


//! DCP (decryption) work packet definition
typedef struct _hw_dcp_packet
{
    void *pNext;                //!< next dcp work packet address
    hw_dcp_packet1_t pkt1;      //!< dcp work packet 1 (control 0)
    hw_dcp_packet2_t pkt2;      //!< dcp work packet 2 (control 1)
    uint8_t *pSrc;              //!< source buffer address
    uint8_t *pDst;              //!< destination buffer address
    uint32_t size;              //!< buffer size in bytes
    uint8_t *pPayload;          //!< payload buffer address
    hw_dcp_chnstat_t stat;      //!< dcp status (written by dcp)
}
hw_dcp_packet_t;


////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

void dcp_Init(void);
void dcp_LoadKey0(uint32_t *pKey);
void dcp_StartMove(hw_dcp_packet_t *pPkt);
uint32_t dcp_GetBusyStatus(void);
uint32_t dcp_GetErrorStatus(void);


#endif // _DCP_H
//! @}
