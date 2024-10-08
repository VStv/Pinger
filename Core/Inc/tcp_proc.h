/*
 * tcp_proc.h
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#ifndef INC_TCP_PROC_H_
#define INC_TCP_PROC_H_

//#include "smtp_proc.h"
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


#define SMTP_SERVER_PORT		25
#define HTTP_SERVER_PORT		80


#define SMTP_SERVER_ADDR0		192
#define SMTP_SERVER_ADDR1		168
#define SMTP_SERVER_ADDR2		10
#define SMTP_SERVER_ADDR3		11




#define SMTP_PROT				2
#define HTTP_PROT				3


typedef	void (*app_func) (void *);

typedef struct {
	struct netconn 	*conn;
	char 			number;
} conn_struct_t;


typedef struct {
	ip_addr_t		ip;
	uint16_t		port;
	app_func		application;
	osThreadId_t	*app_id;
	osSemaphoreId_t	*sem_app_cplt;
} net_struct_t;


osThreadId_t StartTcpServer (void *);
osThreadId_t StartTcpClient (void *);









#endif /* INC_TCP_PROC_H_ */
