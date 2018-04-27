/** @file moal_uap.h
  *
  * @brief This file contains uap driver specific defines etc.
  * 
  * Copyright (C) 2009-2011, Marvell International Ltd.  
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
    02/02/2009: initial version
********************************************************/

#ifndef _MOAL_UAP_H
#define _MOAL_UAP_H

/** Private command ID to Host command */
#define	UAP_HOSTCMD			(SIOCDEVPRIVATE + 1)
/** Private command ID to send ioctl */
#define UAP_IOCTL_CMD		(SIOCDEVPRIVATE + 2)
/** Updating ADDBA variables */
#define UAP_ADDBA_PARA		0
/** Updating priority table for AMPDU/AMSDU */
#define UAP_AGGR_PRIOTBL    1
/** Updating addbareject table */
#define UAP_ADDBA_REJECT    2
/** Get FW INFO */
#define UAP_FW_INFO         4
/** Tx data pause subcommand */
#define UAP_TX_DATA_PAUSE    5

/** Private command ID to Power Mode */
#define	UAP_POWER_MODE			(SIOCDEVPRIVATE + 3)

/** Private command id to start/stop/reset bss */
#define UAP_BSS_CTRL        (SIOCDEVPRIVATE + 4)
/** BSS START */
#define UAP_BSS_START               0
/** BSS STOP */
#define UAP_BSS_STOP                1
/** BSS RESET */
#define UAP_BSS_RESET               2
/** Band config 5GHz */
#define BAND_CONFIG_5GHZ            0x01

/** wapi_msg */
typedef struct _wapi_msg
{
    /** message type */
    t_u16 msg_type;
    /** message len */
    t_u16 msg_len;
    /** message */
    t_u8 msg[96];
} wapi_msg;

/* wapi key msg */
typedef struct _wapi_key_msg
{
    /** mac address */
    t_u8 mac_addr[MLAN_MAC_ADDR_LENGTH];
    /** pad */
    t_u8 pad;
    /** key id */
    t_u8 key_id;
    /** key */
    t_u8 key[32];
} wapi_key_msg;

/** Private command ID to set wapi info */
#define	UAP_WAPI_MSG		(SIOCDEVPRIVATE + 10)
/** set wapi flag */
#define  P80211_PACKET_WAPIFLAG     0x0001
/** set wapi key */
#define  P80211_PACKET_SETKEY      0x0003
/** wapi mode psk */
#define WAPI_MODE_PSK    0x04
/** wapi mode certificate */
#define WAPI_MODE_CERT   0x08

/** Auto Rate */
#define AUTO_RATE 0xFF

/** radio control command */
#define	UAP_RADIO_CTL               (SIOCDEVPRIVATE + 5)
/** tx rate cfg command */
#define UAP_TX_RATE_CFG             (SIOCDEVPRIVATE + 8)

/** Private command ID to BSS config */
#define	UAP_BSS_CONFIG              (SIOCDEVPRIVATE + 6)

/** deauth station */
#define	UAP_STA_DEAUTH	            (SIOCDEVPRIVATE + 7)

/** uap get station list */
#define UAP_GET_STA_LIST            (SIOCDEVPRIVATE + 11)

/** Private command ID to set/get custom IE buffer */
#define	UAP_CUSTOM_IE               (SIOCDEVPRIVATE + 13)

/** HS WAKE UP event id */
#define UAP_EVENT_ID_HS_WAKEUP             0x80000001
/** HS_ACTIVATED event id */
#define UAP_EVENT_ID_DRV_HS_ACTIVATED      0x80000002
/** HS DEACTIVATED event id */
#define UAP_EVENT_ID_DRV_HS_DEACTIVATED    0x80000003

/** Private command ID to set/get Host Sleep configuration */
#define UAP_HS_CFG                  (SIOCDEVPRIVATE + 14)
/** Host sleep flag set */
#define HS_CFG_FLAG_GET         0
/** Host sleep flag get */
#define HS_CFG_FLAG_SET         1
/** Host sleep flag for condition */
#define HS_CFG_FLAG_CONDITION   2
/** Host sleep flag for GPIO */
#define HS_CFG_FLAG_GPIO        4
/** Host sleep flag for Gap */
#define HS_CFG_FLAG_GAP         8
/** Host sleep flag for all */
#define HS_CFG_FLAG_ALL         0x0f
/** Host sleep mask to get condition */
#define HS_CFG_CONDITION_MASK   0x0f

/** ds_hs_cfg */
typedef struct _ds_hs_cfg
{
    /** Bit0: 0 - Get, 1 Set
     *  Bit1: 1 - conditions is valid
     *  Bit2: 2 - gpio is valid
     *  Bit3: 3 - gap is valid
     */
    t_u32 flags;
    /** Host sleep config condition */
    /** Bit0: non-unicast data
     *  Bit1: unicast data
     *  Bit2: mac events
     *  Bit3: magic packet 
     */
    t_u32 conditions;
    /** GPIO */
    t_u32 gpio;
    /** Gap in milliseconds */
    t_u32 gap;
} ds_hs_cfg;

/** Private command ID to get BSS type */
#define	UAP_GET_BSS_TYPE            (SIOCDEVPRIVATE + 15)

/** addba_param */
typedef struct _addba_param
{
    /** subcmd */
    t_u32 subcmd;
    /** Set/Get */
    t_u32 action;
    /** block ack timeout for ADDBA request */
    t_u32 timeout;
    /** Buffer size for ADDBA request */
    t_u32 txwinsize;
    /** Buffer size for ADDBA response */
    t_u32 rxwinsize;
} addba_param;

/** aggr_prio_tbl */
typedef struct _aggr_prio_tbl
{
    /** subcmd */
    t_u32 subcmd;
    /** Set/Get */
    t_u32 action;
    /** ampdu priority table */
    t_u8 ampdu[MAX_NUM_TID];
    /** amsdu priority table */
    t_u8 amsdu[MAX_NUM_TID];
} aggr_prio_tbl;

/** addba_reject parameters */
typedef struct _addba_reject_para
{
    /** subcmd */
    t_u32 subcmd;
    /** Set/Get */
    t_u32 action;
    /** BA Reject paramters */
    t_u8 addba_reject[MAX_NUM_TID];
} addba_reject_para;

/** fw_info */
typedef struct _fw_info
{
    /** subcmd */
    t_u32 subcmd;
    /** Get */
    t_u32 action;
    /** Firmware release number */
    t_u32 fw_release_number;
    /** Device support for MIMO abstraction of MCSs */
    t_u8 hw_dev_mcs_support;
} fw_info;

/** tx_data_pause parameters */
typedef struct _tx_data_pause_para
{
    /** subcmd */
    t_u32 subcmd;
    /** Set/Get */
    t_u32 action;
    /** enable/disable Tx data pause*/
    t_u16 txpause;
    /** Max number of TX buffer allowed for all PS client*/
    t_u16 txbufcnt;
} tx_data_pause_para;

void woal_uap_set_multicast_list(struct net_device *dev);
int woal_uap_do_ioctl(struct net_device *dev, struct ifreq *req, int cmd);
int woal_uap_bss_ctrl(moal_private * priv, t_u8 wait_option, int data);
void woal_uap_get_version(moal_private * priv, char *version, int max_len);
mlan_status woal_uap_get_stats(moal_private * priv, t_u8 wait_option,
                               mlan_ds_uap_stats * ustats);
extern struct iw_handler_def woal_uap_handler_def;
struct iw_statistics *woal_get_uap_wireless_stats(struct net_device *dev);
/** IOCTL function for wireless private IOCTLs */
int woal_uap_do_priv_ioctl(struct net_device *dev, struct ifreq *req, int cmd);
/** Set invalid data for each member of mlan_uap_bss_param */
void woal_set_sys_config_invalid_data(mlan_uap_bss_param * config);
/** Set/Get system configuration parameters */
mlan_status woal_set_get_sys_config(moal_private * priv,
                                    t_u16 action, mlan_uap_bss_param * sys_cfg);
void woal_ioctl_get_uap_info_resp(moal_private * priv, mlan_ds_get_info * info);
#endif /* _MOAL_UAP_H */
