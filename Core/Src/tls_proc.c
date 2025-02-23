/*
 * tls_proc.c
 *
 *  Created on: Nov 16, 2024
 *      Author: dis_stv
 */

#include "tls_proc.h"

osThreadId_t 					TlsContextTaskHandle = NULL;
osThreadId_t 					TlsServerTaskHandle = NULL;
net_struct_t					TlsServerStruct;
osSemaphoreId_t 				sid_TlsContextProcessed = NULL;


mbedtls_x509_crt 				srvcert;
mbedtls_pk_context 				pkey;

uint32_t						rcv_timeout;


extern mbedtls_ssl_context 		ssl;
extern mbedtls_ssl_config 		conf;
extern mbedtls_ctr_drbg_context ctr_drbg;
extern mbedtls_entropy_context 	entropy;

#ifdef DEBUG_TLS_PROC
extern char 					*pp;
#endif


static const char *pers = "ssl_server";

const char serv_cert[] = SERVER_SERT;
const size_t serv_cert_len = sizeof (serv_cert);
const char serv_key[] = SERVER_SERT_KEY;
const size_t serv_key_len = sizeof (serv_key);



static void TlsContext1_thread 	(
								void *arg
								)
{
//	net_struct_t *pTlsServer = (net_struct_t *)arg;
	int ret = 1;
	int len;
	data_struct_t RW_data = {NULL};

	// 6. Handshake
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsContext_thread: Performing the SSL/TLS handshake... ");
#endif
	while ((ret = mbedtls_ssl_handshake (&ssl)) != 0)
	{
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! mbedtls_ssl_handshake returned %d\r\n", ret);
#endif
			goto exit2;
		}
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
	int read_num = 0, write_num = 0;
#endif

	while (1)
	{
		// 7. Read the HTTP Request
#ifdef DEBUG_TLS_PROC
		PRINTF("  < Read from client:");
#endif
		do
		{
			ret = mbedtls_ssl_read (&ssl, (unsigned char *) RW_data.r_data, len);
			if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
			{
				continue;
			}
			if (ret <= 0)
			{
				switch (ret)
				{
					case MBEDTLS_ERR_SSL_TIMEOUT:
#ifdef DEBUG_TLS_PROC
						PRINTF(" reading timeout\n");
#endif
						break;

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
//			PRINTF(" %d bytes read\n\n%s", len, (char *) RW_data.r_data);
			read_num++;
			PRINTF(" %d bytes read %d times\r\n", len, read_num);
#endif
			if (ret > 0)
			{
				break;
			}
		} while (1);
		if (ret < 0)
			break;

		// Application
		TlsServerStruct.application ((void *)&RW_data);
		// 8. Write the 200 Response
#ifdef DEBUG_TLS_PROC
		PRINTF("  > Write to client:");
#endif

		while ((ret = mbedtls_ssl_write (&ssl, (const unsigned char *) RW_data.w_data, (size_t)strlen (RW_data.w_data))) <= 0)
		{
			if (ret == MBEDTLS_ERR_NET_CONN_RESET)
			{
#ifdef DEBUG_TLS_PROC
				PRINTF(" failed\n  ! peer closed the connection\r\n");
#endif
				goto exit2;
			}
			if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
			{
#ifdef DEBUG_TLS_PROC
				PRINTF(" failed\n  ! mbedtls_ssl_write returned %d\r\n", ret);
#endif
				goto exit2;
			}
		}
#ifdef DEBUG_TLS_PROC
//		PRINTF(" %d bytes written\n\n%s\n", ret, (char *)RW_data.w_data);
		write_num++;
		PRINTF(" %d bytes written %d times\r\n", ret, write_num);
#endif
		if (RW_data.w_data != NULL)
		{
			vPortFree (RW_data.w_data);
		}
	}

#ifdef DEBUG_TLS_PROC
	PRINTF("  . Closing the connection...");
#endif
	while ((ret = mbedtls_ssl_close_notify (&ssl)) < 0)
	{
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! mbedtls_ssl_close_notify returned %d\r\n", ret);
#endif
			goto exit2;
		}
	}
	if (ret == 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF(" ok\n");
#endif
	}

exit2:
    osSemaphoreRelease (sid_TlsContextProcessed);
    TlsContextTaskHandle = NULL;
    osThreadExit ();
}


static void TlsContext_thread 	(
								void *arg
								)
{
//	net_struct_t *pTlsServer = (net_struct_t *)arg;
	int ret = 1;
	int len;
	data_struct_t RW_data = {NULL};

	// 6. Handshake
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsContext_thread: Performing the SSL/TLS handshake... ");
#endif
	while ((ret = mbedtls_ssl_handshake (&ssl)) != 0)
	{
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! mbedtls_ssl_handshake returned %d\r\n", ret);
#endif
			goto exit2;
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
		ret = mbedtls_ssl_read (&ssl, (unsigned char *) RW_data.r_data, len);
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
		PRINTF(" %d bytes read\n\n%s", len, (char *) RW_data.r_data);
#endif
		if (ret > 0)
		{
			break;
		}
	} while (1);

	// Application
	TlsServerStruct.application ((void *)&RW_data);
	// 8. Write the 200 Response
#ifdef DEBUG_TLS_PROC
	PRINTF("  > Write to client:");
#endif

	while ((ret = mbedtls_ssl_write (&ssl, (const unsigned char *) RW_data.w_data, (size_t)strlen (RW_data.w_data))) <= 0)
	{
		if (ret == MBEDTLS_ERR_NET_CONN_RESET)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! peer closed the connection\r\n");
#endif
			goto exit2;
		}
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! mbedtls_ssl_write returned %d\r\n", ret);
#endif
			goto exit2;
		}
	}
#ifdef DEBUG_TLS_PROC
	PRINTF(" %d bytes written\n\n%s\n", ret, (char *)RW_data.w_data);
#endif
	if (RW_data.w_data != NULL)
	{
		vPortFree (RW_data.w_data);
	}

#ifdef DEBUG_TLS_PROC
	PRINTF("  . Closing the connection...");
#endif
	while ((ret = mbedtls_ssl_close_notify (&ssl)) < 0)
	{
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF(" failed\n  ! mbedtls_ssl_close_notify returned %d\r\n", ret);
#endif
			goto exit2;
		}
	}
	if (ret == 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF(" ok\n");
#endif
	}

exit2:
    osSemaphoreRelease (sid_TlsContextProcessed);
    TlsContextTaskHandle = NULL;
    osThreadExit ();
}


static void TlsServer_thread 	(
								void *arg
								)
{
	net_struct_t *pTlsServer = (net_struct_t *)arg;
	int ret = 1;
	char port_buf[10];
	char client_adr[14];
	size_t len_ip;
	mbedtls_net_context	listen_fd, client_fd;

	MX_MBEDTLS_Init();
	listen_fd.fd = -1;
	client_fd.fd = -1;
	mbedtls_pk_init (&pkey);

#ifdef DEBUG_TLS_PROC
	uint16_t client_port;
	uint8_t deb_var = 0;
#endif

	// 1. Load the certificates and private RSA key
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Loading the certificate ... ");
#endif
	ret = mbedtls_x509_crt_parse (&srvcert, (const unsigned char*) serv_cert, serv_cert_len);
	if (ret != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  !  mbedtls_x509_crt_parse returned %d\r\n", ret);
#endif
		goto exit1;
	}
	ret =  mbedtls_pk_parse_key (&pkey, (const unsigned char *) serv_key, serv_key_len, NULL, 0);
	if ( ret != 0 )
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  !  mbedtls_pk_parse_key returned %d\r\n", ret);
#endif
		goto exit1;
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
		goto exit1;
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
		goto exit1;
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
		goto exit1;
	}
	mbedtls_ssl_conf_rng (&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_ca_chain (&conf, srvcert.next, NULL);
	if ((ret = mbedtls_ssl_conf_own_cert (&conf, &srvcert, &pkey)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  ! mbedtls_ssl_conf_own_cert returned %d\r\n", ret);
#endif
		goto exit1;
	}

	mbedtls_ssl_conf_read_timeout( &conf, 1000 );

	if ((ret = mbedtls_ssl_setup (&ssl, &conf)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  ! mbedtls_ssl_setup returned %d\r\n", ret);
#endif
		goto exit1;
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
#endif

	mbedtls_net_free (&client_fd);
	mbedtls_ssl_session_reset (&ssl);
	sid_TlsContextProcessed = osSemaphoreNew (1, 0, NULL);
	vQueueAddToRegistry (sid_TlsContextProcessed, "sid_TlsContextProcessed");

	while (1)
	{
		// 5. Wait until a client connects
#ifdef DEBUG_TLS_PROC
		PRINTF("TlsServerThread: Waiting for a remote connection... ");
#endif
		if ((ret = mbedtls_net_accept (&listen_fd, &client_fd, (void *)&client_adr, sizeof(client_adr), &len_ip)) != 0)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF("failed\n  ! mbedtls_net_accept returned %d\r\n", ret);
#endif
			break;
		}
		mbedtls_ssl_set_bio (&ssl, &client_fd, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
#ifdef DEBUG_TLS_PROC
		PRINTF("ok\r\n\r\n");
		++deb_var;
		client_port = ((uint16_t)client_adr[4] << 8) + client_adr[5];
		PRINTF("TlsServerThread: Connection %d to port %d\r\n", deb_var, client_port);
#endif
		const osThreadAttr_t tlsContext_attributes = {
			.name = "TlsContextTask",
			.priority = (osPriority_t) osPriorityNormal,
			.stack_size = 512 * 10,
		};
		TlsContextTaskHandle = osThreadNew (TlsContext1_thread, NULL, &tlsContext_attributes);

		osSemaphoreAcquire (sid_TlsContextProcessed, osWaitForever);
		mbedtls_net_free (&client_fd);
		mbedtls_ssl_session_reset (&ssl);
	}

exit1:
	mbedtls_net_free ( &client_fd );
	mbedtls_net_free ( &listen_fd );
	mbedtls_x509_crt_free ( &srvcert );
	mbedtls_pk_free ( &pkey );
	mbedtls_ssl_free ( &ssl );
	mbedtls_ssl_config_free ( &conf );
	mbedtls_ctr_drbg_free ( &ctr_drbg );
	mbedtls_entropy_free ( &entropy );

	if (sid_TlsContextProcessed != NULL)
	{
		osSemaphoreDelete (sid_TlsContextProcessed);
	}
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
			pTlsServer->application = HttpServer;
			break;
		default:
			return NULL;
	}
    const osThreadAttr_t tlsTask_attributes = {
        .name = "TlsServerTask",
        .stack_size = 512 * 8,
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

