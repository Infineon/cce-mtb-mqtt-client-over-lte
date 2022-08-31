/******************************************************************************
* File Name:   mqtt_task.h
*
* Description: This file is the public interface of mqtt_task.c
*
* Related Document: See README.md
*
*
*******************************************************************************
* $ Copyright 2020-YEAR Cypress Semiconductor $
*******************************************************************************/

#ifndef MQTT_TASK_H_
#define MQTT_TASK_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "cy_mqtt_api.h"


/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for MQTT Client Task. */
#define MQTT_CLIENT_TASK_PRIORITY       (2)
#define MQTT_CLIENT_TASK_STACK_SIZE     (1024 * 2)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the MQTT Client Task. */
typedef enum
{
    HANDLE_MQTT_SUBSCRIBE_FAILURE,
    HANDLE_MQTT_PUBLISH_FAILURE,
    HANDLE_DISCONNECTION
} mqtt_task_cmd_t;

/*******************************************************************************
 * Extern variables
 ******************************************************************************/
extern cy_mqtt_t mqtt_connection;
extern QueueHandle_t mqtt_task_q;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void mqtt_client_task(void *pvParameters);

#endif /* MQTT_TASK_H_ */

/* [] END OF FILE */
