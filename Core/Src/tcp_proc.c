/*
 * tcp_proc.c
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#include "tcp_proc.h"


#if defined(DEBUG_TCP_SERV) || defined(DEBUG_TCP_CLI)
uint32_t 			time1, wait_time;
extern char 		*pp;
#endif


// ---------------------------------  TCP Server  ----------------------------------------

osThreadId_t 		TcpServerTaskHandle[2] = {NULL};
osThreadId_t 		TcpServerConnTaskHandle[TCP_CONNECTION_MAX] = {NULL};
osSemaphoreId_t 	sid_TcpConnCount = NULL;

extern void TlsServerContext_thread (void *);
//extern uint32_t TlsConfig (void);

static void TcpServerConn_thread (
							void *arg
							)
{
	conn_struct_t *pTcpConn = (conn_struct_t *)arg;
	struct netbuf *buf = NULL;
	data_struct_t RW_data = {NULL};
	uint16_t len;

#ifdef DEBUG_TCP_SERV
    const char *tag = "TcpServerConnThread";
//	PRINTF("%s %ld: Connection %p in conn_struct_t %p with remote host: %s:%d\r\n",
//			tag,
//			pTcpConn->number,
//			pTcpConn->conn,
//			pTcpConn,
//			ipaddr_ntoa (pTcpConn->conn->pcb.tcp->remote_ip),
//			pTcpConn->conn->pcb.tcp->remote_port);
	PRINTF("%s %ld: Connection %p in conn_struct_t %p with remote host: %d.%d.%d.%d: %d\r\n",
			tag,
			pTcpConn->number,
			pTcpConn->conn,
			pTcpConn,
			(u8_t)(pTcpConn->conn->pcb.tcp->remote_ip.addr),
			(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>8),
			(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>16),
			(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>24),
			(u16_t)(pTcpConn->conn->pcb.tcp->remote_port));
#endif

	netconn_set_recvtimeout (pTcpConn->conn, 1000);
	// receive the data from the client
	if (netconn_recv (pTcpConn->conn, &buf) == ERR_OK)
	{
		netbuf_data (buf, (void**)&RW_data.r_data, &len);
		HttpServer ((void *)&RW_data);
		// send the message back to the client
		if (netconn_write (pTcpConn->conn, (const unsigned char*)RW_data.w_data, (size_t)strlen (RW_data.w_data), NETCONN_COPY) != ERR_OK)
		{
#ifdef DEBUG_TCP_SERV
			PRINTF ("%s %ld: Write error\r\n\n", tag, pTcpConn->number);
#endif
		}
	}
	else
	{
#ifdef DEBUG_TCP_SERV
		PRINTF ("%s %ld: Receive error\r\n\n", tag, pTcpConn->number);
#endif
	}
	if (RW_data.w_data != NULL)
	{
		vPortFree (RW_data.w_data);
	}

	netbuf_delete (buf);
#ifdef DEBUG_TCP_SERV
	PRINTF ("%s %ld: Connection %p to port %d is closing\r\n\n",
			tag,
			pTcpConn->number,
			pTcpConn->conn,
			(u16_t)(pTcpConn->conn->pcb.tcp->remote_port));
#endif
	netconn_close (pTcpConn->conn);
	netconn_delete (pTcpConn->conn);
	pTcpConn->conn = NULL;
	vPortFree (pTcpConn);
	osSemaphoreRelease (sid_TcpConnCount);
	osThreadExit ();
}


static void TcpServer_thread 	(
								void *arg_port
								)
{
	uint32_t task_number, stack_sz;
    struct netconn *conn, *newconn;
    tcp_task_t	task_handler = NULL;
    err_t err2;
	uint16_t *pPort = (uint16_t *)arg_port;
	char *nameThread;

#ifdef DEBUG_TCP_SERV
	const char *tag_tcp = "TcpServerThread";
	const char *tag_tls = "TlsServerThread";
	char *tag;
	if (*pPort == HTTP_SERVER_PORT)
    	tag = (char *)tag_tcp;
    else
    	tag = (char *)tag_tls;
#endif

//-------------------------------------------------
//	uint32_t tls_conf;
//	if (*pPort == HTTPS_SERVER_PORT && !tls_conf)
//		tls_conf = TlsConfig();
//-------------------------------------------------

    conn = netconn_new (NETCONN_TCP);
	if (conn == NULL)
	{
#ifdef DEBUG_TCP_SERV
		PRINTF("%s: Can't create TCP connection\r\n", tag);
		goto trap;
#endif
		return;
	}
	// Bind connection to the port 80(433)
	err2 = netconn_bind (conn, IP_ADDR_ANY, *pPort);
	if (err2 != ERR_OK)
	{
		err2 = netconn_delete (conn);
		conn = NULL;
#ifdef DEBUG_TCP_SERV
		PRINTF("%s: Can't bind TCP connection %p\r\n", tag, conn);
		goto trap;
#endif
		return;
	}
#ifdef DEBUG_TCP_SERV
	PRINTF("%s: Bind on https://localhost: %d\r\n", tag, *pPort);
#endif
	netconn_listen (conn);

    while (1)
    {
    	if (netconn_accept (conn, &newconn) == ERR_OK)
        {
    		if (osSemaphoreAcquire (sid_TcpConnCount, 0) == osOK)	// take semaphore
    		{
				task_number = TCP_CONNECTION_MAX - osSemaphoreGetCount (sid_TcpConnCount) - 1;
				conn_struct_t *pTcpConn = pvPortMalloc(sizeof (conn_struct_t ));
				if (pTcpConn == NULL) goto cleanup;
				pTcpConn->conn = newconn;
				pTcpConn->number = task_number;
#ifdef DEBUG_TCP_SERV
				PRINTF("%s: Connection %p number %ld in conn_struct_t %p with remote host: %d.%d.%d.%d: %d\r\n",
						tag,
						pTcpConn->conn,
						pTcpConn->number,
						pTcpConn,
						(u8_t)(pTcpConn->conn->pcb.tcp->remote_ip.addr),
						(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>8),
						(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>16),
						(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>24),
						(u16_t)(pTcpConn->conn->pcb.tcp->remote_port));
#endif
				char tlsNameThread[] = {'T','l','s','S','e','r','v','e','r','C','o','n','t','T','a','s','k', (char)(pTcpConn->number + 0x30),'\0'};
				char tcpNameThread[] = {'T','c','p','S','e','r','v','e','r','C','o','n','n','T','a','s','k', (char)(pTcpConn->number + 0x30),'\0'};
				switch (*pPort)
				{
					case HTTP_PORT:
						nameThread = tcpNameThread;
						stack_sz = 2048;
						task_handler = TcpServerConn_thread;
						break;

					case HTTPS_PORT:
						nameThread = tlsNameThread;
						stack_sz = 10240;
						task_handler = TlsServerContext_thread;
						break;

					default:
#ifdef DEBUG_TCP_SERV
						PRINTF("%s: Unknown local port %d\r\n", tag, *pPort);
#endif
						goto cleanup;
				}
				const osThreadAttr_t tcpConn_attributes = {
						.name = nameThread,
						.priority = (osPriority_t) osPriorityNormal,
						.stack_size = stack_sz,
				};
				TcpServerConnTaskHandle[task_number] = osThreadNew (task_handler, (void *)pTcpConn, &tcpConn_attributes);
				newconn = NULL;
				continue;
    		}
cleanup:
    		netconn_close(newconn);
			netconn_delete(newconn);
        }
    }
#ifdef DEBUG_TCP_SERV
trap:
    for (;;);
#endif
}


void RunAppTcpServer(
					u16_t port
					)
{
	char *nameThread;
	uint32_t serv_num;
	const char *tcp_serv_name = "TcpServerTask";
	const char *tls_serv_name = "TlsServerTask";
#ifdef DEBUG_TCP_SERV
	const char *tag = "RunAppTcpServer";
#endif

	if (sid_TcpConnCount == NULL)
	{
		sid_TcpConnCount = osSemaphoreNew (TCP_CONNECTION_MAX, TCP_CONNECTION_MAX, NULL);
		vQueueAddToRegistry (sid_TcpConnCount, "sid_TcpConnCount");
	}

	switch (port)
	{
		case HTTP_PORT:
			serv_num = 0;
			nameThread = (char *)tcp_serv_name;
			break;
		case HTTPS_PORT:
			nameThread = (char *)tls_serv_name;
			serv_num = 1;
			break;
		default:
#ifdef DEBUG_TCP_SERV
			PRINTF("%s: Unknown app protocol %d\r\n", tag, port);
#endif
			return;
	}
	if (TcpServerTaskHandle[serv_num] != NULL)
	{
#ifdef DEBUG_TCP_SERV
		PRINTF("%s: %s already has run \r\n", tag, nameThread);
#endif
		return;
	}
	uint16_t *port_arg = pvPortMalloc(sizeof (uint16_t));
	if (!port_arg) return;
	*port_arg = port;

#ifdef DEBUG_TCP_SERV
	PRINTF("%s: %s runing\r\n", tag, nameThread);
#endif

    const osThreadAttr_t tcpTask_attributes = {
        .name = nameThread,
        .stack_size = 512 * 4,	//2
        .priority = (osPriority_t) osPriorityNormal,
    };
	TcpServerTaskHandle[serv_num] = osThreadNew(TcpServer_thread, (void *)port_arg, &tcpTask_attributes);
}




// ---------------------------------  TCP Client  ----------------------------------------

osThreadId_t 		TcpClientTaskHandle = NULL;
net_struct_t		TcpClientStruct;
osSemaphoreId_t 	sid_Connected = NULL;

extern void TlsClientContext (void *);


static void TcpClientConn 	(
							void *arg
							)
{
	net_struct_t *pTcpClient = (net_struct_t *)arg;
	struct netbuf *buf = NULL;
	uint16_t len;
	data_struct_t RW_data = {NULL};
//**********************************************
	int cnt=0;
	char wbuf[128];
	RW_data.w_data = wbuf;
//**********************************************
	err_t err;
#ifdef DEBUG_TCP_CLI
	const char *tag = "TcpClientConn";
#endif

	while (1)
	{
//		pTcpClient->application ((void *)&RW_data);
//**********************************************
//		sprintf (RW_data.w_data, "Client send %d\r\n", ++cnt);
		++cnt;
		if (cnt%2 == 1)
			sprintf (RW_data.w_data, "Client send %d\r\n", cnt);
		else
			*(RW_data.w_data) = 0;
//**********************************************

		// send the message back to the server
		err = netconn_write (pTcpClient->conn, (const unsigned char*)RW_data.w_data, (size_t)strlen (RW_data.w_data), NETCONN_COPY);
		if (err != ERR_OK)
		{
#ifdef DEBUG_TCP_CLI
			PRINTF ("%s: Write error=%d\r\n", tag, err);
#endif
			break;
		}
#ifdef DEBUG_TCP_CLI
		PRINTF ("%s: Written %d bytes\r\n", tag, strlen(RW_data.w_data));
#endif
//		vPortFree (RW_data.w_data); //!!!!!!!!!!!!!

		// receive the data from the server
		err = netconn_recv (pTcpClient->conn, &buf);
		if (err == ERR_OK)
		{
			netbuf_data (buf, (void**)&RW_data.r_data, &len);
			*(RW_data.r_data+len)=0;
#ifdef DEBUG_TCP_CLI
			PRINTF ("%s: Received %d bytes from server: %s\r\n", tag, len, RW_data.r_data);
#endif
			netbuf_delete(buf);
		}
		else if (err == ERR_TIMEOUT)
		{
#ifdef DEBUG_TCP_CLI
			PRINTF ("%s: Receive timeout\r\n", tag);
#endif
		}
		else
		{
#ifdef DEBUG_TCP_CLI
			PRINTF ("%s: Receive error=%d\r\n", tag, err);
#endif
//			// smtp_status <- 0
//			pTcpClient->application ((void *)&RW_data);

			netbuf_delete (buf);
			break;
		}
		osDelay (1000);
	}
	return;
}


static void my_callback	(
						struct netconn *conn,
						enum netconn_evt evt, u16_t len
						)
{
	(void) len;
#ifdef DEBUG_TCP_CLI
    const char *tag = "my_callback";
#endif

#ifdef DEBUG_TCP_CLI
	wait_time = (sys_now() - time1 == 0) ? 1: sys_now() - time1;
	PRINTF ("%s: Wait for connection %ld ms\r\n", tag, wait_time);
#endif
	switch (evt)
	{
		case NETCONN_EVT_SENDPLUS:
			if (conn->pcb.tcp->state != ESTABLISHED)
				break;
			if (sid_Connected != NULL)
			{
#ifdef DEBUG_TCP_CLI
				wait_time = (sys_now() - time1 == 0) ? 1: sys_now() - time1;
				PRINTF ("%s: Connection is Ok at %ld ms\r\n", tag, wait_time);
#endif
				osSemaphoreRelease(sid_Connected);
			}
			break;

		default:
#ifdef DEBUG_TCP_CLI
			PRINTF ("%s: No connection, evt %d\r\n", tag, evt);
#endif
			break;
	}
}


static void TcpClient1_thread	(
								void *arg
								)
{
	struct netconn *conn = NULL;
	net_struct_t *pTcpClient = (net_struct_t *)arg;
	osStatus_t 	val;
#ifdef DEBUG_TCP_CLI
	const char *tag = "TcpClient1Thread";
#endif

	sid_Connected = osSemaphoreNew (1, 0, NULL);
	vQueueAddToRegistry (sid_Connected, "sid_Connected");
	// Create a new connection identifier
	conn = netconn_new_with_callback (NETCONN_TCP, my_callback);
	if (conn == NULL)
	{
#ifdef DEBUG_TCP_CLI
		PRINTF ("%s: Can't create connection", tag);
#endif
		goto exit2;
	}
#ifdef DEBUG_TCP_CLI
	PRINTF ("%s: Try to connect to port %d\r\n", tag, pTcpClient->port);
	time1 = sys_now ();
#endif
	// connect to remote server at port
	netconn_set_nonblocking (conn, 1);
	netconn_connect (conn, &pTcpClient->ip, pTcpClient->port);
	val = osSemaphoreAcquire (sid_Connected, 1000U);
	if (val != osOK)
	{
#ifdef DEBUG_TCP_CLI
		PRINTF ("%s: Connection dropped\r\n", tag);
#endif
		netconn_set_nonblocking (conn, 0);
		conn->callback = NULL;
		netconn_delete (conn);
		conn = NULL;
		goto exit2;
	}
#ifdef DEBUG_TCP_CLI
	PRINTF ("%s: Connected to server %s:%d\r\n", tag, ipaddr_ntoa (&pTcpClient->ip), pTcpClient->port);
#endif
	netconn_set_nonblocking (conn, 0);
	conn->callback = NULL;
	netconn_set_recvtimeout (conn, 2000);

	pTcpClient->conn = conn;
	switch (pTcpClient->port)
	{
		case SMTP_PORT:
			TcpClientConn (pTcpClient);
			break;

		case SMTPS_PORT:
			TlsClientContext (pTcpClient);
			break;

		default:
			break;
	}
	netconn_close (pTcpClient->conn);
	netconn_delete (pTcpClient->conn);
	pTcpClient->conn = NULL;

exit2:
#ifdef DEBUG_TCP_CLI
	PRINTF ("%s: Connection closed\r\n", tag);
#endif
	if (sid_Connected)
	{
		osSemaphoreDelete (sid_Connected);
		sid_Connected = NULL;
	}
	TcpClientTaskHandle = NULL;
	osThreadExit ();
}


static void TcpClient_thread	(
								void *arg
								)
{
	net_struct_t *pTcpClient = (net_struct_t *)arg;
	struct netconn *conn = NULL;
#ifdef DEBUG_TCP_CLI
	const char *tag = "TcpClientThread";
#endif

	// Create a new connection identifier
	conn = netconn_new (NETCONN_TCP);
	if (conn == NULL)
	{
#ifdef DEBUG_TCP_CLI
		PRINTF ("%s: Can't create connection\r\n", tag);
#endif
		osThreadExit ();
	}
	netconn_set_recvtimeout (conn, 5000);
	// connect to remote server at port
#ifdef DEBUG_TCP_CLI
	PRINTF ("%s: Try to connect to port %d\r\n", tag, pTcpClient->port);
	time1 = sys_now ();
#endif
	while (netconn_connect (conn, &pTcpClient->ip, pTcpClient->port) != ERR_OK)
	{
#ifdef DEBUG_TCP_CLI
		wait_time = (sys_now() - time1 == 0) ? 1: sys_now() - time1;
		PRINTF ("%s: Connection dropped after %ld ms\r\n", tag, wait_time);
#endif
//		osDelay(1000);
		goto cleanup;
	}
#ifdef DEBUG_TCP_CLI
	PRINTF ("%s: Connected to server %s:%d\r\n", tag, ipaddr_ntoa (&pTcpClient->ip), pTcpClient->port);
#endif

	pTcpClient->conn = conn;
	switch (pTcpClient->port)
	{
		case SMTP_PORT:
			TcpClientConn (pTcpClient);
			break;

		case SMTPS_PORT:
			TlsClientContext (pTcpClient);
			break;

		default:
			break;
	}

cleanup:
	netconn_close (conn);
	netconn_delete (conn);
#ifdef DEBUG_TCP_CLI
	PRINTF ("%s: Connection closed\r\n", tag);
#endif
	osThreadExit ();
}


void RunAppTcpClient(
					uint16_t port
					)
{
	net_struct_t *pTcpClient = &TcpClientStruct;

	switch (port)
	{
		case SMTP_PORT:
			// Set remote IP-address & port
			ip4addr_aton (SMTP_SERVER_ADDR, &pTcpClient->ip);
			pTcpClient->port = SMTP_PORT;
			pTcpClient->application = SmtpClient;
			break;

		case SMTPS_PORT:
			// Set remote IP-address & port
			ip4addr_aton (SMTPS_SERVER_ADDR, &pTcpClient->ip);
			pTcpClient->port = SMTPS_PORT;
			pTcpClient->application = SmtpClient;
			break;

		default:
			return;
	}
	const osThreadAttr_t tcpTask_attributes = {
        .name = "TcpClientTask",
        .stack_size = 8192,
        .priority = (osPriority_t) osPriorityNormal,
    };
	TcpClientTaskHandle = osThreadNew (TcpClient1_thread, (void *)pTcpClient, &tcpTask_attributes);
}


