/*
 * $ Copyright Cypress Semiconductor $
*/

#ifndef LIBS_CONNECTIVITY_MANAGER_INTERNAL_HEADER_H_
#define LIBS_CONNECTIVITY_MANAGER_INTERNAL_HEADER_H_

#if defined(CYBSP_ETHERNET_CAPABLE)
#ifdef COMPONENT_CAT1
#include "cy_ethif.h"
#include "cy_ephy.h"

/******************************************************
 *                      Macros
 ******************************************************/
/**
 * Maximum number of interface instances supported.
 */
#define CY_IFACE_MAX_HANDLE   (4U)
#define NO_OF_BUFFERS         (10)

/******************************************************
 *                      Type definitions
 ******************************************************/
/** Buffer created from the pool */
typedef void* cy_buffer_t;

typedef struct cy_rx_buffer_info
{
    uint8_t *rx_data_ptr; /* Pointer to the actual buffer created */
    uint32_t eth_idx;
    uint32_t length;
} cy_rx_buffer_info_t;

typedef struct list_node_t
{
    void *pool_handle; /* Stores the pool handle */
    struct list_node_t *next; /* Points to the next buffer in the list */
    uint8_t *buffer_ptr; /* Pointer to the actual buffer created */
} list_node_t;

typedef struct
{
    uint16_t total_num_buf_created; /* Total number of buffers created */
    uint16_t sizeof_buffer; /* Size of each created buffer */
    uint8_t *data_buffer; /* Base address of the data buffer */
    list_node_t *head; /* Pointer to the head node in the buffer list */
} cy_internal_buffer_pool_t;

#endif
#endif

#endif /* LIBS_CONNECTIVITY_MANAGER_INTERNAL_HEADER_H_ */
