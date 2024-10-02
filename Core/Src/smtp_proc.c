/*
 * smtp_proc.c
 *
 *  Created on: Jun 28, 2024
 *      Author: dis_stv
 */
#include "smtp_proc.h"


osSemaphoreId_t 			sid_SmtpCplt = NULL;

extern	osThreadId_t 		TcpClientTaskHandle;





void SendEmail (void)
{
	uint32_t app = SMTP_PROT;
	TcpClientTaskHandle = StartTcpClient ((void *)&app);
}


static void SmtpClient_thread 	(
								void *arg
								)
{
	osSemaphoreId_t *psid_SmtpCplt = (osSemaphoreId_t *)arg;

//****************************************************************************
	for (uint8_t k = 0; k < 3; k++)
	{
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 1);
		osDelay (500);
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 0);
		osDelay (500);
	}
//****************************************************************************

	// smtp_NULL: client <- server (ready)

	// smtp_HELO: client (HELO) -> server
	//            client <- server (HELO_ready)

	//


	osSemaphoreRelease  (*psid_SmtpCplt);
	osThreadExit ();
}


void StartSmtpClient 	(
						void *arg
						)
{
	net_struct_t *pTcpClient = (net_struct_t *)arg;

	const osThreadAttr_t Task_attributes = {
        .name = "SmtpClientTask",
        .stack_size = 3*512,
        .priority = (osPriority_t) osPriorityNormal,
    };
	pTcpClient->app_id = osThreadNew (SmtpClient_thread, (void *)pTcpClient->sem_app_cplt, &Task_attributes);
	return;
}


