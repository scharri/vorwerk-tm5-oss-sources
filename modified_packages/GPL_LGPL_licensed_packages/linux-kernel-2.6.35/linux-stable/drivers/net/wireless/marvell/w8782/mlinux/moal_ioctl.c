/** @file moal_ioctl.c
  *
  * @brief This file contains ioctl function to MLAN
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

#include        "moal_main.h"
#ifdef UAP_SUPPORT
#include    	"moal_uap.h"
#endif

/********************************************************
                Local Variables
********************************************************/
/* CAC Measure report default time 60 seconds */
#define MEAS_REPORT_TIME 60*HZ

#ifdef STA_SUPPORT
/** Region code mapping */
typedef struct _region_code_mapping_t
{
    /** Region */
    t_u8 region[COUNTRY_CODE_LEN];
    /** Code */
    t_u8 code;
} region_code_mapping_t;

/** Region code mapping table */
static region_code_mapping_t region_code_mapping[] = {
    {"US ", 0x10},              /* US FCC */
    {"CA ", 0x20},              /* IC Canada */
    {"SG ", 0x10},              /* Singapore */
    {"EU ", 0x30},              /* ETSI */
    {"AU ", 0x30},              /* Australia */
    {"KR ", 0x30},              /* Republic Of Korea */
    {"FR ", 0x32},              /* France */
    {"CN ", 0x50},              /* China */
    {"JP ", 0xFF},              /* Japan special */
};
#endif

/********************************************************
                Global Variables
********************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
#ifdef UAP_SUPPORT
/** Network device handlers for uAP */
extern const struct net_device_ops woal_uap_netdev_ops;
#endif
#ifdef STA_SUPPORT
/** Network device handlers for STA */
extern const struct net_device_ops woal_netdev_ops;
#endif
#endif

/********************************************************
                Local Functions
********************************************************/
#ifdef STA_SUPPORT
/** 
 *  @brief This function converts region string to region code
 *
 *  @param region_string    Region string
 *
 *  @return                 Region code
 */
static t_u8
region_string_2_region_code(char *region_string)
{
    t_u8 i;
    t_u8 size = sizeof(region_code_mapping) / sizeof(region_code_mapping_t);

    for (i = 0; i < size; i++) {
        if (!memcmp(region_string,
                    region_code_mapping[i].region, strlen(region_string))) {
            return (region_code_mapping[i].code);
        }
    }
    /* Default is US */
    return (region_code_mapping[0].code);
}
#endif

/** 
 *  @brief Copy multicast table
 *   
 *  @param mlist    A pointer to mlan_multicast_list structure
 *  @param dev      A pointer to net_device structure                 
 *
 *  @return         Number of multicast addresses
 */
static inline int
woal_copy_mcast_addr(mlan_multicast_list * mlist, struct net_device *dev)
{
    int i = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
    struct dev_mc_list *mcptr = dev->mc_list;
#else
    struct netdev_hw_addr *mcptr = NULL;
#endif /* < 2.6.35 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
    for (i = 0; i < dev->mc_count && mcptr; i++) {
        memcpy(&mlist->mac_list[i], mcptr->dmi_addr, ETH_ALEN);
        mcptr = mcptr->next;
    }
#else
    netdev_for_each_mc_addr(mcptr, dev)
        memcpy(&mlist->mac_list[i++], mcptr->addr, ETH_ALEN);
#endif /* < 2.6.35 */

    return i;
}

/** 
 *  @brief Fill in wait queue 
 *   
 *  @param priv         A pointer to moal_private structure
 *  @param wait         A pointer to wait_queue structure                 
 *  @param wait_option  Wait option
 *
 *  @return             N/A
 */
static inline void
woal_fill_wait_queue(moal_private * priv, wait_queue * wait, t_u8 wait_option)
{
    ENTER();
    wait->start_time = jiffies;
    wait->condition = MFALSE;
    switch (wait_option) {
    case MOAL_NO_WAIT:
        break;
    case MOAL_IOCTL_WAIT:
        wait->wait = &priv->ioctl_wait_q;
        break;
    case MOAL_CMD_WAIT:
        wait->wait = &priv->cmd_wait_q;
        break;
    case MOAL_PROC_WAIT:
        wait->wait = &priv->proc_wait_q;
        break;
    case MOAL_WSTATS_WAIT:
        wait->wait = &priv->w_stats_wait_q;
        break;
    }
    LEAVE();
    return;
}

/** 
 *  @brief Wait mlan ioctl complete
 *   
 *  @param priv         A pointer to moal_private structure
 *  @param req          A pointer to mlan_ioctl_req structure   
 *  @param wait_option  Wait option
 *
 *  @return             N/A
 */
static inline void
woal_wait_ioctl_complete(moal_private * priv, mlan_ioctl_req * req,
                         t_u8 wait_option)
{
    mlan_status status;
    wait_queue *wait = (wait_queue *) req->reserved_1;

    ENTER();

    switch (wait_option) {
    case MOAL_NO_WAIT:
        break;
    case MOAL_IOCTL_WAIT:
        wait_event_interruptible(priv->ioctl_wait_q, wait->condition);
        break;
    case MOAL_CMD_WAIT:
        wait_event_interruptible(priv->cmd_wait_q, wait->condition);
        break;
    case MOAL_PROC_WAIT:
        wait_event_interruptible(priv->proc_wait_q, wait->condition);
        break;
    case MOAL_WSTATS_WAIT:
        wait_event_interruptible(priv->w_stats_wait_q, wait->condition);
        break;
    }
    if (wait->condition == MFALSE) {
        req->action = MLAN_ACT_CANCEL;
        status = mlan_ioctl(priv->phandle->pmlan_adapter, req);
        PRINTM(MIOCTL,
               "IOCTL cancel: id=0x%lx, sub_id=0x%lx, wait_option=%d, action=%d, status=%d\n",
               req->req_id, (*(t_u32 *) req->pbuf), wait_option,
               (int) req->action, status);
    }
    LEAVE();
    return;
}

/********************************************************
                Global Functions
********************************************************/

/** 
 *  @brief Send ioctl request to MLAN
 *   
 *  @param priv          A pointer to moal_private structure
 *  @param req           A pointer to mlan_ioctl_req buffer
 *  @param wait_option   Wait option (MOAL_WAIT or MOAL_NO_WAIT)
 *
 *  @return              MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_request_ioctl(moal_private * priv, mlan_ioctl_req * req, t_u8 wait_option)
{
    wait_queue *wait;
    mlan_status status;

    ENTER();

    if (priv->phandle->surprise_removed == MTRUE) {
        PRINTM(MERROR,
               "IOCTL is not allowed while the device is not present\n");
        LEAVE();
        return MLAN_STATUS_FAILURE;
    }
    if (priv->phandle->is_suspended == MTRUE) {
        PRINTM(MERROR, "IOCTL is not allowed while suspended\n");
        LEAVE();
        return MLAN_STATUS_FAILURE;
    }

    wait = (wait_queue *) req->reserved_1;
    req->bss_index = priv->bss_index;
    if (wait_option)
        woal_fill_wait_queue(priv, wait, wait_option);
    else
        req->reserved_1 = 0;

    /* Call MLAN ioctl handle */
    status = mlan_ioctl(priv->phandle->pmlan_adapter, req);
    switch (status) {
    case MLAN_STATUS_PENDING:
        PRINTM(MIOCTL,
               "IOCTL pending: %p id=0x%lx, sub_id=0x%lx wait_option=%d, action=%d\n",
               req, req->req_id, (*(t_u32 *) req->pbuf), wait_option,
               (int) req->action);
        atomic_inc(&priv->phandle->ioctl_pending);
        /* Status pending, wake up main process */
        queue_work(priv->phandle->workqueue, &priv->phandle->main_work);

        /* Wait for completion */
        if (wait_option) {
            woal_wait_ioctl_complete(priv, req, wait_option);
            status = wait->status;
        }
        break;
    case MLAN_STATUS_SUCCESS:
    case MLAN_STATUS_FAILURE:
    case MLAN_STATUS_RESOURCE:
        PRINTM(MIOCTL,
               "IOCTL: %p id=0x%lx, sub_id=0x%lx wait_option=%d, action=%d status=%d\n",
               req, req->req_id, (*(t_u32 *) req->pbuf), wait_option,
               (int) req->action, status);
    default:
        break;
    }
    LEAVE();
    return status;
}

/** 
 *  @brief Send set MAC address request to MLAN
 *   
 *  @param priv   A pointer to moal_private structure
 *
 *  @return       MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_request_set_mac_address(moal_private * priv)
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
    bss->sub_command = MLAN_OID_BSS_MAC_ADDR;
    memcpy(&bss->param.mac_addr, priv->current_addr,
           sizeof(mlan_802_11_mac_addr));
    req->req_id = MLAN_IOCTL_BSS;
    req->action = MLAN_ACT_SET;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, MOAL_CMD_WAIT);
    if (status == MLAN_STATUS_SUCCESS) {
        memcpy(priv->netdev->dev_addr, priv->current_addr, ETH_ALEN);
        HEXDUMP("priv->MacAddr:", priv->current_addr, ETH_ALEN);
    } else {
        PRINTM(MERROR, "set mac address failed! status=%d, error_code=0x%lx\n",
               status, req->status_code);
    }
  done:
    if (req)
        kfree(req);
    LEAVE();
    return status;
}

/** 
 *  @brief Send multicast list request to MLAN
 *   
 *  @param priv   A pointer to moal_private structure
 *  @param dev    A pointer to net_device structure                 
 *
 *  @return       N/A
 */
void
woal_request_set_multicast_list(moal_private * priv, struct net_device *dev)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_bss *bss = NULL;
    mlan_status status;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
    int mc_count = dev->mc_count;
#else
    int mc_count = netdev_mc_count(dev);
#endif

    ENTER();

    /* Allocate an IOCTL request buffer */
    req = (mlan_ioctl_req *) woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        PRINTM(MERROR, "%s:Fail to allocate ioctl req buffer\n", __FUNCTION__);
        goto done;
    }

    /* Fill request buffer */
    bss = (mlan_ds_bss *) req->pbuf;
    bss->sub_command = MLAN_OID_BSS_MULTICAST_LIST;
    req->req_id = MLAN_IOCTL_BSS;
    req->action = MLAN_ACT_SET;
    if (dev->flags & IFF_PROMISC) {
        bss->param.multicast_list.mode = MLAN_PROMISC_MODE;
    } else if (dev->flags & IFF_ALLMULTI ||
               mc_count > MLAN_MAX_MULTICAST_LIST_SIZE) {
        bss->param.multicast_list.mode = MLAN_ALL_MULTI_MODE;
    } else {
        bss->param.multicast_list.mode = MLAN_MULTICAST_MODE;
        if (mc_count)
            bss->param.multicast_list.num_multicast_addr =
                woal_copy_mcast_addr(&bss->param.multicast_list, dev);
    }

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, MOAL_NO_WAIT);
    if (status != MLAN_STATUS_PENDING)
        kfree(req);
  done:
    LEAVE();
    return;
}

/** 
 *  @brief Send deauth command to MLAN
 *   
 *  @param priv          A pointer to moal_private structure
 *  @param wait_option          Wait option
 *  @param mac           MAC address to deauthenticate
 *
 *  @return              MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_disconnect(moal_private * priv, t_u8 wait_option, t_u8 * mac)
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
    bss->sub_command = MLAN_OID_BSS_STOP;
    if (mac)
        memcpy((t_u8 *) & bss->param.bssid, mac, sizeof(mlan_802_11_mac_addr));
    req->req_id = MLAN_IOCTL_BSS;
    req->action = MLAN_ACT_SET;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, wait_option);

  done:
    if (req)
        kfree(req);
#ifdef REASSOCIATION
    priv->reassoc_required = MFALSE;
#endif /* REASSOCIATION */
    LEAVE();
    return status;
}

/** 
 *  @brief Send bss_start command to MLAN
 *   
 *  @param priv          A pointer to moal_private structure
 *  @param wait_option          Wait option  
 *  @param ssid_bssid    A point to mlan_ssid_bssid structure
 *
 *  @return              MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_bss_start(moal_private * priv, t_u8 wait_option,
               mlan_ssid_bssid * ssid_bssid)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_bss *bss = NULL;
    mlan_status status;

    ENTER();

    /* Stop the O.S. TX queue if needed */
    woal_stop_queue(priv->netdev);

    /* Allocate an IOCTL request buffer */
    req = (mlan_ioctl_req *) woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        status = MLAN_STATUS_FAILURE;
        goto done;
    }

    /* Fill request buffer */
    bss = (mlan_ds_bss *) req->pbuf;
    bss->sub_command = MLAN_OID_BSS_START;
    if (ssid_bssid)
        memcpy(&bss->param.ssid_bssid, ssid_bssid, sizeof(mlan_ssid_bssid));
    req->req_id = MLAN_IOCTL_BSS;
    req->action = MLAN_ACT_SET;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, wait_option);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return status;
}

/** 
 *  @brief Get BSS info
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param wait_option          Wait option
 *  @param bss_info             A pointer to mlan_bss_info structure
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail          
 */
mlan_status
woal_get_bss_info(moal_private * priv, t_u8 wait_option,
                  mlan_bss_info * bss_info)
{
    int ret = 0;
    mlan_ioctl_req *req = NULL;
    mlan_ds_get_info *info = NULL;
    mlan_status status = MLAN_STATUS_SUCCESS;
    ENTER();

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Fill request buffer */
    info = (mlan_ds_get_info *) req->pbuf;
    info->sub_command = MLAN_OID_GET_BSS_INFO;
    req->req_id = MLAN_IOCTL_GET_INFO;
    req->action = MLAN_ACT_GET;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, wait_option);
    if (status == MLAN_STATUS_SUCCESS) {
        if (bss_info) {
            memcpy(bss_info, &info->param.bss_info, sizeof(mlan_bss_info));
        }
    }
  done:
    if (req && (status != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return status;
}

/** 
 *  @brief Set/Get retry count
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param action               Action set or get
 *  @param wait_option          Wait option
 *  @param value                Retry value
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_set_get_retry(moal_private * priv, t_u32 action,
                   t_u8 wait_option, int *value)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_snmp_mib *mib = NULL;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    ENTER();

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_snmp_mib));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    /* Fill request buffer */
    mib = (mlan_ds_snmp_mib *) req->pbuf;
    mib->sub_command = MLAN_OID_SNMP_MIB_RETRY_COUNT;
    req->req_id = MLAN_IOCTL_SNMP_MIB;
    req->action = action;

    if (action == MLAN_ACT_SET) {
        if (*value < MLAN_TX_RETRY_MIN || *value > MLAN_TX_RETRY_MAX) {
            ret = MLAN_STATUS_FAILURE;
            goto done;
        }
        mib->param.retry_count = *value;
    }

    /* Send IOCTL request to MLAN */
    ret = woal_request_ioctl(priv, req, wait_option);
    if (ret == MLAN_STATUS_SUCCESS && action == MLAN_ACT_GET) {
        *value = mib->param.retry_count;
    }

  done:
    if (req && (ret != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set/Get RTS threshold
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param action               Action set or get
 *  @param wait_option          Wait option
 *  @param value                RTS threshold value
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_set_get_rts(moal_private * priv, t_u32 action,
                 t_u8 wait_option, int *value)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_snmp_mib *mib = NULL;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    ENTER();

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_snmp_mib));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    /* Fill request buffer */
    mib = (mlan_ds_snmp_mib *) req->pbuf;
    mib->sub_command = MLAN_OID_SNMP_MIB_RTS_THRESHOLD;
    req->req_id = MLAN_IOCTL_SNMP_MIB;
    req->action = action;

    if (action == MLAN_ACT_SET) {
        if (*value < MLAN_RTS_MIN_VALUE || *value > MLAN_RTS_MAX_VALUE) {
            ret = MLAN_STATUS_FAILURE;
            goto done;
        }
        mib->param.rts_threshold = *value;
    }

    /* Send IOCTL request to MLAN */
    ret = woal_request_ioctl(priv, req, wait_option);
    if (ret == MLAN_STATUS_SUCCESS && action == MLAN_ACT_GET) {
        *value = mib->param.rts_threshold;
    }

  done:
    if (req && (ret != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set/Get Fragment threshold
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param action               Action set or get
 *  @param wait_option          Wait option
 *  @param value                Fragment threshold value
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_set_get_frag(moal_private * priv, t_u32 action,
                  t_u8 wait_option, int *value)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_snmp_mib *mib = NULL;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    ENTER();

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_snmp_mib));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    /* Fill request buffer */
    mib = (mlan_ds_snmp_mib *) req->pbuf;
    mib->sub_command = MLAN_OID_SNMP_MIB_FRAG_THRESHOLD;
    req->req_id = MLAN_IOCTL_SNMP_MIB;
    req->action = action;

    if (action == MLAN_ACT_SET) {
        if (*value < MLAN_FRAG_MIN_VALUE || *value > MLAN_FRAG_MAX_VALUE) {
            ret = MLAN_STATUS_FAILURE;
            goto done;
        }
        mib->param.frag_threshold = *value;
    }

    /* Send IOCTL request to MLAN */
    ret = woal_request_ioctl(priv, req, wait_option);
    if (ret == MLAN_STATUS_SUCCESS && action == MLAN_ACT_GET) {
        *value = mib->param.frag_threshold;
    }

  done:
    if (req && (ret != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set/Get generic IE
 *
 *  @param priv         A pointer to moal_private structure
 *  @param action       Action set or get
 *  @param ie           Information element
 *  @param ie_len       Length of the IE
 *
 *  @return             MLAN_STATUS_SUCCESS -- success, otherwise fail
 */
mlan_status
woal_set_get_gen_ie(moal_private * priv, t_u32 action, t_u8 * ie, int *ie_len)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_misc_cfg *misc = NULL;
    mlan_ioctl_req *req = NULL;

    ENTER();

    if (ie == NULL || ie_len == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    if (action == MLAN_ACT_SET && *ie_len > MAX_IE_SIZE) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    misc = (mlan_ds_misc_cfg *) req->pbuf;
    misc->sub_command = MLAN_OID_MISC_GEN_IE;
    req->req_id = MLAN_IOCTL_MISC_CFG;
    req->action = action;
    misc->param.gen_ie.type = MLAN_IE_TYPE_GEN_IE;

    if (action == MLAN_ACT_SET) {
        misc->param.gen_ie.len = *ie_len;
        if (*ie_len)
            memcpy(misc->param.gen_ie.ie_data, ie, *ie_len);
    }

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    if (action == MLAN_ACT_GET) {
        *ie_len = misc->param.gen_ie.len;
        if (*ie_len)
            memcpy(ie, misc->param.gen_ie.ie_data, *ie_len);
    }

  done:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}

/** 
 *  @brief Set/Get TX power
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param action               Action set or get
 *  @param power_cfg            A pinter to mlan_power_cfg_t structure
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_set_get_tx_power(moal_private * priv,
                      t_u32 action, mlan_power_cfg_t * power_cfg)
{
    mlan_ds_power_cfg *pcfg = NULL;
    mlan_ioctl_req *req = NULL;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    ENTER();

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_power_cfg));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }
    pcfg = (mlan_ds_power_cfg *) req->pbuf;
    pcfg->sub_command = MLAN_OID_POWER_CFG;
    req->req_id = MLAN_IOCTL_POWER_CFG;
    req->action = action;
    if (action == MLAN_ACT_SET && power_cfg)
        memcpy(&pcfg->param.power_cfg, power_cfg, sizeof(mlan_power_cfg_t));

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT))
        ret = MLAN_STATUS_FAILURE;

    if (ret == MLAN_STATUS_SUCCESS && power_cfg)
        memcpy(power_cfg, &pcfg->param.power_cfg, sizeof(mlan_power_cfg_t));

  done:
    if (req && (ret != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set/Get IEEE power management
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param action               Action set or get
 *  @param disabled             A pointer to disabled flag
 *  @param power_type           IEEE power type
 *
 *  @return                     MLAN_STATUS_SUCCESS -- success, otherwise fail
 */
mlan_status
woal_set_get_power_mgmt(moal_private * priv,
                        t_u32 action, int *disabled, int power_type)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ioctl_req *req = NULL;
    mlan_ds_pm_cfg *pm_cfg = NULL;

    ENTER();

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_pm_cfg));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }
    pm_cfg = (mlan_ds_pm_cfg *) req->pbuf;
    pm_cfg->sub_command = MLAN_OID_PM_CFG_IEEE_PS;
    req->req_id = MLAN_IOCTL_PM_CFG;
    req->action = action;

    if (action == MLAN_ACT_SET) {
        PRINTM(MINFO, "PS_MODE set power disabled=%d power type=%#x\n",
               *disabled, power_type);
        if (*disabled)
            pm_cfg->param.ps_mode = 0;
        else {
            /* Check not support case only (vwrq->disabled == FALSE) */
            if ((power_type & IW_POWER_TYPE) == IW_POWER_TIMEOUT) {
                PRINTM(MERROR, "Setting power timeout is not supported\n");
                ret = MLAN_STATUS_FAILURE;
                goto done;
            } else if ((power_type & IW_POWER_TYPE) == IW_POWER_PERIOD) {
                PRINTM(MERROR, "Setting power period is not supported\n");
                ret = MLAN_STATUS_FAILURE;
                goto done;
            }
            pm_cfg->param.ps_mode = 1;
        }
    }

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    if (ret == MLAN_STATUS_SUCCESS && action == MLAN_ACT_GET)
        *disabled = pm_cfg->param.ps_mode;

  done:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}

#ifdef STA_SUPPORT
/**
 * @brief Set region code
 * 
 * @param priv     A pointer to moal_private structure
 * @param region   A pointer to region string
 * 
 * @return         MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING --success, otherwise fail
 */
mlan_status
woal_set_region_code(moal_private * priv, char *region)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_misc_cfg *cfg = NULL;
    mlan_ioctl_req *req = NULL;

    ENTER();
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }
    cfg = (mlan_ds_misc_cfg *) req->pbuf;
    cfg->sub_command = MLAN_OID_MISC_REGION;
    req->req_id = MLAN_IOCTL_MISC_CFG;
    req->action = MLAN_ACT_SET;
    cfg->param.region_code = region_string_2_region_code(region);
    ret = woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}
#endif /* STA_SUPPORT */

/** 
 *  @brief get data rate 
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param datarate             A pointer to mlan_ds_rate structure
 *
 *  @return                     MLAN_STATUS_SUCCESS -- success, otherwise fail          
 */
mlan_status
woal_get_data_rate(moal_private * priv, mlan_ds_rate * datarate)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_rate *rate = NULL;
    mlan_ioctl_req *req = NULL;

    ENTER();

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_rate));
    if (req == NULL) {
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    rate = (mlan_ds_rate *) req->pbuf;
    rate->param.rate_cfg.rate_type = MLAN_RATE_VALUE;
    rate->sub_command = MLAN_OID_RATE_CFG;
    req->req_id = MLAN_IOCTL_RATE;
    req->action = MLAN_ACT_GET;

    if (MLAN_STATUS_SUCCESS == woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        if (datarate)
            memcpy(datarate, rate, sizeof(mlan_ds_rate));
    } else {
        ret = MLAN_STATUS_FAILURE;
    }

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief Send get FW info request to MLAN
 *   
 *  @param priv             A pointer to moal_private structure
 *  @param wait_option      Wait option  
 *  @param fw_info          FW information
 *
 *  @return                 MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail          
 */
mlan_status
woal_request_get_fw_info(moal_private * priv, t_u8 wait_option,
                         mlan_fw_info * fw_info)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_get_info *info;
    mlan_status status;
    ENTER();
    memset(priv->current_addr, 0xff, ETH_ALEN);

    /* Allocate an IOCTL request buffer */
    req = (mlan_ioctl_req *) woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        status = MLAN_STATUS_FAILURE;
        goto done;
    }

    /* Fill request buffer */
    info = (mlan_ds_get_info *) req->pbuf;
    req->req_id = MLAN_IOCTL_GET_INFO;
    req->action = MLAN_ACT_GET;
    info->sub_command = MLAN_OID_GET_FW_INFO;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, wait_option);
    if (status == MLAN_STATUS_SUCCESS) {
        priv->phandle->fw_release_number = info->param.fw_info.fw_ver;
        if (priv->current_addr[0] == 0xff)
            memcpy(priv->current_addr, &info->param.fw_info.mac_addr,
                   sizeof(mlan_802_11_mac_addr));
        memcpy(priv->netdev->dev_addr, priv->current_addr, ETH_ALEN);
        if (fw_info)
            memcpy(fw_info, &info->param.fw_info, sizeof(mlan_fw_info));
        DBG_HEXDUMP(MCMD_D, "mac", priv->current_addr, 6);
    } else
        PRINTM(MERROR, "get fw info failed! status=%d, error_code=0x%lx\n",
               status, req->status_code);
  done:
    if (req)
        kfree(req);
    LEAVE();
    return status;
}

#ifdef PROC_DEBUG
/** 
 *  @brief Get debug info
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param wait_option          Wait option
 *  @param debug_info           A pointer to mlan_debug_info structure
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail          
 */
mlan_status
woal_get_debug_info(moal_private * priv, t_u8 wait_option,
                    mlan_debug_info * debug_info)
{
    int ret = 0;
    mlan_ioctl_req *req = NULL;
    mlan_ds_get_info *info = NULL;
    mlan_status status = MLAN_STATUS_SUCCESS;
    ENTER();

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Fill request buffer */
    info = (mlan_ds_get_info *) req->pbuf;
    info->sub_command = MLAN_OID_GET_DEBUG_INFO;
    req->req_id = MLAN_IOCTL_GET_INFO;
    req->action = MLAN_ACT_GET;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, wait_option);
    if (status == MLAN_STATUS_SUCCESS) {
        if (debug_info) {
            memcpy(debug_info, &info->param.debug_info,
                   sizeof(mlan_debug_info));
        }
    }
  done:
    if (req && (status != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return status;
}

/** 
 *  @brief Set debug info
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param wait_option          Wait option
 *  @param debug_info           A pointer to mlan_debug_info structure
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail          
 */
mlan_status
woal_set_debug_info(moal_private * priv, t_u8 wait_option,
                    mlan_debug_info * debug_info)
{
    int ret = 0;
    mlan_ioctl_req *req = NULL;
    mlan_ds_get_info *info = NULL;
    mlan_status status = MLAN_STATUS_SUCCESS;

    ENTER();

    if (!debug_info) {
        ret = -EINVAL;
        LEAVE();
        return MLAN_STATUS_FAILURE;
    }

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Fill request buffer */
    info = (mlan_ds_get_info *) req->pbuf;
    info->sub_command = MLAN_OID_GET_DEBUG_INFO;
    memcpy(&info->param.debug_info, debug_info, sizeof(mlan_debug_info));
    req->req_id = MLAN_IOCTL_GET_INFO;
    req->action = MLAN_ACT_SET;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, wait_option);
  done:
    if (req && (status != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return status;
}
#endif /* PROC_DEBUG */

/** 
 *  @brief host command ioctl function
 *   
 *  @param priv		A pointer to moal_private structure
 *  @param wrq 		A pointer to iwreq structure
 *  @return    		0 --success, otherwise fail
 */
int
woal_host_command(moal_private * priv, struct iwreq *wrq)
{
    HostCmd_Header cmd_header;
    int ret = 0;
    mlan_ds_misc_cfg *misc = NULL;
    mlan_ioctl_req *req = NULL;

    ENTER();

    /* Sanity check */
    if (wrq->u.data.pointer == NULL) {
        PRINTM(MERROR, "hostcmd IOCTL corrupt data\n");
        ret = -EINVAL;
        goto done;
    }
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    misc = (mlan_ds_misc_cfg *) req->pbuf;
    memset(&cmd_header, 0, sizeof(cmd_header));

    /* get command header */
    if (copy_from_user
        (&cmd_header, wrq->u.data.pointer, sizeof(HostCmd_Header))) {
        PRINTM(MERROR, "copy from user failed: Host command header\n");
        ret = -EFAULT;
        goto done;
    }
    misc->param.hostcmd.len = woal_le16_to_cpu(cmd_header.size);

    PRINTM(MINFO, "Host command len = %lu\n", misc->param.hostcmd.len);

    if (!misc->param.hostcmd.len ||
        misc->param.hostcmd.len > MLAN_SIZE_OF_CMD_BUFFER) {
        PRINTM(MERROR, "Invalid data buffer length\n");
        ret = -EINVAL;
        goto done;
    }

    /* get the whole command from user */
    if (copy_from_user
        (misc->param.hostcmd.cmd, wrq->u.data.pointer,
         woal_le16_to_cpu(cmd_header.size))) {
        PRINTM(MERROR, "copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }
    misc->sub_command = MLAN_OID_MISC_HOST_CMD;
    req->req_id = MLAN_IOCTL_MISC_CFG;

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    if (copy_to_user
        (wrq->u.data.pointer, (t_u8 *) misc->param.hostcmd.cmd,
         misc->param.hostcmd.len)) {
        ret = -EFAULT;
        goto done;
    }
    wrq->u.data.length = misc->param.hostcmd.len;
  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

#if defined(WFD_SUPPORT) || defined(UAP_SUPPORT)
/** 
 *  @brief host command ioctl function
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
/*********  format of ifr_data *************/
/*    buf_len + Hostcmd_body 		   */
/*    buf_len: 4 bytes                     */
/*             the length of the buf which */
/*             can be used to return data  */
/*             to application		   */
/*    Hostcmd_body       	           */
/*******************************************/
int
woal_hostcmd_ioctl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    t_u32 buf_len = 0;
    HostCmd_Header cmd_header;
    int ret = 0;
    mlan_ds_misc_cfg *misc = NULL;
    mlan_ioctl_req *ioctl_req = NULL;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "uap_hostcmd_ioctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }

    if (copy_from_user(&buf_len, req->ifr_data, sizeof(buf_len))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    memset(&cmd_header, 0, sizeof(cmd_header));

    /* get command header */
    if (copy_from_user
        (&cmd_header, req->ifr_data + sizeof(buf_len),
         sizeof(HostCmd_Header))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    PRINTM(MINFO, "Host command len = %d\n", woal_le16_to_cpu(cmd_header.size));

    if (woal_le16_to_cpu(cmd_header.size) > MLAN_SIZE_OF_CMD_BUFFER) {
        ret = -EINVAL;
        goto done;
    }

    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    misc = (mlan_ds_misc_cfg *) ioctl_req->pbuf;

    misc->param.hostcmd.len = woal_le16_to_cpu(cmd_header.size);

    /* get the whole command from user */
    if (copy_from_user
        (misc->param.hostcmd.cmd, req->ifr_data + sizeof(buf_len),
         misc->param.hostcmd.len)) {
        PRINTM(MERROR, "copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }
    misc->sub_command = MLAN_OID_MISC_HOST_CMD;
    ioctl_req->req_id = MLAN_IOCTL_MISC_CFG;

    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    if (misc->param.hostcmd.len > buf_len) {
        PRINTM(MERROR, "buf_len is too small, resp_len=%d, buf_len=%d\n",
               (int) misc->param.hostcmd.len, (int) buf_len);
        ret = -EFAULT;
        goto done;
    }
    if (copy_to_user
        (req->ifr_data + sizeof(buf_len), (t_u8 *) misc->param.hostcmd.cmd,
         misc->param.hostcmd.len)) {
        ret = -EFAULT;
        goto done;
    }
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}
#endif

/** 
 *  @brief CUSTOM_IE ioctl handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
int
woal_custom_ie_ioctl(struct net_device *dev, struct ifreq *req)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_misc_cfg *misc = NULL;
    mlan_ds_misc_custom_ie *custom_ie = NULL;
    int ret = 0;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(MERROR, "woal_custom_ie_ioctl() corrupt data\n");
        ret = -EFAULT;
        goto done;
    }

    if (!(custom_ie = kmalloc(sizeof(mlan_ds_misc_custom_ie), GFP_KERNEL))) {
        ret = -ENOMEM;
        goto done;
    }
    memset(custom_ie, 0, sizeof(mlan_ds_misc_custom_ie));

    if (copy_from_user
        (custom_ie, req->ifr_data, sizeof(mlan_ds_misc_custom_ie))) {
        PRINTM(MERROR, "Copy from user failed\n");
        ret = -EFAULT;
        goto done;
    }

    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    misc = (mlan_ds_misc_cfg *) ioctl_req->pbuf;
    misc->sub_command = MLAN_OID_MISC_CUSTOM_IE;
    ioctl_req->req_id = MLAN_IOCTL_MISC_CFG;
    if ((custom_ie->len == 0) ||
        (custom_ie->len == sizeof(custom_ie->ie_data_list[0].ie_index)))
        ioctl_req->action = MLAN_ACT_GET;
    else
        ioctl_req->action = MLAN_ACT_SET;

    memcpy(&misc->param.cust_ie, custom_ie, sizeof(mlan_ds_misc_custom_ie));

    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    if (ioctl_req->action == MLAN_ACT_GET) {
        if (copy_to_user
            (req->ifr_data, &misc->param.cust_ie,
             sizeof(mlan_ds_misc_custom_ie))) {
            PRINTM(MERROR, "Copy to user failed!\n");
            ret = -EFAULT;
            goto done;
        }
    } else if (ioctl_req->status_code == MLAN_ERROR_IOCTL_FAIL) {
        /* send a separate error code to indicate error from driver */
        ret = EFAULT;
    }

  done:
    if (ioctl_req)
        kfree(ioctl_req);
    if (custom_ie)
        kfree(custom_ie);
    LEAVE();
    return ret;
}

/** 
 *  @brief ioctl function get BSS type
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
int
woal_get_bss_type(struct net_device *dev, struct ifreq *req)
{
    int ret = 0;
    moal_private *priv = (moal_private *) netdev_priv(dev);
    int bss_type;

    ENTER();

    bss_type = (int) priv->bss_type;
    if (copy_to_user(req->ifr_data, &bss_type, sizeof(int))) {
        PRINTM(MINFO, "Copy to user failed!\n");
        ret = -EFAULT;
    }

    LEAVE();
    return ret;
}

#if defined(WFD_SUPPORT)
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
/**
 * @brief Set/Get BSS role
 * 
 * @param priv     A pointer to moal_private structure
 * @param wrq      A pointer to iwreq structure
 * 
 * @return         0 --success, otherwise fail
 */
int
woal_set_get_bss_role(moal_private * priv, struct iwreq *wrq)
{
    int ret = 0;
    mlan_ds_bss *bss = NULL;
    mlan_ioctl_req *req = NULL;
    int bss_role = 0;
    struct net_device *dev = priv->netdev;

    ENTER();

    if (priv->bss_type != MLAN_BSS_TYPE_WFD) {
        PRINTM(MWARN, "Command is not allowed for this interface\n");
        ret = -EPERM;
        goto done;
    }
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    bss = (mlan_ds_bss *) req->pbuf;
    bss->sub_command = MLAN_OID_BSS_ROLE;
    req->req_id = MLAN_IOCTL_BSS;
    if (wrq->u.data.length) {
        if (copy_from_user(&bss_role, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(MERROR, "Copy from user failed\n");
            ret = -EFAULT;
            goto done;
        }
        if (bss_role != MLAN_BSS_ROLE_STA && bss_role != MLAN_BSS_ROLE_UAP) {
            PRINTM(MWARN, "Invalid BSS role\n");
            ret = -EINVAL;
            goto done;
        }
        if (bss_role == GET_BSS_ROLE(priv)) {
            PRINTM(MWARN, "Already BSS is in desired role\n");
            ret = -EINVAL;
            goto done;
        }
        req->action = MLAN_ACT_SET;
        bss->param.bss_role = (t_u8) bss_role;
    } else {
        req->action = MLAN_ACT_GET;
    }

    if (req->action == MLAN_ACT_SET) {
        /* Reset interface */
        woal_reset_intf(priv, MOAL_IOCTL_WAIT, MFALSE);
    }
    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    if (!wrq->u.data.length) {
        bss_role = (int) bss->param.bss_role;
        if (copy_to_user(wrq->u.data.pointer, &bss_role, sizeof(int))) {
            ret = -EFAULT;
            goto done;
        }
        wrq->u.data.length = 1;
    } else {
        /* Update moal_private */
        priv->bss_role = bss_role;
        if (priv->bss_type == MLAN_BSS_TYPE_UAP)
            priv->bss_type = MLAN_BSS_TYPE_STA;
        else if (priv->bss_type == MLAN_BSS_TYPE_STA)
            priv->bss_type = MLAN_BSS_TYPE_UAP;

        /* Initialize private structures */
        woal_init_priv(priv, MOAL_IOCTL_WAIT);

        if (bss_role == MLAN_BSS_ROLE_UAP) {
            /* Switch: STA -> uAP */
            /* Setup the OS Interface to our functions */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
            dev->do_ioctl = woal_uap_do_ioctl;
            dev->set_multicast_list = woal_uap_set_multicast_list;
#else
            dev->netdev_ops = &woal_uap_netdev_ops;
#endif
#ifdef WIRELESS_EXT
#if WIRELESS_EXT < 21
            dev->get_wireless_stats = woal_get_uap_wireless_stats;
#endif
            dev->wireless_handlers =
                (struct iw_handler_def *) &woal_uap_handler_def;
#endif /* WIRELESS_EXT */
            init_waitqueue_head(&priv->w_stats_wait_q);
        } else if (bss_role == MLAN_BSS_ROLE_STA) {
            /* Switch: uAP -> STA */
            /* Setup the OS Interface to our functions */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
            dev->do_ioctl = woal_do_ioctl;
            dev->set_multicast_list = woal_set_multicast_list;
#else
            dev->netdev_ops = &woal_netdev_ops;
#endif
#ifdef  WIRELESS_EXT
#if WIRELESS_EXT < 21
            dev->get_wireless_stats = woal_get_wireless_stats;
#endif
            dev->wireless_handlers =
                (struct iw_handler_def *) &woal_handler_def;
#endif
            init_waitqueue_head(&priv->w_stats_wait_q);
        }
        /* Enable interfaces */
        netif_device_attach(dev);
        woal_start_queue(dev);
    }

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}
#endif
#endif /* WFD_SUPPORT && V14_FEATURE */

/** 
 *  @brief Get Host Sleep parameters
 *
 *  @param priv         A pointer to moal_private structure
 *  @param action       Action: set or get
 *  @param wait_option  Wait option (MOAL_WAIT or MOAL_NO_WAIT)
 *  @param hscfg        A pointer to mlan_ds_hs_cfg structure
 *
 *  @return             MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail          
 */
mlan_status
woal_set_get_hs_params(moal_private * priv, t_u16 action, t_u8 wait_option,
                       mlan_ds_hs_cfg * hscfg)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_pm_cfg *pmcfg = NULL;
    mlan_ioctl_req *req = NULL;

    ENTER();

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_pm_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Fill request buffer */
    pmcfg = (mlan_ds_pm_cfg *) req->pbuf;
    pmcfg->sub_command = MLAN_OID_PM_CFG_HS_CFG;
    req->req_id = MLAN_IOCTL_PM_CFG;
    req->action = action;
    if (action == MLAN_ACT_SET)
        memcpy(&pmcfg->param.hs_cfg, hscfg, sizeof(mlan_ds_hs_cfg));

    /* Send IOCTL request to MLAN */
    ret = woal_request_ioctl(priv, req, wait_option);
    if (ret == MLAN_STATUS_SUCCESS) {
        if (hscfg && action == MLAN_ACT_GET) {
            memcpy(hscfg, &pmcfg->param.hs_cfg, sizeof(mlan_ds_hs_cfg));
        }
    }
  done:
    if (req && (ret != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return ret;
}

/**
 *  @brief Cancel Host Sleep configuration
 *
 *  @param priv             A pointer to moal_private structure
 *  @param wait_option      wait option
 *
 *  @return      MLAN_STATUS_SUCCESS, MLAN_STATUS_PENDING,
 *                      or MLAN_STATUS_FAILURE          
 */
mlan_status
woal_cancel_hs(moal_private * priv, t_u8 wait_option)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_hs_cfg hscfg;

    ENTER();

    /* Cancel Host Sleep */
    hscfg.conditions = HOST_SLEEP_CFG_CANCEL;
    hscfg.is_invoke_hostcmd = MTRUE;
    ret = woal_set_get_hs_params(priv, MLAN_ACT_SET, wait_option, &hscfg);

    LEAVE();
    return ret;
}

/**  @brief This function enables the host sleep
 *  
 *  @param priv   A Pointer to the moal_private structure
 *  @return 	  MTRUE or MFALSE
 */
int
woal_enable_hs(moal_private * priv)
{
    mlan_ds_hs_cfg hscfg;
    moal_handle *handle = NULL;
    int hs_actived = MFALSE;
    int timeout = 0;

    ENTER();

    if (priv == NULL) {
        PRINTM(MERROR, "Invalid priv\n");
        goto done;
    }
    handle = priv->phandle;
    if (handle->hs_activated == MTRUE) {
        PRINTM(MIOCTL, "HS Already actived\n");
        hs_actived = MTRUE;
        goto done;
    }

    /* Enable Host Sleep */
    handle->hs_activate_wait_q_woken = MFALSE;
    memset(&hscfg, 0, sizeof(mlan_ds_hs_cfg));
    hscfg.is_invoke_hostcmd = MTRUE;
    if (woal_set_get_hs_params(priv, MLAN_ACT_SET, MOAL_NO_WAIT, &hscfg) ==
        MLAN_STATUS_FAILURE) {
        PRINTM(MIOCTL, "IOCTL request HS enable failed\n");
        goto done;
    }
    timeout = wait_event_interruptible_timeout(handle->hs_activate_wait_q,
                                               handle->hs_activate_wait_q_woken,
                                               HS_ACTIVE_TIMEOUT);
    if ((handle->hs_activated == MTRUE) || (handle->is_suspended == MTRUE)) {
        PRINTM(MCMND, "suspend success! force=%lu skip=%lu\n",
               handle->hs_force_count, handle->hs_skip_count);
        hs_actived = MTRUE;
    }
    if (hs_actived != MTRUE) {
        handle->hs_skip_count++;
        PRINTM(MCMND, "suspend skipped! timeout=%d skip=%lu\n",
               timeout, handle->hs_skip_count);
        woal_cancel_hs(priv, MOAL_NO_WAIT);
    }
  done:
    LEAVE();
    return hs_actived;
}

/** 
 *  @brief This function send soft_reset command to firmware 
 *  
 *  @param handle   A pointer to moal_handle structure
 *  @return 	    MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING on success, otherwise failure code
 */
mlan_status
woal_request_soft_reset(moal_handle * handle)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ioctl_req *req = NULL;
    mlan_ds_misc_cfg *misc = NULL;

    ENTER();
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req) {
        misc = (mlan_ds_misc_cfg *) req->pbuf;
        misc->sub_command = MLAN_OID_MISC_SOFT_RESET;
        req->req_id = MLAN_IOCTL_MISC_CFG;
        req->action = MLAN_ACT_SET;
        ret =
            woal_request_ioctl(woal_get_priv(handle, MLAN_BSS_ROLE_ANY), req,
                               MOAL_PROC_WAIT);
    }

    handle->surprise_removed = MTRUE;
    woal_sched_timeout(5);
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set wapi enable
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param wait_option          Wait option
 *  @param enable               MTRUE or MFALSE
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail          
 */
mlan_status
woal_set_wapi_enable(moal_private * priv, t_u8 wait_option, t_u32 enable)
{
    int ret = 0;
    mlan_ioctl_req *req = NULL;
    mlan_ds_sec_cfg *sec = NULL;
    mlan_status status = MLAN_STATUS_SUCCESS;
    ENTER();

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_sec_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Fill request buffer */
    sec = (mlan_ds_sec_cfg *) req->pbuf;
    sec->sub_command = MLAN_OID_SEC_CFG_WAPI_ENABLED;
    req->req_id = MLAN_IOCTL_SEC_CFG;
    req->action = MLAN_ACT_SET;
    sec->param.wapi_enabled = enable;

    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, wait_option);
  done:
    if (req && (status != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return status;
}

/** 
 *  @brief Get version 
 *   
 *  @param handle 		A pointer to moal_handle structure
 *  @param version		A pointer to version buffer
 *  @param max_len		max length of version buffer
 *
 *  @return 	   		N/A
 */
void
woal_get_version(moal_handle * handle, char *version, int max_len)
{
    union
    {
        t_u32 l;
        t_u8 c[4];
    } ver;
    char fw_ver[32];

    ENTER();

    ver.l = handle->fw_release_number;
    sprintf(fw_ver, "%u.%u.%u.p%u", ver.c[2], ver.c[1], ver.c[0], ver.c[3]);

    snprintf(version, max_len, driver_version, fw_ver);

    LEAVE();
}

/**
 *  @brief Get Driver Version
 *
 *  @param priv         A pointer to moal_private structure
 *  @param req          A pointer to ifreq structure
 *
 *  @return             0 --success, otherwise fail
 */
int
woal_get_driver_version(moal_private * priv, struct ifreq *req)
{
    struct iwreq *wrq = (struct iwreq *) req;
    int len;
    char buf[MLAN_MAX_VER_STR_LEN];
    ENTER();

    woal_get_version(priv->phandle, buf, sizeof(buf) - 1);

    len = strlen(buf);
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, buf, len)) {
            PRINTM(MERROR, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = len;
    }
    PRINTM(MINFO, "MOAL VERSION: %s\n", buf);
    LEAVE();
    return 0;
}

/**
 *  @brief Get extended driver version
 *
 *  @param priv         A pointer to moal_private structure
 *  @param ireq         A pointer to ifreq structure
 *
 *  @return             0 --success, otherwise fail  
 */
int
woal_get_driver_verext(moal_private * priv, struct ifreq *ireq)
{
    struct iwreq *wrq = (struct iwreq *) ireq;
    mlan_ds_get_info *info = NULL;
    mlan_ioctl_req *req = NULL;
    int ret = 0;

    ENTER();

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
    if (req == NULL) {
        LEAVE();
        return -ENOMEM;
    }

    info = (mlan_ds_get_info *) req->pbuf;
    info->sub_command = MLAN_OID_GET_VER_EXT;
    req->req_id = MLAN_IOCTL_GET_INFO;
    req->action = MLAN_ACT_GET;

    if (!wrq->u.data.flags) {
        info->param.ver_ext.version_str_sel =
            *((int *) (wrq->u.name + SUBCMD_OFFSET));
    } else {
        if (copy_from_user
            (&info->param.ver_ext.version_str_sel, wrq->u.data.pointer,
             sizeof(info->param.ver_ext.version_str_sel))) {
            PRINTM(MERROR, "Copy from user failed\n");
            ret = -EFAULT;
            goto done;
        } else {
            if (((t_s32) (info->param.ver_ext.version_str_sel)) < 0) {
                PRINTM(MERROR, "Invalid arguments!\n");
                ret = -EINVAL;
                goto done;
            }
        }
    }

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, info->param.ver_ext.version_str,
                         strlen(info->param.ver_ext.version_str))) {
            PRINTM(MERROR, "Copy to user failed\n");
            ret = -EFAULT;
            goto done;
        }
        wrq->u.data.length = strlen(info->param.ver_ext.version_str);
    }

    PRINTM(MINFO, "MOAL EXTENDED VERSION: %s\n",
           info->param.ver_ext.version_str);
  done:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}

/** 
 *  @brief Handle ioctl resp 
 *   
 *  @param priv 	Pointer to moal_private structure
 *  @param req		Pointer to mlan_ioctl_req structure
 *
 *  @return    		N/A
 */
void
woal_process_ioctl_resp(moal_private * priv, mlan_ioctl_req * req)
{
    ENTER();

    if (priv == NULL)
        return;
    switch (req->req_id) {
    case MLAN_IOCTL_GET_INFO:
#ifdef STA_SUPPORT
        if (GET_BSS_ROLE(priv) == MLAN_BSS_ROLE_STA)
            woal_ioctl_get_info_resp(priv, (mlan_ds_get_info *) req->pbuf);
#endif
#ifdef UAP_SUPPORT
        if (GET_BSS_ROLE(priv) == MLAN_BSS_ROLE_UAP)
            woal_ioctl_get_uap_info_resp(priv, (mlan_ds_get_info *) req->pbuf);
#endif
        break;
#ifdef STA_SUPPORT
    case MLAN_IOCTL_BSS:
        if (GET_BSS_ROLE(priv) == MLAN_BSS_ROLE_STA)
            woal_ioctl_get_bss_resp(priv, (mlan_ds_bss *) req->pbuf);
#endif
    default:
        break;
    }

    LEAVE();
    return;
}

/** 
 *  @brief Get PM info
 *
 *  @param priv                 A pointer to moal_private structure
 *  @param pm_info              A pointer to mlan_ds_ps_info structure
 *
 *  @return                     MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail          
 */
mlan_status
woal_get_pm_info(moal_private * priv, mlan_ds_ps_info * pm_info)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_pm_cfg *pmcfg = NULL;
    mlan_ioctl_req *req = NULL;

    ENTER();

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_pm_cfg));
    if (req == NULL) {
        PRINTM(MERROR, "Fail to alloc mlan_ds_pm_cfg buffer\n");
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }

    /* Fill request buffer */
    pmcfg = (mlan_ds_pm_cfg *) req->pbuf;
    pmcfg->sub_command = MLAN_OID_PM_INFO;
    req->req_id = MLAN_IOCTL_PM_CFG;
    req->action = MLAN_ACT_GET;

    /* Send IOCTL request to MLAN */
    ret = woal_request_ioctl(priv, req, MOAL_CMD_WAIT);
    if (ret == MLAN_STATUS_SUCCESS) {
        if (pm_info) {
            memcpy(pm_info, &pmcfg->param.ps_info, sizeof(mlan_ds_ps_info));
        }
    }
  done:
    if (req && (ret != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return ret;
}

/** 
 *  @brief Cancel CAC period block
 */
void
woal_cancel_cac_block(moal_private * priv)
{
    /* if during CAC period, wake up wait queue */
    if (priv->phandle->cac_period == MTRUE) {
        priv->phandle->cac_period = MFALSE;
        priv->phandle->meas_start_jiffies = 0;
        if (priv->phandle->delay_bss_start == MTRUE) {
            priv->phandle->delay_bss_start = MFALSE;
        }
        if (priv->phandle->meas_wait_q_woken == MFALSE) {
            priv->phandle->meas_wait_q_woken = MTRUE;
            wake_up_interruptible(&priv->phandle->meas_wait_q);
        }
    }
}
