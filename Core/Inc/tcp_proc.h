/*
 * tcp_proc.h
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#ifndef INC_TCP_PROC_H_
#define INC_TCP_PROC_H_

#include "http_proc.h"
#include "console_uart.h"

#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/sys.h"

#include "lwip/tcp.h"

#include "lwip/apps/fs.h"

#include "queue.h"
#include <string.h>
#include <stdlib.h>

#define TCP_CONNECTION_MAX 		4



typedef struct {
	struct netconn 	*conn;
	char 			number;
} TcpConnStruct_t;



osThreadId_t StartTcpServer (void );
osThreadId_t StartTcpClient (void );









#endif /* INC_TCP_PROC_H_ */
