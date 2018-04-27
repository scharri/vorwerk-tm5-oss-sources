/******************************************************************************
 *  (c) Vorwerk Elektrowerke GmbH & Co. KG
 *  Rauental 38
 *  42270 Wuppertal
 *  Deutschland
 ******************************************************************************
 *  All contents of this source file are strictly confidential.
 *
 *  Use, reproduction and dissemination of any contents, is not permitted
 *  without express written authority of VORWERK ELEKTROWERKE
 *  (GmbH & Co KG). Usage for other purposes (e.g. development for third
 *  parties) than contractually intends (e.g. inspection for approval) is
 *  strictly prohibited.
 *
 *  All rights, including copyright, rights created by patent grant or
 *  registration, and rights by protection of utility patents, are reserved.
 *  Violations will be prosecuted by civil and criminal law.
 *
 *  Author: Michael Neuwert, Michael.Neuwert@vorwerk.de
 *
 ******************************************************************************/

#ifndef KEY_STORE_H
#define KEY_STORE_H

#include <linux/ioctl.h>

#define PFX "key_store: "

#include "key_store_ioctl.h"

__u32 _get_key_size(crypto_key_type_e type);
void *_get_key_ptr(crypto_key_type_e type);

#endif
