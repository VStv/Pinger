//****************************** console_uart.h ******************************

#ifndef _CONSOLE_UART_
#define _CONSOLE_UART_

#include "stm32h7xx_hal.h"


//#include "main.h"
#include "cmsis_os.h"

#include "queue.h"
#include <stdio.h>
#include <string.h>

//#include "lwip/tcp.h"



#define UART_RX_DATA_MAX_SIZE 128
#define UART_TX_DATA_MAX_SIZE 128


//uint32_t PingRequestParsing (char *str);
osThreadId_t StartConsoleTask (void);
void LoadToConsole (char *);
void ConsoleFree (void);

osThreadId_t StartUartRxTask (void);
void GetUartRxData (uint16_t);



#define PRINTF(args...)			pp = pvPortMalloc (snprintf (NULL, 0, args) + sizeof ('\0')); \
								sprintf (pp, args);	\
								LoadToConsole (pp); \
								vPortFree (pp)


#endif /* _CONSOLE_UART_ */
