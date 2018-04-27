/** @file  moal_uap_priv.c
  *
  * @brief This file contains standard ioctl functions
  *
  * Copyright (C) 2010-2011, Marvell International Ltd.
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

/************************************************************************
Change log:
    08/06/2010: initial version
************************************************************************/

#include	"moal_main.h"
#include    "moal_uap.h"
#include    "moal_uap_priv.h"

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
 *  @brief Parse AP configuration from ASCII string
 *
 *  @param ap_cfg   A pointer to mlan_uap_bss_param structure
 *  @param buf      A pointer to user data
 *
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_ap_cfg_parse_data(mlan_uap_bss_param * ap_cfg, t_s8 * buf)
{
    int ret = 0, atoi_ret;
    int set_sec = 0, set_key = 0, set_chan = 0;
    int set_preamble = 0, set_scb = 0, set_ssid = 0;
    t_s8 *begin = buf, *value = NULL, *opt = NULL;

    ENTER();

    while (begin) {
        value = woal_strsep(&begin, ',', '/');
        opt = woal_strsep(&value, '=', '/');
        if (opt && !strncmp(opt, "END", strlen("END"))) {
            if (!ap_cfg->ssid.ssid_len) {
                PRINTM(MERROR, "Minimum option required is SSID\n");
                ret = -EINVAL;
                goto done;
            }
            PRINTM(MINFO, "Parsing terminated by string END\n");
            break;
        }
        if (!opt || !value || !value[0]) {
            PRINTM(MERROR, "Invalid option\n");
            ret = -EINVAL;
            goto done;
        } else if (!strncmp(opt, "ASCII_CMD", strlen("ASCII_CMD"))) {
            if (strncmp(value, "AP_CFG", strlen("AP_CFG"))) {
                PRINTM(MERROR, "ASCII_CMD: %s not matched with AP_CFG\n",
                       value);
                ret = -EFAULT;
                goto done;
            }
            value = woal_strsep(&begin, ',', '/');
            opt = woal_strsep(&value, '=', '/');
            if (!opt || !value || !value[0]) {
                PRINTM(MERROR, "Minimum option required is SSID\n");
                ret = -EINVAL;
                goto done;
            } else if (!strncmp(opt, "SSID", strlen("SSID"))) {
                if (set_ssid) {
                    PRINTM(MWARN, "Skipping SSID, found again!\n");
                    continue;
                }
                if (strlen(value) > MLAN_MAX_SSID_LENGTH) {
                    PRINTM(MERROR, "SSID length exceeds max length\n");
                    ret = -EFAULT;
                    goto done;
                }
                ap_cfg->ssid.ssid_len = strlen(value);
                strncpy((char *) ap_cfg->ssid.ssid, value, strlen(value));
                PRINTM(MINFO, "ssid=%s, len=%d\n", ap_cfg->ssid.ssid,
                       (int) ap_cfg->ssid.ssid_len);
                set_ssid = 1;
            } else {
                PRINTM(MERROR, "AP_CFG: Invalid option %s, "
                       "expect SSID\n", opt);
                ret = -EINVAL;
                goto done;
            }
        } else if (!strncmp(opt, "SEC", strlen("SEC"))) {
            if (set_sec) {
                PRINTM(MWARN, "Skipping SEC, found again!\n");
                continue;
            }
            if (!strnicmp(value, "open", strlen("open"))) {
                ap_cfg->auth_mode = MLAN_AUTH_MODE_OPEN;
                if (set_key)
                    ap_cfg->wpa_cfg.length = 0;
                ap_cfg->key_mgmt = KEY_MGMT_NONE;
                ap_cfg->protocol = PROTOCOL_NO_SECURITY;
            } else if (!strnicmp(value, "wpa2-psk", strlen("wpa2-psk"))) {
                ap_cfg->auth_mode = MLAN_AUTH_MODE_OPEN;
                ap_cfg->protocol = PROTOCOL_WPA2;
                ap_cfg->key_mgmt = KEY_MGMT_PSK;
                ap_cfg->wpa_cfg.pairwise_cipher_wpa = CIPHER_AES_CCMP;
                ap_cfg->wpa_cfg.pairwise_cipher_wpa2 = CIPHER_AES_CCMP;
                ap_cfg->wpa_cfg.group_cipher = CIPHER_AES_CCMP;
            } else {
                PRINTM(MERROR, "AP_CFG: Invalid value=%s for %s\n", value, opt);
                ret = -EFAULT;
                goto done;
            }
            set_sec = 1;
        } else if (!strncmp(opt, "KEY", strlen("KEY"))) {
            if (set_key) {
                PRINTM(MWARN, "Skipping KEY, found again!\n");
                continue;
            }
            if (set_sec && ap_cfg->protocol != PROTOCOL_WPA2) {
                PRINTM(MWARN, "Warning! No KEY for open mode\n");
                set_key = 1;
                continue;
            }
            if (strlen(value) < MLAN_MIN_PASSPHRASE_LENGTH ||
                strlen(value) > MLAN_PMK_HEXSTR_LENGTH) {
                PRINTM(MERROR, "Invalid PSK/PMK length\n");
                ret = -EINVAL;
                goto done;
            }
            ap_cfg->wpa_cfg.length = strlen(value);
            memcpy(ap_cfg->wpa_cfg.passphrase, value, strlen(value));
            set_key = 1;
        } else if (!strncmp(opt, "CHANNEL", strlen("CHANNEL"))) {
            if (set_chan) {
                PRINTM(MWARN, "Skipping CHANNEL, found again!\n");
                continue;
            }
            if (woal_atoi(&atoi_ret, value)) {
                ret = -EINVAL;
                goto done;
            }
            if (atoi_ret < 1 || atoi_ret > MLAN_MAX_CHANNEL) {
                PRINTM(MERROR, "AP_CFG: Channel must be between 1 and %d"
                       "(both included)\n", MLAN_MAX_CHANNEL);
                ret = -EINVAL;
                goto done;
            }
            ap_cfg->channel = atoi_ret;
            set_chan = 1;
        } else if (!strncmp(opt, "PREAMBLE", strlen("PREAMBLE"))) {
            if (set_preamble) {
                PRINTM(MWARN, "Skipping PREAMBLE, found again!\n");
                continue;
            }
            if (woal_atoi(&atoi_ret, value)) {
                ret = -EINVAL;
                goto done;
            }
            /* This is a READ only value from FW, so we can not set this and
               pass it successfully */
            set_preamble = 1;
        } else if (!strncmp(opt, "MAX_SCB", strlen("MAX_SCB"))) {
            if (set_scb) {
                PRINTM(MWARN, "Skipping MAX_SCB, found again!\n");
                continue;
            }
            if (woal_atoi(&atoi_ret, value)) {
                ret = -EINVAL;
                goto done;
            }
            if (atoi_ret < 1 || atoi_ret > MAX_STA_COUNT) {
                PRINTM(MERROR, "AP_CFG: MAX_SCB must be between 1 to %d "
                       "(both included)\n", MAX_STA_COUNT);
                ret = -EINVAL;
                goto done;
            }
            ap_cfg->max_sta_count = (t_u16) atoi_ret;
            set_scb = 1;
        } else {
            PRINTM(MERROR, "Invalid option %s\n", opt);
            ret = -EINVAL;
            goto done;
        }
    }

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief Set AP configuration
 *
 *  @param priv     A pointer to moal_private structure
 *  @param wrq      A pointer to user data
 *
 *  @return         0 --success, otherwise fail
 */
static int
woal_uap_set_ap_cfg(moal_private * priv, struct iwreq *wrq)
{
    int ret = 0;
    static t_s8 buf[MAX_BUF_LEN];
    mlan_uap_bss_param sys_config;
    int restart = 0;

    ENTER();

#define MIN_AP_CFG_CMD_LEN   16 /* strlen("ASCII_CMD=AP_CFG") */
    if ((wrq->u.data.length - 1) <= MIN_AP_CFG_CMD_LEN) {
        PRINTM(MERROR, "Invalid length of command\n");
        ret = -EINVAL;
        goto done;
    }

    memset(buf, 0, MAX_BUF_LEN);
    memcpy(buf, wrq->u.data.pointer, wrq->u.data.length);

    /* Initialize the invalid values so that the correct values below are
       downloaded to firmware */
    woal_set_sys_config_invalid_data(&sys_config);

    /* Setting the default values */
    sys_config.channel = 6;
    sys_config.preamble_type = 0;

    if ((ret = woal_uap_ap_cfg_parse_data(&sys_config, buf)))
        goto done;

    /* If BSS already started stop it first and restart after changing the
       setting */
    if (priv->bss_started == MTRUE) {
        if ((ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP)))
            goto done;
        restart = 1;
    }

    if (MLAN_STATUS_SUCCESS !=
        woal_set_get_sys_config(priv, MLAN_ACT_SET, &sys_config)) {
        ret = -EFAULT;
        goto done;
    }

    /* Start the BSS after successful configuration */
    if (restart)
        ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_START);

  done:
    LEAVE();
    return ret;
}

/********************************************************
		Global Functions
********************************************************/

/**
 *  @brief ioctl function for wireless IOCTLs
 *
 *  @param dev		A pointer to net_device structure
 *  @param req	   	A pointer to ifreq structure
 *  @param cmd 		Command
 *
 *  @return          0 --success, otherwise fail
 */
int
woal_uap_do_priv_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    moal_private *priv = (moal_private *) netdev_priv(dev);
    struct iwreq *wrq = (struct iwreq *) req;
    int ret = 0;

    ENTER();

    switch (cmd) {
    case WOAL_UAP_SETNONE_GETNONE:
        switch (wrq->u.data.flags) {
        case WOAL_UAP_START:
            break;
        case WOAL_UAP_STOP:
            ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP);
            break;
        case WOAL_AP_BSS_START:
            ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_START);
            break;
        case WOAL_AP_BSS_STOP:
            ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP);
            break;
        default:
            ret = -EINVAL;
            break;
        }
        break;
    case WOAL_UAP_SETONEINT_GETWORDCHAR:
        switch (wrq->u.data.flags) {
        case WOAL_UAP_VERSION:
            ret = woal_get_driver_version(priv, req);
            break;
        case WOAL_UAP_VEREXT:
            ret = woal_get_driver_verext(priv, req);
            break;
        default:
            ret = -EOPNOTSUPP;
            break;
        }
        break;
    case WOAL_UAP_SET_GET_256_CHAR:
        switch (wrq->u.data.flags) {
        case WOAL_WL_FW_RELOAD:
            break;
        case WOAL_AP_SET_CFG:
            ret = woal_uap_set_ap_cfg(priv, wrq);
            break;
        default:
            ret = -EINVAL;
            break;
        }
        break;
#if defined(WFD_SUPPORT)
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
    case WOAL_UAP_SETONEINT_GETONEINT:
        switch (wrq->u.data.flags) {
        case WOAL_UAP_SET_GET_BSS_ROLE:
            ret = woal_set_get_bss_role(priv, wrq);
            break;
        default:
            ret = -EINVAL;
            break;
        }
        break;
#endif
#endif
    case WOAL_UAP_HOST_CMD:
        ret = woal_host_command(priv, wrq);
        break;
    case WOAL_UAP_FROYO_START:
        break;
    case WOAL_UAP_FROYO_STOP:
        ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP);
        break;
    case WOAL_UAP_FROYO_AP_BSS_START:
        ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_START);
        break;
    case WOAL_UAP_FROYO_AP_BSS_STOP:
        ret = woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP);
        break;
    case WOAL_UAP_FROYO_WL_FW_RELOAD:
        break;
    case WOAL_UAP_FROYO_AP_SET_CFG:
        ret = woal_uap_set_ap_cfg(priv, wrq);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    LEAVE();
    return ret;
}
