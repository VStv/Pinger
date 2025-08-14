/*
 * tls_proc.c
 *
 *  Created on: Nov 16, 2024
 *      Author: dis_stv
 */

#include "tls_proc.h"

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

// ---------------------------------  TLS Server  ----------------------------------------

extern osSemaphoreId_t 			sid_TcpConnCount;

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

    if (client->rx_buf == NULL)
    {
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


static void tls_free_pTcpConn	(
								conn_struct_t *pTcpConn,
								const char* tag
								)
{
    if (pTcpConn == NULL) return;
    PRINTF("%s %ld: Freeing conn_struct at %p\r\n", tag, pTcpConn->number, pTcpConn);
    if (pTcpConn->conn != NULL)
    {
		PRINTF("%s %ld: Closing and deleting netconn %p\r\n", tag, pTcpConn->number, pTcpConn->conn);
		err_t err = netconn_close(pTcpConn->conn);
		if (err != ERR_OK)
		{
			PRINTF("%s %ld: netconn_close returned %d\r\n", tag, pTcpConn->number, err);
		}
		err = netconn_delete(pTcpConn->conn);
		if (err != ERR_OK)
		{
			PRINTF("%s %ld: netconn_delete returned %d\r\n", tag, pTcpConn->number, err);
		}
        pTcpConn->conn = NULL;
    }
    vPortFree(pTcpConn);
}



void TlsContext_thread (
						void *arg
						)
{
	conn_struct_t *pTcpConn = (conn_struct_t *)arg;
    int ret = 0;
    const char *tag = "TlsContextThread";

#ifdef DEBUG_TLS_PROC
	PRINTF("%s %ld: Connection %p in conn_struct_t %p with remote host: %d.%d.%d.%d: %d\r\n",
			tag,
			pTcpConn->number,
			pTcpConn->conn,
			pTcpConn,
			(u8_t)(pTcpConn->conn->pcb.tcp->remote_ip.addr),
			(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>8),
			(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>16),
			(u8_t)((pTcpConn->conn->pcb.tcp->remote_ip.addr)>>24),
			(u16_t)(pTcpConn->conn->pcb.tcp->remote_port));
#endif

	client_args_t client_struct;
	client_struct.conn = pTcpConn->conn;
	client_struct.rx_buf = NULL;
	client_struct.rx_ptr = NULL;
	client_struct.rx_len = 0;

    mbedtls_ssl_config conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_x509_crt cert;
    mbedtls_pk_context key;

//    mbedtls_ssl_context ssl;
//    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_x509_crt_init(&cert);
    mbedtls_pk_init(&key);

    const char *pers = "ssl_server";
	// 1. Load the certificates and private RSA key
#ifdef DEBUG_TLS_PROC
//	PRINTF("%s %ld: Loading the certificate ... \r\n", tag, pTcpConn->number);
#endif
	ret = mbedtls_x509_crt_parse (&cert, (const unsigned char*) serv_cert, serv_cert_len);
	if (ret != 0)
	{
#ifdef DEBUG_TLS_PROC
//		PRINTF("...%s %ld: mbedtls_x509_crt_parse is failed\n  ! returned %d\r\n", tag, pTcpConn->number, ret);
#endif
		goto cleanup;
	}
	ret =  mbedtls_pk_parse_key (&key, (const unsigned char *) serv_key, serv_key_len, NULL, 0);
	if ( ret != 0 )
	{
#ifdef DEBUG_TLS_PROC
//		PRINTF("...%s %ld: mbedtls_pk_parse_key is failed\n ! returned %d\r\n", tag, pTcpConn->number, ret);
#endif
		goto cleanup;
	}
#ifdef DEBUG_TLS_PROC
//	PRINTF("...%s %ld: certificate loading is ok\r\n", tag, pTcpConn->number);
#endif

	// 3. Seed the RNG
#ifdef DEBUG_TLS_PROC
//	PRINTF("%s %ld: Seeding the random number generator...\r\n", tag, pTcpConn->number);
#endif
	if ((ret = mbedtls_ctr_drbg_seed (&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen( (char *)pers))) != 0)
	{
#ifdef DEBUG_TLS_PROC
//		PRINTF("...%s %ld: mbedtls_ctr_drbg_seed is failed\n ! returned %d\r\n", tag, pTcpConn->number, ret);
#endif
		goto cleanup;
	}
#ifdef DEBUG_TLS_PROC
//	PRINTF("...%s %ld: RNG seeding is ok\r\n", tag, pTcpConn->number);
#endif

	// 4. Setup stuff
#ifdef DEBUG_TLS_PROC
//	PRINTF("%s %ld: Setting up the SSL data...\r\n", tag, pTcpConn->number);
#endif
	if ((ret = mbedtls_ssl_config_defaults (&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
#ifdef DEBUG_TLS_PROC
//		PRINTF("...%s %ld: mbedtls_ssl_config_defaults failed\n  ! returned %d\r\n", tag, pTcpConn->number, ret);
#endif
		goto cleanup;
	}
	mbedtls_ssl_conf_rng (&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_ca_chain (&conf, srvcert.next, NULL);
	if ((ret = mbedtls_ssl_conf_own_cert (&conf, &cert, &key)) != 0)
	{
#ifdef DEBUG_TLS_PROC
//		PRINTF("...%s %ld: mbedtls_ssl_conf_own_cert failed\n  ! returned %d\r\n", tag, pTcpConn->number, ret);
#endif
		goto cleanup;
	}
    mbedtls_ssl_conf_read_timeout (&conf, 1000);

    mbedtls_ssl_context ssl;
    mbedtls_ssl_init(&ssl);
	if ((ret = mbedtls_ssl_setup (&ssl, &conf)) != 0)
	{
#ifdef DEBUG_TLS_PROC
//		PRINTF("...%s %ld: mbedtls_ssl_setup failed\n  ! returned %d\r\n", tag, pTcpConn->number, ret);
#endif
		goto cleanup;
	}

#ifdef DEBUG_TLS_PROC
//	PRINTF("...%s %ld: Setting up the SSL data is ok\r\n", tag, pTcpConn->number);
#endif

    // 5. I/O Callbacks
    mbedtls_ssl_set_bio (&ssl, &client_struct, netconn_send_cb, NULL, netconn_recv_timeout_cb);

    // 6. Handshake
#ifdef DEBUG_TLS_PROC
	PRINTF("%s %ld: Performing the SSL/TLS handshake...\r\n", tag, pTcpConn->number);
#endif
    if ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
#ifdef DEBUG_TLS_PROC
		PRINTF("...%s %ld: mbedtls_ssl_handshake is failed\n  ! returned %d\r\n", tag, pTcpConn->number, ret);
#endif
		goto cleanup;
    }
#ifdef DEBUG_TLS_PROC
	PRINTF("...%s %ld: TLS handshake is ok\r\n", tag, pTcpConn->number);
#endif

//*****************************

#define	R_BUF_SIZE	1024

	data_struct_t RW_data = {NULL};
	int len;
	char rbuf[R_BUF_SIZE];
	RW_data.r_data = rbuf;
/*
	len = mbedtls_ssl_read(&ssl, (unsigned char *)rbuf, R_BUF_SIZE-1);
	if (len > 0)
	{
		rbuf[len] = 0;
		// Вивід запиту:
		PRINTF("%s %ld: Received: %s\n", tag, pTcpConn->number, rbuf);

		HttpServer ((void *)&RW_data);

		mbedtls_ssl_write(&ssl, (const unsigned char *) RW_data.w_data, (size_t)strlen (RW_data.w_data));

		if (RW_data.w_data != NULL)
		{
			vPortFree (RW_data.w_data);
		}
    }
*/
//*******************************************

#ifdef DEBUG_TLS_PROC
	int read_num = 0, write_num = 0;
#endif
	while (1)
	{
		// 7. Read the HTTP Request
#ifdef DEBUG_TLS_PROC
		PRINTF("%s %ld:  < Read from client:\r\n", tag, pTcpConn->number);
#endif

		do
		{
			ret = mbedtls_ssl_read(&ssl, (unsigned char *)rbuf, R_BUF_SIZE-1);
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
						PRINTF("%s %ld: reading timeout\r\n", tag, pTcpConn->number);
#endif
						break;

					case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
#ifdef DEBUG_TLS_PROC
						PRINTF("%s %ld: connection was closed gracefully\r\n", tag, pTcpConn->number);
#endif
						break;

					case MBEDTLS_ERR_NET_CONN_RESET:
#ifdef DEBUG_TLS_PROC
						PRINTF("%s %ld: connection was reset by peer\r\n", tag, pTcpConn->number);
#endif
						break;

					default:
#ifdef DEBUG_TLS_PROC
						PRINTF("%s %ld: mbedtls_ssl_read returned %d\r\n", tag, pTcpConn->number, ret);
#endif
						break;
				}
				break;
			}
			len = ret;
#ifdef DEBUG_TLS_PROC
			read_num++;
			PRINTF("%s %ld: %d bytes read %d times\r\n", tag, pTcpConn->number, len, read_num);
#endif
			rbuf[len] = 0;
			break;
		} while (1);
		if (ret < 0)
			goto cleanup;

		// Application
		HttpServer ((void *)&RW_data);

		// 8. Write the 200 Response
#ifdef DEBUG_TLS_PROC
		PRINTF("%s %ld: > Write to client:\r\n", tag, pTcpConn->number);
#endif

		while ((ret = mbedtls_ssl_write (&ssl, (const unsigned char *) RW_data.w_data, (size_t)strlen (RW_data.w_data))) <= 0)
		{
			if (ret == MBEDTLS_ERR_NET_CONN_RESET)
			{
#ifdef DEBUG_TLS_PROC
				PRINTF("%s %ld: failed\n  ! peer closed the connection\r\n", tag, pTcpConn->number);
#endif
				goto cleanup;
			}
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
			{
#ifdef DEBUG_TLS_PROC
				PRINTF("%s %ld: failed\n  ! mbedtls_ssl_write returned %d\r\n", tag, pTcpConn->number, ret);
#endif
				goto cleanup;
			}
		}
#ifdef DEBUG_TLS_PROC
		write_num++;
		PRINTF("%s %ld: %d bytes written %d times\r\n", tag, pTcpConn->number, ret, write_num);
#endif
		if (RW_data.w_data != NULL)
		{
			vPortFree (RW_data.w_data);
		}
	}


//*****************************

cleanup:
	// Завершення
	mbedtls_ssl_close_notify(&ssl);

	// Free
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_x509_crt_free(&cert);
	mbedtls_pk_free(&key);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

	tls_free_pTcpConn (pTcpConn, tag);

	osSemaphoreRelease (sid_TcpConnCount);
	osThreadExit ();
}


// ---------------------------------  TLS Client  ----------------------------------------


