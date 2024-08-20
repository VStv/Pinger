/*
 * smtp_proc.c
 *
 *  Created on: Jun 28, 2024
 *      Author: dis_stv
 */
#include "smtp_proc.h"


extern	osThreadId_t 		TcpClientTaskHandle;






//osThreadId_t StartSmtpClient (void)
//{
//	TcpClientTaskHandle = StartTcpClient();
//	return TcpClientTaskHandle;
//}


//void SmtpProcess (void)
//{
//	for (uint8_t k = 0; k < 3; k++)
//	{
//		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 1);
//		osDelay (500);
//		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 0);
//		osDelay (500);
//	}
//}
