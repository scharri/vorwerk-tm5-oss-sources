/** @file mlan_usb.c
 *
 *  @brief This file contains USB specific code
 * 
 *  (C) Copyright 2011 Marvell International Ltd.
 *  All Rights Reserved
 *
 *  MARVELL CONFIDENTIAL
 *  Copyright 2008 ~ 2011 Marvell International Ltd All Rights Reserved.
 *  The source code contained or described herein and all documents related to
 *  the source code ("Material") are owned by Marvell International Ltd or its
 *  suppliers or licensors. Title to the Material remains with Marvell International Ltd
 *  or its suppliers and licensors. The Material contains trade secrets and
 *  proprietary and confidential information of Marvell or its suppliers and
 *  licensors. The Material is protected by worldwide copyright and trade secret
 *  laws and treaty provisions. No part of the Material may be used, copied,
 *  reproduced, modified, published, uploaded, posted, transmitted, distributed,
 *  or disclosed in any way without Marvell's prior express written permission.
 * 
 *  No license under any patent, copyright, trade secret or other intellectual
 *  property right is granted to or conferred upon you by disclosure or delivery
 *  of the Materials, either expressly, by implication, inducement, estoppel or
 *  otherwise. Any license under such intellectual property rights must be
 *  express and approved by Marvell in writing.
 *
 */

/********************************************************
Change log:
    04/21/2009: initial version
********************************************************/

#include "mlan.h"
#ifdef STA_SUPPORT
#include "mlan_join.h"
#endif
#include "mlan_util.h"
#include "mlan_init.h"
#include "mlan_fw.h"
#include "mlan_main.h"

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/

/********************************************************
        Local Functions
********************************************************/

/**
 *  @brief  This function downloads FW blocks to device
 *
 *  @param pmadapter	A pointer to mlan_adapter
 *  @param pmfw			A pointer to firmware image
 *
 *  @return             MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
static mlan_status
wlan_prog_fw_w_helper(IN pmlan_adapter pmadapter, IN pmlan_fw_image pmfw)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    pmlan_callbacks pcb = &pmadapter->callbacks;
    t_u8 *firmware = pmfw->pfw_buf, *RecvBuff;
    t_u32 retries = MAX_FW_RETRY, DataLength;
    t_u32 FWSeqNum = 0, TotalBytes = 0, DnldCmd = 0;
    FWData *fwdata = MNULL;
    FWSyncHeader SyncFWHeader;

    ENTER();

    if (!firmware && !pcb->moal_get_fw_data) {
        PRINTM(MMSG, "No firmware image found! Terminating download\n");
        ret = MLAN_STATUS_FAILURE;
        goto fw_exit;
    }

    /* Allocate memory for transmit */
    ret =
        pcb->moal_malloc(pmadapter->pmoal_handle, FW_DNLD_TX_BUF_SIZE,
                         MLAN_MEM_DEF | MLAN_MEM_DMA, (t_u8 **) & fwdata);
    if ((ret != MLAN_STATUS_SUCCESS) || !fwdata) {
        PRINTM(MERROR, "Could not allocate buffer for FW download\n");
        goto fw_exit;
    }

    /* Allocate memory for receive */
    ret =
        pcb->moal_malloc(pmadapter->pmoal_handle, FW_DNLD_RX_BUF_SIZE,
                         MLAN_MEM_DEF | MLAN_MEM_DMA, &RecvBuff);
    if ((ret != MLAN_STATUS_SUCCESS) || !RecvBuff) {
        PRINTM(MERROR, "Could not allocate buffer for FW download response\n");
        goto cleanup;
    }

    do {
        /* Copy the header of the firmware data to get the length */
        if (firmware)
            memcpy(pmadapter, &fwdata->fw_header, &firmware[TotalBytes],
                   sizeof(FWHeader));
        else
            pcb->moal_get_fw_data(pmadapter->pmoal_handle,
                                  TotalBytes, sizeof(FWHeader),
                                  (t_u8 *) & fwdata->fw_header);

        DataLength = wlan_le32_to_cpu(fwdata->fw_header.data_length);
        DnldCmd = wlan_le32_to_cpu(fwdata->fw_header.dnld_cmd);
        TotalBytes += sizeof(FWHeader);

        /* Copy the firmware data */
        if (firmware)
            memcpy(pmadapter, fwdata->data, &firmware[TotalBytes], DataLength);
        else
            pcb->moal_get_fw_data(pmadapter->pmoal_handle,
                                  TotalBytes, DataLength,
                                  (t_u8 *) fwdata->data);

        fwdata->seq_num = wlan_cpu_to_le32(FWSeqNum);
        TotalBytes += DataLength;

        /* If the send/receive fails or CRC occurs then retry */
        while (retries) {
            mlan_buffer mbuf;
            int length = FW_DATA_XMIT_SIZE;
            retries--;

            memset(pmadapter, &mbuf, 0, sizeof(mlan_buffer));
            mbuf.pbuf = (t_u8 *) fwdata;
            mbuf.data_len = length;

            /* Send the firmware block */
            if ((ret = pcb->moal_write_data_sync(pmadapter->pmoal_handle,
                                                 &mbuf, MLAN_USB_EP_CMD_EVENT,
                                                 MLAN_USB_BULK_MSG_TIMEOUT)) !=
                MLAN_STATUS_SUCCESS) {
                PRINTM(MERROR, "fw_dnld: write_data failed, ret %d\n", ret);
                continue;
            }

            memset(pmadapter, &mbuf, 0, sizeof(mlan_buffer));
            mbuf.pbuf = RecvBuff;
            mbuf.data_len = FW_DNLD_RX_BUF_SIZE;

            /* Receive the firmware block response */
            if ((ret = pcb->moal_read_data_sync(pmadapter->pmoal_handle,
                                                &mbuf, MLAN_USB_EP_CMD_EVENT,
                                                MLAN_USB_BULK_MSG_TIMEOUT)) !=
                MLAN_STATUS_SUCCESS) {
                PRINTM(MERROR, "fw_dnld: read_data failed, ret %d\n", ret);
                continue;
            }

            memcpy(pmadapter, &SyncFWHeader, RecvBuff, sizeof(FWSyncHeader));
            endian_convert_syncfwheader(&SyncFWHeader);

            /* Check the firmware block response for CRC errors */
            if (SyncFWHeader.cmd) {
                PRINTM(MERROR, "FW received Blk with CRC error 0x%x\n",
                       SyncFWHeader.cmd);
                ret = MLAN_STATUS_FAILURE;
                continue;
            }

            retries = MAX_FW_RETRY;
            break;
        }

        FWSeqNum++;
        PRINTM(MINFO, ".\n");

    } while ((DnldCmd != FW_HAS_LAST_BLOCK) && retries);

  cleanup:
    PRINTM(MINFO, "fw_dnld: %d bytes downloaded\n", TotalBytes);

    if (RecvBuff)
        pcb->moal_mfree(pmadapter->pmoal_handle, RecvBuff);
    if (fwdata)
        pcb->moal_mfree(pmadapter->pmoal_handle, (t_u8 *) fwdata);
    if (retries) {
        ret = MLAN_STATUS_SUCCESS;
    }

  fw_exit:
    LEAVE();
    return ret;
}

/**
 *  @brief  This function receives and checks FW ready event
 *
 *  @param pmadapter		A pointer to mlan_adapter
 *
 *  @return		MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
static mlan_status
wlan_check_fw_ready_event(pmlan_adapter pmadapter)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    pmlan_callbacks pcb = &pmadapter->callbacks;
    t_u8 *event_buf = MNULL;
    t_u32 event_sig, event_id;
    mlan_buffer mbuf;

    ENTER();

    /* Allocate memory for receive */
    ret =
        pcb->moal_malloc(pmadapter->pmoal_handle, MAX_EVENT_SIZE,
                         MLAN_MEM_DEF | MLAN_MEM_DMA, &event_buf);
    if ((ret != MLAN_STATUS_SUCCESS) || !event_buf) {
        PRINTM(MERROR, "Could not allocate buffer for event buf\n");
        goto done;
    }

    memset(pmadapter, &mbuf, 0, sizeof(mlan_buffer));
    mbuf.pbuf = (t_u8 *) event_buf;
    mbuf.data_len = MAX_EVENT_SIZE;
    mbuf.buf_type = MLAN_BUF_TYPE_EVENT;

    /* Wait for FW ready event, timeout 2-second */
    if ((ret = pcb->moal_read_data_sync(pmadapter->pmoal_handle,
                                        &mbuf, MLAN_USB_EP_CMD_EVENT,
                                        MLAN_USB_BULK_MSG_TIMEOUT * 20)) !=
        MLAN_STATUS_SUCCESS) {
        PRINTM(MERROR, "Reading FW_READY event failed: %d\n", ret);
    } else {
        event_sig = wlan_le32_to_cpu(*((t_u32 *) event_buf));
        event_id = wlan_le32_to_cpu(*((t_u32 *) event_buf + 1));

        if (event_sig == MLAN_USB_TYPE_EVENT && event_id == EVENT_INIT_DONE) {
            PRINTM(MINFO, "FW_READY event is received: 0x%x - 0x%x\n",
                   event_sig, event_id);
        } else {
            PRINTM(MERROR, "FW_READY event is not received: 0x%x - 0x%x\n",
                   event_sig, event_id);
            ret = MLAN_STATUS_FAILURE;
        }
    }
  done:
    if (event_buf)
        pcb->moal_mfree(pmadapter->pmoal_handle, event_buf);

    LEAVE();
    return ret;
}

/********************************************************
        Global Functions
********************************************************/

/**
 *  @brief  This function downloads firmware to card
 *
 *  @param pmadapter	A pointer to mlan_adapter
 *  @param pmfw			A pointer to firmware image
 *
 *  @return		MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
wlan_dnld_fw(IN pmlan_adapter pmadapter, IN pmlan_fw_image pmfw)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    ret = wlan_prog_fw_w_helper(pmadapter, pmfw);
    if (ret != MLAN_STATUS_SUCCESS) {
        LEAVE();
        return MLAN_STATUS_FAILURE;
    }

    ret = wlan_check_fw_ready_event(pmadapter);

    LEAVE();
    return ret;
}
