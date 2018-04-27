/** @file moal_usb.h
  *
  * @brief This file contains definitions for USB interface.
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
/*************************************************************
Change Log:
    10/21/2008: initial version
************************************************************/

#ifndef _MOAL_USB_H
#define _MOAL_USB_H

/** USB VID 1 */
#define USB8782_VID_1	0x1286
/** USB PID 1 */
#define USB8782_PID_1	0x2040

/** High watermark for Tx data */
#define MVUSB_TX_HIGH_WMARK    6

/** Number of Rx data URB */
#define MVUSB_RX_DATA_URB   6

#ifdef STA_SUPPORT
/** Default firmaware name */

#define DEFAULT_FW_NAME "mrvl/usb8782_uapsta.bin"

#ifndef DEFAULT_FW_NAME
#define DEFAULT_FW_NAME ""
#endif
#endif /* STA_SUPPORT */

#ifdef UAP_SUPPORT
/** Default firmaware name */

#define DEFAULT_AP_FW_NAME "mrvl/usb8782_uapsta.bin"

#ifndef DEFAULT_AP_FW_NAME
#define DEFAULT_AP_FW_NAME ""
#endif
#endif /* UAP_SUPPORT */

/** Default firmaware name */

#define DEFAULT_AP_STA_FW_NAME "mrvl/usb8782_uapsta.bin"

#ifndef DEFAULT_AP_STA_FW_NAME
#define DEFAULT_AP_STA_FW_NAME ""
#endif

/** urb context */
typedef struct _urb_context
{
        /** Pointer to moal_handle structure */
    moal_handle *handle;
        /** Pointer to mlan buffer */
    mlan_buffer *pmbuf;
        /** URB */
    struct urb *urb;
        /** EP */
    t_u8 ep;
} urb_context;

/** USB card description structure*/
struct usb_card_rec
{
        /** USB device */
    struct usb_device *udev;
        /** MOAL handle */
    void *phandle;
        /** USB interface */
    struct usb_interface *intf;
        /** Rx data endpoint address */
    t_u8 rx_cmd_ep;
        /** Rx cmd contxt */
    urb_context rx_cmd;
        /** Rx command URB pending count */
    atomic_t rx_cmd_urb_pending;
        /** Rx data context list */
    urb_context rx_data_list[MVUSB_RX_DATA_URB];
        /** Rx data endpoint address */
    t_u8 rx_data_ep;
        /** Rx data URB pending count */
    atomic_t rx_data_urb_pending;
        /** Tx data endpoint address */
    t_u8 tx_data_ep;
        /** Tx command endpoint address */
    t_u8 tx_cmd_ep;
        /** Tx data URB pending count */
    atomic_t tx_data_urb_pending;
        /** Tx command URB pending count */
    atomic_t tx_cmd_urb_pending;
        /** Bulk out max packet size */
    int bulk_out_maxpktsize;
        /** Pre-allocated urb for command */
    urb_context tx_cmd;
        /** Index to point to next data urb to use */
    int tx_data_ix;
        /** Pre-allocated urb for data */
    urb_context tx_data_list[MVUSB_TX_HIGH_WMARK];
};

mlan_status woal_write_data_async(moal_handle * handle, mlan_buffer * pmbuf,
                                  t_u8 ep);
mlan_status woal_write_data_sync(moal_handle * handle, mlan_buffer * pmbuf,
                                 t_u8 ep, t_u32 timeout);
mlan_status woal_read_data_sync(moal_handle * handle, mlan_buffer * pmbuf,
                                t_u8 ep, t_u32 timeout);
mlan_status woal_usb_submit_rx_data_urbs(moal_handle * handle);
mlan_status woal_usb_rx_init(moal_handle * handle);
mlan_status woal_usb_tx_init(moal_handle * handle);
void woal_submit_rx_urb(moal_handle * handle, t_u8 ep);
void woal_bus_unregister(void);
mlan_status woal_bus_register(void);
mlan_status woal_register_dev(moal_handle * handle);
void woal_usb_free(struct usb_card_rec *cardp);
#endif /*_MOAL_USB_H */
