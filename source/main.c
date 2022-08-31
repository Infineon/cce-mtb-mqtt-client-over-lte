/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for MQTT Client Example for ModusToolbox.
*
* Related Document: See README.md
*
*******************************************************************************
* $ Copyright 2020-YEAR Cypress Semiconductor $
*******************************************************************************/

/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "mqtt_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ppp_modem_defines.h"
/* Include serial flash library and QSPI memory configurations only for the
 * kits that require the Wi-Fi firmware to be loaded in external QSPI NOR flash.
 */
#if defined(TARGET_CY8CPROTO_062S3_4343W)
#include "cy_serial_flash_qspi.h"
#include "cycfg_qspi_memslot.h"
#endif

/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes retarget IO, sets up 
 *  the MQTT client task, and then starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int main()
{
    cy_rslt_t result;

    /* Initialize the board support package. */
    result = cybsp_init();
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* To avoid compiler warnings. */
    (void) result;

    /* Enable global interrupts. */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port. */
#if defined(CYBSP_PPP_CAPABLE)
    cy_retarget_io_init(UART_HMI_TX, UART_HMI_RX, CY_RETARGET_IO_BAUDRATE);
#else
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);
#endif

#if defined(TARGET_CY8CPROTO_062S3_4343W)
    /* Initialize the QSPI serial NOR flash with clock frequency of 50 MHz. */
    const uint32_t bus_frequency = 50000000lu;
    cy_serial_flash_qspi_init(smifMemConfigs[0], CYBSP_QSPI_D0, CYBSP_QSPI_D1,
                                  CYBSP_QSPI_D2, CYBSP_QSPI_D3, NC, NC, NC, NC,
                                  CYBSP_QSPI_SCK, CYBSP_QSPI_SS, bus_frequency);

    /* Enable the XIP mode to get the Wi-Fi firmware from the external flash. */
    cy_serial_flash_qspi_enable_xip(true);
#endif

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");
    printf("===============================================================\n");
    printf("CCE MTB MQTT Client Over LTE\n");
    printf("===============================================================\n\n");

    /* Create the MQTT Client task. */
    xTaskCreate(mqtt_client_task, "MQTT Client task", MQTT_CLIENT_TASK_STACK_SIZE,
                NULL, MQTT_CLIENT_TASK_PRIORITY, NULL);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);
}

/* [] END OF FILE */
