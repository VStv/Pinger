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

//#include "mbedtls/ssl.h"
//#include "mbedtls/entropy.h"
//#include "mbedtls/ctr_drbg.h"


#include "mbedtls/certs.h"
#include "mbedtls/net_sockets.h"

//#include "mbedtls/x509.h"
//#include "mbedtls/error.h"


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
		"MIIDBDCCAeygAwIBAgIUGZIIJoEY/S7KolPdQfv1ZTPXCEkwDQYJKoZIhvcNAQEL\r\n"	\
		"BQAwIDEeMBwGA1UEAwwVQ2VydGlmaWNhdGUgYXV0aG9yaXR5MB4XDTI1MDExNTA4\r\n"	\
		"MTEyN1oXDTI2MDExNTA4MTEyN1owFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjAN\r\n"	\
		"BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAquReoudvT23pkBSm6pqZq2rIf3az\r\n"	\
		"CJK3szs3nQzPCuP3vEpZ6U+3OecFhACB+vM2KHxc6c+MH/BAi2UuNqXiVGO0gjdj\r\n"	\
		"6o8WJm94ggkUAkZAxTwx1+Zn5WJ8iDColjGOLmy+lukyBVVP96i3NRVUk2Z7qFiu\r\n"	\
		"19MjPJ7Ikw3C0amuV67IiojhSTLCPcJORk7M/2x6VA1lZlHxsPfHKnfHTdUdCSh7\r\n"	\
		"cgGuIyoLN1B/CndI3QSb2w8IiJflbiutORiWCpIbmCQCGtzLqKqcJZYFa6hu0rNa\r\n"	\
		"v74o2F2n66jeD0No9j7nNKlxkdgMLKh211kPQ/DYGGLtaz5iWM13MKQWuwIDAQAB\r\n"	\
		"o0IwQDAdBgNVHQ4EFgQUzthPxU0rP7u9GmIS2dE3+GY+t3gwHwYDVR0jBBgwFoAU\r\n"	\
		"GscY553tcBU9fkmVU7u54WQLe8kwDQYJKoZIhvcNAQELBQADggEBACmX5KEP0f5p\r\n"	\
		"QWHsdRQLeha78cPshctnrg0vkeA31Rou1O8O6mmx43y35nF7d6LZPUN1U3OXKKFN\r\n"	\
		"lH9106oZUWVGvSEaqyQY6Kg7iAla9Y9kNs94Gy1SQVuPw3BJDnogICbPZoOj5jk7\r\n"	\
		"pbUX/CeadhZ032TSo3WDBdlUAIiogLqR0JsNiyTOw9ft/lmQX3/kuszibvSJkxSM\r\n"	\
		"VLKZR0Rx7xMoAkWFNOPqqFrgI5p2z2xdqTz+OG+QBdDJs/zbXdYlRYOoWElDhOU3\r\n"	\
		"JXhUggYODAMStfm6vpisXttUSisoz+SzWjYad3sh2svwSVqok3gkoCjqayqOsSiV\r\n"	\
		"UdsirqwKX3c=\r\n"														\
		"-----END CERTIFICATE-----\r\n"

#define SERVER_SERT_KEY															\
		"-----BEGIN RSA PRIVATE KEY-----\r\n"									\
		"MIIEowIBAAKCAQEAquReoudvT23pkBSm6pqZq2rIf3azCJK3szs3nQzPCuP3vEpZ\r\n"	\
		"6U+3OecFhACB+vM2KHxc6c+MH/BAi2UuNqXiVGO0gjdj6o8WJm94ggkUAkZAxTwx\r\n"	\
		"1+Zn5WJ8iDColjGOLmy+lukyBVVP96i3NRVUk2Z7qFiu19MjPJ7Ikw3C0amuV67I\r\n"	\
		"iojhSTLCPcJORk7M/2x6VA1lZlHxsPfHKnfHTdUdCSh7cgGuIyoLN1B/CndI3QSb\r\n"	\
		"2w8IiJflbiutORiWCpIbmCQCGtzLqKqcJZYFa6hu0rNav74o2F2n66jeD0No9j7n\r\n"	\
		"NKlxkdgMLKh211kPQ/DYGGLtaz5iWM13MKQWuwIDAQABAoIBAAXX80h8w3ii2Ia9\r\n"	\
		"vgttp+2NpDd/lpWndrKhRsCPDJFhxDnjDPoGaMyJEs41ujwbjvGJdx/jofYBoCNk\r\n"	\
		"HVVvDLM4CZceT8NYizhbPXKs3stJHbPg4A9y6ICWgo2hpFImdacuvsGoTbaS+T4N\r\n"	\
		"vd4J0a+MpJPYHHpy1NSg1Vj58nx604MQJia34U3kgZlMO8trDjMZBE875qjIb6v5\r\n"	\
		"4NwC0O9SoM24YuvofFj5dVmZW6exHFmzDFc6cCRBaxYh6gG4tipnUoQhX3weu+eu\r\n"	\
		"9OfUpWxN+ydrfmQ6iQ3aPIrh8CPBmd/t3eopelCOJzI5zFAtFEMVwlI8lEhn0wAF\r\n"	\
		"Rf7xo+kCgYEA58mlHe6uqELRa/DlcsU0V2z6ze5jFyRDyjmQ0URUpvA2f7tUIb22\r\n"	\
		"YFfB66/x2XvPR2ex4wDBeGKuWnyweAyuFRhfxiWX9hXp8UNvsIm7QNgzjXOIC6d4\r\n"	\
		"el5l+s2H6CrYiMyfjrz4in0gmGjsIeBCZEK8/joS2BldApM+vj5fdR0CgYEAvL5I\r\n"	\
		"ldVPFREj0pwpJ9pWeeU9TrItsBQ8e0SBQNl+hTgWFuyYE01SeLG54M6vfvCySDD3\r\n"	\
		"GeuxIizRDrUNEc1nqSS/McEnOL42XOsDyrvG+e3iYQpVQTCcz3IqlG3shjSjzoKG\r\n"	\
		"zY4JRBBvvb7TAMbzbwNBoIXoEgI/HU4JaJ1Rq7cCgYBn0x8vJTb/D88W9rUQj90+\r\n"	\
		"PAasL9gbCZeEAf0of98bWAZReOvaoUwMI8Mte4Zt0NOsPHqmIDSJZEqNJcU2QRfJ\r\n"	\
		"Qz3DWBuVk4NTGs3w2gESrsWI2vNZpQ6GYbp0eZQjHu4XePEP0v3RqvLq0jTTh8y1\r\n"	\
		"dF+L0R+XxOSwvpwgQ3gm0QKBgFgxVbtle9ltM97yhyyPEj9NBZOjIEQZgJVc0kSa\r\n"	\
		"HEtlhLTbgsfqJnItIZzRFyHqmHOxJZVgE1nTtS/5G41I/HoFqK04Avq5rq9GRXRS\r\n"	\
		"v8wDAvezG1klvPAV+Z13q8CeEjiptxGPn/bE82GnK/M+A3vI+r5mM6VOlW09DJps\r\n"	\
		"gEALAoGBAKJpXSfe308bqKVZB3atRMcU9jn9ooAJz9h6yq3n2nnJYvWtZomj6ySl\r\n"	\
		"6VUiFhQXXWfOSrGqLoUwquFEyGovE88RsTn5SQR+OD9FikOQE9ZST3UqkSwwXHnv\r\n"	\
		"NDnz/0Krzi3pUNCgyXkg30o+3Y6b6Cs1TvWyEtjiasrsTv9eXHyG\r\n"				\
		"-----END RSA PRIVATE KEY-----\r\n"


osThreadId_t StartTlsServer (void *);
void RunAppTlsServer (uint32_t);

#endif /* INC_TLS_PROC_H_ */
