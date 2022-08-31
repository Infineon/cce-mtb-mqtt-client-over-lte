/*
 * $ Copyright Cypress Semiconductor $
 */

#ifdef CY_SECURE_SOCKETS_PKCS_SUPPORT
#include "iot_crypto.h"

/* CRYPTO_Init initializes mutex when MBEDTLS context is shared in multiple threads and MBEDTLS_THREADING_C flag is enabled.
 * but our implementation doesnt use MBEDTLS context in multiple threads and MBEDTLS_THREADING_C is not enabled. Hence this
 * is not implemented
 */
void CRYPTO_Init( void )
{

}
#endif
