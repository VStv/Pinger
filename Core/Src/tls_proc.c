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
extern mbedtls_x509_crt 		cert;
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


/*
const char mbedtls_google_root_certificate[] = "-----BEGIN CERTIFICATE-----\r\n"
		"MIIDNzCCAh+gAwIBAgIBAjANBgkqhkiG9w0BAQsFADA7MQswCQYDVQQGEwJOTDER\r\n"
	    "MA8GA1UECgwIUG9sYXJTU0wxGTAXBgNVBAMMEFBvbGFyU1NMIFRlc3QgQ0EwHhcN\r\n"
	    "MTEwMjEyMTQ0NDA2WhcNMjEwMjEyMTQ0NDA2WjA0MQswCQYDVQQGEwJOTDERMA8G\r\n"
	    "A1UECgwIUG9sYXJTU0wxEjAQBgNVBAMMCWxvY2FsaG9zdDCCASIwDQYJKoZIhvcN\r\n"
	    "AQEBBQADggEPADCCAQoCggEBAMFNo93nzR3RBNdJcriZrA545Do8Ss86ExbQWuTN\r\n"
	    "owCIp+4ea5anUrSQ7y1yej4kmvy2NKwk9XfgJmSMnLAofaHa6ozmyRyWvP7BBFKz\r\n"
	    "NtSj+uGxdtiQwWG0ZlI2oiZTqqt0Xgd9GYLbKtgfoNkNHC1JZvdbJXNG6AuKT2kM\r\n"
	    "tQCQ4dqCEGZ9rlQri2V5kaHiYcPNQEkI7mgM8YuG0ka/0LiqEQMef1aoGh5EGA8P\r\n"
	    "hYvai0Re4hjGYi/HZo36Xdh98yeJKQHFkA4/J/EwyEoO79bex8cna8cFPXrEAjya\r\n"
	    "HT4P6DSYW8tzS1KW2BGiLICIaTla0w+w3lkvEcf36hIBMJcCAwEAAaNNMEswCQYD\r\n"
	    "VR0TBAIwADAdBgNVHQ4EFgQUpQXoZLjc32APUBJNYKhkr02LQ5MwHwYDVR0jBBgw\r\n"
	    "FoAUtFrkpbPe0lL2udWmlQ/rPrzH/f8wDQYJKoZIhvcNAQELBQADggEBAGGEshT5\r\n"
	    "kvnRmLVScVeUEdwIrvW7ezbGbUvJ8VxeJ79/HSjlLiGbMc4uUathwtzEdi9R/4C5\r\n"
	    "DXBNeEPTkbB+fhG1W06iHYj/Dp8+aaG7fuDxKVKHVZSqBnmQLn73ymyclZNHii5A\r\n"
	    "3nTS8WUaHAzxN/rajOtoM7aH1P9tULpHrl+7HOeLMpxUnwI12ZqZaLIzxbcdJVcr\r\n"
	    "ra2F00aXCGkYVLvyvbZIq7LC+yVysej5gCeQYD7VFOEks0jhFjrS06gP0/XnWv6v\r\n"
	    "eBoPez9d+CCjkrhseiWzXOiriIMICX48EloO/DrsMRAtvlwq7EDz4QhILz6ffndm\r\n"
	    "e4K1cVANRPN2o9Y=\r\n"
	    "-----END CERTIFICATE-----\r\n";

	"-----BEGIN CERTIFICATE-----\r\n"
    "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\r\n"
    "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\r\n"
    "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\r\n"
    "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\r\n"
    "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\r\n"
    "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\r\n"
    "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\r\n"
    "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\r\n"
    "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\r\n"
    "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\r\n"
    "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\r\n"
    "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\r\n"
    "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\r\n"
    "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\r\n"
    "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\r\n"
    "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\r\n"
    "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\r\n"
    "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\r\n"
    "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\r\n"
    "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\r\n"
    "-----END CERTIFICATE-----\r\n";

	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDhzCCAm+gAwIBAgIHZ9OnbZDdnDANBgkqhkiG9w0BAQsFADBTMSgwJgYDVQQD\r\n"
	"DB9BbGliYWJhIENsb3VkIElvVCBPcGVyYXRpb24gQ0ExMRowGAYDVQQKDBFBbGli\r\n"
	"YWJhIENsb3VkIElvVDELMAkGA1UEBhMCQ04wIBcNMjAwNDAxMDkxNDMxWhgPMjEy\r\n"
	"MDA0MDEwOTE0MzFaMFExJjAkBgNVBAMMHUFsaWJhYmEgQ2xvdWQgSW9UIENlcnRp\r\n"
	"ZmljYXRlMRowGAYDVQQKDBFBbGliYWJhIENsb3VkIElvVDELMAkGA1UEBhMCQ04w\r\n"
	"ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDfniNwg0L3ms6Yntb4BGku\r\n"
	"ZmtGwPj54RMiTYHrZzF6uMiI09evz0bb6ypeRFosp2/hqi+/ltYHplprHFNtlJtr\r\n"
	"alWP8wn8xk4ZE0g9mY33GMoPM+x/G1WSL2QObZFpKJWjWR2P6g/mZfXOMTw1bzMv\r\n"
	"k6Xmogz7+ot8SMR91VjTiUmPAj75hQVW3R7DP78j739hKrpMOCW0C0J4ODOGQfKb\r\n"
	"XlymqOgjFJXoTCpRDqrmqpxfe4XNLYPmJWfjpFxiyowRaV5AgEgROlNtZW89viNP\r\n"
	"2Ftso+9iU2R4OF2nuc+dQcgiQq2jSvVCqshbbtcfCJFmQmPltGtrpcjojb3f9/TX\r\n"
	"AgMBAAGjYDBeMB8GA1UdIwQYMBaAFIo3m6hwzdX5SMiXfiGfWW9JjiQRMB0GA1Ud\r\n"
	"DgQWBBSxfKYMhb5vhWQt2gSpZkpn6hcbTzAOBgNVHQ8BAf8EBAMCA/gwDAYDVR0T\r\n"
	"AQH/BAIwADANBgkqhkiG9w0BAQsFAAOCAQEAcIdrjxByDr2qa/NcD9QDrtWcRGgW\r\n"
	"HZDjmltnkQliLIJcmrj1fAj5xtICE4x6nEgWluBbhIEHraq7X21///ubxJIBKAV+\r\n"
	"FNLo+g1rIQUO2J+07DbyK6xfCK9CPSXLS2ORhwWM6HfzYk+6lYQrUeRSNUSEBZZd\r\n"
	"IuEydvbVx9HuXuxeE+OJnvyD2jMeqvg7C8kFwoyr9izvjih19+zx8D1w7QoGkdXZ\r\n"
	"wMzjnB0bjePc+olHfn+8iVLajG636twSl8R6pw8Lgr9KbVYVBYXgwWw/F6ctajnK\r\n"
	"ZmsgUjOTMnwUuGUWZxKuHgZ0TakxuwZAi89uc/N4OZkGQ116z1nGtBHagw==\r\n"
	"-----END CERTIFICATE-----\r\n";

	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIFDDCCAvSgAwIBAgICEAEwDQYJKoZIhvcNAQELBQAwPzELMAkGA1UEBhMCQ04x\r\n"
	"EjAQBgNVBAgMCUNob25nUWluZzENMAsGA1UECgwEZWRnZTENMAsGA1UEAwwEZWRn\r\n"
	"ZTAeFw0yMDA0MDIwNzE0MjNaFw0zMDA3MDkwNzE0MjNaMEMxCzAJBgNVBAYTAkNO\r\n"
	"MRIwEAYDVQQIDAlDaG9uZ1FpbmcxDTALBgNVBAoMBGVkZ2UxETAPBgNVBAMMCGVk\r\n"
	"Z2VtcXR0MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsweQRU182TnQ\r\n"
	"r+bsNfbwXGhlj5T2q8mvNoEyNqxORF/05TJGbxEfS028CkMDVStg0dwmvkey3p5h\r\n"
	"pVCVav9fAeIf2gsjT/nbgNbFmVCzg/8REHuOMNLV4ajoD9uhXfxQLYHCVlBbAWDB\r\n"
	"mz8TaJEp93/Z54pNDC7XZe3Ua4v2bnOQxOp861uDL+AR780XljoQ9PGcLBiDRMMV\r\n"
	"5hnyxPziaaeG9dZp+p0exahnIUaH0D9Jp2ffUb6j9mHKLLgDIoP0e0Da1pCVOVSl\r\n"
	"xIchvKQJPW/ekEsEDLys5ThtoDhXN4SogJZ/d+zX8vHhGxvYoTmOghnHW7mUGAbu\r\n"
	"VXeg5mokCQIDAQABo4IBDDCCAQgwCQYDVR0TBAIwADARBglghkgBhvhCAQEEBAMC\r\n"
	"BkAwMwYJYIZIAYb4QgENBCYWJE9wZW5TU0wgR2VuZXJhdGVkIFNlcnZlciBDZXJ0\r\n"
	"aWZpY2F0ZTAdBgNVHQ4EFgQUGADp5yT4+gxa78jxB1ZgdqQgqd8wbwYDVR0jBGgw\r\n"
	"ZoAU+r8aKJSSb1JDfs/xlOw/Tnn4ituhQ6RBMD8xCzAJBgNVBAYTAkNOMRIwEAYD\r\n"
	"VQQIDAlDaG9uZ1FpbmcxDTALBgNVBAoMBGVkZ2UxDTALBgNVBAMMBGVkZ2WCCQCB\r\n"
	"wnEGC0GO7DAOBgNVHQ8BAf8EBAMCBaAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDQYJ\r\n"
	"KoZIhvcNAQELBQADggIBAGjMG1RnD6Qk/n6JtRuQITfPRU/MwYSsVzVhfyFrU6NZ\r\n"
	"78sDAfD9FqoEThw4f1YLzlkfyUgxlqMzLQsYoAVCEkBrsUoUVQ8YWj/ybVdaxTRb\r\n"
	"kkS9bacJFP+zxqq8jr3xCH0vCTw6lL/aQwwlhZNQPvHa/KH0tC1JdMKK7FZkAGBY\r\n"
	"RvNBfkB6DEsyoBYFh01enZpE9+wBn/7I7GRt2/TWPlRQuMo/tk50gv9ys+aRiCDM\r\n"
	"0vK4aVxSpVXWg0fXms/tDxl3EkZLkvjtdVqgJHx1saEyOEabPolYsB/ZsFQuUfIp\r\n"
	"rpr0PhEyvTkEBUGlYKY/YO1ImdVX7IQegDT84VBt+swtiVthNl7Eyw/0uq0eSfAZ\r\n"
	"IpyTOKzBIS9hBwJnQ0dhvsM1p6zBG1pBPG3mqdVUJptl1fNCFhxKY+BEDH5KmPH2\r\n"
	"rby9uAp1n4Ybai0DW9TdTH6Vk65FmUZ8wieooT6EzLD4E5iH27jsxNpW09wBaSte\r\n"
	"7PH5zVrT5sA136lSP3HaIr7FtoSgpH9rzvnouQCQAT2IGUc7PlwsGBbOSai2wFnB\r\n"
	"J8uW5qn337SxPos/Qh1O6vZ806sLyM+gZnfwllQST/RQM+DCpmSbDhylN6JI3++9\r\n"
	"dt4dvfd463vYHRoggJnRXMuXtMGnXXgbnGOweHv+w1YOWH+DXcmIXO4IUE5tEmkr\r\n"
	"-----END CERTIFICATE-----\r\n";

const size_t mbedtls_google_root_certificate_len = sizeof(mbedtls_google_root_certificate);
*/

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
/*
	ret = mbedtls_x509_crt_parse(&srvcert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
	if( ret != 0 )
	{
#ifdef DEBUG_TLS_PROC
		PRINTF("failed\n  !  mbedtls_x509_crt_parse returned %d\r\n", ret);
#endif
		goto exit;
	}
*/
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
