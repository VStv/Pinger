/*
 * tcp_proc.h
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#ifndef INC_TCP_PROC_H_
#define INC_TCP_PROC_H_

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

#include "smtp_proc.h"
#include "http_proc.h"

#include "console_uart.h"


#define DEBUG_TCP_PROC
#define TCP_CONNECTION_MAX 		4



typedef	void (*app_func) (void *);
typedef	void (*tcp_task_t) (void *);


typedef struct conn_struct {
	struct netconn 	*conn;
	uint32_t		number;
} conn_struct_t;


typedef struct net_struct {
	ip_addr_t		ip;
	uint16_t		port;
	app_func		application;
	osThreadId_t	*app_id;
	osSemaphoreId_t	*sem_app_cplt;
} net_struct_t;


void RunAppTcpServer (uint16_t);

void RunAppClient (uint32_t);
osThreadId_t StartTcpClient (void *);









#endif /* INC_TCP_PROC_H_ */
