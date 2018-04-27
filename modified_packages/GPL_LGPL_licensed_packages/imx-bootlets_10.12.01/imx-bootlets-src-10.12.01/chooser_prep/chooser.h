/*
 * Chooser Prep common file
 *
 */

#ifndef _CHOOSER_H_
#define _CHOOSER_H_


// persistant register 1
#define TM41_BM_ROM_REDUNDANT_BOOT		0x00000002	// ROM loader sets this if wrong headers, also Image Chooser uses that to switch between images
#define TM41_BM_NAND_SDK_BLOCK_REWRITE		0x00000004	// ROM loader sets this if ECC threshold for any BCB block reached

// persistant register 2
#define TM41_BM_UPDATE_SWITCH_TO_0		0x00000001	// update.sh => Image Chooser 0 => app.sh (update phase 1 finished and trying switch to image 0)
#define TM41_BM_UPDATE_SWITCH_TO_1		0x00000002	// update.sh => Image Chooser 0 => app.sh (update phase 1 finished and trying switch to image 1)

// update status
#define STATUS_NO_UPDATE_PENDING			0
#define STATUS_UPDATE_PENDING_TO_0			1
#define STATUS_UPDATE_PENDING_TO_1			2
#define STATUS_UPDATE_PENDING_TO_0_PHASE1_COMPLETED	3
#define STATUS_UPDATE_PENDING_TO_1_PHASE1_COMPLETED	4
#define STATUS_UPDATE_PENDING_TO_0_PHASE2_REACHED	5
#define STATUS_UPDATE_PENDING_TO_1_PHASE2_REACHED	6
#define STATUS_RECOVERY_CLONING_STARTED			7


#endif