/*
 * $ Copyright Cypress Semiconductor $
*/

#include <string.h>
#include <stdint.h>
#include <string.h>
#include "lwipopts.h"
#include "cy_lwip_dhcp_server.h"
#include "cy_lwip_error.h"
#include "cy_lwip_log.h"
#include "cy_network_mw_core.h"
#include "cy_ppp.h"

#include "cy_result.h"
#include "cybsp_types.h"
#include "cy_retarget_io.h"
#include "cyhal.h"
#include "cy_pdl.h"
#include "cybsp.h"

#include "FreeRTOS.h"
#include "task.h"
#include "ppp_modem_defines.h"
#if defined(CYBSP_PPP_CAPABLE)
/* While using lwIP/sockets errno is required. Since IAR and ARMC6 doesn't define errno variable, the following definition is required for building it successfully. */
#if !( (defined(__GNUC__) && !defined(__ARMCC_VERSION)) )
int errno;
#endif

/******************************************************
 *                      Macros
 ******************************************************/
/**
 * Suppress unused variable warning
 */
#define UNUSED_VARIABLE(x) ( (void)(x) )

#define PPP_WORKER_THREAD_PRIORITY             (CY_RTOS_PRIORITY_ABOVENORMAL)
#define PPP_WORKER_THREAD_STACK_SIZE           (2 * 1024)
#define PPP_AT_RESPONSE_TIMEOUT                (0xffffffff)
#define CY_PPP_MAX_MUTEX_WAIT_TIME_MS          (120000)
#define GPIO_INTERRUPT_PRIORITY                (7u)
/******************************************************
 *               Variable Definitions
 ******************************************************/
cyhal_uart_t                      uart_modem_obj;
static TaskHandle_t               at_read_handle;
static cy_event_t                 event;
static cy_worker_thread_info_t    cy_ppp_status_thread;
static cy_mutex_t                 ppp_mutex;

cy_ppp_link_status_callback_t     cy_ppp_status_cb = NULL;
cy_network_interface_context      *nw_ppp_if_ctx = NULL;

/* Connection status flag */
static uint8_t ppp_connect_status=0;
/* Flag indicating cy_ppp_stop is initiated */
static uint8_t ppp_stop_triggered = 0;

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static cy_rslt_t nw_to_cy_code(uint32_t err_code);
static void cy_ppp_modem_send_cmd(char* command);
static void at_response_processing_thread(void *arg);
static void gpio_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event);

cyhal_gpio_callback_data_t wake_pin_cb = { .callback = gpio_interrupt_handler, .callback_arg = NULL };

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Convert the connectivity middleware PPP error code to ppp-connection manager error code  */
static cy_rslt_t nw_to_cy_code(uint32_t err_code)
{
    switch(err_code) {
    case CY_RSLT_NETWORK_PPP_ERR_NONE:               /* No error. */
         return CY_PPP_ERR_NONE;

    case CY_RSLT_NETWORK_PPP_ERR_PARAM:             /* Invalid parameter. */
         return CY_PPP_ERR_PARAM;

    case CY_RSLT_NETWORK_PPP_ERR_OPEN:              /* Unable to open PPP session. */
        return CY_PPP_ERR_OPEN;

    case CY_RSLT_NETWORK_PPP_ERR_DEVICE:            /* Invalid I/O device for PPP. */
        return CY_PPP_ERR_DEVICE;

    case CY_RSLT_NETWORK_PPP_ERR_ALLOC:             /* Unable to allocate resources. */
        return CY_PPP_ERR_ALLOC;

    case CY_RSLT_NETWORK_PPP_ERR_USER:              /* User interrupt. */
        return CY_PPP_ERR_USER;

    case CY_RSLT_NETWORK_PPP_ERR_CONNECT:           /* Connection lost. */
        return CY_PPP_ERR_CONNECT;

    case CY_RSLT_NETWORK_PPP_ERR_AUTHFAIL:          /* Failed authentication challenge. */
        return CY_PPP_ERR_AUTHFAIL;

    case CY_RSLT_NETWORK_PPP_ERR_PROTOCOL:          /* Failed to meet protocol. */
        return CY_PPP_ERR_PROTOCOL;

    case CY_RSLT_NETWORK_PPP_ERR_PEERDEAD:          /* Connection timeout. */
        return CY_PPP_ERR_PEERDEAD;

    case CY_RSLT_NETWORK_PPP_ERR_IDLETIMEOUT:       /* Idle Timeout. */
        return CY_PPP_ERR_IDLETIMEOUT;

    case CY_RSLT_NETWORK_PPP_ERR_CONNECTTIME:       /* Max connect time reached. */
        return CY_PPP_ERR_CONNECTTIME;

    case CY_RSLT_NETWORK_PPP_ERR_LOOPBACK:          /* Loopback detected. */
        return CY_PPP_ERR_LOOPBACK;

    default:
        return CY_PPP_ERR_GENERIC;
    }
}

/* Send AT commands to modem */
static void cy_ppp_modem_send_cmd(char* command)
{
    cy_rslt_t res;
    size_t cmd_len;
    cmd_len = strlen(command);

    res = cyhal_uart_clear(&uart_modem_obj);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"Failed to clear the uart\n");
    }

    res = cyhal_uart_write(&uart_modem_obj, command, &cmd_len);
    if(res == CY_RSLT_SUCCESS)
    {
        cmd_len = strlen("\r");
        cyhal_uart_write(&uart_modem_obj, "\r", &cmd_len);
        cmd_len = strlen("\n");
        cyhal_uart_write(&uart_modem_obj, "\n", &cmd_len);
    }
}

/* Thread to read the response of AT commands */
static void at_response_processing_thread(void *arg)
{
    char rx[100], response[100];
    size_t rx_length;
    uint8_t rx_index=0, response_index=0;
    uint32_t bits;

    while(1)
    {
        if(cyhal_uart_readable(&uart_modem_obj) != 0)
        {
            memset(rx,0,sizeof(rx));
            rx_index=0;
            do
            {
                cyhal_uart_read(&uart_modem_obj, &rx[rx_index], &rx_length);
                rx_index+=rx_length;
                cy_rtos_delay_milliseconds(10);
            } while(cyhal_uart_readable(&uart_modem_obj) != 0);

            ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"Response: %s\n",rx);
            printf("Response: %s\n",rx);

            response_index = 0;
            memset(response,0,sizeof(response));

            /* Copy the data from rx buffer to response buffer */
            for(int n=0;n<rx_index;n++)
            {
                /* Exclude the characters '\r' and '\n' */
                if((rx[n] != 13) && (rx[n] != 10))
                {
                    response[response_index] = rx[n];
                    response_index++;
                }
            }

            /* Look for required string sequences in the response buffer */
            if(strstr(response,"+SYSSTART") != NULL)
            {
                bits = CY_PPP_EVENT_START;
                cy_rtos_setbits_event(&event, bits, false);
            }
            if(strstr(response,"OK") != NULL)
            {
                bits = CY_PPP_EVENT_OK;
                cy_rtos_setbits_event(&event, bits, false);
            }
            if(strstr(response,"+SQNBANDSEL") != NULL)
            {
                bits = CY_PPP_EVENT_BAND;
                cy_rtos_setbits_event(&event, bits, false);
            }
            if(strstr(response,"+CEREG: 1") != NULL)
            {
                bits = CY_PPP_EVENT_CEREG;
                cy_rtos_setbits_event(&event, bits, false);
            }
            if(strstr(response,"+CEREG: 5") != NULL)
            {
                bits = CY_PPP_EVENT_CEREG;
                cy_rtos_setbits_event(&event, bits, false);
            }
            if(strstr(response,"CONNECT") != NULL)
            {
                bits = CY_PPP_EVENT_MODEM_PPP_ENABLE;
                cy_rtos_setbits_event(&event, bits, false);
            }
        }
        cy_rtos_delay_milliseconds(500);
    }
}

static cy_rslt_t cy_ppp_modem_init()
{
    cy_rslt_t res;
    uint32_t bits;

    cyhal_syspm_lock_deepsleep();

    /* Reset Modem */
    ppp_print(("Initializing modem... \r\n"));
    cyhal_gpio_init(UART_MODEM_RESET,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_write(UART_MODEM_RESET, 1);
    cy_rtos_delay_milliseconds(1000);
    cyhal_gpio_write(UART_MODEM_RESET, 0);

    /* Create a task to read the modem response for AT commands */
    if(at_read_handle == NULL)
    {
        cy_rtos_create_thread(&at_read_handle, at_response_processing_thread,
                             "at_response_processing_thread",
                             NULL, 4*1024, CY_RTOS_PRIORITY_ABOVENORMAL, NULL);
    }

    /* Initialize the RTOS event */
    cy_rtos_init_event(&event);

    bits = CY_PPP_EVENT_START;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\n Modem Reset failed"));
        return CY_PPP_GENERAL_ERROR;
    }

    ppp_print(("\r\nSending: AT+SQNIPSCFG for UART Interface Power Saving Configuration..."));
    cy_ppp_modem_send_cmd("AT+SQNIPSCFG=2,1000");
    bits = CY_PPP_EVENT_OK;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\n AT+SQNIPSCFG failed"));
        return CY_PPP_GENERAL_ERROR;
    }

    ppp_print(("\r\nSending: AT+SQNRICFG to configure the Ring line..."));
    cy_ppp_modem_send_cmd("AT+SQNRICFG=1,3,1000");
    bits = CY_PPP_EVENT_OK;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\n AT+SQNRICFG failed"));
        return CY_PPP_GENERAL_ERROR;
    }

    ppp_print(("\r\nSending: AT..."));
    cy_ppp_modem_send_cmd("AT");
    cy_rtos_delay_milliseconds(1000);
    bits = CY_PPP_EVENT_OK;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\nFailed to send AT command"));
        return CY_PPP_GENERAL_ERROR;
    }
    ppp_print(("\r\nAT command Response Successful!!\r\n"));

    ppp_print(("\r\nSending: AT+SQNBANDSEL..."));
    cy_ppp_modem_send_cmd("AT+SQNBANDSEL=0,\"standard\",\"2,4,12\"");
    cy_rtos_delay_milliseconds(1000);
    bits = CY_PPP_EVENT_BAND;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\nFailed to set the network band"));
        return CY_PPP_GENERAL_ERROR;
    }
    ppp_print(("\r\nNetwork Band Set!!\r\n"));

    ppp_print(("\r\nSending: AT+CFUN..."));
    cy_ppp_modem_send_cmd("AT+CFUN=1");
    cy_rtos_delay_milliseconds(500);
    bits = CY_PPP_EVENT_OK;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\nFailed to send command to attach to network!!"));
        return CY_PPP_GENERAL_ERROR;
    }
    ppp_print(("\r\nCommand to attach to network is sent...\r\n"));

    ppp_print(("\r\nSending: AT+CEREG..."));
    cy_ppp_modem_send_cmd("AT+CEREG=1");
    cy_rtos_delay_milliseconds(1000);
    bits = CY_PPP_EVENT_OK;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\nFailed to send command to check the network status"));
        return CY_PPP_GENERAL_ERROR;
    }
    ppp_print(("\r\nWaiting for network...\r\n"));

    bits = CY_PPP_EVENT_CEREG;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\nFailed to attach to the network"));
        return CY_PPP_GENERAL_ERROR;
    }
    ppp_print(("\r\nNetwork ready!!\r\n"));

    ppp_print(("\r\nSending: AT+CGDATA..."));
    cy_ppp_modem_send_cmd("AT+CGDATA=\"PPP\",1");
    cy_rtos_delay_milliseconds(1000);
    bits = CY_PPP_EVENT_MODEM_PPP_ENABLE;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_print(("\r\nFailed to initiate PPP on the modem"));
        return CY_PPP_GENERAL_ERROR;
    }
    ppp_print(("\r\nPPP Mode enabled!!\r\n"));

    /* Delete the task used to read response of AT commands */
    if (at_read_handle != NULL)
    {
        cy_rtos_terminate_thread(&at_read_handle);
        at_read_handle = NULL;
    }

    cyhal_syspm_unlock_deepsleep();
    return CY_RSLT_SUCCESS;
}

static cy_rslt_t cy_ppp_modem_deinit()
{
    /* De-initialize the RTOS event */
    cy_rtos_deinit_event(&event);

    cyhal_gpio_free(UART_MODEM_RESET);

    return CY_RSLT_SUCCESS;
}

void cy_ppp_link_status_cb(void *err_code)
{
    uint32_t code;
    cy_rslt_t res;
    uint32_t bits;

    ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"Status callback \n");

    struct netif *pppif = cy_network_get_nw_interface(CY_NETWORK_PPP_INTERFACE, 0);

    /* Convert lwip-network-interface-integration code to PPP error code */
    code = nw_to_cy_code(*(uint32_t *)err_code);

    if(cy_ppp_status_cb != NULL)
    {
        if((res = cy_worker_thread_enqueue(&cy_ppp_status_thread, cy_ppp_status_cb, &code)) != CY_RSLT_SUCCESS)
        {
            cm_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "L%d : %s() : ERROR : Failed to send connection status. Err = [%lu]\r\n", __LINE__, __FUNCTION__, res);
        }
    }

    switch(code) {
    case CY_PPP_ERR_NONE:               /* No error. */
        {
            ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_NONE \r \n");
            if(pppif->ip_addr.u_addr.ip4.addr != 0)
            {
                ppp_print(("Assigned an IP address to the device\r\n"));

                bits = CY_PPP_EVENT_CONNECT;
                cy_rtos_setbits_event(&event, bits, false);

                if(cy_rtos_get_mutex(&ppp_mutex, CY_PPP_MAX_MUTEX_WAIT_TIME_MS) != CY_RSLT_SUCCESS)
                {
                    ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to acquire PPP mutex \n");
                    return;
                }
                /* IP address is assigned and DNS addresses are also assigned.
                 * Set connection status
                 */
                ppp_connect_status = 1;

                if (cy_rtos_set_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
                {
                    ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to release PPP mutex \n");
                }
            }
        }
        break;

    case CY_PPP_ERR_PARAM:             /* Invalid parameter. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_OPEN \r \n");
        break;

    case CY_PPP_ERR_OPEN:              /* Unable to open PPP session. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO,"\r\n ppp_link_status_cb: CY_PPP_ERR_OPEN\r\n");
        break;

    case CY_PPP_ERR_DEVICE:            /* Invalid I/O device for PPP. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO,"\r\n ppp_link_status_cb: CY_PPP_ERR_DEVICE\r\n");
        break;

    case CY_PPP_ERR_ALLOC:             /* Unable to allocate resources. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO,"\r\n ppp_link_status_cb: CY_PPP_ERR_ALLOC \r \n");
        break;

    case CY_PPP_ERR_USER:              /* User interrupt. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO,"\r\n ppp_link_status_cb: CY_PPP_ERR_USER \r \n");
        break;

    case CY_PPP_ERR_CONNECT:           /* Connection lost. */
        {
            ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_CONNECT \r \n");
            ppp_print(("Network connection lost!!\r\nPlease re-connect to the network before proceeding!\r\n"));
            break;
        }

    case CY_PPP_ERR_AUTHFAIL:          /* Failed authentication challenge. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_AUTHFAIL \r \n");
        break;

    case CY_PPP_ERR_PROTOCOL:          /* Failed to meet protocol. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_PROTOCOL \r \n");
        break;

    case CY_PPP_ERR_PEERDEAD:          /* Connection timeout. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_PEERDEAD \r \n");
        break;

    case CY_PPP_ERR_IDLETIMEOUT:       /* Idle Timeout. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_IDLETIMEOUT \r \n");
        break;

    case CY_PPP_ERR_CONNECTTIME:       /* Max connect time. */
         ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_CONNECTTIME \r \n");
        break;

    case CY_PPP_ERR_LOOPBACK:          /* Loopback detected. */
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\n ppp_link_status_cb: CY_PPP_ERR_LOOPBACK \r \n");
        break;

    default:
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO," \r\nppp_link_status_cb: unknown errCode \r \n");
        break;
    }
}

static void gpio_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{
    ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"Wake up interrupt\n");
}

cy_rslt_t cy_ppp_init()
{
    cy_rslt_t res;
    
    /* Initialize two UART interface for psoc6+modem communication */
    const cyhal_uart_cfg_t uart_config =
    {
        .data_bits          = 8,
        .stop_bits          = 1,
        .parity             = CYHAL_UART_PARITY_NONE,
        .rx_buffer          = NULL,
        .rx_buffer_size     = 0
    };
    
    res = cyhal_uart_init(&uart_modem_obj, UART_MODEM_TX, UART_MODEM_RX, UART_MODEM_CTS,UART_MODEM_RTS,NULL, &uart_config);

    if (res == CY_RSLT_SUCCESS)
    {
        res = cyhal_uart_set_baud(&uart_modem_obj, UART_MODEM_BAUDRATE, NULL);
        if(res != CY_RSLT_SUCCESS)
        {
            ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"Setting baud rate for psoc6+modem interface failed\n");
            return res;
        }
    }

    cyhal_uart_enable_flow_control(&uart_modem_obj, true,true);
    cyhal_uart_set_fifo_level(&uart_modem_obj, CYHAL_UART_FIFO_RX,64);

    /* Initialize the wake up pin */
    res = cyhal_gpio_init(PSOC_WAKE_UP, CYHAL_GPIO_DIR_INPUT,
                    CYHAL_GPIO_DRIVE_NONE, false);

    cyhal_gpio_register_callback(PSOC_WAKE_UP,
                                 &wake_pin_cb);
    cyhal_gpio_enable_event(PSOC_WAKE_UP, CYHAL_GPIO_IRQ_RISE,
                                 GPIO_INTERRUPT_PRIORITY, true);

    /* Initialize mutex */
    if (cy_rtos_init_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR,"Mutex initialization failed\n");
        cyhal_uart_free(&uart_modem_obj);
        return CY_PPP_MUTEX_ERROR;
    }

    /* Acquire Mutex */
    if(cy_rtos_get_mutex(&ppp_mutex, CY_PPP_MAX_MUTEX_WAIT_TIME_MS) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to acquire PPP mutex \n");
        cy_rtos_deinit_mutex(&ppp_mutex);
        cyhal_uart_free(&uart_modem_obj);
        return CY_PPP_WAIT_TIMEOUT;
    }

    /* Use AT commands to initialize modem and start PPP on modem */
    res = cy_ppp_modem_init();
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"modem initialization failed\n");
        cy_rtos_set_mutex(&ppp_mutex);
        cy_rtos_deinit_mutex(&ppp_mutex);
        cyhal_uart_free(&uart_modem_obj);
        return res;
    }

    /* Release Mutex */
    if (cy_rtos_set_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to release PPP mutex \n");
    }

    return res;
}

cy_rslt_t cy_ppp_start(cy_ppp_link_status_callback_t status_cb)
{
    cy_rslt_t res = CY_RSLT_SUCCESS;
    uint32_t bits;

    if(cy_rtos_get_mutex(&ppp_mutex, CY_PPP_MAX_MUTEX_WAIT_TIME_MS) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to acquire PPP mutex \n");
        return CY_PPP_WAIT_TIMEOUT;
    }

    ppp_stop_triggered =  0;
    if(ppp_connect_status == 1)
    {
        ppp_print(("PPP already connected...\r\n"));
        goto exit;
    }

    ppp_print(("\r\nInitiating PPP connection...Please wait...\r\n"));

    /* Initialize network stack */
    res = cy_network_init();
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"cy_network_init failed\n");
        goto exit;
    }

    /* Add network interface */
    res = cy_network_add_nw_interface(CY_NETWORK_PPP_INTERFACE, 0, &uart_modem_obj, NULL, NULL, &nw_ppp_if_ctx);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"cy_network_add_nw_interface failed\n");
        cy_network_deinit();
        goto exit;
    }

    /* Set Status callbacks */
    cy_network_set_status_cb(cy_ppp_link_status_cb);

    /* Start a worker thread to send the status notification to application */
    if(status_cb != NULL)
    {
        cy_worker_thread_params_t params;

        /* Store the status callback function provided by user */
        cy_ppp_status_cb = status_cb;

        /* Create thread for sending status notification */
        memset(&params, 0, sizeof(params));
        params.name = "ppp_Status_Worker";
        params.priority = PPP_WORKER_THREAD_PRIORITY;
        params.stack = NULL;
        params.stack_size = PPP_WORKER_THREAD_STACK_SIZE;
        params.num_entries = 0;
        /* create a worker thread for handling the status callback */
        if((res = cy_worker_thread_create(&cy_ppp_status_thread, &params)) != CY_RSLT_SUCCESS)
        {
            cm_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "failed to create the worker thread \n");
            cy_ppp_status_cb = NULL;
            cy_network_deinit();
            goto exit;
        }
    }

    /* Bring up the network and initiate connection between psoc6 and peer modem */
    res = cy_network_ip_up(nw_ppp_if_ctx);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"PPP failed\n");
        if(cy_ppp_status_cb != NULL)
        {
            cy_worker_thread_delete(&cy_ppp_status_thread);
        }
        cy_ppp_status_cb = NULL;
        cy_network_deinit();
        goto exit;
    }

    /* Wait till the ppp_connect_status is set, indicating a successful
     * connection between the psoc6 and the peer modem
     */
    bits = CY_PPP_EVENT_CONNECT;
    res = cy_rtos_waitbits_event(&event, &bits, true, false, PPP_AT_RESPONSE_TIMEOUT);
    if(res != CY_RSLT_SUCCESS)
    {
        if(cy_ppp_status_cb != NULL)
        {
            cy_worker_thread_delete(&cy_ppp_status_thread);
        }
        cy_ppp_status_cb = NULL;
        cy_network_deinit();
        goto exit;
    }

    ppp_print(("\r\nPPP connection successful!!\r\n"));

exit:
    if (cy_rtos_set_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to release PPP mutex \n");
    }
    return res;
}

cy_rslt_t cy_ppp_stop()
{
    cy_rslt_t res;

    if(cy_rtos_get_mutex(&ppp_mutex, CY_PPP_MAX_MUTEX_WAIT_TIME_MS) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to acquire PPP mutex \n");
        return CY_PPP_WAIT_TIMEOUT;
    }

    /* Bring down the network */
    res = cy_network_ip_down(nw_ppp_if_ctx);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"cy_network_ip_down failed\n");
        goto exit;
    }

    if(cy_ppp_status_cb != NULL)
    {
        cy_worker_thread_delete(&cy_ppp_status_thread);
    }

    /* Reset connect status to 0 */
    ppp_connect_status = 0;

    /* Set the ppp_stop_triggered flag to 1 */
    ppp_stop_triggered = 1;

    res = cy_network_remove_nw_interface(nw_ppp_if_ctx);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"cy_network_remove_nw_interface failed\n");
        goto exit;
    }

    res = cy_network_deinit();
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"cy_network_deinit failed\n");
    }

exit:
    if (cy_rtos_set_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to release PPP mutex \n");
    }
    return res;
}

cy_rslt_t cy_ppp_deinit()
{
    cy_rslt_t res;

    if(cy_rtos_get_mutex(&ppp_mutex, CY_PPP_MAX_MUTEX_WAIT_TIME_MS) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to acquire PPP mutex \n");
        return CY_PPP_WAIT_TIMEOUT;
    }

    res = cy_ppp_modem_deinit();
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"\r\ncy_ppp_modem_deinit failed\r\n");
    }

    nw_ppp_if_ctx = NULL;

    if (cy_rtos_set_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to release PPP mutex \n");
    }

    /* De-initialize Mutex */
    if((res = cy_rtos_deinit_mutex(&ppp_mutex)) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Error while de initializing mutex \n");
    }

    cyhal_uart_free(&uart_modem_obj);

    return res;
}

cy_rslt_t cy_ppp_get_ip(cy_nw_ip_address_t *ip_addr)
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    if(cy_rtos_get_mutex(&ppp_mutex, CY_PPP_MAX_MUTEX_WAIT_TIME_MS) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to acquire PPP mutex \n");
        return CY_PPP_WAIT_TIMEOUT;
    }

    if(ppp_connect_status == 0)
    {
        ppp_print(("\r\nPPP link in down\r\n"));
        res = CY_PPP_GENERAL_ERROR;
        goto exit;
    }

    res = cy_network_get_ip_address(nw_ppp_if_ctx, ip_addr);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"\r\ncy_network_get_ip_address failed\r\n");
    }
exit:
    if (cy_rtos_set_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to release PPP mutex \n");
    }
    return res;
}

cy_rslt_t cy_ppp_get_ipv6_address(cy_nw_ip_address_t *ip_addr, cy_ppp_ipv6_type_t type)
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    if(cy_rtos_get_mutex(&ppp_mutex, CY_PPP_MAX_MUTEX_WAIT_TIME_MS) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to acquire PPP mutex \n");
        return CY_PPP_WAIT_TIMEOUT;
    }

    if(ppp_connect_status == 0)
    {
        ppp_print(("\r\nPPP link in down\r\n"));
        res = CY_PPP_GENERAL_ERROR;
        goto exit;
    }

    if(type != CY_PPP_IPV6_LINK_LOCAL)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Only CY_PPP_IPV6_LINK_LOCAL address type is supported \n");
        return CY_PPP_GENERAL_ERROR;
    }
    type = CY_NETWORK_IPV6_LINK_LOCAL;
    res = cy_network_get_ipv6_address(nw_ppp_if_ctx, type, ip_addr);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"\r\ncy_ppp_get_ipv6_address failed\r\n");
    }

exit:
    if (cy_rtos_set_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to release PPP mutex \n");
    }
    return res;
}

cy_rslt_t cy_ppp_ping(cy_nw_ip_address_t *address, uint32_t timeout_ms, uint32_t* elapsed_time_ms)
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    if(cy_rtos_get_mutex(&ppp_mutex, CY_PPP_MAX_MUTEX_WAIT_TIME_MS) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to acquire PPP mutex \n");
        return CY_PPP_WAIT_TIMEOUT;
    }

    if(ppp_connect_status == 0)
    {
        ppp_print(("\r\nPPP link in down\r\n"));
        res = CY_PPP_GENERAL_ERROR;
        goto exit;
    }
    res = cy_network_ping(nw_ppp_if_ctx, address, timeout_ms, elapsed_time_ms);
    if(res != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG,"\r\ncy_network_ping failed\r\n");
    }

exit:
    if (cy_rtos_set_mutex(&ppp_mutex) != CY_RSLT_SUCCESS)
    {
        ppp_cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Unable to release PPP mutex \n");
    }
    return res;
}
#endif
