/******************************************************************************
* File Name:   ppp_defines.h
*
* Description: This file is the header for PPP related Modem pins
*
* Related Document: See README.md
*
*
*******************************************************************************
* $ Copyright 2020-YEAR Cypress Semiconductor $
*******************************************************************************/
#if defined(CYBSP_PPP_CAPABLE)
/* Debug UART Pins */
#define UART_HMI_RX P9_0
#define UART_HMI_TX P9_1
/* LTE Modem UART Pins */
#define UART_MODEM_RX P5_0
#define UART_MODEM_TX P5_1
#define UART_MODEM_RTS P5_2
#define UART_MODEM_CTS	P5_3
#define UART_MODEM_RESET P5_5

/* LTE Modem Baud Rate*/
#define UART_MODEM_BAUDRATE 115200
/* Wake Up pin */
#define PSOC_WAKE_UP                   (P5_6)
#endif
