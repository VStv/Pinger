/*
 * http_proc.h
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#ifndef INC_HTTP_PROC_H_
#define INC_HTTP_PROC_H_

#include "settings_proc.h"
#include "ping_proc.h"
#include "console_uart.h"

#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/sys.h"

//#include "lwip/tcp.h"

#include "lwip/apps/fs.h"

#include "queue.h"
#include <string.h>
#include <stdlib.h>


typedef struct
{
	uint8_t 		*unrepl;
	char 			*email;
	char	 		*telega;
	uint8_t 		*led;
} alrm_struc_t;




char *HttpProcess (char *);



#endif /* INC_HTTP_PROC_H_ */
