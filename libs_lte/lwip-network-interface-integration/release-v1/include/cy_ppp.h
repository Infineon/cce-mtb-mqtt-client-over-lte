/*
 * $ Copyright Cypress Semiconductor $
*/

#pragma once

#include "cy_log.h"
#include "cy_result.h"
#include "cy_nw_helper.h"
#include "cy_network_mw_core.h"

#ifdef __cplusplus
extern "C" {
#endif
/**********************************************************/
/* ppp_print function                                     */
/**********************************************************/
#define ppp_print(x)  printf x

/* ppp logs */
#ifdef ENABLE_PPP_MIDDLEWARE_LOGS
#define ppp_cy_log_msg cy_log_msg
#else
#define ppp_cy_log_msg(a,b,c,...)
#endif

typedef enum
{
    CY_PPP_IPV6_LINK_LOCAL = 0,  /**< Denotes IPv6 link-local address type. */
    CY_PPP_IPV6_GLOBAL           /**< Denotes IPv6 global address type. */
} cy_ppp_ipv6_type_t;

/**********************************************************/
/* PPP error code defines                                 */
/**********************************************************/
#define CY_PPP_ERR_NONE                1
#define CY_PPP_ERR_PARAM               2
#define CY_PPP_ERR_OPEN                3
#define CY_PPP_ERR_DEVICE              4
#define CY_PPP_ERR_ALLOC               5
#define CY_PPP_ERR_USER                6
#define CY_PPP_ERR_CONNECT             7
#define CY_PPP_ERR_AUTHFAIL            8
#define CY_PPP_ERR_PROTOCOL            9
#define CY_PPP_ERR_PEERDEAD            10
#define CY_PPP_ERR_IDLETIMEOUT         11
#define CY_PPP_ERR_CONNECTTIME         12
#define CY_PPP_ERR_LOOPBACK            13
#define CY_PPP_ERR_GENERIC             14
#define CY_PPP_GENERAL_ERROR           15
#define CY_PPP_MUTEX_ERROR             16
#define CY_PPP_WAIT_TIMEOUT            17

/**********************************************************/
/* PPP Event defines                                      */
/**********************************************************/
#define CY_PPP_EVENT_START               1
#define CY_PPP_EVENT_OK                  2
#define CY_PPP_EVENT_BAND                3
#define CY_PPP_EVENT_CEREG               4
#define CY_PPP_EVENT_MODEM_PPP_ENABLE    5
#define CY_PPP_EVENT_CONNECT             6
#define CY_PPP_EVENT_UART_INT            7



/* Status callback function pointer */
typedef void (*cy_ppp_link_status_callback_t)( void *err_code );

/**********************************************************/
/* PPP connection manager API declarations                */
/**********************************************************/
/**
 * Initialize the modem in PPP mode.
 *
 * @return CY_RSLT_SUCCESS if successful; failure code otherwise.
 */
cy_rslt_t cy_ppp_init();

/**
 * Initialize network stack and binds the network stack with the modem.
 *
 * @param[in]  status_cb    : Status callback function of type \ref cy_ppp_link_status_callback_t.
 *                            Can be passed as NULL if the user does not want to implement the status callback function.
 *
 * @return CY_RSLT_SUCCESS if successful; failure code otherwise.
 */
cy_rslt_t cy_ppp_start(cy_ppp_link_status_callback_t status_cb);

/**
 * Deinitializes the network stack and terminates link between network stack and the modem.
 * Note: After calling cy_ppp_stop(), the modem will go back to command mode. Hence to re-establish the PPP connection,
 * user should call cy_ppp_init() again.
 *
 * @return CY_RSLT_SUCCESS if successful; failure code otherwise.
 */
cy_rslt_t cy_ppp_stop();

/**
 * Deinitializes the modem.
 * Note: This should always be called after cy_ppp_stop().
 *
 * @return CY_RSLT_SUCCESS if successful; failure code otherwise.
 */
cy_rslt_t cy_ppp_deinit();

/**
 * Retrieves the IPv4 address of the PPP interface.
 *
 * @param[in/out]  ip_addr        : Pointer to an IP address structure.
 *
 * @return CY_RSLT_SUCCESS if IP-address get is successful; failure code otherwise.
 */
cy_rslt_t cy_ppp_get_ip(cy_nw_ip_address_t *ip_addr);

/**
 * Retrieves the IPv6 address of the PPP interface.
 * Note: This API supports only CY_PPP_IPV6_LINK_LOCAL address type.
 *
 * @param[in/out]  ip_addr        : Pointer to an IP address structure.
 * @param[in]      type           : IPv6 address type.
 *
 * @return CY_RSLT_SUCCESS if IP-address get is successful; failure code otherwise.
 */
cy_rslt_t cy_ppp_get_ipv6_address(cy_nw_ip_address_t *ip_addr, cy_ppp_ipv6_type_t type);

/**
 * Sends a ping request to the given IP address. This function is a blocking call; it returns after the specified timeout.
 *
 * @param[in]  address     : Pointer to the destination IP address structure to which the ping request will be sent.
 * @param[in]  timeout_ms  : Ping request timeout in milliseconds.
 * @param[out] elapsed_ms  : Pointer to store the round-trip time (in milliseconds),
 *                           i.e., the time taken to receive the ping response from the destination.
 *
 * @return CY_RSLT_SUCCESS if pinging to the IP address was successful; failure code otherwise.
 */
cy_rslt_t cy_ppp_ping(cy_nw_ip_address_t *address, uint32_t timeout_ms, uint32_t* elapsed_time_ms);

#ifdef __cplusplus
}
#endif
