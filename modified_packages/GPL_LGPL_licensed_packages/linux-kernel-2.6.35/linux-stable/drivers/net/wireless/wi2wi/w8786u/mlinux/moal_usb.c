/** @file moal_usb.c
  *
  * @brief This file contains the interfaceing to USB bus 
  * driver. 
  *
  * Copyright (C) 2008-2009, Marvell International Ltd. 
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
#include	"moal_usb.h"
/********************************************************
		Local Variables
********************************************************/

/** Marvell USB device */
#define MARVELL_USB_DEVICE(vid, pid, name) \
           USB_DEVICE(vid, pid),  \
           .driver_info = (t_u32)name

/** Name of the USB driver */
const char usbdriver_name[] = "usb8xxx";

/** This structure contains the device signature */
struct usb_device_id woal_usb_table[] = {
    /* Enter the device signature inside */
    {MARVELL_USB_DEVICE
     (USB8786_VID_1, USB8786_PID_1, "Marvell WLAN USB Adapter")},
    /* Terminating entry */
    {},
};

static mlan_status woal_usb_submit_rx_urb(urb_context * ctx, int size);
static int woal_usb_probe(struct usb_interface *intf,
                          const struct usb_device_id *id);
static void woal_usb_disconnect(struct usb_interface *intf);
static int woal_usb_suspend(struct usb_interface *intf, pm_message_t message);
static int woal_usb_resume(struct usb_interface *intf);

/** woal_usb_driver */
static struct usb_driver woal_usb_driver = {
    /* Driver name */
    .name = usbdriver_name,

    /* Probe function name */
    .probe = woal_usb_probe,

    /* Disconnect function name */
    .disconnect = woal_usb_disconnect,

    /* Device signature table */
    .id_table = woal_usb_table,
    /* Suspend function name */
    .suspend = woal_usb_suspend,

    /* Resume function name */
    .resume = woal_usb_resume,

    /* Driver supports autosuspend */
    .supports_autosuspend = 1,
};

MODULE_DEVICE_TABLE(usb, woal_usb_table);

/********************************************************
		Global Variables
********************************************************/
/** Default firmaware name */
#define DEFAULT_FW_NAME	"mrvl/usb8786.bin"
extern char *fw_name;

/********************************************************
		Local Functions
********************************************************/
/**
 *  @brief  Free Tx/Rx urb, skb and Rx buffer
 *
 *  @param cardp	Pointer usb_card_rec
 *
 *  @return 	   	N/A
 */
void
woal_usb_free(struct usb_card_rec *cardp)
{
    int i;

    ENTER();

    /* Unlink Rx cmd URB */
    if (atomic_read(&cardp->rx_cmd_urb_pending) && cardp->rx_cmd.urb) {
        usb_kill_urb(cardp->rx_cmd.urb);
    }
    /* Unlink Rx data URBs */
    if (atomic_read(&cardp->rx_data_urb_pending)) {
        for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
            if (cardp->rx_data_list[i].urb)
                usb_kill_urb(cardp->rx_data_list[i].urb);
        }
    }

    /* Free Rx data URBs */
    for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
        if (cardp->rx_data_list[i].urb) {
            usb_free_urb(cardp->rx_data_list[i].urb);
            cardp->rx_data_list[i].urb = NULL;
        }
    }
    /* Free Rx cmd URB */
    if (cardp->rx_cmd.urb) {
        usb_free_urb(cardp->rx_cmd.urb);
        cardp->rx_cmd.urb = NULL;
    }

    /* Free Tx data URBs */
    for (i = 0; i < MVUSB_TX_HIGH_WMARK; i++) {
        if (cardp->tx_data_list[i].urb) {
            usb_free_urb(cardp->tx_data_list[i].urb);
            cardp->tx_data_list[i].urb = NULL;
        }
    }
    /* Free Tx cmd URB */
    if (cardp->tx_cmd.urb) {
        usb_free_urb(cardp->tx_cmd.urb);
        cardp->tx_cmd.urb = NULL;
    }

    LEAVE();
    return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
/**
 *  @brief This function receive packet of the data/cmd/event packet
 *         and pass to MLAN 
 *
 *  @param urb		Pointer to struct urb
 *  @param regs		Registers
 *
 *  @return 	   	N/A
 */
static void
woal_usb_receive(struct urb *urb, struct pt_regs *regs)
#else
/**
 * @brief This function receive packet of the data/cmd/event packet
 *         and pass to MLAN 
 *
 *  @param urb		Pointer to struct urb
 *
 *  @return 	   	N/A
 */
static void
woal_usb_receive(struct urb *urb)
#endif
{
    urb_context *context = (urb_context *) urb->context;
    moal_handle *handle = context->handle;
    mlan_buffer *pmbuf = context->pmbuf;
    struct usb_card_rec *cardp = NULL;
    int recv_length = urb->actual_length;
    int size;
    mlan_status status;

    ENTER();

    if (!handle || !handle->card) {
        PRINTM(ERROR, "moal handle or card structure is not valid\n");
        LEAVE();
        return;
    }
    cardp = (struct usb_card_rec *) handle->card;
    if (cardp->rx_cmd_ep == context->ep)
        atomic_dec(&cardp->rx_cmd_urb_pending);
    else
        atomic_dec(&cardp->rx_data_urb_pending);

    if (recv_length) {
        if (urb->status || (handle->surprise_removed == MTRUE)) {
            PRINTM(ERROR, "URB status is failed: %d\n", urb->status);
            /* Do not free mlan_buffer in case of command ep */
            if (cardp->rx_cmd_ep != context->ep)
                woal_free_mlan_buffer(pmbuf);
            goto setup_for_next;
        }
        /* Send packet to MLAN */
        pmbuf->data_len = recv_length;
        atomic_inc(&handle->rx_pending);
        status = mlan_recv(handle->pmlan_adapter, pmbuf, context->ep);
        PRINTM(INFO, "Receive length = 0x%x, status=%d\n", recv_length, status);
        if (status == MLAN_STATUS_PENDING) {
            queue_work(handle->workqueue, &handle->main_work);
            /* urb for data_ep is re-submitted now */
            /* urb for cmd_ep will be re-submitted in callback
               moal_recv_complete */
            if (cardp->rx_cmd_ep == context->ep)
                goto rx_exit;
        } else {
            atomic_dec(&handle->rx_pending);
            if (status == MLAN_STATUS_FAILURE)
                PRINTM(ERROR, "MLAN fail to process the receive data\n");
            /* Do not free mlan_buffer in case of command ep */
            if (cardp->rx_cmd_ep != context->ep)
                woal_free_mlan_buffer(pmbuf);
        }
    } else if (urb->status) {
        if (!handle->is_suspended) {
            PRINTM(MSG, "Card is removed: %d\n", urb->status);
            handle->surprise_removed = MTRUE;
        }
        woal_free_mlan_buffer(pmbuf);
        goto rx_exit;
    } else {
        /* Do not free mlan_buffer in case of command ep */
        if (cardp->rx_cmd_ep != context->ep)
            woal_free_mlan_buffer(pmbuf);
        goto setup_for_next;
    }
  setup_for_next:
    if (cardp->rx_cmd_ep == context->ep)
        size = MLAN_RX_CMD_BUF_SIZE;
    else
        size = MLAN_RX_DATA_BUF_SIZE;
    woal_usb_submit_rx_urb(context, size);
  rx_exit:

    LEAVE();
    return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
/** 
 *  @brief  Call back function to handle the status of the Tx data URB
 *
 *  @param urb      Pointer to urb structure
 *  @param regs     Registers
 *
 *  @return         N/A
 */
static void
woal_usb_tx_complete(struct urb *urb, struct pt_regs *regs)
#else
/** 
 * @brief  Call back function to handle the status of the Tx data URB
 *
 * @param urb      Pointer to urb structure
 *
 * @return         N/A
 */
static void
woal_usb_tx_complete(struct urb *urb)
#endif
{
    urb_context *context = (urb_context *) (urb->context);
    moal_handle *handle = context->handle;
    struct usb_card_rec *cardp = handle->card;

    ENTER();

    /* Decrease pending URB counter */
    if (context->ep == cardp->tx_data_ep)
        atomic_dec(&cardp->tx_data_urb_pending);
    if (context->ep == cardp->tx_cmd_ep)
        atomic_dec(&cardp->tx_cmd_urb_pending);
    /* Handle the transmission complete validations */
    if (urb->status) {
        PRINTM(ERROR, "Tx data URB in failure status: %d\n", urb->status);
        mlan_write_data_async_complete(handle->pmlan_adapter, context->pmbuf,
                                       context->ep, MLAN_STATUS_FAILURE);
    } else {
        mlan_write_data_async_complete(handle->pmlan_adapter, context->pmbuf,
                                       context->ep, MLAN_STATUS_SUCCESS);
    }
    queue_work(handle->workqueue, &handle->main_work);

    LEAVE();
    return;
}

/**
 *  @brief This function sets up the data to receive
 *
 *  @param ctx		Pointer to urb_context structure
 *  @param size	        Skb size
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
static mlan_status
woal_usb_submit_rx_urb(urb_context * ctx, int size)
{
    moal_handle *handle = ctx->handle;
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    struct sk_buff *skb = NULL;
    mlan_status ret = MLAN_STATUS_FAILURE;

    ENTER();

    if (cardp->rx_cmd_ep != ctx->ep) {
        if (!(ctx->pmbuf = woal_alloc_mlan_buffer(size))) {
            PRINTM(ERROR, "Fail to submit Rx URB due to no memory/skb\n");
            goto rx_ret;
        }
    }

    skb = (struct sk_buff *) ctx->pmbuf->pdesc;
    ctx->pmbuf->data_offset = MLAN_NET_IP_ALIGN;
    usb_fill_bulk_urb(ctx->urb, cardp->udev,
                      usb_rcvbulkpipe(cardp->udev, ctx->ep),
                      skb->tail + MLAN_NET_IP_ALIGN,
                      size - MLAN_NET_IP_ALIGN, woal_usb_receive, (void *) ctx);
    if (cardp->rx_cmd_ep == ctx->ep)
        atomic_inc(&cardp->rx_cmd_urb_pending);
    else
        atomic_inc(&cardp->rx_data_urb_pending);
    if ((ret = usb_submit_urb(ctx->urb, GFP_ATOMIC))) {
        /* Submit URB failure */
        PRINTM(ERROR, "Submit Rx URB failed: %d\n", ret);
        woal_free_mlan_buffer(ctx->pmbuf);
        if (cardp->rx_cmd_ep == ctx->ep)
            atomic_dec(&cardp->rx_cmd_urb_pending);
        else
            atomic_dec(&cardp->rx_data_urb_pending);
        ctx->pmbuf = NULL;
    } else {
        ret = MLAN_STATUS_SUCCESS;
    }
  rx_ret:
    LEAVE();
    return ret;
}

/**
 *  @brief Sets the configuration values
 *
 *  @param intf		Pointer to usb_interface
 *  @param id		Pointer to usb_device_id
 *
 *  @return 	   	Address of variable usb_cardp
 */
static int
woal_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct usb_device *udev;
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    int i;
    struct usb_card_rec *usb_cardp = NULL;

    ENTER();

    udev = interface_to_usbdev(intf);
    usb_cardp = kzalloc(sizeof(struct usb_card_rec), GFP_ATOMIC);
    if (!usb_cardp) {
        LEAVE();
        return (-ENOMEM);
    }

    udev->descriptor.idVendor = woal_cpu_to_le16(udev->descriptor.idVendor);
    udev->descriptor.idProduct = woal_cpu_to_le16(udev->descriptor.idProduct);
    udev->descriptor.bcdDevice = woal_cpu_to_le16(udev->descriptor.bcdDevice);
    udev->descriptor.bcdUSB = woal_cpu_to_le16(udev->descriptor.bcdUSB);

    /* Check probe is for our device */
    for (i = 0; woal_usb_table[i].idVendor; i++) {
        if (udev->descriptor.idVendor == woal_usb_table[i].idVendor &&
            udev->descriptor.idProduct == woal_usb_table[i].idProduct) {
            PRINTM(MSG, "VID/PID = %X/%X, Boot2 version = %X\n",
                   udev->descriptor.idVendor, udev->descriptor.idProduct,
                   udev->descriptor.bcdDevice);
            break;
        }
    }

    if (woal_usb_table[i].idVendor) {
        usb_cardp->udev = udev;
        iface_desc = intf->cur_altsetting;
        usb_cardp->intf = intf;

        PRINTM(INFO, "bcdUSB = 0x%X bDeviceClass = 0x%X"
               " bDeviceSubClass = 0x%X, bDeviceProtocol = 0x%X\n",
               udev->descriptor.bcdUSB,
               udev->descriptor.bDeviceClass,
               udev->descriptor.bDeviceSubClass,
               udev->descriptor.bDeviceProtocol);

        for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
            endpoint = &iface_desc->endpoint[i].desc;
            if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) &&
                ((endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK) ==
                 MLAN_USB_EP_CMD_EVENT) &&
                ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
                 USB_ENDPOINT_XFER_BULK)) {
                /* We found a bulk in command/event endpoint */
                PRINTM(INFO, "Bulk IN: max packet size = %d, address = %d\n",
                       woal_le16_to_cpu(endpoint->wMaxPacketSize),
                       endpoint->bEndpointAddress);
                usb_cardp->rx_cmd_ep =
                    (endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
                atomic_set(&usb_cardp->rx_cmd_urb_pending, 0);
            }
            if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) &&
                ((endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK) ==
                 MLAN_USB_EP_DATA) &&
                ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
                 USB_ENDPOINT_XFER_BULK)) {
                /* We found a bulk in data endpoint */
                PRINTM(INFO, "Bulk IN: max packet size = %d, address = %d\n",
                       woal_le16_to_cpu(endpoint->wMaxPacketSize),
                       endpoint->bEndpointAddress);
                usb_cardp->rx_data_ep =
                    (endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
                atomic_set(&usb_cardp->rx_data_urb_pending, 0);
            }
            if (((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==
                 USB_DIR_OUT) &&
                ((endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK) ==
                 MLAN_USB_EP_DATA) &&
                ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
                 USB_ENDPOINT_XFER_BULK)) {
                /* We found a bulk out data endpoint */
                PRINTM(INFO, "Bulk OUT: max packet size = %d, address = %d\n",
                       woal_le16_to_cpu(endpoint->wMaxPacketSize),
                       endpoint->bEndpointAddress);
                usb_cardp->tx_data_ep = endpoint->bEndpointAddress;
                atomic_set(&usb_cardp->tx_data_urb_pending, 0);
            }
            if (((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==
                 USB_DIR_OUT) &&
                ((endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK) ==
                 MLAN_USB_EP_CMD_EVENT) &&
                ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
                 USB_ENDPOINT_XFER_BULK)) {
                /* We found a bulk out command/event endpoint */
                PRINTM(INFO, "Bulk OUT: max packet size = %d, address = %d\n",
                       woal_le16_to_cpu(endpoint->wMaxPacketSize),
                       endpoint->bEndpointAddress);
                usb_cardp->tx_cmd_ep = endpoint->bEndpointAddress;
                atomic_set(&usb_cardp->tx_cmd_urb_pending, 0);
                usb_cardp->bulk_out_maxpktsize =
                    woal_le16_to_cpu(endpoint->wMaxPacketSize);
            }
        }
        usb_set_intfdata(intf, usb_cardp);
        /* At this point wlan_add_card() will be called */
        if (!(woal_add_card(usb_cardp))) {
            PRINTM(ERROR, "%s: woal_add_callback failed\n", __FUNCTION__);
            goto error;
        }
        // if (udev->actconfig->desc.bmAttributes & USB_CONFIG_ATT_WAKEUP)
        // intf->needs_remote_wakeup = 1;
        usb_get_dev(udev);
        LEAVE();
        return 0;
    } else {
        PRINTM(INFO, "Discard the Probe request\n");
        PRINTM(INFO, "VID = 0x%X PID = 0x%X\n", udev->descriptor.idVendor,
               udev->descriptor.idProduct);
    }
  error:
    usb_reset_device(udev);
    woal_usb_free(usb_cardp);
    kfree(usb_cardp);
    usb_cardp = NULL;
    LEAVE();
    return (-ENOMEM);
}

/**
 *  @brief Free resource and cleanup
 *
 *  @param intf		Pointer to usb_interface
 *
 *  @return 	   	N/A
 */
static void
woal_usb_disconnect(struct usb_interface *intf)
{
    struct usb_card_rec *cardp = usb_get_intfdata(intf);
    moal_handle *phandle = NULL;
    ENTER();
    if (!cardp || !cardp->phandle) {
        PRINTM(ERROR, "Card or phandle is not valid\n");
        LEAVE();
        return;
    }
    phandle = (moal_handle *) cardp->phandle;

    /* 
     * Update Surprise removed to TRUE
     *  Free all the URB's allocated
     */
    phandle->surprise_removed = MTRUE;

    /* Unlink and free urb */
    woal_usb_free(cardp);

    /* Card is removed and we can call wlan_remove_card */
    PRINTM(INFO, "Call remove card\n");
    woal_remove_card(cardp);

    /* Reset device */
    usb_reset_device(cardp->udev);

    usb_set_intfdata(intf, NULL);
    usb_put_dev(interface_to_usbdev(intf));
    kfree(cardp);
    LEAVE();
    return;
}

/**
 *  @brief Handle suspend 
 *
 *  @param intf		  Pointer to usb_interface
 *  @param message	  Pointer to pm_message_t structure 			
 *
 *  @return 	   	  MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
static int
woal_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
    struct usb_card_rec *cardp = usb_get_intfdata(intf);
    moal_handle *handle = NULL;
    int i;
    mlan_ioctl_req *req = NULL;
    mlan_ds_pm_cfg *pmcfg = NULL;

    ENTER();

    if (!cardp || !cardp->phandle) {
        PRINTM(ERROR, "Card or moal_handle structure is not valid\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }
    handle = cardp->phandle;

    /* Enable Host Sleep */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_pm_cfg));
    if (req == NULL) {
        PRINTM(ERROR, "Falied to allocate buffer for IOCTL\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }
    pmcfg = (mlan_ds_pm_cfg *) req->pbuf;
    pmcfg->sub_command = MLAN_OID_PM_CFG_HS_CFG;
    req->req_id = MLAN_IOCTL_PM_CFG;
    req->action = MLAN_ACT_SET;

    woal_get_hs_params(handle->priv[0], MOAL_IOCTL_WAIT,
                       &(pmcfg->param.hs_cfg));
    if (pmcfg->param.hs_cfg.conditions != HOST_SLEEP_CFG_CANCEL)
        handle->hs_activate_wait_q_woken = MFALSE;
    pmcfg->param.hs_cfg.is_invoke_hostcmd = MTRUE;
    if (woal_request_ioctl(handle->priv[0], req, MOAL_IOCTL_WAIT) !=
        MLAN_STATUS_SUCCESS) {
        PRINTM(ERROR, "IOCTL request failed\n");
        kfree(req);
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }

    if (pmcfg->param.hs_cfg.conditions != HOST_SLEEP_CFG_CANCEL) {
        wait_event_interruptible(handle->hs_activate_wait_q,
                                 handle->hs_activate_wait_q_woken);
    }
    kfree(req);

    /* Indicate device suspended */
    /* The flag must be set here before the usb_kill_urb() calls. Reason: In
       the complete handlers, urb->status(= -ENOENT) and 'is_suspended' flag is 
       used in combination to distinguish between a suspended state and a
       'disconnect' one. */
    handle->is_suspended = MTRUE;
    for (i = 0; i <= handle->priv_num; i++)
        netif_carrier_off(handle->priv[i]->netdev);

    /* Unlink Rx cmd URB */
    if (atomic_read(&cardp->rx_cmd_urb_pending) && cardp->rx_cmd.urb) {
        usb_kill_urb(cardp->rx_cmd.urb);
    }
    /* Unlink Rx data URBs */
    if (atomic_read(&cardp->rx_data_urb_pending)) {
        for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
            if (cardp->rx_data_list[i].urb)
                usb_kill_urb(cardp->rx_data_list[i].urb);
        }
    }

    /* Unlink Tx data URBs */
    for (i = 0; i < MVUSB_TX_HIGH_WMARK; i++) {
        if (cardp->tx_data_list[i].urb) {
            usb_kill_urb(cardp->tx_data_list[i].urb);
        }
    }
    /* Unlink Tx cmd URB */
    if (cardp->tx_cmd.urb) {
        usb_kill_urb(cardp->tx_cmd.urb);
    }

    handle->suspend_wait_q_woken = MTRUE;
    wake_up_interruptible(&handle->suspend_wait_q);

    LEAVE();
    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Handle resume
 *
 *  @param intf		  Pointer to usb_interface
 *  
 *  @return 	   	  MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
static int
woal_usb_resume(struct usb_interface *intf)
{
    struct usb_card_rec *cardp = usb_get_intfdata(intf);
    moal_handle *handle = NULL;
    int i;

    ENTER();

    if (!cardp || !cardp->phandle) {
        PRINTM(ERROR, "Card or adapter structure is not valid\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }
    handle = cardp->phandle;

    if (handle->is_suspended == MFALSE) {
        PRINTM(WARN, "Device already resumed\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }

    /* Indicate device resumed. The netdev queue will be resumed only after
       the urbs have been resubmitted */
    handle->is_suspended = MFALSE;
    if (!atomic_read(&cardp->rx_data_urb_pending)) {
        /* Submit multiple Rx data URBs */
        for (i = 0; i < MVUSB_RX_DATA_URB; i++)
            woal_usb_submit_rx_urb(&cardp->rx_data_list[i],
                                   MLAN_RX_DATA_BUF_SIZE);
    }
    if (!atomic_read(&cardp->rx_cmd_urb_pending)) {
        if ((cardp->rx_cmd.pmbuf =
             woal_alloc_mlan_buffer(MLAN_RX_CMD_BUF_SIZE)))
            woal_usb_submit_rx_urb(&cardp->rx_cmd, MLAN_RX_CMD_BUF_SIZE);
    }

    /* Indicate device resumed */
    for (i = 0; i <= handle->priv_num; i++)
        if (handle->priv[i]->media_connected == MTRUE)
            netif_carrier_on(handle->priv[i]->netdev);

    /* Disable Host Sleep */
    woal_hs_cfg_cancel(handle->priv[0]);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#ifdef CONFIG_PM
    /* Resume handler may be called due to remote wakeup, force to exit suspend 
       anyway */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
    cardp->udev->autosuspend_disabled = 1;
#else
    cardp->udev->dev.power.runtime_auto = 0;
#endif /* < 2.6.35 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
    cardp->udev->autoresume_disabled = 0;
#endif /* < 2.6.33 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)
    atomic_inc(&(cardp->udev)->dev.power.usage_count);
#endif /* >= 2.6.34 */
#endif /* CONFIG_PM */
#endif /* >= 2.6.24 */

    LEAVE();
    return MLAN_STATUS_SUCCESS;
}

/********************************************************
		Global Functions
********************************************************/

/**
 *  @brief This function initialize the tx URBs
 *
 *  @param handle 	Pointer to moal_handle structure
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_usb_tx_init(moal_handle * handle)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    int i;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    cardp->tx_cmd.handle = handle;
    cardp->tx_cmd.ep = cardp->tx_cmd_ep;
    /* Allocate URB for command */
    if (!(cardp->tx_cmd.urb = usb_alloc_urb(0, GFP_ATOMIC))) {
        PRINTM(ERROR, "Tx command URB allocation failed\n");
        ret = MLAN_STATUS_FAILURE;
        goto init_exit;
    }

    cardp->tx_data_ix = 0;
    for (i = 0; i < MVUSB_TX_HIGH_WMARK; i++) {
        cardp->tx_data_list[i].handle = handle;
        cardp->tx_data_list[i].ep = cardp->tx_data_ep;
        /* Allocate URB for data */
        if (!(cardp->tx_data_list[i].urb = usb_alloc_urb(0, GFP_ATOMIC))) {
            PRINTM(ERROR, "Tx data URB allocation failed\n");
            ret = MLAN_STATUS_FAILURE;
            goto init_exit;
        }
    }

  init_exit:
    LEAVE();
    return ret;
}

/**
 *  @brief This function initialize the rx URBs and submit them
 *
 *  @param handle 	Pointer to moal_handle structure
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_usb_rx_init(moal_handle * handle)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    int i;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    cardp->rx_cmd.handle = handle;
    cardp->rx_cmd.ep = cardp->rx_cmd_ep;
    /* Allocate URB for command/event */
    if (!(cardp->rx_cmd.urb = usb_alloc_urb(0, GFP_ATOMIC))) {
        PRINTM(ERROR, "Rx command URB allocation failed\n");
        ret = MLAN_STATUS_FAILURE;
        goto init_exit;
    }

    if ((cardp->rx_cmd.pmbuf = woal_alloc_mlan_buffer(MLAN_RX_CMD_BUF_SIZE))) {
        /* Submit Rx command URB */
        if (woal_usb_submit_rx_urb(&cardp->rx_cmd, MLAN_RX_CMD_BUF_SIZE)) {
            ret = MLAN_STATUS_FAILURE;
            goto init_exit;
        }
    }
    for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
        cardp->rx_data_list[i].handle = handle;
        cardp->rx_data_list[i].ep = cardp->rx_data_ep;
        /* Allocate URB for data */
        if (!(cardp->rx_data_list[i].urb = usb_alloc_urb(0, GFP_ATOMIC))) {
            PRINTM(ERROR, "Rx data URB allocation failed\n");
            ret = MLAN_STATUS_FAILURE;
            goto init_exit;
        }
        /* Submit Rx data URB */
        if (woal_usb_submit_rx_urb
            (&cardp->rx_data_list[i], MLAN_RX_DATA_BUF_SIZE)) {
            ret = MLAN_STATUS_FAILURE;
            goto init_exit;
        }
    }

  init_exit:
    LEAVE();
    return ret;
}

/**
 *  @brief  This function downloads data blocks to device
 *
 *  @param handle	Pointer to moal_handle structure
 *  @param pmbuf	Pointer to mlan_buffer structure
 *  @param ep		Endpoint to send
 *  @param timeout 	Time out value
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_write_data_sync(moal_handle * handle, mlan_buffer * pmbuf, t_u8 ep,
                     t_u32 timeout)
{
    struct usb_card_rec *cardp = handle->card;
    t_u8 *data = (t_u8 *) (pmbuf->pbuf + pmbuf->data_offset);
    t_u32 length = pmbuf->data_len;
    int actual_length;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    if (length % cardp->bulk_out_maxpktsize == 0)
        length++;
    /* Send the data block */
    if ((ret = usb_bulk_msg(cardp->udev, usb_sndbulkpipe(cardp->udev,
                                                         ep), (t_u8 *) data,
                            length, &actual_length, timeout))) {
        PRINTM(ERROR, "usb_blk_msg for send failed, ret %d\n", ret);
        ret = MLAN_STATUS_FAILURE;
    }
    pmbuf->data_len = actual_length;

    return ret;
}

/**
 *  @brief  This function read data blocks to device
 *
 *  @param handle	Pointer to moal_handle structure
 *  @param pmbuf	Pointer to mlan_buffer structure
 *  @param ep		Endpoint to receive
 *  @param timeout 	Time out value
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_read_data_sync(moal_handle * handle, mlan_buffer * pmbuf, t_u8 ep,
                    t_u32 timeout)
{
    struct usb_card_rec *cardp = handle->card;
    t_u8 *data = (t_u8 *) (pmbuf->pbuf + pmbuf->data_offset);
    t_u32 buf_len = pmbuf->data_len;
    int actual_length;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    ENTER();
    /* Receive the data response */
    if ((ret = usb_bulk_msg(cardp->udev, usb_rcvbulkpipe(cardp->udev,
                                                         ep), data,
                            buf_len, &actual_length, timeout))) {
        PRINTM(ERROR, "usb_bulk_msg failed: %d\n", ret);
        ret = MLAN_STATUS_FAILURE;
    }
    pmbuf->data_len = actual_length;
    DBG_HEXDUMP(IF_D, "read sync", data, actual_length);
    LEAVE();
    return ret;
}

/**
 *  @brief  This function downloads data/comand packet to device
 *
 *  @param handle	Pointer to moal_handle structure
 *  @param pmbuf	Pointer to mlan_buffer structure
 *  @param ep		Endpoint to send
 *
 *  @return 	   	MLAN_STATUS_PENDING or MLAN_STATUS_FAILURE
 */
mlan_status
woal_write_data_async(moal_handle * handle, mlan_buffer * pmbuf, t_u8 ep)
{
    struct usb_card_rec *cardp = handle->card;
    urb_context *context = NULL;
    t_u8 *data = (t_u8 *) (pmbuf->pbuf + pmbuf->data_offset);
    struct urb *tx_urb = NULL;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    /* Check if device is removed */
    if (handle->surprise_removed) {
        PRINTM(ERROR, "Device removed\n");
        ret = MLAN_STATUS_FAILURE;
        goto tx_ret;
    }

    if ((ep == cardp->tx_data_ep) &&
        (atomic_read(&cardp->tx_data_urb_pending) >= MVUSB_TX_HIGH_WMARK)) {
        ret = MLAN_STATUS_RESOURCE;
        goto tx_ret;
    }

    PRINTM(DATA, "woal_write_data_async: ep=%d\n", ep);

    if (ep == cardp->tx_cmd_ep) {
        context = &cardp->tx_cmd;
    } else {
        if (cardp->tx_data_ix >= MVUSB_TX_HIGH_WMARK)
            cardp->tx_data_ix = 0;
        context = &cardp->tx_data_list[cardp->tx_data_ix++];
    }

    context->handle = handle;
    context->ep = ep;
    context->pmbuf = pmbuf;
    tx_urb = context->urb;

    /* 
     * Use USB API usb_fill_bulk_urb() to set the
     * configuration information of the Tx bulk URB
     * and initialize the Tx callback
     */
    usb_fill_bulk_urb(tx_urb, cardp->udev,
                      usb_sndbulkpipe(cardp->udev,
                                      ep), data, pmbuf->data_len,
                      woal_usb_tx_complete, (void *) context);

    tx_urb->transfer_flags |= URB_ZERO_PACKET;
    if (ep == cardp->tx_cmd_ep)
        atomic_inc(&cardp->tx_cmd_urb_pending);
    else
        atomic_inc(&cardp->tx_data_urb_pending);
    if ((ret = usb_submit_urb(tx_urb, GFP_ATOMIC))) {
        /* Submit URB failure */
        PRINTM(ERROR, "Submit Tx data URB failed: %d\n", ret);
        if (ep == cardp->tx_cmd_ep)
            atomic_dec(&cardp->tx_cmd_urb_pending);
        else {
            atomic_dec(&cardp->tx_data_urb_pending);
            if (cardp->tx_data_ix)
                cardp->tx_data_ix--;
            else
                cardp->tx_data_ix = MVUSB_TX_HIGH_WMARK;
        }
        ret = MLAN_STATUS_FAILURE;
    } else {
        ret = MLAN_STATUS_SUCCESS;
    }

  tx_ret:
    if (!ret)
        ret = MLAN_STATUS_PENDING;

    LEAVE();
    return ret;
}

/**
 *  @brief  This function register usb device and initialize parameter
 *
 *  @param handle	Pointer to moal_handle structure
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_register_dev(moal_handle * handle)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    cardp->phandle = handle;
    handle->hotplug_device = &(cardp->udev->dev);
    if (!fw_name) {
        fw_name = DEFAULT_FW_NAME;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief This function registers driver.
 *
 *  @return 	 MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_bus_register(void)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    ENTER();
    /* 
     * API registers the Marvell USB driver
     * to the USB system
     */
    if (usb_register(&woal_usb_driver)) {
        PRINTM(FATAL, "USB Driver Registration Failed \n");
        ret = MLAN_STATUS_FAILURE;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief This function removes usb driver.
 *
 *  @return 	   	N/A
 */
void
woal_bus_unregister(void)
{
    ENTER();
    /* API unregisters the driver from USB subsystem */
    usb_deregister(&woal_usb_driver);
    LEAVE();
    return;
}

/** 
 *  @brief This function makes USB device to suspend.
 *  
 *  @param handle  A pointer to moal_handle structure
 *
 *  @return             0 --success, otherwise fail
 */
int
woal_enter_usb_suspend(moal_handle * handle)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#ifdef CONFIG_PM
    struct usb_device *udev = ((struct usb_card_rec *) (handle->card))->udev;
#endif /* CONFIG_PM */
#endif /* >= 2.6.24 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    struct usb_interface *intf = ((struct usb_card_rec *) (handle->card))->intf;
#endif /* < 2.6.34 */

    ENTER();

    if (handle->is_suspended == MTRUE) {
        PRINTM(ERROR, "Device already suspended\n");
        LEAVE();
        return -EFAULT;
    }

    handle->suspend_wait_q_woken = MFALSE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#ifdef CONFIG_PM
    /* Enter into USB suspend */
    usb_lock_device(udev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
    udev->autosuspend_delay = 0;        /* Autosuspend delay in jiffies */
#else
    pm_runtime_set_autosuspend_delay(&udev->dev, 0);    /* Autosuspend delay in 
                                                           jiffies */
#endif /* < 2.6.38 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    udev->autosuspend_disabled = 0;     /* /sys/bus/usb/devices/.../power/level 
                                           < auto */
#endif /* < 2.6.34 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
    udev->autoresume_disabled = 0;
#endif /* < 2.6.33 */
    usb_unlock_device(udev);
#endif /* CONFIG_PM */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    intf->pm_usage_cnt = 1;
#else
    atomic_set(&intf->pm_usage_cnt, 1);
#endif /* < 2.6.32 */
    usb_autopm_put_interface(intf);
#else
    usb_lock_device(udev);
    atomic_set(&udev->dev.power.usage_count, 1);
    usb_enable_autosuspend(udev);
    usb_unlock_device(udev);
#endif /* < 2.6.34 */
#endif /* >= 2.6.24 */

    /* Wait for suspend to complete */
    wait_event_interruptible(handle->suspend_wait_q,
                             handle->suspend_wait_q_woken);

    LEAVE();
    return 0;
}

/** 
 *  @brief This function makes USB device to resume.
 *  
 *  @param handle  A pointer to moal_handle structure
 *
 *  @return             0 --success, otherwise fail
 */
int
woal_exit_usb_suspend(moal_handle * handle)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#ifdef CONFIG_PM
    struct usb_device *udev = ((struct usb_card_rec *) (handle->card))->udev;
#endif /* CONFIG_PM */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    struct usb_interface *intf = ((struct usb_card_rec *) (handle->card))->intf;
#endif /* < 2.6.34 */
#endif /* >= 2.6.24 */

    ENTER();

    if (handle->is_suspended == MFALSE) {
        PRINTM(ERROR, "Device already resumed\n");
        LEAVE();
        return -EFAULT;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#ifdef CONFIG_PM
    /* Exit from USB suspend */
    usb_lock_device(udev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    udev->autosuspend_disabled = 1;     /* /sys/bus/usb/devices/.../power/level 
                                           < on */
#endif /* < 2.6.34 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
    udev->autoresume_disabled = 0;
#endif /* < 2.6.33 */
    usb_unlock_device(udev);
#endif /* CONFIG_PM */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    intf->pm_usage_cnt = 0;
#else
    atomic_set(&intf->pm_usage_cnt, 0);
#endif /* < 2.6.32 */
    usb_autopm_get_interface(intf);
#else
    usb_lock_device(udev);
    atomic_set(&udev->dev.power.usage_count, 0);
    usb_disable_autosuspend(udev);
    usb_unlock_device(udev);
#endif /* < 2.6.34 */
#endif /* >= 2.6.24 */

    LEAVE();
    return 0;
}

/**
 *  @brief This function will submit rx urb.
 *
 *  @param handle   Pointer to moal_handle
 *  @param ep       Endpoint to re-submit urb
 *
 *  @return 	   	N/A
 */
void
woal_submit_rx_urb(moal_handle * handle, t_u8 ep)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;

    ENTER();

    if ((ep == cardp->rx_cmd_ep) && (!atomic_read(&cardp->rx_cmd_urb_pending))) {
        woal_usb_submit_rx_urb(&cardp->rx_cmd, MLAN_RX_CMD_BUF_SIZE);
    }

    LEAVE();
    return;
}
