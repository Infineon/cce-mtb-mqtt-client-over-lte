/*
 * $ Copyright Cypress Semiconductor $
 */

/** @file
 *  Implementation of logging function for printing PKCS11 messages
 */

/* Standard includes. */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cy_log.h"
#include "cy_pkcs_psa_logging.h"

/*-----------------------------------------------------------*/
void vLoggingPrintf( const char * pcFormat, ... )
{
    va_list args;

    va_start( args, pcFormat );

    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, pcFormat, args);

    va_end( args );
}
