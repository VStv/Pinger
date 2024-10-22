/*
 * settings_proc.h
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#ifndef INC_SETTINGS_PROC_H_
#define INC_SETTINGS_PROC_H_

//#include "ping_process.h"
//#include "console_uart.h"

#include "main.h"
#include "cmsis_os.h"


#include "lwip.h"




#include "queue.h"
#include <string.h>
#include <stdlib.h>


#define STATIC_MODE 			1
#define DHCP_MODE 				0

#define ALARM_LED_RED			(1<<0)
#define ALARM_LED_BLUE			(1<<1)
#define ALARM_LED_GREEN			(1<<2)
/**/

#define FLASH_SECTOR_7_BASE		(FLASH_BANK2_BASE + FLASH_SECTOR_7 * FLASH_SECTOR_SIZE)
#define FLASH_IPMODE_ADDR		FLASH_SECTOR_7_BASE
#define FLASH_IPSETTINGS_ADDR	(FLASH_SECTOR_7_BASE + 32)
#define FLASH_WIFISETTINGS_ADDR	(FLASH_SECTOR_7_BASE + 64)


#define FLASH_IPADDR_ADDR		FLASH_IPSETTINGS_ADDR
#define FLASH_IPMASK_ADDR		(FLASH_IPADDR_ADDR + 4)
#define FLASH_IPGATE_ADDR		(FLASH_IPMASK_ADDR + 4)

#define FLASH_WIFISSID_ADDR		FLASH_WIFISETTINGS_ADDR
#define FLASH_WIFISSID_SIZE		10
#define FLASH_WIFIPSW_ADDR		(FLASH_WIFISSID_ADDR + FLASH_WIFISSID_SIZE)
#define FLASH_WIFIPSW_SIZE		10

#define FLASH_READ_BYTE(addr)   (*(__IO uint8_t *)(addr))
#define FLASH_READ_WORD(addr)	(*(__IO uint32_t *)(addr))



typedef struct
{
	uint8_t 		*ip_mode;
	ip4_addr_t 		*ipa;
	ip4_addr_t 		*ipm;
	ip4_addr_t 		*ipg;
	char 			*ssid;
	char			*psw;
} set_struc_t;



void FlashReadBuf (uint8_t *, uint32_t, uint32_t);
uint32_t ResetSettings (void);
void SaveToFlash (set_struc_t *);
void ReadSettings (uint8_t *, uint32_t *, uint32_t *, uint32_t *);

osThreadId_t StartFlashProcess (void );



#endif /* INC_SETTINGS_PROC_H_ */
