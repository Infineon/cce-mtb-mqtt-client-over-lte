/*
 * $ Copyright Cypress Semiconductor $
 */

/** @file
 *  Function for initializing threading functions for cryptography libraries
 */

#ifndef INCLUDED_CY_PKCS_CRYPTO_H_
#define INCLUDED_CY_PKCS_CRYPTO_H_

#ifdef CY_SECURE_SOCKETS_PKCS_SUPPORT
#include "cyabs_rtos.h"

/**
 * Initializes threading functions for cryptography libraries
 */
void CRYPTO_Init( void );

#endif
#endif /* ifndef INCLUDED_CY_PKCS_CRYPTO_H_ */
