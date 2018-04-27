#include "dat_common.h"
#include "hab.h"
#include "hab_types.h"
#include "hab_cmd.h"

#ifdef PITC_CALL
extern int32_t pitc_entry(uint32_t arg, uint32_t *pID);
#else
extern int start(uint32_t arg);
#endif

#pragma ghs section data=".ivt"
struct hab_ivt input_ivt ={
    /** @ref hdr word with tag #HAB_TAG_IVT, length and HAB version fields
     *  (see @ref data)
     */
    IVT_HDR(sizeof(struct hab_ivt), HAB_VER(4, 0)),
    /** Absolute address of the first instruction to execute from the
     *  image
     */
#ifdef PITC_CALL
    (hab_image_entry_f) (&pitc_entry),
#else
    (hab_image_entry_f) (&start),
#endif
    /** Reserved in this version of HAB: should be NULL. */
    NULL,
    /** Absolute address of the image DCD: may be NULL. */
    NULL,
    /** Absolute address of the Boot Data: may be NULL, but not interpreted
     *  any further by HAB
     */
    NULL,
    /** Absolute address of the IVT.*/
    (const void*) (&input_ivt),
    /** Absolute address of the image CSF.*/
    (const void*) 0x2800,
    /** Reserved in this version of HAB: should be zero. */
    0
};

#pragma ghs section data=default

#pragma ghs section data=".length"
uint32_t input_bytes[4] = {
    0x4000, 0, 0, 0
};
#pragma ghs section data=default

#pragma ghs section data="dcd"
uint8_t input_dcd[] = {
      /* DCD header */
      DCD_HDR(HDR_BYTES + 3 * WRT_DAT_BYTES, HAB_VER(4,0)),

      /* first DCD write command - 32-bit write */
      WRT_DAT((0|0), HAB_DATA_WIDTH_WORD, 
              0x3F00, // 32-bit aligned, OCRAM address
              DCD_INPUT_32_BIT),
            
      /* second DCD write command - 16-bit write */   
      WRT_DAT((0|0), HAB_DATA_WIDTH_HALF, 
              0x3F00 + 6, // 16-bit aligned
              DCD_INPUT_16_BIT),
            
      /* third DCD write command - 8-bit write */
      WRT_DAT((0|0), HAB_DATA_WIDTH_BYTE, 
              0x3F00 + 9, // 8-bit aligned
              DCD_INPUT_8_BIT),
};

#pragma ghs section data=default

