/*
 * settings_proc.c
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#include "settings_proc.h"


osMessageQueueId_t 	mid_FlashData = NULL;
osSemaphoreId_t 	sid_FlashReady = NULL;



static void FlashProcess_thread (
								void *arg
								)
{
	HAL_StatusTypeDef status;
	FLASH_EraseInitTypeDef EraseInitStruct;
	char Data[33];

	mid_FlashData = osMessageQueueNew (4, 33, NULL);
    vQueueAddToRegistry (mid_FlashData, "mid_FlashData");
    sid_FlashReady = osSemaphoreNew(1, 0, NULL);
    vQueueAddToRegistry (sid_FlashReady, "sid_FlashReady");
    // Infinite loop
    for(;;)
    {
        osMessageQueueGet (mid_FlashData, Data, NULL, osWaitForever);
        switch (Data[32])
        {
        	case '0':
        	// Erase (full sector)
        	{
                HAL_FLASH_Unlock();
            	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
            	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
            	EraseInitStruct.Banks         = FLASH_BANK_2;
            	EraseInitStruct.Sector        = FLASH_SECTOR_7;
            	EraseInitStruct.NbSectors     = 1;

            	status = HAL_FLASHEx_Erase_IT (&EraseInitStruct);
            	while (status != HAL_OK)
            	{
            		// erase error loop
            	}
            	osSemaphoreAcquire (sid_FlashReady, osWaitForever);
        		break;
        	}
        	case '1':
        	// Write IP-mode
        	{
                HAL_FLASH_Unlock();
                status = HAL_FLASH_Program_IT (FLASH_TYPEPROGRAM_FLASHWORD, FLASH_IPMODE_ADDR, (uint32_t)Data);
            	while (status != HAL_OK)
            	{
            		// write error loop
            	}
            	osSemaphoreAcquire (sid_FlashReady, osWaitForever);
        		break;
        	}
        	case '2':
        	// Write IP-settings
        	{
                HAL_FLASH_Unlock();
                status = HAL_FLASH_Program_IT (FLASH_TYPEPROGRAM_FLASHWORD, FLASH_IPADDR_ADDR, (uint32_t)Data);
            	while (status != HAL_OK)
            	{
            		// write error loop
            	}
            	osSemaphoreAcquire (sid_FlashReady, osWaitForever);
        		break;
        	}
        	case '3':
        	// Write Wifi-settings
        	{
                HAL_FLASH_Unlock();
                status = HAL_FLASH_Program_IT (FLASH_TYPEPROGRAM_FLASHWORD, FLASH_WIFISETTINGS_ADDR, (uint32_t)Data);
            	while (status != HAL_OK)
            	{
            		// write error loop
            	}
            	osSemaphoreAcquire (sid_FlashReady, osWaitForever);
            	NVIC_SystemReset();
        		break;
        	}
        	default:
        	{
        		break;
        	}
        }
    }
}


void FlashReadBuf	(
					uint8_t *buf,
                    uint32_t address,
                    uint32_t arr_size
                    )
{
    uint32_t addr = address;
    uint32_t size = arr_size;

	while(size)
	{
		*buf++ = FLASH_READ_BYTE(addr++);
        size--;
	}
}


static void FlashProgComplet(void)
{
	HAL_FLASH_Lock();
	osSemaphoreRelease (sid_FlashReady);
}


static void FlashEraseComplet (void)
{
	HAL_FLASH_Lock();
	osSemaphoreRelease (sid_FlashReady);
}


osThreadId_t StartFlashProcess (void)
{
	const osThreadAttr_t flashProcess_attributes = {
        .name = "FlashTask",
        .stack_size = 512 * 2,
        .priority = (osPriority_t) osPriorityNormal,
    	};
    return osThreadNew(FlashProcess_thread, NULL, &flashProcess_attributes);
}


void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue)
{
	if (ReturnValue == 0xFFFFFFFFU)
		FlashEraseComplet();
	else
		FlashProgComplet();
}


void HAL_FLASH_OperationErrorCallback	(
										uint32_t ReturnValue
										)
{
	if (ReturnValue == FLASH_SECTOR_7)
	{
		while(1)
		{
			// erase error
		}
	}
	else
	{
		while(1)
		{
			// program error
		}
	}
}


void SaveToFlash	(
					set_struc_t *p
					)
{
	char Data[33];

	// Erase
	Data[32] = '0';
	osMessageQueuePut(mid_FlashData, Data, 0U, 0U);

	// Write IP-mode
	memset (Data, 0xff, 32);
	Data[0] = *(p->ip_mode);
	Data[32] = '1';
	osMessageQueuePut(mid_FlashData, Data, 0U, 0U);

	// Write IP-settings in static mode
	if (Data[0] == STATIC_MODE)
	{
		memset (Data, 0xff, 32);
		memcpy(Data, p->ipa, 4);
		memcpy(Data + 4, p->ipm, 4);
		memcpy(Data + 8, p->ipg, 4);
		Data[32] = '2';
		osMessageQueuePut(mid_FlashData, Data, 0U, 0U);
	}

	// Write wifi-settings
	memset (Data, 0xff, 32);
	memcpy(Data, p->ssid, FLASH_WIFISSID_SIZE);
	memcpy(Data + FLASH_WIFISSID_SIZE, p->psw, FLASH_WIFIPSW_SIZE);
	Data[32] = '3';
	osMessageQueuePut(mid_FlashData, Data, 0U, 0U);
}


uint32_t ResetSettings (void)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError = 0;

	HAL_FLASH_Unlock();
	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.Banks         = FLASH_BANK_2;
	EraseInitStruct.Sector        = FLASH_SECTOR_7;
	EraseInitStruct.NbSectors     = 1;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
		return 1;

	HAL_FLASH_Lock();
	return 0;
}


void ReadSettings 	(
					uint8_t *ipmode,
					uint32_t *ipa,
					uint32_t *ipm,
					uint32_t *ipg
					)
{
	uint32_t addr, mask, gate;

	*ipmode = FLASH_READ_BYTE(FLASH_IPMODE_ADDR);
	if (*ipmode == STATIC_MODE)
	{
		addr = FLASH_READ_WORD(FLASH_IPADDR_ADDR);
		mask = FLASH_READ_WORD(FLASH_IPMASK_ADDR);
		gate = FLASH_READ_WORD(FLASH_IPGATE_ADDR);

		if (	(uint8_t)(addr >> 24)!=255 &&
				(uint8_t)(addr >> 16)!=255 &&
				(uint8_t)(addr >> 8)!=255 &&
				(uint8_t)(addr)!=255  )
		{
			if ((uint8_t)(mask)==255)
			{
				*ipa = addr;
				*ipm = mask;
				*ipg = gate;
			}
		}
	}
}


