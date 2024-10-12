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
//#include "console_uart.h"

#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/sys.h"


#include "lwip/apps/fs.h"

#include "queue.h"
#include <string.h>
#include <stdlib.h>


#define HTTP_SERVER_PORT		80


typedef struct
{
	uint8_t 		*unrepl;
	char 			*email;
	char	 		*telega;
	uint8_t 		*led;
} alrm_struct_t;


//typedef struct
//{
//	char 			*r_data;
//	char	 		*w_data;
//} data_struct_t;


void RunHttpServer (void);
void HttpProcess (void *);


#endif /* INC_HTTP_PROC_H_ */
