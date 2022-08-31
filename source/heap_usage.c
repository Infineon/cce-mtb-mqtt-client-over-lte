/******************************************************************************
* File Name:   heap_usage.c
*
* Description: This file contains the code for printing heap usage.
*              Supports only GCC_ARM compiler. Define PRINT_HEAP_USAGE for
*              printing the heap usage numbers.
*
* Related Document: See README.md
*
*
*******************************************************************************
* $ Copyright 2021-YEAR Cypress Semiconductor $
*******************************************************************************/

/*******************************************************************************
 * Header file includes
 ******************************************************************************/
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

/* ARM compiler also defines __GNUC__ */
#if defined (__GNUC__) && !defined(__ARMCC_VERSION)
#include <malloc.h>
#endif /* #if defined (__GNUC__) && !defined(__ARMCC_VERSION) */


/*******************************************************************************
 * Macros
 ******************************************************************************/
#define TO_KB(size_bytes)  ((float)(size_bytes)/1024)


/*******************************************************************************
 * Function Definitions
 ******************************************************************************/

/*******************************************************************************
* Function Name: print_heap_usage
********************************************************************************
* Summary:
* Prints the available heap and utilized heap by using mallinfo().
*
*******************************************************************************/
void print_heap_usage(char *msg)
{
    /* ARM compiler also defines __GNUC__ */
#if defined(PRINT_HEAP_USAGE) && defined (__GNUC__) && !defined(__ARMCC_VERSION)
    struct mallinfo mall_info = mallinfo();

    extern uint8_t __HeapBase;  /* Symbol exported by the linker. */
    extern uint8_t __HeapLimit; /* Symbol exported by the linker. */

    uint8_t* heap_base = (uint8_t *)&__HeapBase;
    uint8_t* heap_limit = (uint8_t *)&__HeapLimit;
    uint32_t heap_size = (uint32_t)(heap_limit - heap_base);

    printf("\r\n\n********** Heap Usage **********\r\n");
    printf(msg);
    printf("\r\nTotal available heap        : %"PRIu32" bytes/%.2f KB\r\n", heap_size, TO_KB(heap_size));

    printf("Maximum heap utilized so far: %u bytes/%.2f KB, %.2f%% of available heap\r\n",
            mall_info.arena, TO_KB(mall_info.arena), ((float) mall_info.arena * 100u)/heap_size);

    printf("Heap in use at this point   : %u bytes/%.2f KB, %.2f%% of available heap\r\n",
            mall_info.uordblks, TO_KB(mall_info.uordblks), ((float) mall_info.uordblks * 100u)/heap_size);

    printf("********************************\r\n\n");
#endif /* #if defined(PRINT_HEAP_USAGE) && defined (__GNUC__) && !defined(__ARMCC_VERSION) */
}

/* [] END OF FILE */
