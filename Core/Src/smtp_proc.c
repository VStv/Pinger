/*
 * smtp_proc.c
 *
 *  Created on: Jun 28, 2024
 *      Author: dis_stv
 */
#include "smtp_proc.h"


extern char					alrm_email_str[15];


const char			QUIT_COM[] = "QUIT\r\n";
const char			HELO_COM[] = "HELO ";
const char			EHLO_COM[] = "EHLO ";
const char			MAIL_COM[] = "MAIL FROM: ";
const char			RCPT_COM[] = "RCPT TO: ";
const char			DATA_COM[] = "DATA\r\n";
const char			DATA_FROM[] = "From: <";
const char			DATA_TO[] = "To: <";
const char			DATA_SUBJECT[] = "Subject: ";



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

	if (data)
	{
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
					sprintf (wbuf, "%s", MAIL_COM);
					strcat (wbuf, "pinger@ukr.net");
					strcat (wbuf, "\r\n");
					s_smtp_state = SMTP_MAIL;
				}
				else
				{
					wbuf = NULL;
					s_smtp_state = SMTP_NULL;
				}
				break;

			case SMTP_MAIL:
				if (strstr ((const char*)data, "250"))
				{
					wbuf = pvPortMalloc (200);
					sprintf (wbuf, "%s", RCPT_COM);
					strcat (wbuf, alrm_email_str);
					strcat (wbuf, "\r\n");
					s_smtp_state = SMTP_RCPT;
				}
				else
				{
					wbuf = NULL;
					s_smtp_state = SMTP_NULL;
				}
				break;

			case SMTP_RCPT:
				if (strstr ((const char*)data, "250"))
				{
					wbuf = pvPortMalloc (200);
					sprintf (wbuf, "%s", DATA_COM);
					s_smtp_state = SMTP_DATA;
				}
				else
				{
					wbuf = NULL;
					s_smtp_state = SMTP_NULL;
				}
				break;

			case SMTP_DATA:
				if (strstr ((const char*)data, "354"))
				{
					wbuf = pvPortMalloc (200);
					sprintf (wbuf, "%s", DATA_FROM);
					strcat (wbuf, "pinger@ukr.net");
					strcat (wbuf, ">\r\n");
					strcat (wbuf, DATA_TO);
					strcat (wbuf, alrm_email_str);
					strcat (wbuf, ">\r\n");
					strcat (wbuf, DATA_SUBJECT);
					strcat (wbuf, "test\r\n");
					strcat (wbuf, "\r\n");
					strcat (wbuf, "This is test message\r\n");
					strcat (wbuf, ".\r\n");
					s_smtp_state = SMTP_BODY;
				}
				else
				{
					wbuf = NULL;
					s_smtp_state = SMTP_NULL;
				}
				break;

			case SMTP_BODY:
				if (strstr ((const char*)data, "250"))
				{
					wbuf = pvPortMalloc (200);
					sprintf (wbuf, "%s", QUIT_COM);
					s_smtp_state = SMTP_QUIT;
				}
				else
				{
					wbuf = NULL;
					s_smtp_state = SMTP_NULL;
				}
				break;

			case SMTP_QUIT:
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

			default:
				wbuf = NULL;
				s_smtp_state = SMTP_NULL;
				break;
		}
	}
	else
	{
		wbuf = NULL;
		s_smtp_state = SMTP_NULL;
	}
	pRW_data->w_data = wbuf;
	return;
}




