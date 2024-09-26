/*
 * smtp_proc.h
 *
 *  Created on: Jun 28, 2024
 *      Author: dis_stv
 */

#ifndef INC_SMTP_PROC_H_
#define INC_SMTP_PROC_H_

#include "tcp_proc.h"
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



void SendEmail (void);
osThreadId_t StartSmtpClient (void *);



#endif /* INC_SMTP_PROC_H_ */
