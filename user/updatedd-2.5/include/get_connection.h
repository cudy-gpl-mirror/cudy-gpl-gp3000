/* -- updatedd: get_connection.h --
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

#ifndef GET_CONNECTION_H
#  define GET_CONNECTION_H

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <config/autoconf.h>
#ifdef CONFIG_USER_DDNS_LIBSSL
#include <openssl/ssl.h>
#endif

extern int g_ddns_running;

#ifdef CONFIG_USER_DDNS_LIBSSL
static inline void ssl_handshake(SSL *ssl, int fd) {
	int rv;
	if (SSL_set_fd(ssl, fd) == 0) {
		printf("SSL_set_fd", ERR_error_string(ERR_get_error(), NULL));
	}

	rv = SSL_connect(ssl);
	if (rv <= 0) {
		printf("SSL_connect", ERR_error_string(ERR_get_error(), NULL));
	}
}

static inline void create_ssl_session(SSL_CTX **context, SSL **conn, int fd)
{
	SSL_load_error_strings ();
	SSL_library_init ();
	*context = SSL_CTX_new(SSLv23_client_method());
	*conn = SSL_new(*context);
	ssl_handshake(*conn, fd);
}

static inline void free_ssl_session(SSL_CTX **context, SSL **conn)
{
	SSL_shutdown(*conn);
	SSL_free(*conn);
	SSL_CTX_free(*context);
}
#endif

static inline int
get_connection(const char *hostname, const int port, const char **err)
{

	struct	sockaddr_in addr;
	struct	hostent *host;
	int s;
	FILE *f1, *f2, *f3;
	int spid, c;

	if((host = gethostbyname(hostname)) == NULL) {
		// Mason Yu
#if 0		
		if ( (f1=fopen("/var/run/updatedd.pid","r")) != NULL ) {
			fscanf(f1, "%d\n", &spid);
			printf("get_connection: Remove pid=%d from updatedd.pid.....\n", spid);
		
			f2=fopen("/var/run/updatedd.pid","w");
		
			while ( fscanf(f1, "%d\n", &spid) != EOF ) {
				fprintf(f2,"%d\n",spid);
			}			
					
			fclose(f1);
			fclose(f2);
		
			if ( (f3=fopen("/var/run/updatedd.pid","r")) != NULL ) {			
				if ( fscanf(f3, "%d\n", &spid) == EOF ) {
					g_ddns_running = 2;    // DDNS client is stop
				} else {
					g_ddns_running = 1;    // DDNS client is running
				}
				fclose(f3);
			}
		}				
#endif		
		*err = "gethostbyname() failed";
		return -1;
	}

	addr.sin_family	=	AF_INET;
	addr.sin_port	=	htons(port);
	addr.sin_addr	=	*(struct in_addr*)host->h_addr;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == -1) {
		*err = "socket() failed";
		return -1;
	}

	if(connect(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		*err = "connect() failed";
		return -1;
	}

	return s;

}

#endif /* GET_CONNECTION_H */
