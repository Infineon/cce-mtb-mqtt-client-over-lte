/******************************************************************************
* File Name: wifi_config.h
*
* Description: This file contains the configuration macros required for the
*              Wi-Fi connection.
*
* Related Document: See README.md
*
*
*******************************************************************************
* $ Copyright 2020-YEAR Cypress Semiconductor $
*******************************************************************************/

#ifndef WIFI_CONFIG_H_
#define WIFI_CONFIG_H_

#include "cy_wcm.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* SSID of the Wi-Fi Access Point to which the MQTT client connects. */
#define WIFI_SSID                         "MY_WIFI_SSID"

/* Passkey of the above mentioned Wi-Fi SSID. */
#define WIFI_PASSWORD                     "MY_WIFI_PASSWORD"

/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
 * in "cy_wcm.h" for more details.
 */
#define WIFI_SECURITY                     CY_WCM_SECURITY_WPA2_AES_PSK

/* Maximum Wi-Fi re-connection limit. */
#define MAX_WIFI_CONN_RETRIES             (120u)

/* Wi-Fi re-connection time interval in milliseconds. */
#define WIFI_CONN_RETRY_INTERVAL_MS       (5000)

#endif /* WIFI_CONFIG_H_ */
