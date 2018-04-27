/** @file moal_wext.h
 *
 * @brief This file contains definition for wireless extension IOCTL call.
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

#ifndef _WOAL_WEXT_H_
#define _WOAL_WEXT_H_

/** Custom event : AdHoc link sensed */
#define CUS_EVT_ADHOC_LINK_SENSED	"EVENT=ADHOC_LINK_SENSED"
/** Custom event : AdHoc link lost */
#define CUS_EVT_ADHOC_LINK_LOST		"EVENT=ADHOC_LINK_LOST"
/** Custom event : MIC failure, unicast */
#define CUS_EVT_MLME_MIC_ERR_UNI	"MLME-MICHAELMICFAILURE.indication unicast "
/** Custom event : MIC failure, multicast */
#define CUS_EVT_MLME_MIC_ERR_MUL	"MLME-MICHAELMICFAILURE.indication multicast "
/** Custom event : Beacon RSSI low */
#define CUS_EVT_BEACON_RSSI_LOW		"EVENT=BEACON_RSSI_LOW"
/** Custom event : Beacon SNR low */
#define CUS_EVT_BEACON_SNR_LOW		"EVENT=BEACON_SNR_LOW"
/** Custom event : Beacon RSSI high */
#define CUS_EVT_BEACON_RSSI_HIGH	"EVENT=BEACON_RSSI_HIGH"
/** Custom event : Beacon SNR high */
#define CUS_EVT_BEACON_SNR_HIGH		"EVENT=BEACON_SNR_HIGH"
/** Custom event : Max fail */
#define CUS_EVT_MAX_FAIL		"EVENT=MAX_FAIL"
/** Custom event : Data RSSI low */
#define CUS_EVT_DATA_RSSI_LOW		"EVENT=DATA_RSSI_LOW"
/** Custom event : Data SNR low */
#define CUS_EVT_DATA_SNR_LOW		"EVENT=DATA_SNR_LOW"
/** Custom event : Data RSSI high */
#define CUS_EVT_DATA_RSSI_HIGH		"EVENT=DATA_RSSI_HIGH"
/** Custom event : Data SNR high */
#define CUS_EVT_DATA_SNR_HIGH		"EVENT=DATA_SNR_HIGH"
/** Custom event : Link Quality */
#define CUS_EVT_LINK_QUALITY		"EVENT=LINK_QUALITY"
/** Custom event : Port Release */
#define CUS_EVT_PORT_RELEASE		"EVENT=PORT_RELEASE"
/** Custom event : Pre-Beacon Lost */
#define CUS_EVT_PRE_BEACON_LOST		"EVENT=PRE_BEACON_LOST"

/** Custom event : Host Sleep activated */
#define CUS_EVT_HS_ACTIVATED		"HS_ACTIVATED "
/** Custom event : Host Sleep deactivated */
#define CUS_EVT_HS_DEACTIVATED		"HS_DEACTIVATED "
/** Custom event : Host Sleep wakeup */
#define CUS_EVT_HS_WAKEUP		"HS_WAKEUP"

/** Custom event : Heart Beat */
#define CUS_EVT_HEART_BEAT		"EVENT=HEART_BEAT"

/** Custom event : WEP ICV error */
#define CUS_EVT_WEP_ICV_ERR		"EVENT=WEP_ICV_ERR"

/** Custom event : Channel Switch Announcment */
#define CUS_EVT_CHANNEL_SWITCH_ANN	"EVENT=CHANNEL_SWITCH_ANN"

/** Custom event : BW changed */
#define CUS_EVT_BW_CHANGED		"EVENT=BW_CHANGED"
/** Custom event : OBSS scan parameter */
#define CUS_EVT_OBSS_SCAN_PARAM		"EVENT=OBSS_SCAN_PARAM"
/** Custom indiciation message sent to the application layer for WMM changes */
#define WMM_CONFIG_CHANGE_INDICATION  "WMM_CONFIG_CHANGE.indication"

#ifdef UAP_SUPPORT
/** Custom event : STA connected */
#define CUS_EVT_STA_CONNECTED           "EVENT=STA_CONNECTED"
/** Custom event : STA disconnected */
#define CUS_EVT_STA_DISCONNECTED        "EVENT=STA_DISCONNECTED"
#endif
/** NF value for default scan */
#define MRVDRV_NF_DEFAULT_SCAN_VALUE		(-96)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
/** Add event */
#define IWE_STREAM_ADD_EVENT(i, c, e, w, l) iwe_stream_add_event((i), (c), (e), (w), (l))
/** Add point */
#define IWE_STREAM_ADD_POINT(i, c, e, w, p) iwe_stream_add_point((i), (c), (e), (w), (p))
/** Add value */
#define IWE_STREAM_ADD_VALUE(i, c, v, e, w, l)  iwe_stream_add_value((i), (c), (v), (e), (w), (l))
#else
/** Add event */
#define IWE_STREAM_ADD_EVENT(i, c, e, w, l) iwe_stream_add_event((c), (e), (w), (l))
/** Add point */
#define IWE_STREAM_ADD_POINT(i, c, e, w, p) iwe_stream_add_point((c), (e), (w), (p))
/** Add value */
#define IWE_STREAM_ADD_VALUE(i, c, v, e, w, l)  iwe_stream_add_value((c), (v), (e), (w), (l))
#endif

/** combo scan header */
#define WEXT_CSCAN_HEADER		"CSCAN S\x01\x00\x00S\x00"
/** combo scan header size */
#define WEXT_CSCAN_HEADER_SIZE		12
/** combo scan ssid section */
#define WEXT_CSCAN_SSID_SECTION		'S'
/** commbo scan channel section */
#define WEXT_CSCAN_CHANNEL_SECTION	'C'
/** commbo scan passive dwell section */
#define WEXT_CSCAN_PASV_DWELL_SECTION	'P'
/** commbo scan home dwell section */
#define WEXT_CSCAN_HOME_DWELL_SECTION	'H'

/** band AUTO */
#define	WIFI_FREQUENCY_BAND_AUTO		0
/** band 5G */
#define	WIFI_FREQUENCY_BAND_5GHZ        1
/** band 2G */
#define	WIFI_FREQUENCY_BAND_2GHZ		2
/** All band */
#define WIFI_FREQUENCY_ALL_BAND         3

/** Rx filter: IPV4 multicast */
#define RX_FILTER_IPV4_MULTICAST        1
/** Rx filter: broadcast */
#define RX_FILTER_BROADCAST             2
/** Rx filter: unicast */
#define RX_FILTER_UNICAST               4
/** Rx filter: IPV6 multicast */
#define RX_FILTER_IPV6_MULTICAST        8

extern struct iw_handler_def woal_handler_def;
struct iw_statistics *woal_get_wireless_stats(struct net_device *dev);
#endif /* _WOAL_WEXT_H_ */
