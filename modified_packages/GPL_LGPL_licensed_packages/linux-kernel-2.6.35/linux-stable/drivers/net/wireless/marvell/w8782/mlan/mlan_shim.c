/** @file mlan_shim.c
 *  
 *  @brief This file contains APIs to MOAL module.
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

/**
 *  @mainpage MLAN Driver
 *
 *  @section overview_sec Overview
 *
 *  The MLAN is an OS independent WLAN driver for Marvell 802.11
 *  embedded chipset.
 * 
 *  @section copyright_sec Copyright
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
    10/13/2008: initial version
********************************************************/

#include "mlan.h"
#ifdef STA_SUPPORT
#include "mlan_join.h"
#endif
#include "mlan_util.h"
#include "mlan_fw.h"
#include "mlan_main.h"
#include "mlan_wmm.h"
#ifdef UAP_SUPPORT
#include "mlan_uap.h"
#endif

/********************************************************
                Local Variables
********************************************************/

/********************************************************
        Global Variables
********************************************************/
#ifdef STA_SUPPORT
mlan_operations mlan_sta_ops = {
    /* init cmd handler */
    wlan_ops_sta_init_cmd,
    /* ioctl handler */
    wlan_ops_sta_ioctl,
    /* cmd handler */
    wlan_ops_sta_prepare_cmd,
    /* cmdresp handler */
    wlan_ops_sta_process_cmdresp,
    /* rx handler */
    wlan_ops_sta_process_rx_packet,
    /* Event handler */
    wlan_ops_sta_process_event,
    /* txpd handler */
    wlan_ops_sta_process_txpd,
    /* BSS role: STA */
    MLAN_BSS_ROLE_STA,
};
#endif
#ifdef UAP_SUPPORT
mlan_operations mlan_uap_ops = {
    /* init cmd handler */
    wlan_ops_uap_init_cmd,
    /* ioctl handler */
    wlan_ops_uap_ioctl,
    /* cmd handler */
    wlan_ops_uap_prepare_cmd,
    /* cmdresp handler */
    wlan_ops_uap_process_cmdresp,
    /* rx handler */
    wlan_ops_uap_process_rx_packet,
    /* Event handler */
    wlan_ops_uap_process_event,
    /* txpd handler */
    wlan_ops_uap_process_txpd,
    /* BSS role: uAP */
    MLAN_BSS_ROLE_UAP,
};
#endif

/** mlan function table */
mlan_operations *mlan_ops[] = {
#ifdef STA_SUPPORT
    &mlan_sta_ops,
#endif
#ifdef UAP_SUPPORT
    &mlan_uap_ops,
#endif
    MNULL,
};

/** Global moal_assert callback */
t_void(*assert_callback) (IN t_void * pmoal_handle, IN t_u32 cond) = MNULL;
#ifdef DEBUG_LEVEL1
/** Global moal_print callback */
t_void(*print_callback) (IN t_void * pmoal_handle,
                         IN t_u32 level, IN t_s8 * pformat, IN...) = MNULL;
#endif

/********************************************************
        Local Functions
*******************************************************/

/********************************************************
        Global Functions
********************************************************/

/**
 *  @brief This function registers MOAL to MLAN module.
 *  
 *  @param pmdevice        A pointer to a mlan_device structure
 *                         allocated in MOAL
 *  @param ppmlan_adapter  A pointer to a t_void pointer to store
 *                         mlan_adapter structure pointer as the context
 *
 *  @return                MLAN_STATUS_SUCCESS
 *                             The registration succeeded.
 *                         MLAN_STATUS_FAILURE
 *                             The registration failed.
 *
 * mlan_status mlan_register (
 *   IN pmlan_device     pmdevice,
 *   OUT t_void          **ppmlan_adapter
 * );
 *
 * Comments
 *   MOAL constructs mlan_device data structure to pass moal_handle and
 *   mlan_callback table to MLAN. MLAN returns mlan_adapter pointer to
 *   the ppmlan_adapter buffer provided by MOAL.
 * Headers:
 *   declared in mlan_decl.h
 * See Also
 *   mlan_unregister
 */
mlan_status
mlan_register(IN pmlan_device pmdevice, OUT t_void ** ppmlan_adapter)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    pmlan_adapter pmadapter = MNULL;
    pmlan_callbacks pcb = MNULL;
    t_u8 i = 0;
    t_u32 j = 0;

    MASSERT(pmdevice);
    MASSERT(ppmlan_adapter);
    MASSERT(pmdevice->callbacks.moal_print);
#ifdef DEBUG_LEVEL1
    print_callback = pmdevice->callbacks.moal_print;
#endif
    assert_callback = pmdevice->callbacks.moal_assert;

    ENTER();

    MASSERT(pmdevice->callbacks.moal_malloc);
    MASSERT(pmdevice->callbacks.moal_memset);
    MASSERT(pmdevice->callbacks.moal_memmove);

    /* Allocate memory for adapter structure */
    if ((pmdevice->callbacks.
         moal_malloc(pmdevice->pmoal_handle, sizeof(mlan_adapter), MLAN_MEM_DEF,
                     (t_u8 **) & pmadapter) != MLAN_STATUS_SUCCESS)
        || !pmadapter) {
        ret = MLAN_STATUS_FAILURE;
        goto exit_register;
    }

    pmdevice->callbacks.moal_memset(pmadapter, pmadapter,
                                    0, sizeof(mlan_adapter));

    pcb = &pmadapter->callbacks;

    /* Save callback functions */
    pmdevice->callbacks.moal_memmove(pmadapter->pmoal_handle, pcb,
                                     &pmdevice->callbacks,
                                     sizeof(mlan_callbacks));

    /* Assertion for all callback functions */
    MASSERT(pcb->moal_init_fw_complete);
    MASSERT(pcb->moal_shutdown_fw_complete);
    MASSERT(pcb->moal_send_packet_complete);
    MASSERT(pcb->moal_recv_packet);
    MASSERT(pcb->moal_recv_event);
    MASSERT(pcb->moal_ioctl_complete);
    MASSERT(pcb->moal_write_data_async);
    MASSERT(pcb->moal_recv_complete);
    MASSERT(pcb->moal_write_data_sync);
    MASSERT(pcb->moal_read_data_sync);
    MASSERT(pcb->moal_mfree);
    MASSERT(pcb->moal_memcpy);
    MASSERT(pcb->moal_memcmp);
    MASSERT(pcb->moal_get_system_time);
    MASSERT(pcb->moal_init_timer);
    MASSERT(pcb->moal_free_timer);
    MASSERT(pcb->moal_start_timer);
    MASSERT(pcb->moal_stop_timer);
    MASSERT(pcb->moal_init_lock);
    MASSERT(pcb->moal_free_lock);
    MASSERT(pcb->moal_spin_lock);
    MASSERT(pcb->moal_spin_unlock);

    /* Save pmoal_handle */
    pmadapter->pmoal_handle = pmdevice->pmoal_handle;

#ifdef MFG_CMD_SUPPORT
    pmadapter->init_para.mfg_mode = pmdevice->mfg_mode;
#endif
    pmadapter->init_para.ps_mode = pmdevice->ps_mode;
    if (pmdevice->max_tx_buf == MLAN_TX_DATA_BUF_SIZE_2K ||
        pmdevice->max_tx_buf == MLAN_TX_DATA_BUF_SIZE_4K ||
        pmdevice->max_tx_buf == MLAN_TX_DATA_BUF_SIZE_8K)
        pmadapter->init_para.max_tx_buf = pmdevice->max_tx_buf;
#ifdef STA_SUPPORT
    pmadapter->init_para.cfg_11d = pmdevice->cfg_11d;
#else
    pmadapter->init_para.cfg_11d = 0;
#endif

    pmadapter->priv_num = 0;
    for (i = 0; i < MLAN_MAX_BSS_NUM; i++) {
        pmadapter->priv[i] = MNULL;
        if (pmdevice->bss_attr[i].active == MTRUE) {
            /* For valid bss_attr, allocate memory for private structure */
            if ((pcb->
                 moal_malloc(pmadapter->pmoal_handle, sizeof(mlan_private),
                             MLAN_MEM_DEF,
                             (t_u8 **) & pmadapter->priv[i]) !=
                 MLAN_STATUS_SUCCESS)
                || !pmadapter->priv[i]) {
                ret = MLAN_STATUS_FAILURE;
                goto error;
            }

            pmadapter->priv_num++;
            memset(pmadapter, pmadapter->priv[i], 0, sizeof(mlan_private));

            pmadapter->priv[i]->adapter = pmadapter;

            /* Save bss_type, frame_type & bss_priority */
            pmadapter->priv[i]->bss_type =
                (t_u8) pmdevice->bss_attr[i].bss_type;
            pmadapter->priv[i]->frame_type =
                (t_u8) pmdevice->bss_attr[i].frame_type;
            pmadapter->priv[i]->bss_priority =
                (t_u8) pmdevice->bss_attr[i].bss_priority;
            if (pmdevice->bss_attr[i].bss_type == MLAN_BSS_TYPE_STA)
                pmadapter->priv[i]->bss_role = MLAN_BSS_ROLE_STA;
            else if (pmdevice->bss_attr[i].bss_type == MLAN_BSS_TYPE_UAP)
                pmadapter->priv[i]->bss_role = MLAN_BSS_ROLE_UAP;
#ifdef WFD_SUPPORT
            else if (pmdevice->bss_attr[i].bss_type == MLAN_BSS_TYPE_WFD)
                pmadapter->priv[i]->bss_role = MLAN_BSS_ROLE_STA;
#endif
            /* Save bss_index and bss_num */
            pmadapter->priv[i]->bss_index = i;
            pmadapter->priv[i]->bss_num = (t_u8) pmdevice->bss_attr[i].bss_num;

            /* init function table */
            for (j = 0; mlan_ops[j]; j++) {
                if (mlan_ops[j]->bss_role == GET_BSS_ROLE(pmadapter->priv[i])) {
                    memcpy(pmadapter, &pmadapter->priv[i]->ops, mlan_ops[j],
                           sizeof(mlan_operations));
                }
            }
        }
    }

    /* Initialize lock variables */
    if (wlan_init_lock_list(pmadapter) != MLAN_STATUS_SUCCESS) {
        ret = MLAN_STATUS_FAILURE;
        goto error;
    }

    /* Allocate memory for member of adapter structure */
    if (wlan_allocate_adapter(pmadapter)) {
        ret = MLAN_STATUS_FAILURE;
        goto error;
    }

    /* Initialize timers */
    if (wlan_init_timer(pmadapter) != MLAN_STATUS_SUCCESS) {
        ret = MLAN_STATUS_FAILURE;
        goto error;
    }
    /* Return pointer of mlan_adapter to MOAL */
    *ppmlan_adapter = pmadapter;

    goto exit_register;

  error:
    PRINTM(MINFO, "Leave mlan_register with error\n");
    /* Free timers */
    wlan_free_timer(pmadapter);
    /* Free adapter structure */
    wlan_free_adapter(pmadapter);
    /* Free lock variables */
    wlan_free_lock_list(pmadapter);
    for (i = 0; i < MLAN_MAX_BSS_NUM; i++) {
        if (pmadapter->priv[i])
            pcb->moal_mfree(pmadapter->pmoal_handle,
                            (t_u8 *) pmadapter->priv[i]);
    }
    pcb->moal_mfree(pmadapter->pmoal_handle, (t_u8 *) pmadapter);

  exit_register:
    LEAVE();
    return ret;
}

/**
 *  @brief This function unregisters MOAL from MLAN module.
 *  
 *  @param pmlan_adapter   A pointer to a mlan_device structure
 *                         allocated in MOAL
 *
 *  @return                MLAN_STATUS_SUCCESS
 *                             The deregistration succeeded.
 */
mlan_status
mlan_unregister(IN t_void * pmlan_adapter)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;
    pmlan_callbacks pcb;
    t_s32 i = 0;

    MASSERT(pmlan_adapter);

    ENTER();

    pcb = &pmadapter->callbacks;

    /* Free adapter structure */
    wlan_free_adapter(pmadapter);

    /* Free timers */
    wlan_free_timer(pmadapter);

    /* Free lock variables */
    wlan_free_lock_list(pmadapter);

    /* Free private structures */
    for (i = 0; i < pmadapter->priv_num; i++) {
        if (pmadapter->priv[i]) {
            pcb->moal_mfree(pmadapter->pmoal_handle,
                            (t_u8 *) pmadapter->priv[i]);
        }
    }

    /* Free mlan_adapter */
    pcb->moal_mfree(pmadapter->pmoal_handle, (t_u8 *) pmadapter);

    LEAVE();
    return ret;
}

/**
 *  @brief This function downloads the firmware
 *  
 *  @param pmlan_adapter   A pointer to a t_void pointer to store
 *                         mlan_adapter structure pointer
 *  @param pmfw            A pointer to firmware image
 *
 *  @return                MLAN_STATUS_SUCCESS
 *                             The firmware download succeeded.
 *                         MLAN_STATUS_FAILURE
 *                             The firmware download failed.
 */
mlan_status
mlan_dnld_fw(IN t_void * pmlan_adapter, IN pmlan_fw_image pmfw)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;

    ENTER();
    MASSERT(pmlan_adapter);

    if (pmfw) {
        /* Download helper/firmware */
        ret = wlan_dnld_fw(pmadapter, pmfw);
        if (ret != MLAN_STATUS_SUCCESS) {
            PRINTM(MERROR, "wlan_dnld_fw fail ret=0x%x\n", ret);
            LEAVE();
            return ret;
        }
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function pass init param to MLAN
 *
 *  @param pmlan_adapter  A pointer to a t_void pointer to store
 *                        mlan_adapter structure pointer
 *  @param pparam         A pointer to mlan_init_param structure
 *
 *  @return               MLAN_STATUS_SUCCESS
 *
 */
mlan_status
mlan_set_init_param(IN t_void * pmlan_adapter, IN pmlan_init_param pparam)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;

    ENTER();
    MASSERT(pmlan_adapter);

    /** Save cal data in MLAN */
    if ((pparam->pcal_data_buf) && (pparam->cal_data_len > 0)) {
        pmadapter->pcal_data = pparam->pcal_data_buf;
        pmadapter->cal_data_len = pparam->cal_data_len;
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function initializes the firmware
 *  
 *  @param pmlan_adapter   A pointer to a t_void pointer to store
 *                         mlan_adapter structure pointer
 *
 *  @return                MLAN_STATUS_SUCCESS
 *                             The firmware initialization succeeded.
 *                         MLAN_STATUS_PENDING
 *                             The firmware initialization is pending.
 *                         MLAN_STATUS_FAILURE
 *                             The firmware initialization failed.
 */
mlan_status
mlan_init_fw(IN t_void * pmlan_adapter)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;

    ENTER();
    MASSERT(pmlan_adapter);

    pmadapter->hw_status = WlanHardwareStatusInitializing;

    /* Initialize firmware, may return PENDING */
    ret = wlan_init_fw(pmadapter);
    PRINTM(MINFO, "wlan_init_fw returned ret=0x%x\n", ret);

    LEAVE();
    return ret;
}

/**
 *  @brief Shutdown firmware
 *
 *  @param pmlan_adapter	A pointer to mlan_adapter structure
 *
 *  @return			MLAN_STATUS_SUCCESS
 *                      		The firmware shutdown call succeeded.
 *				MLAN_STATUS_PENDING
 *      	                	The firmware shutdown call is pending.
 *				MLAN_STATUS_FAILURE
 *      	                	The firmware shutdown call failed.
 */
mlan_status
mlan_shutdown_fw(IN t_void * pmlan_adapter)
{
    mlan_status ret = MLAN_STATUS_PENDING;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;
    pmlan_buffer pmbuf;
    pmlan_callbacks pcb;
    t_s32 i = 0;

    ENTER();
    MASSERT(pmlan_adapter);
    /* mlan already shutdown */
    if (pmadapter->hw_status == WlanHardwareStatusNotReady)
        return MLAN_STATUS_SUCCESS;

    pmadapter->hw_status = WlanHardwareStatusClosing;
    /* wait for mlan_process to complete */
    if (pmadapter->mlan_processing) {
        PRINTM(MWARN, "mlan main processing is still running\n");
        return ret;
    }

    /* shut down mlan */
    PRINTM(MINFO, "Shutdown mlan...\n");

    /* Clean up priv structures */
    for (i = 0; i < pmadapter->priv_num; i++) {
        if (pmadapter->priv[i]) {
            wlan_free_priv(pmadapter->priv[i]);
        }
    }

    pcb = &pmadapter->callbacks;

    while ((pmbuf = (pmlan_buffer) util_dequeue_list(pmadapter->pmoal_handle,
                                                     &pmadapter->rx_data_queue,
                                                     pcb->moal_spin_lock,
                                                     pcb->moal_spin_unlock))) {
        pcb->moal_recv_complete(pmadapter->pmoal_handle, pmbuf,
                                MLAN_USB_EP_DATA, MLAN_STATUS_FAILURE);
    }

    if (pcb->moal_spin_lock(pmadapter->pmoal_handle, pmadapter->pmlan_lock)
        != MLAN_STATUS_SUCCESS) {
        ret = MLAN_STATUS_FAILURE;
        goto exit_shutdown_fw;
    }

    if (pcb->moal_spin_unlock(pmadapter->pmoal_handle, pmadapter->pmlan_lock)
        != MLAN_STATUS_SUCCESS) {
        ret = MLAN_STATUS_FAILURE;
        goto exit_shutdown_fw;
    }

    /* Notify completion */
    ret = wlan_shutdown_fw_complete(pmadapter);

  exit_shutdown_fw:
    LEAVE();
    return ret;
}

/**
 *  @brief The main process
 *
 *  @param pmlan_adapter	A pointer to mlan_adapter structure
 *
 *  @return			MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
mlan_main_process(IN t_void * pmlan_adapter)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;
    pmlan_callbacks pcb;
    pmlan_buffer pmbuf;

    ENTER();

    MASSERT(pmlan_adapter);

    pcb = &pmadapter->callbacks;

    pcb->moal_spin_lock(pmadapter->pmoal_handle, pmadapter->pmain_proc_lock);

    /* Check if already processing */
    if (pmadapter->mlan_processing) {
        pcb->moal_spin_unlock(pmadapter->pmoal_handle,
                              pmadapter->pmain_proc_lock);
        goto exit_main_proc;
    } else {
        pmadapter->mlan_processing = MTRUE;
        pcb->moal_spin_unlock(pmadapter->pmoal_handle,
                              pmadapter->pmain_proc_lock);
    }
  process_start:
    do {
        /* Is MLAN shutting down or not ready? */
        if ((pmadapter->hw_status == WlanHardwareStatusClosing) ||
            (pmadapter->hw_status == WlanHardwareStatusNotReady))
            break;

        /* Need to wake up the card ? */
        if ((pmadapter->ps_state == PS_STATE_SLEEP) &&
            (pmadapter->pm_wakeup_card_req &&
             !pmadapter->pm_wakeup_fw_try) &&
            (util_peek_list
             (pmadapter->pmoal_handle, &pmadapter->cmd_pending_q,
              pcb->moal_spin_lock, pcb->moal_spin_unlock)
             || !wlan_bypass_tx_list_empty(pmadapter)
             || !wlan_wmm_lists_empty(pmadapter)
            )) {
            pmadapter->pm_wakeup_fw_try = MTRUE;
            wlan_pm_wakeup_card(pmadapter);
            continue;
        }
        if (IS_CARD_RX_RCVD(pmadapter)) {
            pmadapter->data_received = MFALSE;
            if (pmadapter->hs_activated == MTRUE) {
                pmadapter->is_hs_configured = MFALSE;
                wlan_host_sleep_activated_event(wlan_get_priv
                                                (pmadapter, MLAN_BSS_ROLE_ANY),
                                                MFALSE);
            }
            pmadapter->pm_wakeup_fw_try = MFALSE;
            if (pmadapter->ps_state == PS_STATE_SLEEP)
                pmadapter->ps_state = PS_STATE_AWAKE;
        } else {
            /* We have tried to wakeup the card already */
            if (pmadapter->pm_wakeup_fw_try)
                break;
            /* Check if we need to confirm Sleep Request received previously */
            if (pmadapter->ps_state == PS_STATE_PRE_SLEEP) {
                if (!pmadapter->cmd_sent && !pmadapter->curr_cmd) {
                    wlan_check_ps_cond(pmadapter);
                }
            }
            if (pmadapter->ps_state != PS_STATE_AWAKE ||
                (pmadapter->tx_lock_flag == MTRUE))
                break;

            if (pmadapter->scan_processing || pmadapter->data_sent
                || (wlan_bypass_tx_list_empty(pmadapter) &&
                    wlan_wmm_lists_empty(pmadapter))
                ) {
                if (pmadapter->cmd_sent || pmadapter->curr_cmd ||
                    (!util_peek_list
                     (pmadapter->pmoal_handle, &pmadapter->cmd_pending_q,
                      pcb->moal_spin_lock, pcb->moal_spin_unlock))) {
                    break;
                }
            }
        }

        /* Check for Rx data */
        while ((pmbuf =
                (pmlan_buffer) util_dequeue_list(pmadapter->pmoal_handle,
                                                 &pmadapter->rx_data_queue,
                                                 pmadapter->callbacks.
                                                 moal_spin_lock,
                                                 pmadapter->callbacks.
                                                 moal_spin_unlock))) {
            wlan_handle_rx_packet(pmadapter, pmbuf);
        }

        /* Check for Cmd Resp */
        if (pmadapter->cmd_resp_received) {
            pmadapter->cmd_resp_received = MFALSE;
            wlan_process_cmdresp(pmadapter);

            /* call moal back when init_fw is done */
            if (pmadapter->hw_status == WlanHardwareStatusInitdone) {
                pmadapter->hw_status = WlanHardwareStatusReady;
                wlan_init_fw_complete(pmadapter);
            }
        }

        /* Check for event */
        if (pmadapter->event_received) {
            pmadapter->event_received = MFALSE;
            wlan_process_event(pmadapter);
        }

        /* Check if we need to confirm Sleep Request received previously */
        if (pmadapter->ps_state == PS_STATE_PRE_SLEEP) {
            if (!pmadapter->cmd_sent && !pmadapter->curr_cmd) {
                wlan_check_ps_cond(pmadapter);
            }
        }

        /* 
         * The ps_state may have been changed during processing of
         * Sleep Request event.
         */
        if ((pmadapter->ps_state == PS_STATE_SLEEP)
            || (pmadapter->ps_state == PS_STATE_PRE_SLEEP)
            || (pmadapter->ps_state == PS_STATE_SLEEP_CFM)
            || (pmadapter->tx_lock_flag == MTRUE)
            )
            continue;

        if (!pmadapter->cmd_sent && !pmadapter->curr_cmd) {
            if (wlan_exec_next_cmd(pmadapter) == MLAN_STATUS_FAILURE) {
                ret = MLAN_STATUS_FAILURE;
                break;
            }
        }

        if (!pmadapter->scan_processing && !pmadapter->data_sent &&
            !wlan_bypass_tx_list_empty(pmadapter)) {
            PRINTM(MINFO, "mlan_send_pkt(): deq(bybass_txq)\n");
            wlan_process_bypass_tx(pmadapter);
            if (pmadapter->hs_activated == MTRUE) {
                pmadapter->is_hs_configured = MFALSE;
                wlan_host_sleep_activated_event(wlan_get_priv
                                                (pmadapter, MLAN_BSS_ROLE_ANY),
                                                MFALSE);
            }
        }

        if (!pmadapter->scan_processing && !pmadapter->data_sent &&
            !wlan_wmm_lists_empty(pmadapter)
            ) {
            wlan_wmm_process_tx(pmadapter);
            if (pmadapter->hs_activated == MTRUE) {
                pmadapter->is_hs_configured = MFALSE;
                wlan_host_sleep_activated_event(wlan_get_priv
                                                (pmadapter, MLAN_BSS_ROLE_ANY),
                                                MFALSE);
            }
        }

#ifdef STA_SUPPORT
        if (pmadapter->delay_null_pkt && !pmadapter->cmd_sent &&
            !pmadapter->curr_cmd && !IS_COMMAND_PENDING(pmadapter) &&
            wlan_bypass_tx_list_empty(pmadapter) &&
            wlan_wmm_lists_empty(pmadapter)) {
            if (wlan_send_null_packet
                (wlan_get_priv(pmadapter, MLAN_BSS_ROLE_STA),
                 MRVDRV_TxPD_POWER_MGMT_NULL_PACKET |
                 MRVDRV_TxPD_POWER_MGMT_LAST_PACKET)
                == MLAN_STATUS_SUCCESS) {
                pmadapter->delay_null_pkt = MFALSE;
            }
            break;
        }
#endif

    } while (MTRUE);

    if (IS_CARD_RX_RCVD(pmadapter)) {
        goto process_start;
    }
    pcb->moal_spin_lock(pmadapter->pmoal_handle, pmadapter->pmain_proc_lock);
    pmadapter->mlan_processing = MFALSE;
    pcb->moal_spin_unlock(pmadapter->pmoal_handle, pmadapter->pmain_proc_lock);

  exit_main_proc:
    if (pmadapter->hw_status == WlanHardwareStatusClosing)
        mlan_shutdown_fw(pmadapter);
    LEAVE();
    return ret;
}

/**
 *  @brief Function to send packet
 *
 *  @param pmlan_adapter	A pointer to mlan_adapter structure
 *  @param pmbuf		A pointer to mlan_buffer structure
 *
 *  @return			MLAN_STATUS_PENDING
 */
mlan_status
mlan_send_packet(IN t_void * pmlan_adapter, IN pmlan_buffer pmbuf)
{
    mlan_status ret = MLAN_STATUS_PENDING;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;

    ENTER();
    MASSERT(pmlan_adapter && pmbuf);

    MASSERT(pmbuf->bss_index < pmadapter->priv_num);
    pmbuf->flags = MLAN_BUF_FLAG_MOAL_TX_BUF;
    if ((pmadapter->priv[pmbuf->bss_index]->port_ctrl_mode == MTRUE) &&
        ((mlan_ntohs(*(t_u16 *) & pmbuf->pbuf[pmbuf->data_offset +
                                              MLAN_ETHER_PKT_TYPE_OFFSET]) ==
          MLAN_ETHER_PKT_TYPE_EAPOL)
         ||
         (mlan_ntohs
          (*(t_u16 *) & pmbuf->
           pbuf[pmbuf->data_offset + MLAN_ETHER_PKT_TYPE_OFFSET]) ==
          MLAN_ETHER_PKT_TYPE_WAPI)
        )) {
        PRINTM(MINFO, "mlan_send_pkt(): enq(bybass_txq)\n");
        wlan_add_buf_bypass_txqueue(pmadapter, pmbuf);
    } else {
        /* Transmit the packet */
        wlan_wmm_add_buf_txqueue(pmadapter, pmbuf);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief MLAN ioctl handler
 *
 *  @param adapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING --success, otherwise fail
 */
mlan_status
mlan_ioctl(IN t_void * adapter, IN pmlan_ioctl_req pioctl_req)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    pmlan_adapter pmadapter = (pmlan_adapter) adapter;
    pmlan_private pmpriv = MNULL;

    ENTER();

    if (pioctl_req == MNULL) {
        PRINTM(MERROR, "MLAN IOCTL information buffer is NULL\n");
        ret = MLAN_STATUS_FAILURE;
        goto exit;
    }
    if (pioctl_req->action == MLAN_ACT_CANCEL) {
        wlan_cancel_pending_ioctl(pmadapter, pioctl_req);
        ret = MLAN_STATUS_SUCCESS;
        goto exit;
    }
    pmpriv = pmadapter->priv[pioctl_req->bss_index];
    ret = pmpriv->ops.ioctl(adapter, pioctl_req);
  exit:
    LEAVE();
    return ret;
}

/**
 *  @brief Packet send completion callback
 *
 *  @param pmlan_adapter	A pointer to mlan_adapter structure
 *  @param pmbuf		A pointer to mlan_buffer structure
 *  @param port			Data port
 *  @param status		Callback status
 *
 *  @return			MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
mlan_write_data_async_complete(IN t_void * pmlan_adapter,
                               IN pmlan_buffer pmbuf,
                               IN t_u32 port, IN mlan_status status)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;

    ENTER();

    if (status == MLAN_STATUS_SUCCESS &&
        pmadapter->hb_host_timer_is_set && pmadapter->host_to_dev_traffic <= 1)
        pmadapter->host_to_dev_traffic++;
    if (port == MLAN_USB_EP_CMD_EVENT) {
        pmadapter->cmd_sent = MFALSE;
        PRINTM(MCMND, "mlan_write_data_async_complete: CMD\n");
        /* pmbuf was allocated by MLAN */
        wlan_free_mlan_buffer(pmadapter, pmbuf);
    } else {
        ret = wlan_write_data_complete(pmadapter, pmbuf, status);
    }

    LEAVE();
    return ret;
}

/**
 *  @brief Packet receive handler
 *
 *  @param pmlan_adapter	A pointer to mlan_adapter structure
 *  @param pmbuf		A pointer to mlan_buffer structure
 *  @param port			Data port
 *
 *  @return			MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE or MLAN_STATUS_PENDING
 */
mlan_status
mlan_recv(IN t_void * pmlan_adapter, IN pmlan_buffer pmbuf, IN t_u32 port)
{
    mlan_status ret = MLAN_STATUS_PENDING;
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;
    t_u8 *pbuf;
    t_u32 len, recv_type;
    t_u32 event_cause;
    t_u32 sec, usec;

    ENTER();

    MASSERT(pmlan_adapter && pmbuf);

    if (pmadapter->hb_dev_timer_is_set) {
        pmadapter->hb_dev_counter = 0;
    }
    if (pmadapter->hs_activated == MTRUE)
        wlan_process_hs_config(pmadapter);
    pbuf = pmbuf->pbuf + pmbuf->data_offset;
    len = pmbuf->data_len;

    MASSERT(len >= MLAN_TYPE_LEN);
    recv_type = *(t_u32 *) pbuf;
    recv_type = wlan_le32_to_cpu(recv_type);
    pbuf += MLAN_TYPE_LEN;
    len -= MLAN_TYPE_LEN;

    pmadapter->callbacks.moal_get_system_time(pmadapter->pmoal_handle, &sec,
                                              &usec);

    switch (port) {
    case MLAN_USB_EP_CTRL:
        PRINTM(MINFO, "mlan_recv: CTRL\n");
        ret = MLAN_STATUS_SUCCESS;
        break;
    case MLAN_USB_EP_CMD_EVENT:
        switch (recv_type) {
        case MLAN_USB_TYPE_CMD:
            PRINTM(MCMND, "mlan_recv: CMD (%lu.%06lu)\n", sec, usec);
            if (len > MRVDRV_SIZE_OF_CMD_BUFFER) {
                pmbuf->status_code = MLAN_ERROR_CMD_RESP_FAIL;
                ret = MLAN_STATUS_FAILURE;
                PRINTM(MERROR, "mlan_recv: CMD too large\n");
            } else if (!pmadapter->curr_cmd) {
                if (pmadapter->ps_state == PS_STATE_SLEEP_CFM) {
                    pmbuf->data_offset += MLAN_TYPE_LEN;
                    pmbuf->data_len -= MLAN_TYPE_LEN;
                    wlan_process_sleep_confirm_resp(pmadapter,
                                                    pmbuf->pbuf +
                                                    pmbuf->data_offset,
                                                    pmbuf->data_len);
                    ret = MLAN_STATUS_SUCCESS;

                } else {
                    pmbuf->status_code = MLAN_ERROR_CMD_RESP_FAIL;
                    ret = MLAN_STATUS_FAILURE;
                }
                PRINTM(MINFO, "mlan_recv: no curr_cmd\n");
            } else {
                pmadapter->upld_len = len;
                pmbuf->data_offset += MLAN_TYPE_LEN;
                pmbuf->data_len -= MLAN_TYPE_LEN;
                pmadapter->curr_cmd->respbuf = pmbuf;
                pmadapter->cmd_resp_received = MTRUE;
            }
            break;
        case MLAN_USB_TYPE_EVENT:
            MASSERT(len >= sizeof(t_u32));
            memmove(pmadapter, &event_cause, pbuf, sizeof(t_u32));
            pmadapter->event_cause = wlan_le32_to_cpu(event_cause);
            PRINTM(MEVENT, "mlan_recv: EVENT 0x%x (%lu.%06lu)\n",
                   pmadapter->event_cause, sec, usec);
            pbuf += sizeof(t_u32);
            len -= sizeof(t_u32);
            if ((len > 0) && (len < MAX_EVENT_SIZE))
                memmove(pmadapter, pmadapter->event_body, pbuf, len);
            pmadapter->event_received = MTRUE;
            pmadapter->pmlan_buffer_event = pmbuf;
            /* remove 4 byte recv_type */
            pmbuf->data_offset += MLAN_TYPE_LEN;
            pmbuf->data_len -= MLAN_TYPE_LEN;
            /* MOAL to call mlan_main_process for processing */
            break;
        default:
            pmbuf->status_code = MLAN_ERROR_PKT_INVALID;
            ret = MLAN_STATUS_FAILURE;
            PRINTM(MERROR, "mlan_recv: unknown recv_type 0x%x\n", recv_type);
            break;
        }
        break;
    case MLAN_USB_EP_DATA:
        PRINTM(MDATA, "mlan_recv: DATA (%lu.%06lu)\n", sec, usec);
        if (len > MLAN_RX_DATA_BUF_SIZE) {
            pmbuf->status_code = MLAN_ERROR_PKT_SIZE_INVALID;
            ret = MLAN_STATUS_FAILURE;
            PRINTM(MERROR, "mlan_recv: DATA too large\n");
        } else {
            pmadapter->upld_len = len;
            pmadapter->callbacks.moal_get_system_time(pmadapter->pmoal_handle,
                                                      &pmbuf->in_ts_sec,
                                                      &pmbuf->in_ts_usec);
            util_enqueue_list_tail(pmadapter->pmoal_handle,
                                   &pmadapter->rx_data_queue,
                                   (pmlan_linked_list) pmbuf,
                                   pmadapter->callbacks.moal_spin_lock,
                                   pmadapter->callbacks.moal_spin_unlock);
            pmadapter->data_received = MTRUE;
        }
        break;
    default:
        pmbuf->status_code = MLAN_ERROR_PKT_INVALID;
        ret = MLAN_STATUS_FAILURE;
        PRINTM(MERROR, "mlan_recv: unknown port number 0x%x\n", port);
        break;
    }

    LEAVE();
    return ret;
}

/**
 *  @brief Packet receive completion callback handler
 *
 *  @param pmlan_adapter	A pointer to mlan_adapter structure
 *  @param pmbuf		A pointer to mlan_buffer structure
 *  @param status		Callback status
 *
 *  @return			MLAN_STATUS_SUCCESS
 */
mlan_status
mlan_recv_packet_complete(IN t_void * pmlan_adapter,
                          IN pmlan_buffer pmbuf, IN mlan_status status)
{
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;

    ENTER();
    wlan_recv_packet_complete(pmadapter, pmbuf, status);
    LEAVE();
    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief select wmm queue
 *
 *  @param pmlan_adapter	A pointer to mlan_adapter structure
 *  @param bss_num		BSS number
 *  @param tid			TID
 *
 *  @return			wmm queue priority (0 - 3)
 */
t_u8
mlan_select_wmm_queue(IN t_void * pmlan_adapter, IN t_u8 bss_num, IN t_u8 tid)
{
    mlan_adapter *pmadapter = (mlan_adapter *) pmlan_adapter;
    pmlan_private pmpriv = pmadapter->priv[bss_num];
    return wlan_wmm_select_queue(pmpriv, tid);
}
