/*
 * smtp_proc.c
 *
 *  Created on: Jun 28, 2024
 *      Author: dis_stv
 */
#include "smtp_proc.h"


osSemaphoreId_t 			sid_SmtpCplt = NULL;
osThreadId_t 				SmtpClientTaskHandle;


//*************************************************************
const char			DEBUG_TEXT1[] = "SMTP1_debug\r\n";
const char			QUIT_COM[] = "QUIT\r\n";
//*************************************************************


//----------------------------------------------------------------------------
void SmtpProcess 	(
					void *arg
					)
{
	data_struct_t *pRW_data = (data_struct_t *)arg;
	char *wbuf, *data;
	data = pRW_data->r_data;

	// Create and fill structure for smtp

	//

	// SMTP_NULL: client <- server (ready)

	// SMTP_HELO: client (HELO) -> server
	//            client <- server (HELO_ready)

			// SMTP_AUTH_PLAIN:
			// SMTP_AUTH_LOGIN_UNAME:
			// SMTP_AUTH_LOGIN_PASS:
			// SMTP_AUTH_LOGIN:

	// SMTP_MAIL:

	// SMTP_RCPT:

	// SMTP_DATA:

	// SMTP_BODY:

	// SMTP_QUIT:

	// SMTP_CLOSED:
	// Check if
	if (strstr ((const char*)data, "221"))
	{
		pRW_data->w_data = NULL;
		return;
	}

	if (strncmp((char const *)data, "GET / ", 6)==0)
	{
		wbuf = pvPortMalloc (200);
		sprintf (wbuf, "%s", DEBUG_TEXT1);
	}
	else
	{
		wbuf = pvPortMalloc (200);
		sprintf (wbuf, "%s", QUIT_COM);
	}
	pRW_data->w_data = wbuf;
	return;
}



//----------------------------------------------------------------------------
static void SmtpClient_thread 	(
								void *arg
								)
{
	osSemaphoreId_t *psid_SmtpCplt = (osSemaphoreId_t *)arg;

//****************************************************************************
//	for (uint8_t k = 0; k < 3; k++)
//	{
//		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 1);
//		osDelay (500);
//		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 0);
//		osDelay (500);
//	}
//****************************************************************************

	// Create and fill structure for smtp

	//

	// SMTP_NULL: client <- server (ready)

	// SMTP_HELO: client (HELO) -> server
	//            client <- server (HELO_ready)

			// SMTP_AUTH_PLAIN:
			// SMTP_AUTH_LOGIN_UNAME:
			// SMTP_AUTH_LOGIN_PASS:
			// SMTP_AUTH_LOGIN:

	// SMTP_MAIL:

	// SMTP_RCPT:

	// SMTP_DATA:

	// SMTP_BODY:

	// SMTP_QUIT:

	// SMTP_CLOSED:


	osSemaphoreRelease (*psid_SmtpCplt);
	SmtpClientTaskHandle = NULL;
	osThreadExit ();
}


void StartSmtpClient 	(
						void *arg
						)
{
	const osThreadAttr_t Task_attributes = {
        .name = "SmtpClientTask",
        .stack_size = 3*512,
        .priority = (osPriority_t) osPriorityNormal,
    };
	SmtpClientTaskHandle = osThreadNew (SmtpClient_thread, arg, &Task_attributes);
	return;
}


