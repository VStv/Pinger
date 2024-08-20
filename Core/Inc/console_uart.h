//****************************** console_uart.h ******************************

#ifndef _CONSOLE_UART_
#define _CONSOLE_UART_

#include "main.h"
#include "cmsis_os.h"

#include "queue.h"
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>

#define UART_RX_DATA_MAX_SIZE 128
#define UART_TX_DATA_MAX_SIZE 128

extern uint32_t PingRequestParsing(char *str);
osThreadId_t StartConsoleTask(void);
void LoadToConsole(char *);
void ConsoleFree(void);

osThreadId_t StartUartRxTask(void);
void GetUartRxData(uint16_t);



#define PRINTF(args...)			pp = pvPortMalloc(snprintf(NULL, 0, args) + sizeof('\0')); \
								sprintf(pp, args);	\
								LoadToConsole(pp); \
								vPortFree(pp)
/*
#define safe_printf(FORMAT, ...) \
	static_assert(checkFormat( FORMAT, __VA_ARGS__ ), "Format is incorrect"); \
	printf(FORMAT, __VA_ARGS__)
*/
#endif /* _CONSOLE_UART_ */
