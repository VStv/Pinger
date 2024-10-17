/*
 * smtp_proc.c
 *
 *  Created on: Jun 28, 2024
 *      Author: dis_stv
 */
#include "smtp_proc.h"



const char			QUIT_COM[] = "QUIT\r\n";
const char			HELO_COM[] = "HELO ";
const char			EHLO_COM[] = "EHLO ";


//----------------------------------------------------------------------------
void SmtpProcess 	(
					void *arg
					)
{
	data_struct_t *pRW_data = (data_struct_t *)arg;
	char *wbuf, *data;
	data = pRW_data->r_data;
	static enum smtp_session_state s_smtp_state = SMTP_NULL;
//	const char *ipa = ipaddr_ntoa(altcp_get_ip(pcb, 1));
//	ipa_len = strlen(ipa);

	switch (s_smtp_state)
	{
		case SMTP_NULL:
			if (strstr ((const char*)data, "220"))
			{
				wbuf = pvPortMalloc (200);

				sprintf (wbuf, "%s", HELO_COM);
				strcat (wbuf, SMTP_SERVER_ADDR);
				strcat (wbuf, "\r\n");
				s_smtp_state = SMTP_HELO;
			}
			else
			{
				wbuf = NULL;
				s_smtp_state = SMTP_NULL;
			}
			break;

		case SMTP_HELO:
			// SMTP_AUTH_PLAIN:
			// SMTP_AUTH_LOGIN_UNAME:
			// SMTP_AUTH_LOGIN_PASS:
			// SMTP_AUTH_LOGIN:
			if (strstr ((const char*)data, "250"))
			{
				wbuf = pvPortMalloc (200);
				sprintf (wbuf, "%s", QUIT_COM);
				s_smtp_state = SMTP_MAIL;
			}
			else
			{
				wbuf = NULL;
				s_smtp_state = SMTP_NULL;
			}
			break;

		case SMTP_MAIL:
		case SMTP_RCPT:
		case SMTP_DATA:
		case SMTP_BODY:
		case SMTP_QUIT:
		case SMTP_CLOSED:
		default:
			if (strstr ((const char*)data, "221"))
			{
				wbuf = NULL;
				s_smtp_state = SMTP_NULL;
			}
			else
			{
				wbuf = NULL;
				s_smtp_state = SMTP_NULL;
			}
			break;
	}
	pRW_data->w_data = wbuf;
	return;
}




