/* -- updatedd: noip.c --
 *
 * Copyright (C) 2002, 2003, 2004, 2005 Philipp Benner
 *
 * This file is part of UpdateDD - http://updatedd.philipp-benner.de.
 *
 * UpdateDD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * UpdateDD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with UpdateDD; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#include <get_connection.h>
#include <libexception_handle.h>
#include <ret_codes.h>
#include <version.h>

#include "noip.h"
#include <base64encode.h>
#ifdef CONFIG_USER_DDNS_LIBSSL
static SSL_CTX	*context;		/* Context for encryption */
static SSL *conn;			/* Connection for encryption */
static int g_noip_isSSL=0;
#endif

static void
print_usage(char *pname, FILE *fp)
{
	(void)fprintf(fp,
		      "\nUsage: %s [...] %s -- [OPTION]... [USERNAME:PASSWORD] HOSTNAME\n\n",
		      pname, COLORED("noip"));
	(void)fprintf(fp,
		      "For security reasons use the environment variable LOGIN instead of\n"
		      "passing the login information directly.\n\n"
		      "Options:\n"
		      "   -4    --ipv4 <address>        ip address version 4\n"
			  "   -6    --ipv6 <address>        ip address version 6\n"
			  "   -p    --port <num>  	        server port\n"
/*                       "   -g    --group                 use group instead of HOSTNAME\n" */
		      "         --help                  print help and exit\n"
		      "         --version               display version information and exit\n\n"
		      "Report bugs to <"EMAIL">.\n\n");

	return;
}

static void
print_version(FILE *fp)
{

	(void)fprintf(fp,
		      "\n" PNAME " plugin for no-ip.com version " VERSION ",\n"
		      "Copyright (C) 2005 Philipp Benner.\n"
		      HOMEPAGE "\n\n"

		      "This is free software, and you are welcome to redistribute it\n"
		      "under certain conditions; see the source for copying conditions.\n"
		      "There is NO warranty; not even for MERCHANTABILITY or FITNESS\n"
		      "FOR A PARTICULAR PURPOSE.\n\n");

	return;

}

static void
noip_ret_msg(int mode, const char *fmt, ...)
{

	va_list az;

	va_start(az, fmt);
	(void)vs_warn(ret_msg_buf, BUFSIZE, mode, fmt, az);
	va_end(az);

	return;

}

int
noip_dyndns(int argc, char *argv[])
{

	struct arguments args;
	const char *ptr;
	int s, ret;
	int num = 0;
	(void)memset(&args, 0, sizeof(struct arguments));

	if(noip_get_flags(&args, argc, argv) != RET_OK) {
		return RET_WRONG_USAGE;
	}

	// Mason Yu.
	// If we can not connect to the DDNS server, we will try three times.
	//s = get_connection(DYNDNSHOST, PORT, &ptr);
	//if(s == -1) {
	while( ((s = get_connection(DYNDNSHOST, args.server_port, &ptr)) == -1) && (num < 3) ) {
		noip_ret_msg(HERR, "%s: %s", ptr, DYNDNSHOST);
		ret = RET_WARNING;
		sleep(2);
		num++;
	}

#ifdef CONFIG_USER_DDNS_LIBSSL
	if (args.server_port == SSL_PORT) {
		create_ssl_session(&context, &conn, s);
		g_noip_isSSL = 1;
	}
#endif

	if(s != -1) {
		ret = noip_update_dyndns(s, &args);
		if(ret == RET_OK) {
                        if(args.hostname) {
                                ret = noip_check_server_msg(s, args.hostname);
                        } else {
                                ret = noip_check_server_msg(s, args.group);
                        }
		}
		(void)close(s);
	}

#ifdef CONFIG_USER_DDNS_LIBSSL
	if (args.server_port == SSL_PORT) {
		free_ssl_session(&context, &conn);
	}
#endif
	return ret;

}

static int
noip_get_flags(struct arguments *args, int argc, char *argv[])
{

	int c;
	char **ptr = &args->hostname;

	args->server_port = PORT;
	for(;;) {

		int option_index = 0;
		static struct option long_options[] = {
			{ "ipv4",		1, 0, '4' },
			{ "ipv6",		1, 0, '6' },
			{ "port",		1, 0, 'p' },
			{ "help",		0, 0, 'h' },
/* 			{ "group",		0, 0, 'g' }, */
			{ "version",		0, 0, 'v' },
			{ NULL,			0, 0, 0   }
		};

		c = getopt_long(argc, argv, "4:6:p:",
				long_options, &option_index);

		if(c == -1) break;

		switch(c) {
		case '4':
			args->ipv4 = optarg;
			break;
		case '6':
			args->ipv6 = optarg;
			break;
		case 'p':
			args->server_port = strtol(optarg, NULL, 0);
			break;
/* 		case 'g': */
/* 			ptr = &args->group; */
/* 			break; */
		case 'h':
			print_usage(argv[ARGV_PNAME], stdout);
			exit(EXIT_SUCCESS);
		case 'v':
			print_version(stdout);
			exit(EXIT_SUCCESS);
		}
	}

	switch(argc-optind) {
        default:
		noip_ret_msg(NONE, "wrong usage");
		return RET_WRONG_USAGE;

        case 2:
		args->login = getenv("LOGIN");
		if(args->login == NULL) {
			noip_ret_msg(NONE,
				"environment variable LOGIN is empty");
			return RET_WRONG_USAGE;
		}
		break;
        case 3:
		args->login = argv[ARGV_LOGIN];
	}
	*ptr = argv[ARGV_HOSTNAME];

	return RET_OK;

}

#define BUFLEN		4096
#define BUFFREE(name)	BUFLEN - strlen(name)

static int
noip_update_dyndns(int s, struct arguments *args)
{

	char message[BUFLEN];
	int i;
	int len = strlen(args->login);
	char login[len], *pass = NULL;
#ifdef USE_BASE64
	char *b64user;
#endif

#ifdef USE_BASE64
	b64user = (char *)malloc((2 * strlen(args->login) + 1));
	if(b64user == NULL) {
		noip_ret_msg(PERR, "malloc() failed");
		return RET_WARNING;
	}
	(void)memset(b64user, 0, 2 * strlen(args->login) + 1);
	base64encode(args->login, b64user);
#endif

	strcpy(login, args->login);
	for(i = 0; i < len; i++) {
		if(login[i] == ':') {
			login[i] = '\0';
                        pass = login+i+1;
			break;
		}
	}
	if(pass == NULL) {
		noip_ret_msg(NONE, "password is missing");
		return RET_WRONG_USAGE;
	}

#ifdef USE_BASE64
	// GET /nic/update?hostname=mytest.example.com&myip=192.0.2.25
	(void)snprintf(message, BUFLEN,
                       "GET /nic/update?hostname=%s",args->hostname);
#else
	(void)snprintf(message, BUFLEN,
		       "GET /update.php?username=%s&pass=%s",
		       login, pass );
#endif

	#if 0
	if(args->ipv4) {
		(void)strncat(message, "&ip=", BUFFREE(message));
		(void)strncat(message, args->ipv4, BUFFREE(message));
	}
	#else
	/* Add IPv6 support */
	if(args->ipv4 || args->ipv6) {
		(void)strncat(message, "&myip=", BUFFREE(message));
		if (args->ipv4) {
			(void)strncat(message, args->ipv4, BUFFREE(message));
			if (args->ipv6) {
				(void)strncat(message, ",", BUFFREE(message));
				(void)strncat(message, args->ipv6, BUFFREE(message));
			}
		}
		else
			(void)strncat(message, args->ipv6, BUFFREE(message));
	}
	#endif

#ifndef USE_BASE64
	if(args->hostname) {
		(void)strncat(message, "&host=", BUFFREE(message));
		(void)strncat(message, args->hostname, BUFFREE(message));
/*	} else if(args->group) {                                       *
 *		(void)strncat(message, "&group=", BUFFREE(message));   *
 *		(void)strncat(message, args->group, BUFFREE(message)); */
	} else {
		noip_ret_msg(NONE, "wrong usage");
		return RET_WRONG_USAGE;
	}
#endif

	{
		char buffer[1024];

		(void)snprintf(buffer, 1024,
                               " HTTP/1.1\r\n"
                               "Host: %s\r\n"
#ifdef USE_BASE64
                               "Authorization: Basic %s\r\n"
#endif
                               "User-Agent: %s %s - %s\r\n"
                               "Connection: close\r\n"
                               "Pragma: no-cache\r\n\r\n",
                               DYNDNSHOST,
#ifdef USE_BASE64
							   b64user,
#endif
							   PNAME, VERSION, HOMEPAGE);
		(void)strncat(message, buffer, BUFFREE(message));
	}
	print_debug("\n\nMessage:"
		    "\n--------------------------------------\n"
		    "%s--------------------------------------\n\n",
                    message);

	if (args->server_port == PORT) {
		if(write(s, message, strlen(message)) == -1) {
			noip_ret_msg(PERR, "write() failed");
			free(b64user);
			return RET_WARNING;
		}
		free(b64user);
	}

#ifdef CONFIG_USER_DDNS_LIBSSL
	if (args->server_port == SSL_PORT) {
		if(SSL_write(conn, message, strlen(message)) == -1) {
			noip_ret_msg(PERR, "write() failed");
			free(b64user);
			return RET_WARNING;
		}
	}
#endif
	return RET_OK;

}

#if 0
static int
noip_check_server_msg(int s, char *hostname)
{

	int bytes_read, index = 0;
	int n, ret_code;
	char server_msg[BUFSIZE], buffer[BUFSIZE], *ptr;

	(void)memset(server_msg, 0, sizeof(server_msg));
	(void)memset(buffer, 0, sizeof(buffer));

	while ((bytes_read = read(s, buffer, sizeof(buffer) - 1)) > 0)
	{
		memcpy(&server_msg[index], buffer, bytes_read);
		memset(buffer, 0, sizeof(buffer));
		index += bytes_read;
	}

	if(bytes_read < 0)
	{
		noip_ret_msg(PERR, "read() failed");
		return RET_WARNING;
	}

	print_debug("\n\nServer message:"
		    "\n--------------------------------------\n"
		    "%s--------------------------------------\n\n",
		    server_msg);

	if(strstr(server_msg, "HTTP/1.1 200 OK") ||
           strstr(server_msg, "HTTP/1.0 200 OK") ) {

		ptr = strstr(server_msg, "status=");
		if (ptr == NULL)
			return RET_ERROR;

		ret_code = atoi((ptr+7));
		for(n=0; return_codes[n].message != NULL; n++) {
			if(ret_code == return_codes[n].code) {
				noip_ret_msg(NONE, "%s: %s", hostname, return_codes[n].message);
				if(return_codes[n].error == 1) {
					return RET_ERROR;
				} else {
					break;
				}
			}
		}
	} else {
		noip_ret_msg(NONE, "no-ip.com: Internal Server Error");
		return RET_ERROR;
	}

	return RET_OK;
}
#else
noip_check_server_msg(int s, char *hostname)
{
	int n;
	char server_msg[BUFSIZE], *ptr;

	/* get server_msg */
	(void)memset(server_msg, 0, sizeof(server_msg));
#ifdef CONFIG_USER_DDNS_LIBSSL
	if (g_noip_isSSL) {
		if(SSL_read(conn, server_msg, sizeof(server_msg) - 1) < 0) {
			noip_ret_msg(PERR, "read() failed");
			return RET_WARNING;
		}
	}
	else {
#endif
		if(read(s, server_msg, sizeof(server_msg) - 1) < 0) {
			noip_ret_msg(PERR, "read() failed");
			return RET_WARNING;
		}
#ifdef CONFIG_USER_DDNS_LIBSSL
	}
#endif

	print_debug("\n\nServer message:"
		    "\n--------------------------------------\n"
		    "%s--------------------------------------\n\n",
		    server_msg);

	if(strstr(server_msg, "HTTP/1.1 200 OK") ||
	   strstr(server_msg, "HTTP/1.0 200 OK")) {

		(void)strtok(server_msg, "\n");
		while((ptr = strtok(NULL, "\n")) != NULL) {
			for(n=0; return_codes[n].code != NULL; n++) {
				if(strstr(ptr, return_codes[n].code)) {
					noip_ret_msg(NONE, "%s: %s",
						hostname, return_codes[n].message);
					if(return_codes[n].error == 1) {
						return RET_ERROR;
					} else {
						return RET_OK;
					}
				}
			}
		}
	} else if(strstr(server_msg, "401 Authorization Required")) {
		noip_ret_msg(NONE, "dyndns.org: wrong username or password");
	} else {
		noip_ret_msg(NONE, "dyndns.org: Internal Server Error");
	}

	return RET_ERROR;
}
#endif