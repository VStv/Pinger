/*
 * ping_proc.h
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#ifndef INC_PING_PROC_H_
#define INC_PING_PROC_H_

#include "console_uart.h"
#include "smtp_proc.h"

#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/sys.h"

#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"
#include "lwip/ip.h"


#include <string.h>





#define ALARM_LED_RED			(1<<0)
#define ALARM_LED_BLUE			(1<<1)
#define ALARM_LED_GREEN			(1<<2)

#define PING_ID        			0xAFAF
#define PING_DATA_SIZE 			32
#define PING_RCV_TIMEO 			800
#define PING_DELAY     			1000


typedef struct
{
	ip4_addr_t 		ip_host;
	uint16_t		n_pings;
} ping_struc_t;


typedef struct
{
	uint16_t		ping_cnt;
	uint16_t		ttl;
	uint16_t 		cur_time;
	uint16_t 		min_time;
	uint16_t 		max_time;
	uint16_t		lost;
	uint16_t		alarm_cnt;
	uint16_t		status;
} reply_struc_t;



osThreadId_t StartPings (void);
void StopPing (void);
void GetPingRes (void);


#endif /* INC_PING_PROC_H_ */
