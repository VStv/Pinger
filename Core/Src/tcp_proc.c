/*
 * tcp_proc.c
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#include "tcp_proc.h"


osThreadId_t 		TcpConnHandle[TCP_CONNECTION_MAX] = {NULL};
TcpConnStruct_t 	TcpConnStruct[TCP_CONNECTION_MAX];
osThreadId_t 		TcpClientTaskHandle = NULL;

osSemaphoreId_t 	sid_Connected = NULL;



extern char 				*pp;
extern osMessageQueueId_t 	mid_PingData;
extern osMessageQueueId_t 	mid_PingRes;




static void TcpConn_thread 	(
							void *arg
							)
{
	TcpConnStruct_t *pTcpConn = (TcpConnStruct_t*)arg;
	struct netbuf *buf;
	char *rdata, *wdata;
	uint16_t len;

	PRINTF ("accepted new connection %c\r\n", pTcpConn->number);
	netconn_set_recvtimeout (pTcpConn->conn, 1000);
	// receive the data from the client
	if (netconn_recv (pTcpConn->conn, &buf) == ERR_OK)
	{
		netbuf_data (buf, (void**)&rdata, &len);
		wdata = HttpProcess (rdata);
		// send the message back to the client
		if (netconn_write (pTcpConn->conn, (const unsigned char*)wdata, (size_t)strlen (wdata), NETCONN_COPY) != ERR_OK)
		{
			vPortFree (wdata);
			netbuf_delete (buf);
			netconn_close (pTcpConn->conn);
			netconn_delete (pTcpConn->conn);
			pTcpConn->conn = NULL;
			PRINTF ("Write error. Connection %c closed\r\n\n", pTcpConn->number);
			osThreadExit ();
		}
		vPortFree (wdata);
	}
	else
	{
		PRINTF ("Receive error\r\n\n");
	}
	PRINTF ("Connection %c closed\r\n\n", pTcpConn->number);
	netbuf_delete (buf);
	netconn_close (pTcpConn->conn);
	netconn_delete (pTcpConn->conn);
	pTcpConn->conn = NULL;
	osThreadExit ();
}


static void TcpServer_thread 	(
								void *arg
								)
{
	struct netconn *conn, *newconn;
	err_t err2;

	// Create a new connection identifier
    conn = netconn_new(NETCONN_TCP);
	if (conn == NULL)
	{
		PRINTF("Can't create TCP connection\r\n");
		goto exit1;
	}
	// Bind connection to the port 80
	err2 = netconn_bind(conn, IP_ADDR_ANY, 80);
	if (err2 != ERR_OK)
	{
		err2 = netconn_delete(conn);
		conn = NULL;
		PRINTF("Can't bind TCP connection %p\r\n", conn);
		goto exit1;
	}
	// Tell connection to go into listening mode
	netconn_listen(conn);
	// The while loop will run every time this Task
	while (1)
	{
		// Grab & Process new connection
		if (netconn_accept(conn, &newconn) == ERR_OK)
		{
			TcpConnStruct_t *pTcpConn = TcpConnStruct;
			for (uint8_t i = 0; i < TCP_CONNECTION_MAX; i++)
			{
				if (pTcpConn->conn == NULL)
				{
//					PRINTF("Connected with remote IP: 192.168.10.%d\r\n", (uint8_t)((newconn->pcb.tcp->remote_ip.addr)>>24));
					PRINTF("Connected with remote port %d\r\n", newconn->pcb.tcp->remote_port);
					PRINTF("Connection time %ld\r\n", sys_now());
//					HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 1);
					pTcpConn->conn = newconn;
					char nameThread[] = {'T','C','P','C','o','n','n','T','a','s','k', pTcpConn->number,'\0'};
					const osThreadAttr_t tcpConn_attributes = {
						.name = nameThread,
						.priority = (osPriority_t) osPriorityNormal,
						.stack_size = 512 * 4,
					};
					TcpConnHandle[i] = osThreadNew(TcpConn_thread, pTcpConn, &tcpConn_attributes);
					newconn = NULL;
					break;
				}
				pTcpConn++;
			}
			if (newconn != NULL)
			{
				PRINTF("No free conn-structures\r\n");
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
//	HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 0);
	for(;;);
}


void my_callback(struct netconn *conn, enum netconn_evt evt, u16_t len)
{
	(void) len;

	switch (evt)
	{
		case NETCONN_EVT_SENDPLUS:
			if (conn->pcb.tcp->state != ESTABLISHED)
				break;
			if (sid_Connected != NULL)
			{
				osSemaphoreRelease(sid_Connected);
//				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 1);
			}
			break;
		default:
			break;
	}
}


osThreadId_t StartTcpServer (void)
{
	TcpConnStruct_t *pTcpConn = TcpConnStruct;
	mid_PingData = osMessageQueueNew (1, sizeof(ping_struc_t), NULL);
	vQueueAddToRegistry (mid_PingData, "mid_PingData");
//	mid_PingRes = osMessageQueueNew (2, sizeof(reply_struc_t), NULL);
//	vQueueAddToRegistry (mid_PingRes, "mid_PingRes");

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
    return osThreadNew(TcpServer_thread, NULL, &tcpTask_attributes);
}


static void TcpClient_thread (void *arg)
{
	struct netconn *conn = NULL;
	err_t err;
	uint16_t src_port;
	ip_addr_t dest_addr;
	uint16_t dst_port = SMTP_SERVER_PORT;
	osStatus_t val;
	uint8_t serv_addr[4] = {192, 168, 10, 10};

	sid_Connected = osSemaphoreNew(1, 0, NULL);
	vQueueAddToRegistry (sid_Connected, "sid_Connected");
	// Create a new connection identifier
	conn = netconn_new_with_callback (NETCONN_TCP, my_callback);
	if (conn == NULL)
	{
		PRINTF ("Can't create connection");
		goto exit2;
	}
	// Bind connection to random local port
	src_port = (uint16_t) SysTick->VAL;
	err = netconn_bind(conn, IP_ADDR_ANY, src_port);
	if (err != ERR_OK)
	{
		PRINTF ("Can't bind TCP connection, err = %d", err);
		err = netconn_delete(conn);
		goto exit2;
	}
	// Connect to remote IP-address
	IP_ADDR4 (&dest_addr, serv_addr[0], serv_addr[1], serv_addr[2], serv_addr[3]);
	PRINTF ("Try to connect to port %d\r\n", dst_port);
	// connect to remote server at port
	netconn_set_nonblocking(conn, 1);
	err = netconn_connect (conn, &dest_addr, dst_port);
	val = osSemaphoreAcquire(sid_Connected, 1000U);
	if (val != osOK)
	{
		PRINTF ("Connection dropped\r\n");
		err = netconn_delete(conn);
		conn = NULL;
		goto exit2;
	}
	netconn_set_nonblocking(conn, 0);


//	SmtpProcess ();
	for (uint8_t k = 0; k < 3; k++)
	{
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 1);
		osDelay (500);
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 0);
		osDelay (500);
	}


	err = netconn_close(conn);
	err = netconn_delete(conn);
	conn = NULL;
	PRINTF ("Connection closed\r\n");

exit2:
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 0);
	if (sid_Connected)
	{
		osSemaphoreDelete (sid_Connected);
		sid_Connected = NULL;
	}
	TcpClientTaskHandle = NULL;
	osThreadExit();
}


osThreadId_t StartTcpClient (void)
{
    const osThreadAttr_t tcpTask_attributes = {
        .name = "TcpClientTask",
        .stack_size = 3*512,
        .priority = (osPriority_t) osPriorityNormal,
    };
    return osThreadNew(TcpClient_thread, NULL, &tcpTask_attributes);
}
