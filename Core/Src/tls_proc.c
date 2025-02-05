/*
 * tls_proc.c
 *
 *  Created on: Nov 16, 2024
 *      Author: dis_stv
 */

#include "tls_proc.h"


osThreadId_t 					TlsServerTaskHandle = NULL;
net_struct_t					TlsServerStruct;

mbedtls_x509_crt 				srvcert;
mbedtls_pk_context 				pkey;

static mbedtls_net_context 		listen_fd, client_fd;
static uint8_t 					buf[1024];


extern mbedtls_ssl_context 		ssl;
extern mbedtls_ssl_config 		conf;
//extern mbedtls_x509_crt 		cert;
extern mbedtls_ctr_drbg_context ctr_drbg;
extern mbedtls_entropy_context 	entropy;

#ifdef DEBUG_TLS_PROC
extern char 					*pp;
#endif


static const char *pers = "ssl_server";
/**/
const char serv_cert[] = SERVER_SERT;
const size_t serv_cert_len = sizeof (serv_cert);
const char serv_key[] = SERVER_SERT_KEY;
const size_t serv_key_len = sizeof (serv_key);



static void TlsContext_thread 	(
								void *arg
								)
{

}


static void TlsServer_thread 	(
								void *arg
								)
{
	net_struct_t *pTlsServer = (net_struct_t *)arg;
	int ret = 1, len;
	char port_buf[10];

	MX_MBEDTLS_Init();
	listen_fd.fd = -1;
	client_fd.fd = -1;
	mbedtls_pk_init( &pkey );

	// 1. Load the certificates and private RSA key
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Loading the certificate ... ");
#endif
//	ret = mbedtls_x509_crt_parse (&srvcert, (const unsigned char*) mbedtls_google_root_certificate, mbedtls_google_root_certificate_len);
	ret = mbedtls_x509_crt_parse (&srvcert, (const unsigned char*) serv_cert, serv_cert_len);
	if (ret != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  !  mbedtls_x509_crt_parse returned %d\r\n", ret);
#endif
		goto exit;
	}

	ret =  mbedtls_pk_parse_key (&pkey, (const unsigned char *) serv_key, serv_key_len, NULL, 0);
	if ( ret != 0 )
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  !  mbedtls_pk_parse_key returned %d\r\n", ret);
#endif
		goto exit;
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
#endif

	// 2. Setup the listening TCP socket
	itoa (pTlsServer->port, port_buf, 10);
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Bind on https://localhost:%s/ ... ", port_buf);
#endif
	if ((ret = mbedtls_net_bind (&listen_fd, NULL, port_buf, MBEDTLS_NET_PROTO_TCP)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  ! mbedtls_net_bind returned %d\r\n", ret);
#endif
		goto exit;
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
#endif

	// 3. Seed the RNG
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Seeding the random number generator... ");
#endif
	if ((ret = mbedtls_ctr_drbg_seed (&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen( (char *)pers))) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  ! mbedtls_ctr_drbg_seed returned %d\r\n", ret);
#endif
		goto exit;
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
#endif

	// 4. Setup stuff
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Setting up the SSL data.... ");
#endif
	if ((ret = mbedtls_ssl_config_defaults (&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  ! mbedtls_ssl_config_defaults returned %d\r\n", ret);
#endif
		goto exit;
	}
	mbedtls_ssl_conf_rng (&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_ca_chain (&conf, srvcert.next, NULL);
	if((ret = mbedtls_ssl_conf_own_cert (&conf, &srvcert, &pkey)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  ! mbedtls_ssl_conf_own_cert returned %d\r\n", ret);
#endif
		goto exit;
	}
	if ((ret = mbedtls_ssl_setup (&ssl, &conf)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  ! mbedtls_ssl_setup returned %d\r\n", ret);
#endif
		goto exit;
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
#endif

//	HAL_GPIO_WritePin (LED_GREEN_GPIO_Port, LED_GREEN_Pin, 1);
//	goto exit;


reset:
//	mbedtls_net_free(&client_fd);
//	listen_fd.fd = -1;
	client_fd.fd = -1;
	mbedtls_ssl_session_reset(&ssl);

	// 5. Wait until a client connects
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Waiting for a remote connection... ");
#endif
	if ((ret = mbedtls_net_accept (&listen_fd, &client_fd, NULL, 0, NULL)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  ! mbedtls_net_accept returned %d\r\n", ret);
#endif
		goto exit;
	}
	mbedtls_ssl_set_bio(&ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
#endif

	// 6. Handshake
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Performing the SSL/TLS handshake... ");
#endif
	while ((ret = mbedtls_ssl_handshake (&ssl)) != 0)
	{
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! mbedtls_ssl_handshake returned %d\r\n", ret);
#endif
			goto reset;
		}
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
#endif

	// 7. Read the HTTP Request
#ifdef DEBUG_TLS_PROC
	PRINTF("  < Read from client:");
#endif
	do
	{
		len = sizeof (buf) - 1;
		memset (buf, 0, sizeof (buf));
		ret = mbedtls_ssl_read (&ssl, buf, len);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			continue;
		}
		if (ret <= 0)
		{
			switch (ret)
			{
				case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
#ifdef DEBUG_TLS_PROC
					PRINTF(" connection was closed gracefully\n");
#endif
					break;

				case MBEDTLS_ERR_NET_CONN_RESET:
#ifdef DEBUG_TLS_PROC
					PRINTF(" connection was reset by peer\n");
#endif
					break;

				default:
#ifdef DEBUG_TLS_PROC
					PRINTF(" mbedtls_ssl_read returned %d\n", ret);
#endif
					break;
			}
			HAL_Delay (200);
			break;
		}
		len = ret;
#ifdef DEBUG_TLS_PROC
		PRINTF(" %d bytes read\n\n%s", len, (char *) buf);
#endif
		if (ret > 0)
		{
			break;
		}
	} while (1);

	// Application



	// 8. Write the 200 Response
#ifdef DEBUG_TLS_PROC
	PRINTF("  > Write to client:");
#endif
	len = sprintf ((char *)buf, HTTP_RESPONSE, mbedtls_ssl_get_ciphersuite(&ssl));
	while((ret = mbedtls_ssl_write (&ssl, buf, len)) <= 0)
	{
		if(ret == MBEDTLS_ERR_NET_CONN_RESET)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! peer closed the connection\r\n");
#endif
			goto reset;
		}
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! mbedtls_ssl_write returned %d\r\n", ret);
#endif
			goto exit;
		}
	}
	len = ret;
#ifdef DEBUG_TLS_PROC
	PRINTF(" %d bytes written\n\n%s\n", len, (char *)buf);
	PRINTF("  . Closing the connection...");
#endif
	while((ret = mbedtls_ssl_close_notify(&ssl)) < 0)
	{
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! mbedtls_ssl_close_notify returned %d\r\n", ret);
#endif
			goto reset;
		}
	}
	if (ret == 0)
	{
#ifdef DEBUG_TLS_PROC
			PRINTF(" ok\n");
#endif
	}
	ret = 0;
	goto reset;
exit:
	mbedtls_net_free( &client_fd );
	mbedtls_net_free( &listen_fd );
	mbedtls_x509_crt_free( &srvcert );
	mbedtls_pk_free( &pkey );
	mbedtls_ssl_free( &ssl );
	mbedtls_ssl_config_free( &conf );
	mbedtls_ctr_drbg_free( &ctr_drbg );
	mbedtls_entropy_free( &entropy );

	TlsServerTaskHandle = NULL;
	osThreadExit ();
}


osThreadId_t StartTlsServer (
							void *arg
							)
{
	uint32_t app = *(uint32_t *)arg;
	net_struct_t *pTlsServer = &TlsServerStruct;

	switch (app)
	{
		case HTTPS_PROT:
			// Set local port
			pTlsServer->port = HTTPS_SERVER_PORT;
			pTlsServer->application = HttpProcess;
			break;
		default:
			return NULL;
	}
    const osThreadAttr_t tlsTask_attributes = {
        .name = "TlsServerTask",
        .stack_size = 512 * 10,
        .priority = (osPriority_t) osPriorityNormal,
    };
	return osThreadNew(TlsServer_thread, (void *)pTlsServer, &tlsTask_attributes);
}


void RunAppTlsServer 	(
						uint32_t app
						)
{
	TlsServerTaskHandle = StartTlsServer ((void *)&app);
}
