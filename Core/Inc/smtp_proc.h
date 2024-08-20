/*
 * smtp_proc.h
 *
 *  Created on: Jun 28, 2024
 *      Author: dis_stv
 */

#ifndef INC_SMTP_PROC_H_
#define INC_SMTP_PROC_H_

#include "console_uart.h"

#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/sys.h"

#include "queue.h"
#include <string.h>
#include <stdlib.h>



#define SMTP_SERVER_PORT		25
//#define SMTP_SERVER_ADDR[4]		{127, 0, 0, 1}



/** State for SMTP client state machine */
enum smtp_session_state {
  SMTP_NULL,
  SMTP_HELO,
  SMTP_AUTH_PLAIN,
  SMTP_AUTH_LOGIN_UNAME,
  SMTP_AUTH_LOGIN_PASS,
  SMTP_AUTH_LOGIN,
  SMTP_MAIL,
  SMTP_RCPT,
  SMTP_DATA,
  SMTP_BODY,
  SMTP_QUIT,
  SMTP_CLOSED
};


void SmtpProcess (void);
osThreadId_t StartSmtpClient (void);

#endif /* INC_SMTP_PROC_H_ */
