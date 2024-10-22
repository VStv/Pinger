/*
 * tcp_proc.h
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#ifndef INC_TCP_PROC_H_
#define INC_TCP_PROC_H_

#include "smtp_proc.h"
#include "http_proc.h"

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

void RunAppServer (uint32_t);
void RunAppClient (uint32_t);







#endif /* INC_TCP_PROC_H_ */
