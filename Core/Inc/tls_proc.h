/*
 * tls_proc.h
 *
 *  Created on: Nov 16, 2024
 *      Author: dis_stv
 */

#ifndef INC_TLS_PROC_H_
#define INC_TLS_PROC_H_

#include "main.h"
#include "cmsis_os.h"

#include "queue.h"
#include <string.h>
#include <stdlib.h>

#include "tcp_proc.h"
#include "http_proc.h"
#include "console_uart.h"

#include "tls_dat.h"


#define DEBUG_TLS_PROC

//#define TLS_PROC 1
//#define TLS_PROC 2
#define TLS_PROC 3



#if TLS_PROC != 1
#define TLS_CONTEXT_MAX 		3
#endif



typedef struct context_struct {
	mbedtls_net_context *sock;
	mbedtls_ssl_context	*ssl;
	char 				number;
} context_struct_t;


#if TLS_PROC == 3
typedef struct {
    struct netconn *conn;
    struct netbuf *rx_buf;
    uint8_t *rx_ptr;
    u16_t rx_len;
} client_args_t;
#endif


void RunAppTlsServer (uint32_t);


#endif /* INC_TLS_PROC_H_ */
