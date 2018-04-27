/** @file moal_uap.c
  *
  * @brief This file contains the major functions in UAP
  * driver. 
  *
  * Copyright (C) 2008-2011, Marvell International Ltd. 
  * 
  * This software file (the "File") is distributed by Marvell International 
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
  * (the "License").  You may use, redistribute and/or modify this File in 
  * accordance with the terms and conditions of the License, a copy of which 
  * is available by writing to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
  * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
  * this warranty disclaimer.
  *
  */

/********************************************************
Change log:
    10/21/2008: initial version
********************************************************/

#include	"moal_main.h"
#include    "moal_uap.h"

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
 *  @brief uap addba parameter handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_addba_param(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    addba_param param;
    int ret = 0;

    ENTER();
    memset(&param, 0, sizeof(param));

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_addba_param() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&param, req->ifr_data, sizeof(param))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }
    PRINTM(MIOCTL,
           "addba param: action=%d, timeout=%d, txwinsize=%d, rxwinsize=%d\n",
           (int) param.action, (int) param.timeout, (int) param.txwinsize,
           (int) param.rxwinsize);
    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (ioctl_req == NULL) {
        LEAVE();
        return -ENOMEM;
    }
    cfg_11n = (mlan_ds_11n_cfg *) ioctl_req->pbuf;
    cfg_11n->sub_command = MLAN_OID_11N_CFG_ADDBA_PARAM;
    ioctl_req->req_id = MLAN_IOCTL_11N_CFG;

    if (!param.action) {
        /* Get addba param from MLAN */
        ioctl_req->action = MLAN_ACT_GET;
    } else {
        /* Set addba param in MLAN */
        ioctl_req->action = MLAN_ACT_SET;
        cfg_11n->param.addba_param.timeout = param.timeout;
        cfg_11n->param.addba_param.txwinsize = param.txwinsize;
        cfg_11n->param.addba_param.rxwinsize = param.rxwinsize;
    }
    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    param.timeout = cfg_11n->param.addba_param.timeout;
    param.txwinsize = cfg_11n->param.addba_param.txwinsize;
    param.rxwinsize = cfg_11n->param.addba_param.rxwinsize;

    /* Copy to user */
    if (copy_to_user(req->ifr_data, &param, sizeof(param))) {
        PRINTM(MERROR, "Copy to user failed!\n");
        ret = -EFAULT;
        goto done;
    }
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/** 
 *  @brief uap aggr priority tbl 
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_aggr_priotbl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    aggr_prio_tbl param;
    int ret = 0;
    int i = 0;

    ENTER();
    memset(&param, 0, sizeof(param));

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "woal_uap_aggr_priotbl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&param, req->ifr_data, sizeof(param))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }
    DBG_HEXDUMP(MCMD_D, "aggr_prio_tbl", (t_u8 *) & param, sizeof(param));

    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (ioctl_req == NULL) {
        LEAVE();
        return -ENOMEM;
    }
    cfg_11n = (mlan_ds_11n_cfg *) ioctl_req->pbuf;
    cfg_11n->sub_command = MLAN_OID_11N_CFG_AGGR_PRIO_TBL;
    ioctl_req->req_id = MLAN_IOCTL_11N_CFG;

    if (!param.action) {
        /* Get aggr_prio_tbl from MLAN */
        ioctl_req->action = MLAN_ACT_GET;
    } else {
        /* Set aggr_prio_tbl in MLAN */
        ioctl_req->action = MLAN_ACT_SET;
        for (i = 0; i < MAX_NUM_TID; i++) {
            cfg_11n->param.aggr_prio_tbl.ampdu[i] = param.ampdu[i];
            cfg_11n->param.aggr_prio_tbl.amsdu[i] = param.amsdu[i];
        }
    }
    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    for (i = 0; i < MAX_NUM_TID; i++) {
        param.ampdu[i] = cfg_11n->param.aggr_prio_tbl.ampdu[i];
        param.amsdu[i] = cfg_11n->param.aggr_prio_tbl.amsdu[i];
    }
    /* Copy to user */
    if (copy_to_user(req->ifr_data, &param, sizeof(param))) {
        PRINTM(MERROR, "Copy to user failed!\n");
        ret = -EFAULT;
        goto done;
    }
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/** 
 *  @brief uap addba reject tbl 
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_addba_reject(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    addba_reject_para param;
    int ret = 0;
    int i = 0;

    ENTER();
    memset(&param, 0, sizeof(param));

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "woal_uap_addba_reject() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&param, req->ifr_data, sizeof(param))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }
    DBG_HEXDUMP(MCMD_D, "addba_reject tbl", (t_u8 *) & param, sizeof(param));

    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (ioctl_req == NULL) {
        LEAVE();
        return -ENOMEM;
    }
    cfg_11n = (mlan_ds_11n_cfg *) ioctl_req->pbuf;
    cfg_11n->sub_command = MLAN_OID_11N_CFG_ADDBA_REJECT;
    ioctl_req->req_id = MLAN_IOCTL_11N_CFG;

    if (!param.action) {
        /* Get addba_reject tbl from MLAN */
        ioctl_req->action = MLAN_ACT_GET;
    } else {
        /* Set addba_reject tbl in MLAN */
        ioctl_req->action = MLAN_ACT_SET;
        for (i = 0; i < MAX_NUM_TID; i++) {
            cfg_11n->param.addba_reject[i] = param.addba_reject[i];
        }
    }
    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    for (i = 0; i < MAX_NUM_TID; i++) {
        param.addba_reject[i] = cfg_11n->param.addba_reject[i];
    }
    /* Copy to user */
    if (copy_to_user(req->ifr_data, &param, sizeof(param))) {
        PRINTM(MERROR, "Copy to user failed!\n");
        ret = -EFAULT;
        goto done;
    }
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/** 
 *  @brief uap get_fw_info handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_get_fw_info(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    fw_info fw;
    mlan_fw_info fw_info;
    int ret = 0;

    ENTER();
    memset(&fw, 0, sizeof(fw));
    memset(&fw_info, 0, sizeof(fw_info));

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "woal_uap_get_fw_info() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&fw, req->ifr_data, sizeof(fw))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }
    if (MLAN_STATUS_SUCCESS !=
        woal_request_get_fw_info(priv, MOAL_IOCTL_WAIT, &fw_info)) {
        ret = -EFAULT;
        goto done;
    }
    fw.fw_release_number = fw_info.fw_ver;
    fw.hw_dev_mcs_support = fw_info.hw_dev_mcs_support;
    /* Copy to user */
    if (copy_to_user(req->ifr_data, &fw, sizeof(fw))) {
        PRINTM(MERROR, "Copy to user failed!\n");
        ret = -EFAULT;
        goto done;
    }
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief configure tx_pause settings
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_txdatapause(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_misc_cfg *misc = NULL;
    tx_data_pause_para param;
    int ret = 0;

    ENTER();
    memset(&param, 0, sizeof(param));

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "woal_uap_txdatapause corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&param, req->ifr_data, sizeof(param))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }
    DBG_HEXDUMP(MCMD_D, "tx_data_pause_para", (t_u8 *) & param, sizeof(param));

    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (ioctl_req == NULL) {
        LEAVE();
        return -ENOMEM;
    }
    misc = (mlan_ds_misc_cfg *) ioctl_req->pbuf;
    misc->sub_command = MLAN_OID_MISC_TX_DATAPAUSE;
    ioctl_req->req_id = MLAN_IOCTL_MISC_CFG;

    if (!param.action) {
        /* Get Tx data pause status from MLAN */
        ioctl_req->action = MLAN_ACT_GET;
    } else {
        /* Set Tx data pause in MLAN */
        ioctl_req->action = MLAN_ACT_SET;
        misc->param.tx_datapause.tx_pause = param.txpause;
        misc->param.tx_datapause.tx_buf_cnt = param.txbufcnt;
    }
    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    param.txpause = misc->param.tx_datapause.tx_pause;
    param.txbufcnt = misc->param.tx_datapause.tx_buf_cnt;

    /* Copy to user */
    if (copy_to_user(req->ifr_data, &param, sizeof(param))) {
        PRINTM(MERROR, "Copy to user failed!\n");
        ret = -EFAULT;
        goto done;
    }
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/** 
 *  @brief uap ioctl handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_ioctl(struct net_device *dev, struct ifreq *req)
{
    int ret = 0;
    t_u32 subcmd = 0;
    ENTER();
    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_ioctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&subcmd, req->ifr_data, sizeof(subcmd))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    PRINTM(MIOCTL, "ioctl subcmd=%d\n", (int) subcmd);
    switch (subcmd) {
    case UAP_ADDBA_PARA:
        ret = woal_uap_addba_param(dev, req);
        break;
    case UAP_AGGR_PRIOTBL:
        ret = woal_uap_aggr_priotbl(dev, req);
        break;
    case UAP_ADDBA_REJECT:
        ret = woal_uap_addba_reject(dev, req);
        break;
    case UAP_FW_INFO:
        ret = woal_uap_get_fw_info(dev, req);
        break;
    case UAP_TX_DATA_PAUSE:
        ret = woal_uap_txdatapause(dev, req);
        break;
    default:
        break;
    }
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief uap station deauth ioctl handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_sta_deauth_ioctl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_bss *bss = NULL;
    mlan_deauth_param deauth_param;
    int ret = 0;

    ENTER();

    memset(&deauth_param, 0, sizeof(mlan_deauth_param));
    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_sta_deauth_ioctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&deauth_param, req->ifr_data, sizeof(mlan_deauth_param))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    PRINTM(MIOCTL,
           "ioctl deauth station: %02x:%02x:%02x:%02x:%02x:%02x, reason=%d\n",
           deauth_param.mac_addr[0], deauth_param.mac_addr[1],
           deauth_param.mac_addr[2], deauth_param.mac_addr[3],
           deauth_param.mac_addr[4], deauth_param.mac_addr[5],
           deauth_param.reason_code);

    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    bss = (mlan_ds_bss *) ioctl_req->pbuf;

    bss->sub_command = MLAN_OID_UAP_DEAUTH_STA;
    ioctl_req->req_id = MLAN_IOCTL_BSS;
    ioctl_req->action = MLAN_ACT_SET;

    memcpy(&bss->param.deauth_param, &deauth_param, sizeof(mlan_deauth_param));
    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        if (copy_to_user(req->ifr_data, &ioctl_req->status_code, sizeof(t_u32)))
            PRINTM(MERROR, "Copy to user failed!\n");
        goto done;
    }

  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/**
 * @brief Set/Get radio
 * 
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 * 
 * @return           0 --success, otherwise fail
 */
static int
woal_uap_radio_ctl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    int ret = 0;
    mlan_ds_radio_cfg *radio = NULL;
    mlan_ioctl_req *mreq = NULL;
    int data[2] = { 0, 0 };
    mlan_bss_info bss_info;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_radio_ctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }

    /* Get user data */
    if (copy_from_user(&data, req->ifr_data, sizeof(data))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    if (data[0]) {
        mreq = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_radio_cfg));
        if (mreq == NULL) {
            ret = -ENOMEM;
            goto done;
        }
        radio = (mlan_ds_radio_cfg *) mreq->pbuf;
        radio->sub_command = MLAN_OID_RADIO_CTRL;
        mreq->req_id = MLAN_IOCTL_RADIO_CFG;
        mreq->action = MLAN_ACT_SET;
        radio->param.radio_on_off = (t_u32) data[1];
        if (MLAN_STATUS_SUCCESS !=
            woal_request_ioctl(priv, mreq, MOAL_IOCTL_WAIT)) {
            ret = -EFAULT;
        }
        if (mreq)
            kfree(mreq);
    } else {
        /* Get radio status */
        memset(&bss_info, 0, sizeof(bss_info));
        woal_get_bss_info(priv, MOAL_IOCTL_WAIT, &bss_info);

        data[1] = bss_info.radio_on;
        if (copy_to_user(req->ifr_data, data, sizeof(data))) {
            PRINTM(MERROR, "Copy to user failed\n");
            ret = -EFAULT;
        }
    }
  done:
    LEAVE();
    return ret;
}

/**
 * @brief Set/Get tx rate
 * 
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 * 
 * @return           0 --success, otherwise fail
 */
static int
woal_uap_tx_rate_cfg(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    int data[2] = { 0, 0 };
    int ret = 0;
    mlan_ds_rate *rate = NULL;
    mlan_ioctl_req *mreq = NULL;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_radio_ctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }

    /* Get user data */
    if (copy_from_user(&data, req->ifr_data, sizeof(data))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    mreq = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_rate));
    if (mreq == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    rate = (mlan_ds_rate *) mreq->pbuf;
    rate->param.rate_cfg.rate_type = MLAN_RATE_INDEX;
    rate->sub_command = MLAN_OID_RATE_CFG;
    mreq->req_id = MLAN_IOCTL_RATE;
    if (!data[0])
        mreq->action = MLAN_ACT_GET;
    else {
        mreq->action = MLAN_ACT_SET;
        if (data[1] == AUTO_RATE)
            rate->param.rate_cfg.is_rate_auto = 1;
        else {
            if ((data[1] != MLAN_RATE_INDEX_MCS32) &&
                ((data[1] < 0) || (data[1] > MLAN_RATE_INDEX_MCS7))) {
                ret = -EINVAL;
                goto done;
            }
        }
        rate->param.rate_cfg.rate = data[1];
    }

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, mreq, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    if (data[0]) {
        priv->rate_index = data[1];
    } else {
        if (rate->param.rate_cfg.is_rate_auto)
            data[1] = AUTO_RATE;
        else
            data[1] = rate->param.rate_cfg.rate;

        if (copy_to_user(req->ifr_data, data, sizeof(data))) {
            PRINTM(MERROR, "Copy to user failed\n");
            ret = -EFAULT;
        }
    }
  done:
    if (mreq)
        kfree(mreq);
    LEAVE();
    return ret;
}

/** 
 *  @brief uap bss control ioctl handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_bss_ctrl_ioctl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    int ret = 0, data = 0;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_bss_ctrl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&data, req->ifr_data, sizeof(data))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, data);

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief uap power mode ioctl handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_power_mode_ioctl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_pm_cfg *pm_cfg = NULL;
    mlan_ds_ps_mgmt ps_mgmt;
    int ret = 0;

    ENTER();

    memset(&ps_mgmt, 0, sizeof(mlan_ds_ps_mgmt));

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_power_mode_ioctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&ps_mgmt, req->ifr_data, sizeof(mlan_ds_ps_mgmt))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    PRINTM(MIOCTL,
           "ioctl power: flag=0x%x ps_mode=%d ctrl_bitmap=%d min_sleep=%d max_sleep=%d "
           "inact_to=%d min_awake=%d max_awake=%d\n", ps_mgmt.flags,
           (int) ps_mgmt.ps_mode, (int) ps_mgmt.sleep_param.ctrl_bitmap,
           (int) ps_mgmt.sleep_param.min_sleep,
           (int) ps_mgmt.sleep_param.max_sleep,
           (int) ps_mgmt.inact_param.inactivity_to,
           (int) ps_mgmt.inact_param.min_awake,
           (int) ps_mgmt.inact_param.max_awake);

    if (ps_mgmt.
        flags & ~(PS_FLAG_PS_MODE | PS_FLAG_SLEEP_PARAM |
                  PS_FLAG_INACT_SLEEP_PARAM)) {
        PRINTM(MERROR, "Invalid parameter: flags = 0x%x\n", ps_mgmt.flags);
        ret = -EINVAL;
        goto done;
    }

    if (ps_mgmt.ps_mode > PS_MODE_INACTIVITY) {
        PRINTM(MERROR, "Invalid parameter: ps_mode = %d\n",
               (int) ps_mgmt.flags);
        ret = -EINVAL;
        goto done;
    }

    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_pm_cfg));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    pm_cfg = (mlan_ds_pm_cfg *) ioctl_req->pbuf;
    pm_cfg->sub_command = MLAN_OID_PM_CFG_PS_MODE;
    ioctl_req->req_id = MLAN_IOCTL_PM_CFG;
    if (ps_mgmt.flags) {
        ioctl_req->action = MLAN_ACT_SET;
        memcpy(&pm_cfg->param.ps_mgmt, &ps_mgmt, sizeof(mlan_ds_ps_mgmt));
    } else {
        ioctl_req->action = MLAN_ACT_GET;
    }

    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        if (copy_to_user(req->ifr_data, &ioctl_req->status_code, sizeof(t_u32)))
            PRINTM(MERROR, "Copy to user failed!\n");
        goto done;
    }
    if (!ps_mgmt.flags) {
        /* Copy to user */
        if (copy_to_user
            (req->ifr_data, &pm_cfg->param.ps_mgmt, sizeof(mlan_ds_ps_mgmt))) {
            PRINTM(MERROR, "Copy to user failed!\n");
            ret = -EFAULT;
            goto done;
        }
    }
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/** 
 *  @brief uap BSS config ioctl handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_bss_cfg_ioctl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    int ret = 0;
    mlan_ds_bss *bss = NULL;
    mlan_ioctl_req *ioctl_req = NULL;
    int offset = 0;
    t_u32 action = 0;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_bss_cfg_ioctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }

    /* Get action */
    if (copy_from_user(&action, req->ifr_data + offset, sizeof(action))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }
    offset += sizeof(action);

    /* Allocate an IOCTL request buffer */
    ioctl_req =
        (mlan_ioctl_req *) woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    bss = (mlan_ds_bss *) ioctl_req->pbuf;
    bss->sub_command = MLAN_OID_UAP_BSS_CONFIG;
    ioctl_req->req_id = MLAN_IOCTL_BSS;
    if (action == 1) {
        ioctl_req->action = MLAN_ACT_SET;
    } else {
        ioctl_req->action = MLAN_ACT_GET;
    }

    if (ioctl_req->action == MLAN_ACT_SET) {
        /* Get the BSS config from user */
        if (copy_from_user
            (&bss->param.bss_config, req->ifr_data + offset,
             sizeof(mlan_uap_bss_param))) {
            PRINTM(MERROR, "Copy from user failed\n");
            ret = -EFAULT;
            goto done;
        }
    }

    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    if (ioctl_req->action == MLAN_ACT_GET) {
        offset = sizeof(action);

        /* Copy to user : BSS config */
        if (copy_to_user
            (req->ifr_data + offset, &bss->param.bss_config,
             sizeof(mlan_uap_bss_param))) {
            PRINTM(MERROR, "Copy to user failed!\n");
            ret = -EFAULT;
            goto done;
        }
    }
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/** 
 *  @brief uap get station list handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_get_sta_list_ioctl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    int ret = 0;
    mlan_ds_get_info *info = NULL;
    mlan_ioctl_req *ioctl_req = NULL;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_get_sta_list_ioctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }

    /* Allocate an IOCTL request buffer */
    ioctl_req =
        (mlan_ioctl_req *) woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    info = (mlan_ds_get_info *) ioctl_req->pbuf;
    info->sub_command = MLAN_OID_UAP_STA_LIST;
    ioctl_req->req_id = MLAN_IOCTL_GET_INFO;
    ioctl_req->action = MLAN_ACT_GET;

    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    if (ioctl_req->action == MLAN_ACT_GET) {
        /* Copy to user : sta_list */
        if (copy_to_user
            (req->ifr_data, &info->param.sta_list, sizeof(mlan_ds_sta_list))) {
            PRINTM(MERROR, "Copy to user failed!\n");
            ret = -EFAULT;
            goto done;
        }
    }
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/** 
 *  @brief uAP set WAPI key ioctl
 *   
 *  @param priv      A pointer to moal_private structure
 *  @param msg       A pointer to wapi_msg structure
 *
 *  @return          0 --success, otherwise fail
 */
static int
woal_uap_set_wapi_key_ioctl(moal_private * priv, wapi_msg * msg)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_sec_cfg *sec = NULL;
    int ret = 0;
    wapi_key_msg *key_msg = NULL;
    t_u8 bcast_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    ENTER();
    if (msg->msg_len != sizeof(wapi_key_msg)) {
        ret = -EINVAL;
        goto done;
    }
    key_msg = (wapi_key_msg *) msg->msg;

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_sec_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    sec = (mlan_ds_sec_cfg *) req->pbuf;
    sec->sub_command = MLAN_OID_SEC_CFG_ENCRYPT_KEY;
    req->req_id = MLAN_IOCTL_SEC_CFG;
    req->action = MLAN_ACT_SET;

    sec->param.encrypt_key.is_wapi_key = MTRUE;
    sec->param.encrypt_key.key_len = MLAN_MAX_KEY_LENGTH;
    memcpy(sec->param.encrypt_key.mac_addr, key_msg->mac_addr, ETH_ALEN);
    sec->param.encrypt_key.key_index = key_msg->key_id;
    if (0 == memcmp(key_msg->mac_addr, bcast_addr, ETH_ALEN))
        sec->param.encrypt_key.key_flags = KEY_FLAG_GROUP_KEY;
    else
        sec->param.encrypt_key.key_flags = KEY_FLAG_SET_TX_KEY;
    memcpy(sec->param.encrypt_key.key_material, key_msg->key,
           sec->param.encrypt_key.key_len);

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT))
        ret = -EFAULT;
  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief Enable/Disable wapi in firmware
 *   
 *  @param priv          A pointer to moal_private structure
 *  @param enable        MTRUE/MFALSE
 *
 *  @return              MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
static mlan_status
woal_enable_wapi(moal_private * priv, t_u8 enable)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_bss *bss = NULL;
    mlan_status status;

    ENTER();

    /* Allocate an IOCTL request buffer */
    req = (mlan_ioctl_req *) woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        status = MLAN_STATUS_FAILURE;
        goto done;
    }

    /* Fill request buffer */
    bss = (mlan_ds_bss *) req->pbuf;
    bss->sub_command = MLAN_OID_UAP_BSS_CONFIG;
    req->req_id = MLAN_IOCTL_BSS;
    req->action = MLAN_ACT_GET;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT);
    if (status != MLAN_STATUS_SUCCESS) {
        PRINTM(MERROR, "Get AP setting  failed! status=%d, error_code=0x%lx\n",
               status, req->status_code);
    }

    /* Change AP default setting */
    req->action = MLAN_ACT_SET;
    if (enable == MFALSE) {
        bss->param.bss_config.auth_mode = MLAN_AUTH_MODE_OPEN;
        bss->param.bss_config.protocol = PROTOCOL_NO_SECURITY;
    } else {
        bss->param.bss_config.auth_mode = MLAN_AUTH_MODE_OPEN;
        bss->param.bss_config.protocol = PROTOCOL_WAPI;
    }

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT);
    if (status != MLAN_STATUS_SUCCESS) {
        PRINTM(MERROR, "Set AP setting failed! status=%d, error_code=0x%lx\n",
               status, req->status_code);
    }
    if (enable)
        woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_START);
  done:
    if (req)
        kfree(req);
    LEAVE();
    return status;
}

/** 
 *  @brief uAP set WAPI flag ioctl
 *   
 *  @param priv      A pointer to moal_private structure
 *  @param msg       A pointer to wapi_msg structure
 *
 *  @return          0 --success, otherwise fail
 */
static int
woal_uap_set_wapi_flag_ioctl(moal_private * priv, wapi_msg * msg)
{
    t_u8 wapi_psk_ie[] =
        { 0x44, 0x14, 0x01, 0x00, 0x01, 0x00, 0x00, 0x14, 0x72, 0x02,
        0x01, 0x00, 0x00, 0x14, 0x72, 0x01, 0x00, 0x14, 0x72, 0x01,
        0x00, 0x00
    };
    t_u8 wapi_cert_ie[] =
        { 0x44, 0x14, 0x01, 0x00, 0x01, 0x00, 0x00, 0x14, 0x72, 0x01,
        0x01, 0x00, 0x00, 0x14, 0x72, 0x01, 0x00, 0x14, 0x72, 0x01,
        0x00, 0x00
    };
    mlan_ds_misc_cfg *misc = NULL;
    mlan_ioctl_req *req = NULL;
    int ret = 0;

    ENTER();

    woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP);

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    misc = (mlan_ds_misc_cfg *) req->pbuf;
    misc->sub_command = MLAN_OID_MISC_GEN_IE;
    req->req_id = MLAN_IOCTL_MISC_CFG;
    req->action = MLAN_ACT_SET;

    misc->param.gen_ie.type = MLAN_IE_TYPE_GEN_IE;
    misc->param.gen_ie.len = sizeof(wapi_psk_ie);
    if (msg->msg[0] & WAPI_MODE_PSK) {
        memcpy(misc->param.gen_ie.ie_data, wapi_psk_ie, misc->param.gen_ie.len);
    } else if (msg->msg[0] & WAPI_MODE_CERT) {
        memcpy(misc->param.gen_ie.ie_data, wapi_cert_ie,
               misc->param.gen_ie.len);
    } else if (msg->msg[0] == 0) {
        /* disable WAPI in driver */
        if (MLAN_STATUS_SUCCESS !=
            woal_set_wapi_enable(priv, MOAL_IOCTL_WAIT, 0))
            ret = -EFAULT;
        woal_enable_wapi(priv, MFALSE);
        goto done;
    } else {
        ret = -EINVAL;
        goto done;
    }
    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    woal_enable_wapi(priv, MTRUE);
  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief set wapi ioctl function
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_set_wapi(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    wapi_msg msg;
    int ret = 0;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_set_wapi() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }

    memset(&msg, 0, sizeof(msg));

    if (copy_from_user(&msg, req->ifr_data, sizeof(msg))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    PRINTM(MIOCTL, "set wapi msg_type = %d, msg_len=%d\n", msg.msg_type,
           msg.msg_len);
    DBG_HEXDUMP(MCMD_D, "wapi msg", msg.msg, msg.msg_len);

    switch (msg.msg_type) {
    case P80211_PACKET_WAPIFLAG:
        ret = woal_uap_set_wapi_flag_ioctl(priv, &msg);
        break;
    case P80211_PACKET_SETKEY:
        ret = woal_uap_set_wapi_key_ioctl(priv, &msg);
        break;
    default:
        ret = -EOPNOTSUPP;
        break;
    }
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief uap hs_cfg ioctl handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_hs_cfg(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    mlan_ds_hs_cfg hscfg;
    ds_hs_cfg hs_cfg;
    t_u16 action;
    int ret = 0;

    ENTER();

    memset(&hscfg, 0, sizeof(mlan_ds_hs_cfg));
    memset(&hs_cfg, 0, sizeof(ds_hs_cfg));

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_hs_cfg() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }
    if (copy_from_user(&hs_cfg, req->ifr_data, sizeof(ds_hs_cfg))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    PRINTM(MIOCTL,
           "ioctl hscfg: flags=0x%lx condition=0x%lx gpio=%d gap=0x%lx\n",
           hs_cfg.flags, hs_cfg.conditions, (int) hs_cfg.gpio, hs_cfg.gap);

    if (hs_cfg.flags & HS_CFG_FLAG_SET) {
        action = MLAN_ACT_SET;
        if (hs_cfg.flags != HS_CFG_FLAG_ALL) {
            woal_set_get_hs_params(priv, MLAN_ACT_GET, MOAL_IOCTL_WAIT, &hscfg);
        }
        if (hs_cfg.flags & HS_CFG_FLAG_CONDITION)
            hscfg.conditions = hs_cfg.conditions;
        if (hs_cfg.flags & HS_CFG_FLAG_GPIO)
            hscfg.gpio = hs_cfg.gpio;
        if (hs_cfg.flags & HS_CFG_FLAG_GAP)
            hscfg.gap = hs_cfg.gap;

        /* Issue IOCTL to set up parameters */
        hscfg.is_invoke_hostcmd = MFALSE;
        if (MLAN_STATUS_SUCCESS !=
            woal_set_get_hs_params(priv, action, MOAL_IOCTL_WAIT, &hscfg)) {
            ret = -EFAULT;
            goto done;
        }
    } else {
        action = MLAN_ACT_GET;
    }

    /* Issue IOCTL to invoke hostcmd */
    hscfg.is_invoke_hostcmd = MTRUE;
    if (MLAN_STATUS_SUCCESS !=
        woal_set_get_hs_params(priv, action, MOAL_IOCTL_WAIT, &hscfg)) {
        ret = -EFAULT;
        goto done;
    }
    if (!(hs_cfg.flags & HS_CFG_FLAG_SET)) {
        hs_cfg.flags =
            HS_CFG_FLAG_CONDITION | HS_CFG_FLAG_GPIO | HS_CFG_FLAG_GAP;
        hs_cfg.conditions = hscfg.conditions;
        hs_cfg.gpio = hscfg.gpio;
        hs_cfg.gap = hscfg.gap;
        /* Copy to user */
        if (copy_to_user(req->ifr_data, &hs_cfg, sizeof(ds_hs_cfg))) {
            PRINTM(MERROR, "Copy to user failed!\n");
            ret = -EFAULT;
            goto done;
        }
    }

  done:
    LEAVE();
    return ret;
}

/********************************************************
		Global Functions
********************************************************/

/**
 *  @brief uap BSS control ioctl handler
 *   
 *  @param priv             A pointer to moal_private structure
 *  @param wait_option      Wait option
 *  @param data             BSS control type
 *  @return                 0 --success, otherwise fail
 */
int
woal_uap_bss_ctrl(moal_private * priv, t_u8 wait_option, int data)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_bss *bss = NULL;
    int ret = 0;

    ENTER();

    PRINTM(MIOCTL, "ioctl bss ctrl=%d\n", data);
    if ((data != UAP_BSS_START) && (data != UAP_BSS_STOP) &&
        (data != UAP_BSS_RESET)) {
        PRINTM(MERROR, "Invalid parameter: %d\n", data);
        ret = -EINVAL;
        goto done;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    bss = (mlan_ds_bss *) req->pbuf;
    switch (data) {
    case UAP_BSS_START:
        if (priv->bss_started == MTRUE) {
            PRINTM(MWARN, "Warning: BSS already started!\n");
            // goto done;
        }
        bss->sub_command = MLAN_OID_BSS_START;
        break;
    case UAP_BSS_STOP:
        if (priv->bss_started == MFALSE) {
            PRINTM(MWARN, "Warning: BSS already stopped!\n");
            // goto done;
        }
        bss->sub_command = MLAN_OID_BSS_STOP;
        break;
    case UAP_BSS_RESET:
        bss->sub_command = MLAN_OID_UAP_BSS_RESET;
        woal_cancel_cac_block(priv);
        break;
    }
    req->req_id = MLAN_IOCTL_BSS;
    req->action = MLAN_ACT_SET;

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    if (data == UAP_BSS_STOP || data == UAP_BSS_RESET)
        priv->bss_started = MFALSE;

  done:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function sets multicast addresses to firmware
 *  
 *  @param dev     A pointer to net_device structure
 *  @return        N/A
 */
void
woal_uap_set_multicast_list(struct net_device *dev)
{
    ENTER();

    LEAVE();
}

/**
 *  @brief ioctl function - entry point
 *
 *  @param dev		A pointer to net_device structure
 *  @param req	   	A pointer to ifreq structure
 *  @param cmd 		Command
 *
 *  @return          0 --success, otherwise fail
 */
int
woal_uap_do_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    int ret = 0;
    ENTER();
    PRINTM(MIOCTL, "uap_do_ioctl: ioctl cmd = 0x%x\n", cmd);
    switch (cmd) {
    case UAP_HOSTCMD:
        ret = woal_hostcmd_ioctl(dev, req);
        break;
    case UAP_IOCTL_CMD:
        ret = woal_uap_ioctl(dev, req);
        break;
    case UAP_POWER_MODE:
        ret = woal_uap_power_mode_ioctl(dev, req);
        break;
    case UAP_BSS_CTRL:
        ret = woal_uap_bss_ctrl_ioctl(dev, req);
        break;
    case UAP_WAPI_MSG:
        ret = woal_uap_set_wapi(dev, req);
        break;
    case UAP_BSS_CONFIG:
        ret = woal_uap_bss_cfg_ioctl(dev, req);
        break;
    case UAP_STA_DEAUTH:
        ret = woal_uap_sta_deauth_ioctl(dev, req);
        break;
    case UAP_RADIO_CTL:
        ret = woal_uap_radio_ctl(dev, req);
        break;
    case UAP_TX_RATE_CFG:
        ret = woal_uap_tx_rate_cfg(dev, req);
        break;
    case UAP_GET_STA_LIST:
        ret = woal_uap_get_sta_list_ioctl(dev, req);
        break;
    case UAP_CUSTOM_IE:
        ret = woal_custom_ie_ioctl(dev, req);
        break;
    case UAP_GET_BSS_TYPE:
        ret = woal_get_bss_type(dev, req);
        break;
    case UAP_HS_CFG:
        ret = woal_uap_hs_cfg(dev, req);
        break;
    default:
        ret = woal_uap_do_priv_ioctl(dev, req, cmd);
        break;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get version 
 *   
 *  @param priv 		A pointer to moal_private structure
 *  @param version		A pointer to version buffer
 *  @param max_len		max length of version buffer
 *
 *  @return 	   		N/A
 */
void
woal_uap_get_version(moal_private * priv, char *version, int max_len)
{
    mlan_ds_get_info *info = NULL;
    mlan_ioctl_req *req = NULL;
    mlan_status status = MLAN_STATUS_SUCCESS;

    ENTER();

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
    if (req == NULL) {
        LEAVE();
        return;
    }

    info = (mlan_ds_get_info *) req->pbuf;
    info->sub_command = MLAN_OID_GET_VER_EXT;
    req->req_id = MLAN_IOCTL_GET_INFO;
    req->action = MLAN_ACT_GET;

    status = woal_request_ioctl(priv, req, MOAL_PROC_WAIT);
    if (status == MLAN_STATUS_SUCCESS) {
        PRINTM(MINFO, "MOAL UAP VERSION: %s\n",
               info->param.ver_ext.version_str);
        snprintf(version, max_len, driver_version,
                 info->param.ver_ext.version_str);
    }

    if (req && (status != MLAN_STATUS_PENDING))
        kfree(req);

    LEAVE();
    return;
}

/** 
 *  @brief Get uap statistics 
 *   
 *  @param priv 		        A pointer to moal_private structure
 *  @param wait_option          Wait option
 *  @param ustats               A pointer to mlan_ds_uap_stats structure
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail 
 */
mlan_status
woal_uap_get_stats(moal_private * priv, t_u8 wait_option,
                   mlan_ds_uap_stats * ustats)
{
    mlan_ds_get_info *info = NULL;
    mlan_ioctl_req *req = NULL;
    mlan_status status = MLAN_STATUS_SUCCESS;

    ENTER();

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
    if (req == NULL) {
        LEAVE();
        return MLAN_STATUS_FAILURE;
    }

    info = (mlan_ds_get_info *) req->pbuf;
    info->sub_command = MLAN_OID_GET_STATS;
    req->req_id = MLAN_IOCTL_GET_INFO;
    req->action = MLAN_ACT_GET;

    status = woal_request_ioctl(priv, req, wait_option);
    if (status == MLAN_STATUS_SUCCESS) {
        if (ustats)
            memcpy(ustats, &info->param.ustats, sizeof(mlan_ds_uap_stats));
        priv->w_stats.discard.fragment = info->param.ustats.fcs_error_count;
        priv->w_stats.discard.retries = info->param.ustats.retry_count;
        priv->w_stats.discard.misc = info->param.ustats.ack_failure_count;
    }

    if (req && (status != MLAN_STATUS_PENDING))
        kfree(req);

    LEAVE();
    return status;
}

/**
 *  @brief Set/Get system configuration parameters
 *
 *  @param priv 		    A pointer to moal_private structure
 *  @param action           MLAN_ACT_SET or MLAN_ACT_GET
 *  @param sys_cfg          A pointer to mlan_uap_bss_param structure
 *
 *  @return                 MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_set_get_sys_config(moal_private * priv, t_u16 action,
                        mlan_uap_bss_param * sys_cfg)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_bss *bss = NULL;
    mlan_ioctl_req *req = NULL;

    ENTER();

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    bss = (mlan_ds_bss *) req->pbuf;
    bss->sub_command = MLAN_OID_UAP_BSS_CONFIG;
    req->req_id = MLAN_IOCTL_BSS;
    req->action = action;

    if (action == MLAN_ACT_SET) {
        memcpy(&bss->param.bss_config, sys_cfg, sizeof(mlan_uap_bss_param));
    }

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    if (action == MLAN_ACT_GET) {
        memcpy(sys_cfg, &bss->param.bss_config, sizeof(mlan_uap_bss_param));
    }

  done:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}

/**
 *  @brief Set invalid data for each member of mlan_uap_bss_param
 *  structure
 *
 *  @param config   A pointer to mlan_uap_bss_param structure
 *
 *  @return         N/A
 */
void
woal_set_sys_config_invalid_data(mlan_uap_bss_param * config)
{
    ENTER();

    memset(config, 0, sizeof(mlan_uap_bss_param));
    config->bcast_ssid_ctl = 0x7F;
    config->radio_ctl = 0x7F;
    config->dtim_period = 0x7F;
    config->beacon_period = 0x7FFF;
    config->tx_data_rate = 0x7FFF;
    config->mcbc_data_rate = 0x7FFF;
    config->tx_power_level = 0x7F;
    config->tx_antenna = 0x7F;
    config->rx_antenna = 0x7F;
    config->pkt_forward_ctl = 0x7F;
    config->max_sta_count = 0x7FFF;
    config->auth_mode = 0x7F;
    config->sta_ageout_timer = 0x7FFFFFFF;
    config->pairwise_update_timeout = 0x7FFFFFFF;
    config->pwk_retries = 0x7FFFFFFF;
    config->groupwise_update_timeout = 0x7FFFFFFF;
    config->gwk_retries = 0x7FFFFFFF;
    config->mgmt_ie_passthru_mask = 0x7FFFFFFF;
    config->ps_sta_ageout_timer = 0x7FFFFFFF;
    config->rts_threshold = 0x7FFF;
    config->frag_threshold = 0x7FFF;
    config->retry_limit = 0x7FFF;
    config->filter.filter_mode = 0x7FFF;
    config->filter.mac_count = 0x7FFF;
    config->wpa_cfg.rsn_protection = 0x7F;
    config->wpa_cfg.gk_rekey_time = 0x7FFFFFFF;
    config->enable_2040coex = 0x7F;

    LEAVE();
}

/** 
 *  @brief Handle get info resp
 *   
 *  @param priv 	Pointer to moal_private structure
 *  @param info 	Pointer to mlan_ds_get_info structure
 *
 *  @return    		N/A
 */
void
woal_ioctl_get_uap_info_resp(moal_private * priv, mlan_ds_get_info * info)
{
    ENTER();
    switch (info->sub_command) {
    case MLAN_OID_GET_STATS:
        priv->w_stats.discard.fragment = info->param.ustats.fcs_error_count;
        priv->w_stats.discard.retries = info->param.ustats.retry_count;
        priv->w_stats.discard.misc = info->param.ustats.ack_failure_count;
        break;
    default:
        break;
    }
    LEAVE();
}
