/*
 * tcp_proc.c
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#include "tcp_proc.h"

osThreadId_t 		TcpServerTaskHandle = NULL;
osThreadId_t 		TcpConnHandle[TCP_CONNECTION_MAX] = {NULL};
osThreadId_t 		TcpClientTaskHandle = NULL;

conn_struct_t 		TcpConnStruct[TCP_CONNECTION_MAX];

net_struct_t		TcpServerStruct;
net_struct_t		TcpClientStruct;

osSemaphoreId_t 	sid_Connected = NULL;

#ifdef DEBUG_TCP_PROC
uint32_t time1, wait_time;
#endif



extern char 				*pp;

//->->->->->->->->->->->->->->->->->->->->->->->->
extern osMessageQueueId_t 	mid_PingData;
//->->->->->->->->->->->->->->->->->->->->->->->->


//---------------------------------------------------------------------------------------
static void TcpConn_thread 	(
							void *arg
							)
{
	conn_struct_t *pTcpConn = (conn_struct_t *)arg;
	struct netbuf *buf = NULL;
	data_struct_t RW_data = {NULL};
	uint16_t len;

#ifdef DEBUG_TCP_PROC
	PRINTF ("TcpConnThread: accepted new connection %c\r\n", pTcpConn->number);
#endif



	netconn_set_recvtimeout (pTcpConn->conn, 1000);
	// receive the data from the client
	if (netconn_recv (pTcpConn->conn, &buf) == ERR_OK)
	{
		netbuf_data (buf, (void**)&RW_data.r_data, &len);
		TcpServerStruct.application ((void *)&RW_data);
		// send the message back to the client
		if (netconn_write (pTcpConn->conn, (const unsigned char*)RW_data.w_data, (size_t)strlen (RW_data.w_data), NETCONN_COPY) != ERR_OK)
		{
#ifdef DEBUG_TCP_PROC
			PRINTF ("TcpConnThread: Write error\r\n\n");
#endif
		}
	}
	else
	{
#ifdef DEBUG_TCP_PROC
		PRINTF ("TcpConnThread: Receive error\r\n\n");
#endif
	}
	if (RW_data.w_data != NULL)
	{
		vPortFree (RW_data.w_data);
	}



//	if (buf != NULL)
//	{
		netbuf_delete (buf);
//	}
	netconn_close (pTcpConn->conn);
	netconn_delete (pTcpConn->conn);
	pTcpConn->conn = NULL;
#ifdef DEBUG_TCP_PROC
	PRINTF ("TcpConnThread: Connection %c closed\r\n\n", pTcpConn->number);
#endif
	osThreadExit ();
}


static void TcpServer_thread 	(
								void *arg
								)
{
	net_struct_t *pTcpServer = (net_struct_t *)arg;
	struct netconn *conn, *newconn;
	err_t err2;
	conn_struct_t *pTcpConn;// = TcpConnStruct;

	sid_Connected = osSemaphoreNew (1, 0, NULL);
	// Create a new connection identifier
    conn = netconn_new (NETCONN_TCP);
	if (conn == NULL)
	{
#ifdef DEBUG_TCP_PROC
		PRINTF("TcpServerThread: Can't create TCP connection\r\n");
#endif
		goto exit1;
	}
	// Bind connection to the port 80
	err2 = netconn_bind (conn, IP_ADDR_ANY, pTcpServer->port);
	if (err2 != ERR_OK)
	{
		err2 = netconn_delete (conn);
		conn = NULL;
#ifdef DEBUG_TCP_PROC
		PRINTF("TcpServerThread: Can't bind TCP connection %p\r\n", conn);
#endif
		goto exit1;
	}
	// Tell connection to go into listening mode
	netconn_listen (conn);
	// The while loop will run every time this Task
	while (1)
	{
		// Grab & Process new connection
		if (netconn_accept(conn, &newconn) == ERR_OK)
		{
			pTcpConn = TcpConnStruct;
			for (uint8_t i = 0; i < TCP_CONNECTION_MAX; i++)
			{
				if (pTcpConn->conn == NULL)
				{
#ifdef DEBUG_TCP_PROC
					PRINTF("TcpServerThread: Connected with remote host: %d.%d.%d.%d: %d\r\n", (uint8_t)(newconn->pcb.tcp->remote_ip.addr), (uint8_t)((newconn->pcb.tcp->remote_ip.addr)>>8), (uint8_t)((newconn->pcb.tcp->remote_ip.addr)>>16), (uint8_t)((newconn->pcb.tcp->remote_ip.addr)>>24), newconn->pcb.tcp->remote_port);
					PRINTF("TcpServerThread: Connection time %ld\r\n", sys_now());
#endif
					pTcpConn->conn = newconn;
					char nameThread[] = {'T','C','P','C','o','n','n','T','a','s','k', pTcpConn->number,'\0'};
					const osThreadAttr_t tcpConn_attributes = {
						.name = nameThread,
						.priority = (osPriority_t) osPriorityNormal,
						.stack_size = 512 * 4,
					};
					TcpConnHandle[i] = osThreadNew (TcpConn_thread, (void *)pTcpConn, &tcpConn_attributes);
					newconn = NULL;
					break;
				}
				pTcpConn++;
			}
			if (newconn != NULL)
			{
#ifdef DEBUG_TCP_PROC
				PRINTF("TcpServerThread: No free conn-structures\r\n");
#endif
				netconn_close(newconn);
				netconn_delete(newconn);
			}
		}
		else
		{
			netconn_delete(newconn);
		}
    }
exit1:
	for(;;);
}


osThreadId_t StartTcpServer (
							void *arg
							)
{
	uint32_t app = *(uint32_t *)arg;
	net_struct_t *pTcpServer = &TcpServerStruct;

//->->->->->->->->->->->->->->->->->->->->->->->->
	mid_PingData = osMessageQueueNew (1, sizeof(ping_struc_t), NULL);
	vQueueAddToRegistry (mid_PingData, "mid_PingData");
//->->->->->->->->->->->->->->->->->->->->->->->->

	switch (app)
	{
		case HTTP_PROT:
			// Set local port
			pTcpServer->port = HTTP_SERVER_PORT;
			pTcpServer->application = HttpProcess;
			break;
		default:
			return NULL;
	}
	conn_struct_t *pTcpConn = TcpConnStruct;
	for (uint8_t i = 0; i < TCP_CONNECTION_MAX; i++)
	{
		pTcpConn->number = '1'+i;
		pTcpConn->conn = NULL;
		pTcpConn++;
	}
    const osThreadAttr_t tcpTask_attributes = {
        .name = "TcpServerTask",
        .stack_size = 512 * 3,
        .priority = (osPriority_t) osPriorityNormal,
    	};
    return osThreadNew(TcpServer_thread, (void *)pTcpServer, &tcpTask_attributes);
}


void RunAppServer 	(
					uint32_t app
					)
{
	TcpServerTaskHandle = StartTcpServer ((void *)&app);
}


// ---------------------------------------------------------------------------------------
void my_callback	(
					struct netconn *conn,
					enum netconn_evt evt, u16_t len
					)
{
	(void) len;

#ifdef DEBUG_TCP_PROC
	wait_time = (sys_now() - time1 == 0) ? 1: sys_now() - time1;
	PRINTF ("my_callback: Wait for connection %ld ms\r\n", wait_time);
#endif
	switch (evt)
	{
		case NETCONN_EVT_SENDPLUS:
			if (conn->pcb.tcp->state != ESTABLISHED)
				break;
			if (sid_Connected != NULL)
			{
#ifdef DEBUG_TCP_PROC
				wait_time = (sys_now() - time1 == 0) ? 1: sys_now() - time1;
				PRINTF ("my_callback: Connection is Ok at %ld ms\r\n", wait_time);
#endif
				osSemaphoreRelease(sid_Connected);
			}
			break;
		default:
#ifdef DEBUG_TCP_PROC
			PRINTF ("my_callback: No connection, evt %d\r\n", evt);
#endif
			break;
	}
}

/*
static void TcpClient2_thread	(
								void *arg
								)
{
	net_struct_t *pTcpClient = (net_struct_t *)arg;
	struct netconn *conn = NULL;
	osStatus_t 	val;
	err_t 		err;
	uint16_t 	src_port;
	osSemaphoreId_t	sid_AppCplt = NULL;

	sid_Connected = osSemaphoreNew (1, 0, NULL);
	vQueueAddToRegistry (sid_Connected, "sid_Connected");
	// Create a new connection identifier
	conn = netconn_new_with_callback (NETCONN_TCP, my_callback);
	if (conn == NULL)
	{
#ifdef DEBUG_TCP_PROC
		PRINTF ("TcpClientThread: Can't create connection");
#endif
		goto exit2;
	}
	// Bind connection to random local port
	src_port = (uint16_t) SysTick->VAL;
	err = netconn_bind (conn, IP_ADDR_ANY, src_port);
	if (err != ERR_OK)
	{
#ifdef DEBUG_TCP_PROC
		PRINTF ("TcpClientThread: Can't bind TCP connection, err = %d", err);
#endif
		err = netconn_delete (conn);
		goto exit2;
	}
#ifdef DEBUG_TCP_PROC
	PRINTF ("TcpClientThread: Try to connect to port %d\r\n", pTcpClient->port);
	time1 = sys_now ();
#endif
	// connect to remote server at port
	netconn_set_nonblocking (conn, 1);
	err = netconn_connect (conn, &pTcpClient->ip, pTcpClient->port);
	val = osSemaphoreAcquire (sid_Connected, 1000U);
	if (val != osOK)
	{
#ifdef DEBUG_TCP_PROC
		PRINTF ("TcpClientThread: Connection dropped\r\n");
#endif
		netconn_set_nonblocking (conn, 0);
		conn->callback = NULL;
		err = netconn_delete (conn);
		conn = NULL;
		goto exit2;
	}
#ifdef DEBUG_TCP_PROC
	PRINTF ("TcpClientThread: Connected to server\r\n");
#endif
	netconn_set_nonblocking (conn, 0);
	conn->callback = NULL;

	// Create complete-semaphore & start client application
	sid_AppCplt = osSemaphoreNew (1, 0, NULL);
	vQueueAddToRegistry (sid_AppCplt, "sid_AppCplt");
	pTcpClient->sem_app_cplt = &sid_AppCplt;

	pTcpClient->application ((void *)pTcpClient->sem_app_cplt);
	osSemaphoreAcquire (sid_AppCplt, osWaitForever);

	// delete client application and semaphores
	osSemaphoreDelete (sid_AppCplt);
	sid_AppCplt = NULL;

	err = netconn_close (conn);
	err = netconn_delete (conn);
	conn = NULL;

exit2:
#ifdef DEBUG_TCP_PROC
	PRINTF ("TcpClientThread: Connection closed\r\n");
#endif
	if (sid_Connected)
	{
		osSemaphoreDelete (sid_Connected);
		sid_Connected = NULL;
	}
	TcpClientTaskHandle = NULL;
	osThreadExit ();
}
*/


static void TcpClient_thread	(
								void *arg
								)
{
	net_struct_t *pTcpClient = (net_struct_t *)arg;
	struct netconn *conn = NULL;
	struct netbuf *buf = NULL;
	uint16_t len;
	osStatus_t 	val;
	err_t 		err;
	uint16_t 	src_port;
	data_struct_t RW_data = {NULL};

	sid_Connected = osSemaphoreNew (1, 0, NULL);
	vQueueAddToRegistry (sid_Connected, "sid_Connected");
	// Create a new connection identifier
	conn = netconn_new_with_callback (NETCONN_TCP, my_callback);
	if (conn == NULL)
	{
#ifdef DEBUG_TCP_PROC
		PRINTF ("TcpClientThread: Can't create connection");
#endif
		goto exit2;
	}
	// Bind connection to random local port
	src_port = (uint16_t) SysTick->VAL;
	err = netconn_bind (conn, IP_ADDR_ANY, src_port);
	if (err != ERR_OK)
	{
#ifdef DEBUG_TCP_PROC
		PRINTF ("TcpClientThread: Can't bind TCP connection, err = %d", err);
#endif
		err = netconn_delete (conn);
		goto exit2;
	}
#ifdef DEBUG_TCP_PROC
	PRINTF ("TcpClientThread: Try to connect to port %d\r\n", pTcpClient->port);
	time1 = sys_now ();
#endif
	// connect to remote server at port
	netconn_set_nonblocking (conn, 1);
	err = netconn_connect (conn, &pTcpClient->ip, pTcpClient->port);
	val = osSemaphoreAcquire (sid_Connected, 1000U);
	if (val != osOK)
	{
#ifdef DEBUG_TCP_PROC
		PRINTF ("TcpClientThread: Connection dropped\r\n");
#endif
		netconn_set_nonblocking (conn, 0);
		conn->callback = NULL;
		err = netconn_delete (conn);
		conn = NULL;
		goto exit2;
	}
#ifdef DEBUG_TCP_PROC
	PRINTF ("TcpClientThread: Connected to server\r\n");
#endif
	netconn_set_nonblocking (conn, 0);
	conn->callback = NULL;


	netconn_set_recvtimeout (conn, 5000);
	while (1)
	{
		// receive the data from the client
		if (netconn_recv (conn, &buf) == ERR_OK)
		{
			netbuf_data (buf, (void**)&RW_data.r_data, &len);
			pTcpClient->application ((void *)&RW_data);

			if (RW_data.w_data == NULL)
				break;

			// send the message back to the client
			if (netconn_write (conn, (const unsigned char*)RW_data.w_data, (size_t)strlen (RW_data.w_data), NETCONN_COPY) != ERR_OK)
			{
#ifdef DEBUG_TCP_PROC
				PRINTF ("TcpClientThread: Write error\r\n\n");
#endif
				break;
			}
			vPortFree (RW_data.w_data);
			netbuf_delete(buf);
		}
		else
		{
#ifdef DEBUG_TCP_PROC
			PRINTF ("TcpClientThread: Receive error\r\n\n");
#endif
			break;
		}
	}
	vPortFree (RW_data.w_data);
	netbuf_delete (buf);
	err = netconn_close (conn);
	err = netconn_delete (conn);
	conn = NULL;

exit2:
#ifdef DEBUG_TCP_PROC
	PRINTF ("TcpClientThread: Connection closed\r\n");
#endif
	if (sid_Connected)
	{
		osSemaphoreDelete (sid_Connected);
		sid_Connected = NULL;
	}
	TcpClientTaskHandle = NULL;
	osThreadExit ();
}


osThreadId_t StartTcpClient (
							void *arg
							)
{
	uint32_t app = *(uint32_t *)arg;
	net_struct_t *pTcpClient = &TcpClientStruct;

	switch (app)
	{
		case SMTP_PROT:
			// Set remote IP-address & port
			ip4addr_aton (SMTP_SERVER_ADDR, &pTcpClient->ip);
			pTcpClient->port = SMTP_SERVER_PORT;
			pTcpClient->application = SmtpProcess;//StartSmtpClient;//
			break;

		default:
			return NULL;
	}
	const osThreadAttr_t tcpTask_attributes = {
        .name = "TcpClientTask",
        .stack_size = 3*512,
        .priority = (osPriority_t) osPriorityNormal,
    };
    return osThreadNew (TcpClient_thread, (void *)pTcpClient, &tcpTask_attributes);
}


void RunAppClient 	(
					uint32_t app
					)
{
	TcpClientTaskHandle = StartTcpClient ((void *)&app);
}

