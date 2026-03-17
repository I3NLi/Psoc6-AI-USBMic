/*******************************************************************************
* File Name   : bt_hci_bridge.h
*
* Description : Bluetooth HCI bridge over USB CDC ACM.
*
*******************************************************************************/

#ifndef BT_HCI_BRIDGE_H
#define BT_HCI_BRIDGE_H

#include "cy_result.h"

#if defined(__cplusplus)
extern "C" {
#endif

cy_rslt_t bt_hci_bridge_init(void);

#if defined(__cplusplus)
}
#endif

#endif /* BT_HCI_BRIDGE_H */
