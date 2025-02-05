/*
 * tls_proc.h
 *
 *  Created on: Nov 16, 2024
 *      Author: dis_stv
 */

#ifndef INC_TLS_PROC_H_
#define INC_TLS_PROC_H_

#include "main.h"
#include "cmsis_os.h"

#include "queue.h"
#include <string.h>
#include <stdlib.h>


#include "mbedtls.h"
#include "mbedtls/net_sockets.h"


#include "tcp_proc.h"
#include "http_proc.h"
#include "console_uart.h"


#define DEBUG_TLS_PROC

//#ifdef DEBUG_TLS_PROC
//	#define PRINTF_TLS			PRINTF
//#else
//	#define PRINTF_TLS			 //
//#endif


#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n" \
    "<p>Successful connection using: %s</p>\r\n"


#define SERVER_SERT																\
		"-----BEGIN CERTIFICATE-----\r\n"										\
		"MIIDIDCCAgigAwIBAgIUGZIIJoEY/S7KolPdQfv1ZTPXCE8wDQYJKoZIhvcNAQEL\r\n"	\
		"BQAwIDEeMBwGA1UEAwwVQ2VydGlmaWNhdGUgYXV0aG9yaXR5MB4XDTI1MDIwNDE0\r\n"	\
		"MDMxOFoXDTI2MDIwNDE0MDMxOFowFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjAN\r\n"	\
		"BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAkozKhzUhq6nd0ik84SRoIAKlaN+E\r\n"	\
		"a+K1PPOfC3ibockFqBXpbXlmIhet1So7L4yIL4Yx8jVpNHNXLfkctjMRSgyhDxoT\r\n"	\
		"1VqLUP5Ks6vKLL1SQAbQHFe/m0In+u+V2zYekRpfiO+vzdJobAqYO9eIM+6Trc1x\r\n"	\
		"iEGGbZfz6E95ldSX+swYd4uMkuOtWsvFH7Ns2WXNvvCc73JcnkyaWI7SlnlhSn8K\r\n"	\
		"1Cf+LSdfr+OdjmAHnxPNSC1dEm9qwQ+KQymCbN0wkWXA/3SRcNtfDJt9/dG58P2w\r\n"	\
		"pCskKNbR9Lved3C7Lowc+EZ0ic60dlSDe/KJWw+3K82+/nVfx0Hba9MOcQIDAQAB\r\n"	\
		"o14wXDAaBgNVHREEEzARgglsb2NhbGhvc3SHBMCoCgkwHQYDVR0OBBYEFNMTxfRN\r\n"	\
		"cod2V6OH8hBuQ/GMtMQWMB8GA1UdIwQYMBaAFBrHGOed7XAVPX5JlVO7ueFkC3vJ\r\n"	\
		"MA0GCSqGSIb3DQEBCwUAA4IBAQCfee1boh/kHcQO2WkfRSFL7vtMZHiFWGXaYQmn\r\n"	\
		"sX20vfLhmUVEKKzbhsVZ5KcnSgzK7BV9g0ar+EoprOHA4gww14N6KyjyPcSCGEq0\r\n"	\
		"+RZEOIN+/4rK4jn4hTNri2ICUi2WSrEllJ6at/i+EnhhBvY5Xjtm6rXqJZfG+0tc\r\n"	\
		"DxSEnGL1ff9TcjE+CBHIZysvHVDkYo56+XsRUtdqSVvYzbyXvn/qaALeQ8CqWUPe\r\n"	\
		"ctuq5tNAFT3rmkQdvP/SPyiX8QMIas9LRpKkQeUUnGWO70AY222MIy+CT2a4a1nj\r\n"	\
		"IKV69zWYa6lnQQdi5+GYZ+RkoVeAAdWI/WTpPaaAFaSffjNn\r\n"					\
		"-----END CERTIFICATE-----\r\n"


#define SERVER_SERT_KEY															\
		"-----BEGIN RSA PRIVATE KEY-----\r\n"									\
		"MIIEowIBAAKCAQEAkozKhzUhq6nd0ik84SRoIAKlaN+Ea+K1PPOfC3ibockFqBXp\r\n"	\
		"bXlmIhet1So7L4yIL4Yx8jVpNHNXLfkctjMRSgyhDxoT1VqLUP5Ks6vKLL1SQAbQ\r\n"	\
		"HFe/m0In+u+V2zYekRpfiO+vzdJobAqYO9eIM+6Trc1xiEGGbZfz6E95ldSX+swY\r\n"	\
		"d4uMkuOtWsvFH7Ns2WXNvvCc73JcnkyaWI7SlnlhSn8K1Cf+LSdfr+OdjmAHnxPN\r\n"	\
		"SC1dEm9qwQ+KQymCbN0wkWXA/3SRcNtfDJt9/dG58P2wpCskKNbR9Lved3C7Lowc\r\n"	\
		"+EZ0ic60dlSDe/KJWw+3K82+/nVfx0Hba9MOcQIDAQABAoIBAB75CJjY3tvkE9Cm\r\n"	\
		"DIrc4fDZ/lGS4+7VRE60gomvHN1tmfdzYhlUDgTokkG6IjYjcmjw6L9zEGAYfHVn\r\n"	\
		"7+yGEIJg9u01Krnt4AHnLKyagyk/fhGwHu3Okd1jdwWu+zIQVxd9xnEvjy1l6dHj\r\n"	\
		"z1beb5fiNW4HPJZ6msmw0sjnex/yM4dP6Jc9uDQi0pOy0QjCLi92kViCIE07KrBf\r\n"	\
		"bX+pEayCg9DvT0b2ZjF3T0lTJjFhsb9tGgD2hVnBHoawQ4JLwHI/xnTUYnrHCMRZ\r\n"	\
		"gS2XCo+PHFYqh3hlsBFI0EgRbHZGs5vll9oBplNV9xx6frPSQtTJG79cfSPn0w6u\r\n"	\
		"B79phL0CgYEAzQEKptGUKXAt/zYo9LVA3N0vfX/y1AxS5Ws5rl5ZQAvSdVZRaso/\r\n"	\
		"JOszQNHequYn0CKEBKVHMHybn/o8yVAMR7d+NEGkI4tLGkWKguetXPqXSJXEpaY/\r\n"	\
		"eT/wv4RC2SnU2UNuKqjobJ18Arq7jiEPcwrtlBiNziEKpPdN4OnBiWMCgYEAtwFO\r\n"	\
		"jxtEvxaabIoUJzxPuV+53/bFlUvRMa1Z+HhJLRS2VHtpaW/FrHoq9mXDwDVyI2TZ\r\n"	\
		"+5KbPZo8Zbyf9dq/4R/J6W4IkDekGtLti0znUxt7uYGv7beS1R/GdOW7DHFjSpw6\r\n"	\
		"8zka7wJUiMR+rE6BJ723e8z07hEYK2EhfistexsCgYEAuSWorhL4AhjLogQTJzcP\r\n"	\
		"1ql4+5p0ACkFMSgPFzkk2CAVOl3z+EOilcBKMM+aj7R/3o1duChhTBwuHWTOQ26l\r\n"	\
		"OJwzQhTKnkNuV9LYjvOYcjHsMeT5jjXAe8xQrVdRXHpYPsSUmbik+XueBYUKYQng\r\n"	\
		"vyDuguNOJw1WZLjpwCi930sCgYBRN35C2oo29/QOqXTqOMT08vvN3nmvmVc84b8l\r\n"	\
		"G1T2cdO9SIvupBEpS4qXkXA/dDi0ZoSrNlQ5EaMuT3j6JluzsGTueMvKHTdyRBvy\r\n"	\
		"D242HuNY36pRKIA8n3520KGjkwrKyO0MllJSskkL7ZB+LdT56yNsCPjGUsXUMqYn\r\n"	\
		"lUf48wKBgBE+QpfnTfMtq5Zn+692ySlnfZMjJzDcGvs9wsw0l+MkNFdk/bTHN6iZ\r\n"	\
		"QyL/j5uniB5wtNBFq3cu2KKPRx7ID7JDxKqUV2x3pqnKFU2XhpRdM46EvZ0q8SvE\r\n"	\
		"OCfEEZyiCeO3qsULJ8nKOl2YEhyPJykysA7VzWkkYGiJPSpjoQUj\r\n"				\
		"-----END RSA PRIVATE KEY-----\r\n"



osThreadId_t StartTlsServer (void *);
void RunAppTlsServer (uint32_t);

#endif /* INC_TLS_PROC_H_ */
