/*
 * ping_proc.c
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */
#include "ping_proc.h"


osThreadId_t 		PingHandle = NULL;
osThreadId_t 		PingLoopHandle = NULL;
osMessageQueueId_t 	mid_PingData = NULL;
osSemaphoreId_t 	sid_PingReady = NULL;
osMessageQueueId_t 	mid_PingRes = NULL;

reply_struc_t 		Ping_res;
uint32_t			Alarm;

static uint16_t 	ping_seq_num;

char				alrm_email_str[15];
char				alrm_telega_str[15];
uint8_t				alrm_unrepl = 4;
uint8_t				alrm_led;

uint32_t			email_is_OK;

extern char 				*pp;

extern void RunAppClient (uint32_t);




// Prepare a echo ICMP request
static void PingPrepareEcho	(
							struct icmp_echo_hdr *iecho,
							u16_t len
							)
{
	size_t i;
	size_t data_len = len - sizeof(struct icmp_echo_hdr);

	ICMPH_TYPE_SET(iecho, ICMP_ECHO);
	ICMPH_CODE_SET(iecho, 0);
	iecho->chksum = 0;
	iecho->id     = PING_ID;
	iecho->seqno  = lwip_htons(++ping_seq_num);

	// fill the additional data buffer with some data
	for(i = 0; i < data_len; i++)
	{
		((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
	}
	iecho->chksum = inet_chksum(iecho, len);
}


static uint32_t AnalyseReply	(
								void *req_dat,
								void *rep_dat
								)
{
	struct icmp_echo_hdr *req_hdr = (struct icmp_echo_hdr *) req_dat;
	struct icmp_echo_hdr *rep_hdr = (struct icmp_echo_hdr *) rep_dat;
	uint32_t ref_dat_num = sizeof(struct icmp_echo_hdr);

	if (req_hdr->type != 8 || rep_hdr->type != 0)
		return 2;
	if (req_hdr->id != PING_ID || rep_hdr->id != PING_ID)
		return 3;

	return memcmp (req_dat+ref_dat_num, rep_dat+ref_dat_num, PING_DATA_SIZE);
}


static void Ping_thread (
						void *arg
						)
{
	struct netconn *conn = NULL;
	struct icmp_echo_hdr *iecho;
	err_t err;
	size_t ping_size;
	struct netbuf *txBuf;
	struct netbuf *rxBuf;
	ip_addr_t *p_dest_addr;
	char smsg[200];
	uint32_t ping_time1, reply_time;
	reply_struc_t ping_res;

	// get data from Ping_res
	ping_res.ping_cnt = Ping_res.ping_cnt;
	ping_res.min_time = Ping_res.min_time;
	ping_res.max_time = Ping_res.max_time;
	ping_res.lost = Ping_res.lost;
	ping_res.alarm_cnt = Ping_res.alarm_cnt;

	p_dest_addr = (ip_addr_t *)arg;
	// Create a new connection identifier
	conn = netconn_new_with_proto_and_callback(NETCONN_RAW, IP_PROTO_ICMP, NULL);
	if (conn == NULL)
	{
#ifdef DEBUG_PING_PROC
		PRINTF("PingThread: Can't create connection\r\n");
#endif
		goto exit1;
	}

	// Set receive timeout
	netconn_set_recvtimeout(conn, PING_RCV_TIMEO);

	ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
	if (ping_size > 0xffff)
	{
#ifdef DEBUG_PING_PROC
		PRINTF("PingThread: Ping size is too big");
#endif
		netconn_delete(conn);
		goto exit1;
	}
	iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
	if (!iecho)
	{
#ifdef DEBUG_PING_PROC
		PRINTF("PingThread: Can't allocate memory");
#endif
		netconn_delete(conn);
		goto exit1;
	}
	PingPrepareEcho (iecho, (u16_t)ping_size);

	// Create a new netbuf
	txBuf = netbuf_new();

	// Remote IP-address
	txBuf->addr = *p_dest_addr;

	err = netbuf_ref(txBuf, iecho, ping_size);

	// send the netbuf to the server
	netconn_send (conn, txBuf);

	ping_time1 = sys_now();

	// waiting for data from server
	err = netconn_recv (conn, &rxBuf);
	switch (err)
	{
		case ERR_OK:
		{
			// Handling received data
			reply_time = (sys_now() - ping_time1 == 0) ? 1: sys_now() - ping_time1;
			memset (smsg, '\0', 200);
			memcpy (smsg, rxBuf->ptr->payload + sizeof(struct ip_hdr), rxBuf->ptr->len - sizeof(struct ip_hdr));
			err = AnalyseReply ((void *)iecho, (void *) smsg);
			if (err == ERR_OK)
			{
#ifdef DEBUG_PING_PROC
				PRINTF ("PingThread: Received reply for %ld ms\r\n", reply_time);
#endif
				ping_res.status = 0;
				ping_res.cur_time = reply_time;
				if (ping_res.ping_cnt == 0)
				{
					ping_res.min_time = ping_res.cur_time;
					ping_res.max_time = ping_res.cur_time;
				}
				else
				{
					if (ping_res.cur_time < ping_res.min_time)
						ping_res.min_time = ping_res.cur_time;
					if (ping_res.cur_time > ping_res.max_time)
						ping_res.max_time = ping_res.cur_time;
				}
				if (ping_res.alarm_cnt < alrm_unrepl)
					ping_res.alarm_cnt = 0;
			}
			else
			{
#ifdef DEBUG_PING_PROC
				PRINTF ("PingThread: Reply has error %d\r\n", err);
#endif
				ping_res.status = err;
				ping_res.lost++;
				ping_res.alarm_cnt++;
			}
			break;
		}
		case ERR_TIMEOUT:
		{
			// Send request to server again
#ifdef DEBUG_PING_PROC
			PRINTF ("PingThread: Timeout %ld ms\r\n", (sys_now() - ping_time1));
#endif
			ping_res.status = 5;
			ping_res.lost++;
			ping_res.alarm_cnt++;
			break;
		}
		default:
		{
			// close, delete conn, terminate thread
#ifdef DEBUG_PING_PROC
			PRINTF ("PingThread: Receiving error %d\r\n", err);
#endif
			ping_res.status = 6;
			ping_res.lost++;
			ping_res.alarm_cnt++;
			break;
		}
	}

	mem_free(iecho);
	netbuf_delete (txBuf);
	netbuf_delete (rxBuf);
	netconn_delete(conn);
#ifdef DEBUG_PING_PROC
	PRINTF ("PingThread: Connection removed\r\n");
#endif

	// return data to Ping_res (message)
	ping_res.ping_cnt++;
	osMessageQueuePut(mid_PingRes, &ping_res, 0U, 0U);

exit1:
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 0);
	PingHandle = NULL;
    osSemaphoreRelease (sid_PingReady);
	osThreadExit();
}


static osThreadId_t StartOnePing 	(
									void *arg
									)
{
    const osThreadAttr_t PingTask_attributes = {
        .name = "PingTask",
        .stack_size = 3*512,
        .priority = (osPriority_t) osPriorityNormal,
    };
    return osThreadNew(Ping_thread, arg, &PingTask_attributes);
}


static void SwitchLeds 	(
						uint32_t state
						)
{
	if (alrm_led & ALARM_LED_RED)
	{
		HAL_GPIO_WritePin (LED_RED_GPIO_Port, LED_RED_Pin, state);
	}
	if (alrm_led & ALARM_LED_BLUE)
	{
		HAL_GPIO_WritePin (LED_BLUE_GPIO_Port, LED_BLUE_Pin, state);
	}
	if (alrm_led & ALARM_LED_GREEN)
	{
		HAL_GPIO_WritePin (LED_GREEN_GPIO_Port, LED_GREEN_Pin, state);
	}
}


static void PingLoop_thread (
							void *arg
							)
{
	uint32_t  ticks;
	osStatus_t err;
	ping_struc_t ping_data;
	reply_struc_t ping_res = {0};

	Ping_res.ping_cnt = 0;
	Ping_res.min_time = 0;
	Ping_res.max_time = 0;
	Ping_res.lost = 0;
	Ping_res.alarm_cnt = 0;
	Alarm = 0;
	SwitchLeds (0);


	mid_PingRes = osMessageQueueNew (2, sizeof(reply_struc_t), NULL);
	vQueueAddToRegistry (mid_PingRes, "mid_PingRes");

	sid_PingReady = osSemaphoreNew (1, 0, NULL);
    vQueueAddToRegistry (sid_PingReady, "sid_PingReady");
	osMessageQueueGet (mid_PingData, &ping_data, NULL, osWaitForever);
	for (uint32_t i = 0; i < ping_data.n_pings; i++)
	{
		ticks = sys_now();
		PingHandle = StartOnePing ((void *)&(ping_data.ip_host));
#ifdef DEBUG_PING_PROC
		PRINTF("PingLoopThread: Ping %d\r\n\n", Ping_res.ping_cnt);
#endif

		// get semaphore
		err = osSemaphoreAcquire (sid_PingReady, 2*PING_DELAY);
		if (err != osOK)
		{
#ifdef DEBUG_PING_PROC
			PRINTF("PingLoopThread: Ping error %d\r\n\n", err);
#endif
			// return status = 10 to Ping_res (message)
			ping_res.status = 10;
			osMessageQueuePut(mid_PingRes, &ping_res, 0U, 0U);
			break;
		}
		osDelayUntil(ticks + PING_DELAY);
	}
#ifdef DEBUG_PING_PROC
	PRINTF("PingLoopThread: Ping task deleted\r\n\n");
#endif
	osSemaphoreDelete (sid_PingReady);

	osMessageQueueDelete (mid_PingRes);
	mid_PingRes = NULL;

	PingLoopHandle = NULL;
	osThreadExit ();
}


osThreadId_t StartPings (void)
{
    const osThreadAttr_t PingLoopTask_attributes = {
        .name = "PingLoopTask",
        .stack_size = 3*512,
        .priority = (osPriority_t) osPriorityNormal,
    };
    return osThreadNew(PingLoop_thread, NULL, &PingLoopTask_attributes);
}


void StopPing (void)
{
	if (PingLoopHandle != NULL)
	{
#ifdef DEBUG_PING_PROC
		PRINTF ("StopPing: Ping task terminated\r\n\n");
#endif
		osSemaphoreDelete (sid_PingReady);
		osThreadTerminate (PingHandle);
		osThreadTerminate (PingLoopHandle);
		PingLoopHandle = NULL;
	}
}


static void StartSignal (void)
{
	// switch leds
	SwitchLeds (1);

	// send e-mail
	if (email_is_OK)
	{
		RunAppClient (SMTP_PROT);
	}

	// send telegram message: tg_send_message ()

}


void GetPingRes (void)
{
	if (mid_PingRes != NULL)
	{
		osMessageQueueGet (mid_PingRes, &Ping_res, NULL, 0);
		if (!Alarm && Ping_res.alarm_cnt >= alrm_unrepl)
		{
			// alarm
			Alarm = 1;

			// start signaling
			StartSignal ();
		}
	}
}
