//****************************** console_uart.c ******************************
#include "console_uart.h"

extern UART_HandleTypeDef huart3;

#if defined( __ICCARM__ )
  #define DMA_BUFFER     _Pragma("location=\".dma_buffer\"")
#else
  #define DMA_BUFFER     __attribute__((section(".dma_buffer")))
#endif


DMA_BUFFER char strng1[UART_TX_DATA_MAX_SIZE], strng2[UART_RX_DATA_MAX_SIZE], strng3[UART_RX_DATA_MAX_SIZE];
uint16_t uart_rx_data_size;

char 	*pp;

osSemaphoreId_t sid_ConsoleUartReady = NULL;
osMessageQueueId_t mid_ConsoleData = NULL;
osMessageQueueId_t mid_UartRxData = NULL;

// --------------------------------------------------------------------------
//                                 Transmit
// --------------------------------------------------------------------------

int _write (int fd, char * ptr, int len)
{
	strcpy(strng3, ptr);
	HAL_UART_Transmit(&huart3, (uint8_t *) strng3, len, 0xffff);
	return len;
}


void LoadToConsole (char *str)
{
	osMessageQueuePut (mid_ConsoleData, str, 0U, 0U);
}


void ConsoleFree (void)
{
	osSemaphoreRelease (sid_ConsoleUartReady);
}


static void ConsoleTask (void * argument)
{
    mid_ConsoleData = osMessageQueueNew(8, sizeof(strng1), NULL);
    vQueueAddToRegistry(mid_ConsoleData, "mid_ConsoleData");
    sid_ConsoleUartReady = osSemaphoreNew(1, 0, NULL);
    vQueueAddToRegistry(sid_ConsoleUartReady, "sid_ConsoleUartReady");
    // Infinite loop
    for(;;)
    {
        osMessageQueueGet(mid_ConsoleData, strng1, NULL, osWaitForever);
        HAL_UART_Transmit_DMA(&huart3, (uint8_t *) strng1, strlen(strng1));
        osSemaphoreAcquire (sid_ConsoleUartReady, osWaitForever);
    }
}


osThreadId_t StartConsoleTask(void)
{
    const osThreadAttr_t consoleTask_attributes = {
    .name = "consoleTask",
    .stack_size = 2*512,
    .priority = (osPriority_t) osPriorityNormal,
    };
	return osThreadNew(ConsoleTask, NULL, &consoleTask_attributes);
}



void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart3)
	{
		ConsoleFree();
	}
}



// --------------------------------------------------------------------------
//                                 Receive
// --------------------------------------------------------------------------
static void UartRxTask(void * argument)
{
//	uint32_t err;
	char uart_rx_data[UART_RX_DATA_MAX_SIZE];
//	char str[UART_RX_DATA_MAX_SIZE];

	mid_UartRxData = osMessageQueueNew(4, UART_RX_DATA_MAX_SIZE, NULL);
    vQueueAddToRegistry(mid_UartRxData, "mid_UartRxData");
    // Start rx
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, (uint8_t *)strng2, UART_RX_DATA_MAX_SIZE);
    // Infinite loop
    for(;;)
    {
        osMessageQueueGet(mid_UartRxData, uart_rx_data, NULL, osWaitForever);

        // Process recieved data
        // Echo
        PRINTF("Received: %s \r\n", uart_rx_data);

		// Request parsing
/*        err = PingRequestParsing(uart_rx_data);
		if (err != 0)
			__printf("Ping request error: %d\r\n", (void *)err);
*/
        // Start next rx
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, (uint8_t *)strng2, UART_RX_DATA_MAX_SIZE);
    }
}


osThreadId_t StartUartRxTask(void)
{
    const osThreadAttr_t UartRxTask_attributes = {
    .name = "UartRxTask",
    .stack_size = 2*512,
    .priority = (osPriority_t) osPriorityNormal,
    };
	return osThreadNew(UartRxTask, NULL, &UartRxTask_attributes);
}


void GetUartRxData(uint16_t size)
{
	uart_rx_data_size = size;
	strng2[size] = '\0';
	osMessageQueuePut(mid_UartRxData, strng2, 0U, 0U);
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart == &huart3)
	{
		if (huart->RxEventType == HAL_UART_RXEVENT_IDLE)
		{
			GetUartRxData(Size);
		}
	}
}

