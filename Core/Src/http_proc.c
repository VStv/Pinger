/*
 * http_proc.c
 *
 *  Created on: 1 черв. 2024 р.
 *      Author: stv
 */

#include "http_proc.h"


char 						ssid_str[10];
char						psw_str[10];


extern char					alrm_email_str[15];
extern char					alrm_telega_str[15];
extern uint8_t				alrm_unrepl;
extern uint8_t				alrm_led;
extern reply_struc_t 		Ping_res;
extern osThreadId_t 		PingLoopHandle;
extern osMessageQueueId_t 	mid_PingData;
extern uint32_t				email_is_OK;
extern char 				*pp;
extern ip4_addr_t 			ipaddr;
extern ip4_addr_t 			netmask;
extern ip4_addr_t 			gw;
extern uint8_t				static_ip;
extern osThreadId_t 		TcpServerTaskHandle;
extern uint32_t				TcpServerApp;

const char 			*str_list[12] = {"ipa1=", "ipa2=", "ipa3=", "ipa4=",
									"ipm1=", "ipm2=", "ipm3=", "ipm4=",
									"ipg1=", "ipg2=", "ipg3=", "ipg4="};
const char 			*led_list[3] = {"red=", "blue=", "green="};
const char 			*str_list2[4] = {"hipa1=", "hipa2=", "hipa3=", "hipa4="};

const char			PAGE_HEADER_200_OK[] = "HTTP/1.1 200 OK\r\n";
const char			PAGE_HEADER_CONTENT_TEXT[] = "Content-type: text/html\r\n\r\n";
//const char			DEFAULT_EMAIL[] = "192.168.0.2";




void RunHttpServer (void)
{
	TcpServerApp = HTTP_PROT;
	TcpServerTaskHandle = StartTcpServer ((void *)&TcpServerApp);
}


static uint32_t ParseNetsetting	(
								char *str,
								set_struc_t *pSettings
								)
{
	uint32_t i, j;
	char *p, *p2, *p3;
	uint8_t IP[12] = {0};

	p = strstr (str, "ip=");
	if (p == NULL)
	{
		return 1;
	}
	if (strstr (str, "ip=static"))
	{
		*(pSettings->ip_mode) = STATIC_MODE;
	}
	else if (strstr (str, "ip=dhcp"))
	{
		*(pSettings->ip_mode) = DHCP_MODE;
	}
	else
		return 1;
	if (*(pSettings->ip_mode) == STATIC_MODE)
	{
		for (i = 0; i < 12; i++)
		{
			p = strstr (str, str_list[i]);
			if (p != NULL)
			{
				p2 = p + 5;
				j = 0;
				while (*p2 >= '0' && *p2 <= '9')
				{
					IP[i] *= 10;
					IP[i] += (*(p2++) - 0x30);
					if (++j > 3)
						return 2;
				}
				if (!j)
					return 2;
			}
			else
				return 1;
		}
		IP_ADDR4(pSettings->ipa, IP[0], IP[1], IP[2], IP[3]);
		IP_ADDR4(pSettings->ipm, IP[4], IP[5], IP[6], IP[7]);
		IP_ADDR4(pSettings->ipg, IP[8], IP[9], IP[10], IP[11]);
	}
	memset ((pSettings->ssid), 0xff, 10);
	memset ((pSettings->psw), 0xff, 10);
	p = strstr (str, "ssid=");
	if (p != NULL)
	{
		p2 = p + 5;
		p = strstr (str, "psw=");
		if (p != NULL)
		{
			p3 = p - 1;
			if (p3 > p2)
			{
				memcpy ((pSettings->ssid), p2, p3 - p2);
				p2 = p + 4;
				p = strstr (str, "HTTP/1.");
				if (p != NULL)
				{
					p3 = p - 1;
					if (p3 > p2)
					{
						memcpy ((pSettings->psw), p2, p3 - p2);
						return 0;
					}
				}
			}
		}
	}
	return 0;
}


static uint32_t ParseAlarms	(
							char *str,
							alrm_struc_t *pAlarms
							)
{
	uint32_t j;
	char *p, *p2, *p3;
	uint8_t unrepl_ping = 0, led_state = 0;
	char str1[15] = {0}, str2[15] = {0};

	p = strstr (str, "unreplied=");
	if (p == NULL)
		return 1;
	p2 = p + 10;
	j = 0;
	while (*p2 >= '0' && *p2 <= '9')
	{
		unrepl_ping *= 10;
		unrepl_ping += (*(p2++) - 0x30);
		if (++j > 3)
			break;
	}
	if (j == 0 || j > 3)
		unrepl_ping = 10;

	p = strstr (str, "email=");
	if (p == NULL)
		return 1;
	p2 = p + 6;
	p = strstr (str, "telega=");
	if (p == NULL)
		return 1;
	p3 = p - 1;
	if (p3 > p2)
	{
		memcpy (str1, p2, p3 - p2);
		if (strchr (str1, '@') != NULL)
		{
			email_is_OK = 1;
			memcpy (pAlarms->email, str1, 15);
		}
		else
		{
			email_is_OK = 0;
		}
	}

	p2 = p + 7;
	p = strstr (str, "red=");
	if (p == NULL)
		return 1;
	p3 = p - 1;
	if (p3 > p2)
		memcpy (str2, p2, p3 - p2);

	p2 = p + 4;
	if (strstr (str, "red=true"))
		led_state += ALARM_LED_RED;

	if (strstr (str, "blue=true"))
		led_state += ALARM_LED_BLUE;

	if (strstr (str, "green=true"))
		led_state += ALARM_LED_GREEN;

	*(pAlarms->unrepl) = unrepl_ping;

	memcpy (pAlarms->telega, str2, 15);
	*(pAlarms->led) = led_state;
	return 0;
}


static uint32_t ParsePingRequest(
								char *str,
								ping_struc_t *pPing
								)
{
	// "GET /start?&hipa1=192&hipa2=168&hipa3=1&hipa4=10&n=5"
	uint32_t j;
	char *p, *p2;
	uint8_t IP[4] = {0};
	uint16_t temp = 0;

	for (uint32_t i = 0; i < 4; i++)
	{
		p = strstr (str, str_list2[i]);
		if (p != NULL)
		{
			p2 = p + 6;
			j = 0;
			while (*p2 >= '0' && *p2 <= '9')
			{
				IP[i] *= 10;
				IP[i] += (*(p2++) - 0x30);
				if (++j > 3)
					return 2;
			}
			if (!j)
				return 2;
		}
		else
			return 1;
	}
	// Remote IP-address
	IP_ADDR4(&(pPing->ip_host), IP[0], IP[1], IP[2], IP[3]);

	p = strstr (str, "&n=");
	if (p != NULL)
	{
		p2 = p + 3;
		j = 0;
		while (*p2 >= '0' && *p2 <= '9')
		{
			temp *= 10;
			temp += (*(p2++) - 0x30);
			if (++j > 5)
				return 4;
		}
		if (!j)
			return 4;
	}
	else
		return 3;
	// amount of ping
	pPing->n_pings = temp;
	return 0;
}


static void ResponseToGetconfig	(
								set_struc_t *pSettings,
								char *pdat
								)
{
	char ip_str[50] = {0};
	char ip_s[20], s1[4], *p1, *p2;
	const char dot_s[] = ".", and_s[] = "&";

	memset (pdat, 0, 200);
	sprintf (pdat, "%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_CONTENT_TEXT);

	if (*pSettings->ip_mode == STATIC_MODE)
	{
		strcat (pdat, "ip=static");
	}
	else
	{
		strcat (pdat, "ip=dhcp");
	}
	// prepare string ip_str as ".192.168.10.8.255.255.255.0.192.168.10.1."
	strcat (ip_str, dot_s);
	memset (ip_s, 0, 20);
	strcpy (ip_s, ip4addr_ntoa (pSettings->ipa));
	strcat (ip_s, dot_s);
	strcat (ip_str, ip_s);
	memset (ip_s, 0, 20);
	strcpy (ip_s, ip4addr_ntoa (pSettings->ipm));
	strcat (ip_s, dot_s);
	strcat (ip_str, ip_s);
	memset (ip_s, 0, 20);
	strcpy (ip_s, ip4addr_ntoa (pSettings->ipg));
	strcat (ip_s, dot_s);
	strcat (ip_str, ip_s);

	// add to pdat "&", "ipa1=", "192" and repeat 12 times
	p2 = ip_str;
	for (uint32_t i = 0; i < 12; i++)
	{
		p1 = p2 + 1;
		p2 = strchr (p1, '.');
		strcat (pdat, and_s);
		strcat (pdat, str_list[i]);
		memset (s1, 0, 4);
		memcpy (s1, p1, p2 - p1);
		strcat (pdat, s1);
	}
}


static void ResponseToGetalarms	(
								alrm_struc_t *pAlarms,
								char *pdat
								)
{
	char s1[4];
	const char and_s[] = "&";

	memset (pdat, 0, 200);
	sprintf (pdat, "%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_CONTENT_TEXT);
	strcat (pdat, "unreplied=");
	itoa(*(pAlarms->unrepl), s1, 10);
	strcat (pdat, s1);
	strcat (pdat, "&email=");
	strcat (pdat, pAlarms->email);
	strcat (pdat, "&telega=");
	strcat (pdat, pAlarms->telega);
	for (uint32_t i = 0; i < 3; i++)
	{
		strcat (pdat, and_s);
		strcat (pdat, led_list[i]);
		if (*(pAlarms->led) & (1 << i))
		{
			strcat (pdat, "true");
		}
		else
		{
			strcat (pdat, "false");
		}
	}
}


static void ResponseToGetdata	(
								reply_struc_t *pReply,
								char *pdat
								)
{
	char s1[10];
	memset (pdat, 0, 200);
	sprintf (pdat, "%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_CONTENT_TEXT);

	memset (s1, 0, 5);
	strcat (pdat, "cnt=");
	itoa(pReply->ping_cnt, s1, 10);
	strcat (pdat, s1);

	memset (s1, 0, 5);
	strcat (pdat, "&lost=");
	itoa(pReply->lost, s1, 10);
	strcat (pdat, s1);

	memset (s1, 0, 5);
	strcat (pdat, "&tcur=");
	if (pReply->status == 0)
	{
		itoa(pReply->cur_time, s1, 10);
	}
	else
	{
		strcpy (s1, "-");
	}
	strcat (pdat, s1);

	memset (s1, 0, 5);
	strcat (pdat, "&tmin=");
	if (pReply->status == 0)
	{
		itoa(pReply->min_time, s1, 10);
	}
	else
	{
		strcpy (s1, "-");
	}
	strcat (pdat, s1);

	memset (s1, 0, 5);
	strcat (pdat, "&tmax=");
	if (pReply->status == 0)
	{
		itoa(pReply->max_time, s1, 10);
	}
	else
	{
		strcpy (s1, "-");
	}
	strcat (pdat, s1);

	memset (s1, 0, 10);
	strcat (pdat, "&status=");
	switch (pReply->status)
	{
		case 0:
			strcpy (s1, "Ok");
			break;
		case 5:
			strcpy (s1, "timeout");
			break;
		default:
			strcpy (s1, "error");
	}
	strcat (pdat, s1);
}


char *HttpProcess 	(
					char *data
					)
{
	struct fs_file file;
	char *wbuf;

	set_struc_t ipSettings;
	alrm_struc_t Alarms_struc;
	ping_struc_t pingSettings;

	FlashReadBuf ((uint8_t *)ssid_str, FLASH_WIFISSID_ADDR, FLASH_WIFISSID_SIZE);
	FlashReadBuf ((uint8_t *)psw_str, FLASH_WIFIPSW_ADDR, FLASH_WIFIPSW_SIZE);

	ipSettings.ip_mode = &static_ip;
	ipSettings.ipa = &ipaddr;
	ipSettings.ipm = &netmask;
	ipSettings.ipg = &gw;
	ipSettings.ssid = ssid_str;
	ipSettings.psw = psw_str;

	Alarms_struc.unrepl = &alrm_unrepl;
	Alarms_struc.email = alrm_email_str;
	Alarms_struc.telega = alrm_telega_str;
	Alarms_struc.led = &alrm_led;

	// Check if request to get the index.html
	if (strncmp((char const *)data, "GET /index.html", 15)==0 || strncmp((char const *)data, "GET / ", 6)==0)
	{
		fs_open(&file, "/index.html");
		wbuf = pvPortMalloc(file.len);
		memcpy (wbuf, (char *)file.data, file.len);
		fs_close(&file);
	}
	else if (strncmp((char const *)data, "GET /alarms.html", 16)==0)
	{
		fs_open(&file, "/alarms.html");
		wbuf = pvPortMalloc(file.len);
		memcpy (wbuf, (char *)file.data, file.len);
		fs_close(&file);
	}
	else if (strncmp((char const *)data, "GET /ping.html", 14)==0)
	{
		fs_open(&file, "/ping.html");
		wbuf = pvPortMalloc(file.len);
		memcpy (wbuf, (char *)file.data, file.len);
		fs_close(&file);
	}
	else if (strncmp((char const *)data, "GET /style.css", 14)==0)
	{
		fs_open(&file, "/style.css");
		wbuf = pvPortMalloc(file.len);
		memcpy (wbuf, (char *)file.data, file.len);
		fs_close(&file);
	}
	else if (strncmp((char const *)data, "GET /myScript.js", 16)==0)
	{
		fs_open(&file, "/myScript.js");
		wbuf = pvPortMalloc(file.len);
		memcpy (wbuf, (char *)file.data, file.len);
		fs_close(&file);
	}
	else if (strncmp((char const *)data, "GET /setconfig?", 15)==0)
	{
		if (ParseNetsetting (data+15, &ipSettings) == 0)
		{
			// save and set flag for reset
			SaveToFlash (&ipSettings);
		}
		wbuf = pvPortMalloc(200);
		sprintf (wbuf, "%s\r\n", PAGE_HEADER_200_OK);
	}
	else if (strncmp((char const *)data, "GET /setalarm?", 14)==0)
	{
		ParseAlarms (data+14, &Alarms_struc);
		wbuf = pvPortMalloc(200);
		sprintf (wbuf, "%s\r\n", PAGE_HEADER_200_OK);
	}
	else if (strncmp((char const *)data, "GET /start?", 11)==0)
	{
		if (PingLoopHandle == NULL)
		{
			if (ParsePingRequest (data+11, &pingSettings) == 0)
			{
				osMessageQueuePut(mid_PingData, &pingSettings, 0U, 0U);
				PingLoopHandle = StartPings ();
				wbuf = pvPortMalloc(200);
				sprintf (wbuf, "%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_CONTENT_TEXT);
				strcat (wbuf, "state=OK");
			}
		}
		else
		{
			wbuf = pvPortMalloc(200);
			sprintf (wbuf, "%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_CONTENT_TEXT);
			strcat (wbuf, "state=BUSY");
		}
	}
	else if (strncmp((char const *)data, "GET /stop", 9)==0)
	{
		if (PingLoopHandle != NULL)
		{
			StopPing ();
			wbuf = pvPortMalloc(200);
			ResponseToGetdata (&Ping_res, wbuf);
		}
		else
		{
			wbuf = pvPortMalloc (200);
			sprintf (wbuf, "%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_CONTENT_TEXT);
			strcat (wbuf, "state=ERROR");
		}
	}
	else if (strncmp ((char const *)data, "GET /getconfig", 14) == 0)
	{
		wbuf = pvPortMalloc (200);
		ResponseToGetconfig (&ipSettings, wbuf);
	}
	else if (strncmp ((char const *)data, "GET /getalarms", 14) == 0)
	{
		wbuf = pvPortMalloc (200);
		ResponseToGetalarms (&Alarms_struc, wbuf);
	}
	else if (strncmp ((char const *)data, "GET /getdata", 12) == 0)
	{
		// update Ping_res from message

		wbuf = pvPortMalloc (200);
		ResponseToGetdata (&Ping_res, wbuf);
	}
	else
	{
		fs_open (&file, "/404.html");
		wbuf = pvPortMalloc (file.len);
		memcpy (wbuf, (char *)file.data, file.len);
		fs_close (&file);
	}
	return wbuf;
}


