/******************************************************************************
* File Name:   publisher_task.h
*
* Description: This file is the public interface of publisher_task.c
*
* Related Document: See README.md
*
*
*******************************************************************************
* $ Copyright 2020-YEAR Cypress Semiconductor $
*******************************************************************************/

#ifndef PUBLISHER_TASK_H_
#define PUBLISHER_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for Button Task. */
#define PUBLISHER_TASK_PRIORITY               (2)
#define PUBLISHER_TASK_STACK_SIZE             (1024 * 1)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the Publisher Task. */
typedef enum
{
    PUBLISHER_INIT,
    PUBLISHER_DEINIT,
    PUBLISH_MQTT_MSG
} publisher_cmd_t;

/* Struct to be passed via the publisher task queue */
typedef struct{
    publisher_cmd_t cmd;
    char *data;
} publisher_data_t;

/*******************************************************************************
* Extern Variables
********************************************************************************/
extern TaskHandle_t publisher_task_handle;
extern QueueHandle_t publisher_task_q;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void publisher_task(void *pvParameters);

#endif /* PUBLISHER_TASK_H_ */

/* [] END OF FILE */
