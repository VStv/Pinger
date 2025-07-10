/*
 * tls_proc.c
 *
 *  Created on: Nov 16, 2024
 *      Author: dis_stv
 */

#include <tls_proc.h>

osThreadId_t 					TlsServerTaskHandle = NULL;
net_struct_t					TlsServerStruct;

mbedtls_x509_crt 				srvcert;
mbedtls_pk_context 				pkey;

const char 						serv_cert[] = SERVER_SERT;
const size_t 					serv_cert_len = sizeof (serv_cert);
const char 						serv_key[] = SERVER_SERT_KEY;
const size_t 					serv_key_len = sizeof (serv_key);

extern mbedtls_ssl_config 		conf;
extern mbedtls_ctr_drbg_context ctr_drbg;
extern mbedtls_entropy_context 	entropy;


#ifdef DEBUG_TLS_PROC
extern char 					*pp;
#endif

#if TLS_PROC == 1 || TLS_PROC == 2
static const char 				*pers = "ssl_server";
#endif


#if TLS_PROC == 1

osThreadId_t 					TlsContextTaskHandle = NULL;
osSemaphoreId_t 				sid_TlsContextProcessed = NULL;

extern mbedtls_ssl_context 		ssl;



static void TlsContext_thread 	(
								void *arg
								)
{
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
/*
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
//				HAL_Delay (200);
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
*/
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

/*
static void TlsContext1_thread 	(
								void *arg
								)
{
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
//			HAL_Delay (200);
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
*/

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

	mbedtls_ssl_conf_read_timeout( &conf, 2000 );

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
		TlsContextTaskHandle = osThreadNew (TlsContext_thread, NULL, &tlsContext_attributes);

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



#elif TLS_PROC == 2

osThreadId_t 					TlsContextTaskHandle[TLS_CONTEXT_MAX] = {NULL};
context_struct_t 				TlsContextArray[TLS_CONTEXT_MAX];
mbedtls_ssl_context 			ssl_inst[TLS_CONTEXT_MAX];


static void TlsContext_thread 	(
								void *arg
								)
{
	context_struct_t *pTlsContext = (context_struct_t *)arg;
	int ret = 1;
	int len;
	data_struct_t RW_data = {NULL};
	int goexit = 0;

//----------------------------------------------------------------------------------------
	// 6. Handshake
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsContextThread: Performing the SSL/TLS handshake... ");
#endif
	while ((ret = mbedtls_ssl_handshake (pTlsContext->ssl)) != 0)
	{
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
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
//----------------------------------------------------------------------------------------

#ifdef DEBUG_TLS_PROC
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
			ret = mbedtls_ssl_read (pTlsContext->ssl, (unsigned char *) RW_data.r_data, len);
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
//				HAL_Delay (200);
				break;
			}
			len = ret;
#ifdef DEBUG_TLS_PROC
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

		while ((ret = mbedtls_ssl_write (pTlsContext->ssl, (const unsigned char *) RW_data.w_data, (size_t)strlen (RW_data.w_data))) <= 0)
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
		write_num++;
		PRINTF(" %d bytes written %d times\r\n", ret, write_num);
#endif
		if (RW_data.w_data != NULL)
		{
			vPortFree (RW_data.w_data);
		}
	}

	osDelay (200);

#ifdef DEBUG_TLS_PROC
	PRINTF("TlsContextThread:  Closing the connection...");
#endif

//	osThreadSuspend (TlsServerTaskHandle);
	while ((ret = mbedtls_ssl_close_notify (pTlsContext->ssl)) < 0)
	{
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			goexit = 1;
			break;
		}
	}
//	osThreadResume (TlsServerTaskHandle);

	if(goexit)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF(" failed\n  ! mbedtls_ssl_close_notify returned %d\r\n", ret);
#endif
		goexit = 0;
		goto exit2;
	}
	if (ret == 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF(" ok\n");
#endif
	}

exit2:
	mbedtls_net_free (pTlsContext->sock);
	mbedtls_ssl_session_reset (pTlsContext->ssl);
	pTlsContext->ssl = NULL;
#ifdef DEBUG_TLS_PROC
	PRINTF ("TlsContextThread: Socket %c closed\r\n\n", pTlsContext->number);
#endif
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
	context_struct_t *pTlsContext;
	mbedtls_net_context	listen_fd;
	mbedtls_net_context	sock_inst[TLS_CONTEXT_MAX];

	MX_MBEDTLS_Init();
	mbedtls_net_free (&listen_fd);
	for (uint8_t i = 0; i < TLS_CONTEXT_MAX; i++)
	{
		mbedtls_net_free (&sock_inst[i]);
		mbedtls_ssl_init (&ssl_inst[i]);
	}
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

	pTlsContext = TlsContextArray;
	for (uint8_t i = 0; i < TLS_CONTEXT_MAX; i++)
	{
		pTlsContext->number = '1'+i;
		pTlsContext->sock = &sock_inst[i];
		pTlsContext->ssl = NULL;
		if ((ret = mbedtls_ssl_setup (&ssl_inst[i], &conf)) != 0)
		{
#ifdef DEBUG_TLS_PROC
			PRINTF("failed\n  ! mbedtls_ssl_setup returned %d\r\n", ret);
#endif
			goto exit1;
		}
		pTlsContext++;

	}
#ifdef DEBUG_TLS_PROC
	PRINTF("ok\r\n");
#endif

	while (1)
	{
		pTlsContext = TlsContextArray;
		uint32_t sslOk = 0;
		for (uint8_t i = 0; i < TLS_CONTEXT_MAX; i++)
		{
			if (pTlsContext->ssl == NULL)
			{
				if ((ret = mbedtls_net_accept (&listen_fd, pTlsContext->sock, (void *)&client_adr, sizeof (client_adr), &len_ip)) != 0)
				{
#ifdef DEBUG_TLS_PROC
					PRINTF("failed\n  ! mbedtls_net_accept returned %d\r\n", ret);
#endif
					break;
				}
				pTlsContext->ssl = &ssl_inst[i];
				mbedtls_ssl_set_bio (pTlsContext->ssl, (void *)pTlsContext->sock, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
				char nameThread[] = {'T','L','S','C','o','n','t','e','x','t','T','a','s','k', pTlsContext->number,'\0'};
				const osThreadAttr_t tlsContext_attributes = {
					.name = nameThread,
					.priority = (osPriority_t) osPriorityNormal,
					.stack_size = 512 * 10,
				};
#ifdef DEBUG_TLS_PROC
				PRINTF("ok\r\n\r\n");
				++deb_var;
				client_port = ((uint16_t)client_adr[4] << 8) + client_adr[5];
				PRINTF("TlsServerThread: Connection %d to port %d use socket %c\r\n", deb_var, client_port, pTlsContext->number);
#endif
				TlsContextTaskHandle[i] = osThreadNew (TlsContext_thread, (void *)pTlsContext, &tlsContext_attributes);
//					mbedtls_net_free (&listen_fd);
				sslOk = 1;
				break;
			}
			pTlsContext++;
		}
		// if no free ssl - wait
		if (!sslOk)
		{
			osDelay (100);
		}
	}
exit1:
	for (uint8_t i = 0; i < TLS_CONTEXT_MAX; i++)
	{
		mbedtls_net_free (&sock_inst[i]);
	}
	mbedtls_net_free (&listen_fd);
	mbedtls_x509_crt_free (&srvcert);
	mbedtls_pk_free (&pkey);
	for (uint8_t i = 0; i < TLS_CONTEXT_MAX; i++)
	{
		mbedtls_ssl_free (&ssl_inst[i]);
	}
	mbedtls_ssl_config_free (&conf);
	mbedtls_ctr_drbg_free (&ctr_drbg);
	mbedtls_entropy_free (&entropy);
	TlsServerTaskHandle = NULL;
	osThreadExit ();
}

#else

//	#include "lwip.h"
//	#include "lwip/opt.h"


#include "lwip/api.h"


//	#include "lwip/sys.h"
//	#include "lwip/tcp.h"


osThreadId_t 		TlsContextTaskHandle[TLS_CONTEXT_MAX] = {NULL};
osSemaphoreId_t 	sid_TlsTaskCount = NULL;


static err_t my_netbuf_pull	(
							client_args_t *client,
							u16_t len
							)
{
	if (client->rx_buf == NULL || client->rx_len == 0) return ERR_ARG;

	if (client->rx_len > len)
	{
		client->rx_ptr += len;
		client->rx_len -= len;
		return ERR_OK;
	}
	else
	{
		netbuf_delete(client->rx_buf);
		client->rx_buf = NULL;
		client->rx_ptr = NULL;
		client->rx_len = 0;
		return ERR_OK;
	}
}


static int netconn_send_cb	(
							void *ctx,
							const unsigned char *buf,
							size_t len
							)
{
    client_args_t *client = (client_args_t *)ctx;
    err_t err = netconn_write(client->conn, buf, len, NETCONN_COPY);
    return (err == ERR_OK) ? (int)len : MBEDTLS_ERR_NET_SEND_FAILED;
}


static int netconn_recv_timeout_cb	(
									void *ctx,
									unsigned char *buf,
									size_t len,
									uint32_t timeout
									)
{
    client_args_t *client = (client_args_t *)ctx;
    void *data;
    u16_t data_len;
    err_t err;

    if (client->rx_buf == NULL) {
        netconn_set_recvtimeout(client->conn, timeout);
        err = netconn_recv(client->conn, &client->rx_buf);
        if (err != ERR_OK)
		{
        	PRINTF("LWIP receive failed: err = %d\r\n", err);
        	return MBEDTLS_ERR_NET_RECV_FAILED;
		}

        netbuf_data(client->rx_buf, &data, &data_len);
        client->rx_ptr = (uint8_t *)data;
        client->rx_len = data_len;
    }

    if (client->rx_len == 0) return MBEDTLS_ERR_NET_RECV_FAILED;

    size_t to_copy = (client->rx_len > len) ? len : client->rx_len;
    memcpy(buf, client->rx_ptr, to_copy);
    my_netbuf_pull(client, to_copy);

    PRINTF("LWIP receive got: bytes = %d\r\n", to_copy);
    return (int)to_copy;
}


static void TlsContext_thread 	(
								void *arg
								)
{
	client_args_t *args = (client_args_t *)arg;
    int ret = 0;

    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_x509_crt cert;
    mbedtls_pk_context key;

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_x509_crt_init(&cert);
    mbedtls_pk_init(&key);

    const char *pers = "ssl_server";
	// 1. Load the certificates and private RSA key
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Loading the certificate ... \r\n");
#endif
	ret = mbedtls_x509_crt_parse (&cert, (const unsigned char*) serv_cert, serv_cert_len);
	if (ret != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("... mbedtls_x509_crt_parse is failed\n  ! returned %d\r\n", ret);
#endif
		goto cleanup;
	}
	ret =  mbedtls_pk_parse_key (&key, (const unsigned char *) serv_key, serv_key_len, NULL, 0);
	if ( ret != 0 )
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("... mbedtls_pk_parse_key is failed\n ! returned %d\r\n", ret);
#endif
		goto cleanup;
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("... certificate loading is ok\r\n");
#endif

	// 3. Seed the RNG
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Seeding the random number generator...\r\n");
#endif
	if ((ret = mbedtls_ctr_drbg_seed (&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen( (char *)pers))) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("... mbedtls_ctr_drbg_seed is failed\n ! returned %d\r\n", ret);
#endif
		goto cleanup;
	}
#ifdef DEBUG_TLS_PROC
	PRINTF("... RNG seeding is ok\r\n");
#endif

	// 4. Setup stuff
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Setting up the SSL data...\r\n");
#endif
	if ((ret = mbedtls_ssl_config_defaults (&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("... mbedtls_ssl_config_defaults failed\n  ! returned %d\r\n", ret);
#endif
		goto cleanup;
	}
	mbedtls_ssl_conf_rng (&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_ca_chain (&conf, srvcert.next, NULL);
	if ((ret = mbedtls_ssl_conf_own_cert (&conf, &cert, &key)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("... mbedtls_ssl_conf_own_cert failed\n  ! returned %d\r\n", ret);
#endif
		goto cleanup;
	}

	if ((ret = mbedtls_ssl_setup (&ssl, &conf)) != 0)
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("... mbedtls_ssl_setup failed\n  ! returned %d\r\n", ret);
#endif
		goto cleanup;
	}

    mbedtls_ssl_conf_read_timeout (&conf, 5000);
#ifdef DEBUG_TLS_PROC
	PRINTF("... Setting up the SSL data is ok\r\n");
#endif

    // 5. I/O Callbacks
    mbedtls_ssl_set_bio (&ssl, args, netconn_send_cb, NULL, netconn_recv_timeout_cb);//mbedtls_net_recv_timeout);//

    // 6. Handshake
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsContextThread: Performing the SSL/TLS handshake...\r\n");
#endif
    if ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
#ifdef DEBUG_TLS_PROC
			PRINTF("...mbedtls_ssl_handshake is failed\n  ! returned %d\r\n", ret);
#endif
			goto cleanup;
    }
#ifdef DEBUG_TLS_PROC
	PRINTF("... TLS handshake is ok\r\n");
#endif
    char buf[1024];
	int len = mbedtls_ssl_read(&ssl, (unsigned char *)buf, sizeof(buf)-1);
	if (len > 0)
	{
		buf[len] = 0;
		// Вивід запиту:
		PRINTF("Received: %s\n", buf);

		const char *resp =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 12\r\n\r\n"
			"Hello HTTPS!";

		mbedtls_ssl_write(&ssl, (const unsigned char *)resp, strlen(resp));
    }

cleanup:
    // Завершення
	if (args->rx_buf)
	{
		netbuf_delete (args->rx_buf);
		args->rx_buf = NULL;
	}
    mbedtls_ssl_close_notify(&ssl);
    netconn_close(args->conn);
    netconn_delete(args->conn);

    // Free
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&cert);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    osSemaphoreRelease (sid_TlsTaskCount);    // release semaphore
    vPortFree(args);
    osThreadExit(); // Завершити задачу
}


static void TlsServer_thread 	(
								void *arg
								)
{
	uint32_t task_number;
	net_struct_t *pTlsServer = (net_struct_t *)arg;
    struct netconn *server_conn, *client_conn;
#ifdef DEBUG_TLS_PROC
	uint8_t deb_var = 0;
#endif

	sid_TlsTaskCount = osSemaphoreNew (TLS_CONTEXT_MAX, TLS_CONTEXT_MAX, NULL);	// create counting semaphore
    vQueueAddToRegistry (sid_TlsTaskCount, "sid_TlsTaskCount");

    server_conn = netconn_new(NETCONN_TCP);
    netconn_bind(server_conn, NULL, pTlsServer->port);
#ifdef DEBUG_TLS_PROC
	PRINTF("TlsServerThread: Bind on https://localhost:%d/ ... ", pTlsServer->port);
#endif
    netconn_listen (server_conn);

    while (1)
    {
    	if (netconn_accept (server_conn, &client_conn) == ERR_OK)
        {
    		if (osSemaphoreAcquire (sid_TlsTaskCount, 0) == osOK)	// take semaphore
    		{
				task_number = TLS_CONTEXT_MAX - osSemaphoreGetCount (sid_TlsTaskCount);
				client_args_t *args = pvPortMalloc(sizeof(client_args_t));
				args->conn = client_conn;
				args->rx_buf = NULL;
				args->rx_ptr = NULL;
				args->rx_len = 0;
				if (args != NULL)
				{
					args->conn = client_conn;
#ifdef DEBUG_TLS_PROC
					++deb_var;
PRINTF("TlsServerThread: Connection %d with remote host: %d.%d.%d.%d: %d\r\n",
					deb_var,
					(u8_t)(client_conn->pcb.tcp->remote_ip.addr),
					(u8_t)((client_conn->pcb.tcp->remote_ip.addr)>>8),
					(u8_t)((client_conn->pcb.tcp->remote_ip.addr)>>16),
					(u8_t)((client_conn->pcb.tcp->remote_ip.addr)>>24),
					client_conn->pcb.tcp->remote_port);
#endif
					char nameThread[] = {'T','L','S','C','o','n','t','e','x','t','T','a','s','k', task_number,'\0'};
					const osThreadAttr_t tlsContext_attributes = {
						.name = nameThread,
						.priority = (osPriority_t) osPriorityNormal,
						.stack_size = 8192
					};
					TlsContextTaskHandle[task_number-1] = osThreadNew (TlsContext_thread, args, &tlsContext_attributes);
				}
				else
				{
					// Не вистачило пам'яті — закриваємо
					netconn_close(client_conn);
					netconn_delete(client_conn);
				    osSemaphoreRelease (sid_TlsTaskCount);    // release semaphore
				}
    		}
    		else
    		{
				// Too many connections
				netconn_close(client_conn);
				netconn_delete(client_conn);
    		}
        }
        osDelay(1); // невелика пауза для RTOS
    }
    osSemaphoreDelete(sid_TlsTaskCount);	// delete semaphore
}

#endif


static osThreadId_t StartTlsServer	(
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


