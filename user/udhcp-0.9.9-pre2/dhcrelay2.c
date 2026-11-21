/* dhcrelay.c

   DHCP/BOOTP Relay Agent. */

/*
 * Copyright (c) 1997, 1998, 1999 The Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.
 */

#ifndef EMBED
#ifndef lint
static char ocopyright [] =
"$Id: dhcrelay2.c,v 1.6 2011/01/26 13:46:18 kaohj Exp $ Copyright (c) 1997, 1998, 1999 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */
#endif

#include "debug.h"
#include "dhcpd2.h"
#include <rtk/sysconfig.h>
#include <rtk/utility.h>
#include "dhcpd.h"
#include "options.h"
#include "serverpacket.h"
#include "socket.h"
//#define RELAY_AGENT_ON_WAN

#include <sys/ioctl.h>
#ifdef DHCPD_MULTI_THREAD_SUPPORT
#include <pthread.h>
#endif

#ifdef USE_SOCKET_FALLBACK
# if !defined (USE_SOCKET_SEND)
#  define if_register_send if_register_fallback
#  define send_packet send_fallback
#  define if_reinitialize_send if_reinitialize_fallback
# endif
#endif

// Mason Yu.try123
struct server_list {
	struct server_list *next;
	struct sockaddr_in to;
} *servers;

// Kaohj
int resync_itf = 0;
extern char g_server_ip[];

static void do_percentm PROTO ((char *obuf, char *ibuf));

static char mbuf [1024];
static char fbuf [1024];
struct interface_info *interfaces, *dummy_interfaces, *fallback_interface;
int quiet_interface_discovery;
struct protocol *protocols;
static struct subnet *subnets;

void (*bootp_packet_handler) PROTO ((struct interface_info *, struct dhcp_packet *, int));
static void _got_one(struct protocol *l, struct dhcpMessage *packet, struct sockaddr_in *from, int pkt_len);

int warnings_occurred;

/* Log a note... */

int note (ANSI_DECL (char *) fmt, VA_DOTDOTDOT)
     KandR (char *fmt;)
     va_dcl
{
  va_list list;

  do_percentm (fbuf, fmt);

  VA_start (list, fmt);
  vsnprintf (mbuf, sizeof mbuf, fbuf, list);
  va_end (list);

#ifndef DEBUG
  syslog (log_priority | LOG_INFO, mbuf);
#endif

  if (log_perror) {
	  write (2, mbuf, strlen (mbuf));
	  write (2, "\n", 1);
  }

  return 0;
}

/* Log a debug message... */

int debug (ANSI_DECL (char *) fmt, VA_DOTDOTDOT)
     KandR (char *fmt;)
     va_dcl
{
  va_list list;

  do_percentm (fbuf, fmt);

  VA_start (list, fmt);
  vsnprintf (mbuf, sizeof mbuf, fbuf, list);
  va_end (list);

#ifndef DEBUG
  syslog (log_priority | LOG_DEBUG, mbuf);
#endif

  if (log_perror) {
	  write (2, mbuf, strlen (mbuf));
	  write (2, "\n", 1);
  }

  return 0;
}

char *print_hw_addr (htype, hlen, data)
	int htype;
	int hlen;
	unsigned char *data;
{
	static char habuf [49];
	char *s;
	int i;

	if (htype == 0 || hlen == 0) {
		strcpy (habuf, "<null>");
	} else {
		s = habuf;
		for (i = 0; i < hlen; i++) {
			sprintf (s, "%02x", data [i]);
			s += strlen (s);
			*s++ = ':';
		}
		*--s = 0;
	}
	return habuf;
}

/* Find %m in the input string and substitute an error message string. */

static void do_percentm (obuf, ibuf)
     char *obuf;
     char *ibuf;
{
	char *s = ibuf;
	char *p = obuf;
	int infmt = 0;
	char *m;

	while (*s)
	{
		if (infmt)
		{
			if (*s == 'm')
			{
#ifndef __CYGWIN32__
				m = strerror (errno);
#else
				m = pWSAError ();
#endif
				if (!m)
					m = "<unknown error>";
				strcpy (p - 1, m);
				p += strlen (p);
				++s;
			}
			else
				*p++ = *s++;
			infmt = 0;
		}
		else
		{
			if (*s == '%')
				infmt = 1;
			*p++ = *s++;
		}
	}
	*p = 0;
}

VOIDPTR dmalloc (size, name)
	int size;
	char *name;
{
	VOIDPTR foo = (VOIDPTR)malloc (size);
	if (!foo)
		warn ("No memory for %s.", name);
	else
		memset (foo, 0, size);
	return foo;
}

#if defined (USE_SOCKET_FALLBACK)
/* This just reads in a packet and silently discards it. */

void fallback_discard (protocol)
	struct protocol *protocol;
{
	char buf [1540];
	struct sockaddr_in from;
	int flen = sizeof from;
	int status;
	struct interface_info *interface = protocol -> local;

	status = recvfrom (interface -> wfdesc, buf, sizeof buf, 0,
			   (struct sockaddr *)&from, &flen);
	if (status < 0)
		warn ("fallback_discard: %m");
}
#endif /* USE_SOCKET_FALLBACK */

#if defined (USE_SOCKET_SEND)
int can_receive_unicast_unconfigured (void)
{
#if defined (SOCKET_CAN_RECEIVE_UNICAST_UNCONFIGURED)
	return 1;
#else
	return 0;
#endif
}

/* If we have SO_BINDTODEVICE, set up a fallback interface; otherwise,
   do not. */

void maybe_setup_fallback ()
{
#if defined (USE_SOCKET_FALLBACK)
	struct interface_info *fbi;
	fbi = setup_fallback ();
	if (fbi) {
		fbi -> wfdesc = if_register_socket (fbi);
		add_protocol ("fallback",
			      fbi -> wfdesc, fallback_discard, fbi);
	}
#endif
}
#endif /* USE_SOCKET_SEND */

struct shared_network *new_shared_network (name)
	char *name;
{
	struct shared_network *rval =
		dmalloc (sizeof (struct shared_network), name);
	return rval;
}

struct interface_info *setup_fallback ()
{
	fallback_interface =
		((struct interface_info *)
		 dmalloc (sizeof *fallback_interface, "discover_interfaces"));
	if (!fallback_interface)
		error ("Insufficient memory to record fallback interface.");
	memset (fallback_interface, 0, sizeof *fallback_interface);
	strcpy (fallback_interface -> name, "fallback");
	fallback_interface -> shared_network =
		new_shared_network ("parse_statement");
	if (!fallback_interface -> shared_network)
		error ("No memory for shared subnet");
	memset (fallback_interface -> shared_network, 0,
		sizeof (struct shared_network));
	fallback_interface -> shared_network -> name = "fallback-net";
	return fallback_interface;
}

#include <linux/version.h>
#if LINUX_VERSION_CODE < 0x020100
extern void set_broadcast_route(int add, char *interface_name);
void set_broadcast_route(int add, char *interface_name)
{
	pid_t pid;
	char *argv[16];
	int s, argc = 0;

	/* route add -host 255.255.255.255 ethX */
	if ((pid = vfork()) == 0) { /* child */
		argv[argc++] = "/bin/route";
		if (add)
			argv[argc++] = "add";
		else
			argv[argc++] = "del";
		argv[argc++] = "-host";
		argv[argc++] = "255.255.255.255";
		argv[argc++] = interface_name;
		argv[argc] = NULL;
		execvp(argv[0], argv);
		_exit(0);
	} else if (pid > 0) {
		waitpid(pid, &s, 0);
	}
}
#else
#define set_broadcast_route(x,y)
#endif /* LINUX_VERSION_CODE */

ssize_t send_packet (interface, raw, len, to)
	struct interface_info *interface;	
	struct dhcp_packet *raw;
	size_t len;	
	struct sockaddr_in *to;	
{
	int result;
#ifdef IGNORE_HOSTUNREACH
	int retry = 0;
#endif	
	set_broadcast_route(1, interface->name);

#ifdef IGNORE_HOSTUNREACH
	do {
#endif
		result = sendto (interface -> wfdesc, (char *)raw, len, 0,
				 (struct sockaddr *)to, sizeof *to);
#ifdef IGNORE_HOSTUNREACH
	} while (to -> sin_addr.s_addr == htonl (INADDR_BROADCAST) &&
		 result < 0 &&
		 (errno == EHOSTUNREACH ||
		  errno == ECONNREFUSED) &&
		 retry++ < 10);
#endif
	if (result < 0) {
		warn ("send_packet_sock: %m");
		if (errno == ENETUNREACH)
			warn ("send_packet: please consult README file %s",
			      "regarding broadcast address.");
	}

	set_broadcast_route(0, interface->name);

	return result;
}

ssize_t send_packet_toLAN (interface, raw, to, hto)
	struct interface_info *interface;
	struct dhcp_packet *raw;
	struct sockaddr_in *to;
	struct hardware *hto;
{
	int ifindex;
	u_int32_t serverip;
	unsigned char arp[6];

	if (read_interface(interface->name, &ifindex, &serverip, arp) < 0) {
		note("Read ifindex of br0 fail");
		return -1;
	}

	return relay_raw_packet(raw, serverip, SERVER_PORT,
			to->sin_addr.s_addr, CLIENT_PORT, hto->haddr, ifindex);
}

ssize_t receive_packet (interface, buf, len, from)
	struct interface_info *interface;
	unsigned char *buf;
	size_t len;
	struct sockaddr_in *from;	
{
	int flen = sizeof *from;
	int result;	
	
#ifdef IGNORE_HOSTUNREACH
	int retry = 0;
	do {
#endif
		result = recvfrom (interface -> rfdesc, (char *)buf, len, 0,
				   (struct sockaddr *)from, (socklen_t *)&flen);
#ifdef IGNORE_HOSTUNREACH
	} while (result < 0 &&
		 (errno == EHOSTUNREACH ||
		  errno == ECONNREFUSED) &&
		 retry++ < 10);
#endif
	return result;
}

/* Generic interface registration routine... */
int if_register_socket (info)
	struct interface_info *info;
{
	struct sockaddr_in name;
	int sock;
	int flag;

#if !defined (HAVE_SO_BINDTODEVICE) && !defined (USE_FALLBACK)
	/* Make sure only one interface is registered. */
	if (once)
		error ("The standard socket API can only support %s",
		       "hosts with a single network interface.");
	once = 1;
#endif

	/* Set up the address we're going to bind to. */
	name.sin_family = AF_INET;
	name.sin_port = local_port;
	name.sin_addr.s_addr = INADDR_ANY;
	memset (name.sin_zero, 0, sizeof (name.sin_zero));

	/* Make a socket... */
	if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		error ("Can't create dhcp socket: %m");

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
		error("%s Could not set noblock\n", __func__);
#endif

	/* Set the REUSEADDR option so that we don't fail to start if
	   we're being restarted. */
	flag = 1;
	if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&flag, sizeof flag) < 0)
		error ("Can't set SO_REUSEADDR option on dhcp socket: %m");

	/* Set the BROADCAST option so that we can broadcast DHCP responses. */
	if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST,
			(char *)&flag, sizeof flag) < 0)
		error ("Can't set SO_BROADCAST option on dhcp socket: %m");

	/* Bind the socket to this interface's IP address. */
	if (bind (sock, (struct sockaddr *)&name, sizeof name) < 0)
		error ("Can't bind to dhcp address: %m");

#if defined (HAVE_SO_BINDTODEVICE)
	/* Bind this socket to this interface. */
	if (info -> ifp &&
	    setsockopt (sock, SOL_SOCKET, SO_BINDTODEVICE,
			(char *)(info -> ifp), sizeof *(info -> ifp)) < 0) {
		error("setsockopt: SO_BINDTODEVICE: %m");
	}
#endif

	return sock;
}

/* Add a protocol to the list of protocols... */
void add_protocol (name, fd, handler, local)
	char *name;
	int fd;
	void (*handler) PROTO ((struct protocol *));
	void *local;
{
	struct protocol *p;

	p = (struct protocol *)malloc (sizeof *p);
	if (!p)
		error ("can't allocate protocol struct for %s", name);

	p -> fd = fd;
	p -> handler = handler;
	p -> local = local;

	p -> next = protocols;
	protocols = p;
}

/* Return just the network number of an internet address... */

struct iaddr *subnet_number (addr, mask)
	struct iaddr *addr;
	struct iaddr *mask;
{
	int i;
	static struct iaddr rv;

	memset(&rv, 0, sizeof(rv));

	/* Both addresses must have the same length... */
	if (addr->len != mask->len)
		return &rv;

	rv.len = addr->len;
	for (i = 0; i < rv.len; i++)
		rv.iabuf [i] = addr->iabuf [i] & mask->iabuf [i];
	return &rv;
}

int addr_eq (addr1, addr2)
	struct iaddr *addr1, *addr2;
{
	if (addr1->len != addr2->len)
		return 0;
	return memcmp (addr1->iabuf, addr2->iabuf, addr1->len) == 0;
}


struct subnet *find_subnet (addr)
	struct iaddr *addr;
{
	struct subnet *rv;

	for (rv = subnets; rv; rv = rv -> next_subnet) {
		if (addr_eq (subnet_number (addr, &rv -> netmask), &rv -> net))
			return rv;
	}
	return (struct subnet *)0;
}

void if_register_send (info)
	struct interface_info *info;
{
	// Kaohj, not register interface twice
	if (info->wfdesc!=0)
		return;
#ifndef USE_SOCKET_RECEIVE
	info -> wfdesc = if_register_socket (info);
#else
	info -> wfdesc = info -> rfdesc;
#endif
	if (!quiet_interface_discovery)
		note ("Sending on   Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}

void if_register_receive (info)
	struct interface_info *info;
{
	// Kaohj, not register interface twice
	if (info->rfdesc!=0)
		return;
	/* If we're using the socket API for sending and receiving,
	   we don't need to register this interface twice. */
	info -> rfdesc = if_register_socket (info);
	if (!quiet_interface_discovery)
		note ("Listening on Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}

#ifdef DHCPD_MULTI_THREAD_SUPPORT
struct thread_para_st {
	struct protocol *l;
	struct sockaddr_in from;
	struct dhcpMessage *packet;
	int pkt_len;
};

void * server_rx_process(void *data)
{
	struct thread_para_st *tp = (struct thread_para_st *)data;

	_got_one(tp->l, tp->packet, &tp->from, tp->pkt_len);

	/* free receive packet here to avoid memory leak */
	free(tp->packet);
	free(data);
	
	// We can use pthread_exit() or return to terminate this thread.
	pthread_exit(NULL);
}
#endif

#ifdef USE_POLL
/* Wait for packets to come in using poll().  When a packet comes in,
   call receive_packet to receive the packet and possibly strip hardware
   addressing information from it, and then call through the
   bootp_packet_handler hook to try to do something with it. */
// Mason Yu. signal123
void dispatch ()  // try123
{
	struct protocol *l;
	int nfds = 0;
	struct pollfd *fds;
	int count;
	int i;
	int to_msec;

	nfds = 0;
	for (l = protocols; l; l = l -> next) {
		++nfds;
	}
	fds = (struct pollfd *)malloc ((nfds) * sizeof (struct pollfd));
	if (!fds)
		error ("Can't allocate poll structures.");

	do {
#if defined(_DHCP_DEBUG_ON_OFF_)
		/* Enable dhcp_debug dynamically */
		dhcp_debug_onoff_by_file(DHCPDEBUG_FLAG_READ);
#endif
	
		to_msec = -1;

		/* Set up the descriptors to be polled. */
		i = 0;
		for (l = protocols; l; l = l -> next) {
			fds [i].fd = l -> fd;
			fds [i].events = POLLIN;
			fds [i].revents = 0;
			++i;
		}

		/* Wait for a packet or a timeout... XXX */
		count = poll (fds, nfds, to_msec);

		/* Not likely to be transitory... */
		if (count < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			else
				error ("poll: %m");
		}

		i = 0;
		for (l = protocols; l; l = l -> next) {
			if ((fds [i].revents & POLLIN)) {
				fds [i].revents = 0;
				if (l -> handler)
					(*(l -> handler)) (l);
			}
			++i;
		}
	} while (1);
}
#else

// Mason Yu. signal123. Start
static int signal_pipe[2];

/* Signal handler */
static void signal_handler(int sig)
{
	if (send(signal_pipe[1], &sig, sizeof(sig), MSG_DONTWAIT) < 0) {
		//LOG(LOG_ERR, "Could not send signal: %s",
		//	strerror(errno));
		printf("Could not send signal: %s", strerror(errno));
	}
}
// Mason Yu. signal123. End

/* Wait for packets to come in using select().   When one does, call
   receive_packet to receive the packet and possibly strip hardware
   addressing information from it, and then call through the
   bootp_packet_handler hook to try to do something with it. */
// Mason Yu. signal123
void dispatch ()  // try123
{
	fd_set r, w, x;
	struct protocol *l;
	int max = 0;
	int count;
	struct timeval tv, *tvp;
	// Mason Yu. combine server with relay.
	int sig;
	//unsigned long timeout_end;
	struct sigaction sigact;	
	sigset_t  tmpset;

#if defined(CONFIG_CMCC_AP) || defined(CONFIG_CU_AP) || defined(CONFIG_CTC_AP)
	int ret=0,fd_flag=0;
	usock_entry_t dhcp_usock_entry;
	
	unlink(DHCPD_USOCK_SERVER);
	ret = unixsock_open(1, &dhcp_usock_entry, DHCPD_USOCK_SERVER);

	if(ret < 0){
		perror("dhcpd unixsock_open fail");
		unlink(DHCPD_USOCK_SERVER);
		return;
	}
	fd_flag = fcntl(dhcp_usock_entry.sock, F_GETFL, 0); 
	fcntl(dhcp_usock_entry.sock, F_SETFL, fd_flag | O_NONBLOCK); 
#endif
	// Mason Yu. signal123
	socketpair(AF_UNIX, SOCK_STREAM, 0, signal_pipe);
	sigact.sa_handler = signal_handler;
	sigact.sa_flags = 0;
	sigemptyset(&sigact.sa_mask);
	sigaction(SIGUSR1, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGALRM, &sigact, NULL);
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
	sigaction(38, &sigact, NULL);
	signal(SIGPIPE, SIG_IGN);
	
	sigfillset(&tmpset);
	sigprocmask(SIG_UNBLOCK, &tmpset, NULL);

	FD_ZERO (&w);
	FD_ZERO (&x);

	do {
#if defined(_DHCP_DEBUG_ON_OFF_)
		/* Enable dhcp_debug dynamically */
		dhcp_debug_onoff_by_file(DHCPDEBUG_FLAG_READ);
#endif

		tvp = (struct timeval *)0;

		/* Set up the read mask. */
		FD_ZERO (&r);

		for (l = protocols; l; l = l -> next) {
			FD_SET (l -> fd, &r);
			if (l -> fd > max)
				max = l -> fd;
		}

		// Masonn Yu. Add sock for DHCP Server pipe. // try123
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	if ( g_dhcp_mode == LAN_SERVER || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {
		memcpy(&server_config,p_serverpool_config,sizeof(struct server_config_t));
	}
#endif
		// Mason Yu. signal123
		FD_SET(signal_pipe[0], &r);
		max = max > signal_pipe[0] ? max : signal_pipe[0];
		
#if defined(CONFIG_CMCC_AP) || defined(CONFIG_CU_AP) || defined(CONFIG_CTC_AP)
		FD_SET(dhcp_usock_entry.sock, &r);
		max = max > dhcp_usock_entry.sock ? max : dhcp_usock_entry.sock;
#endif

		/* Wait for a packet or a timeout... XXX */
		count = select (max + 1, &r, &w, &x, tvp);

		// Mason Yu. signal123
		if (FD_ISSET(signal_pipe[0], &r)) {
			if (read(signal_pipe[0], &sig, sizeof(sig)) < 0)
				continue; /* probably just EINTR */
			switch (sig) {
			case SIGUSR1:
//				LOG(LOG_INFO, "Received a SIGUSR1");
				if ( g_dhcp_mode == LAN_SERVER || g_dhcp_mode == LAN_SERVER_AND_RELAY )
				{
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					thread_mutex_lock();
#endif
					write_leases();
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					thread_mutex_unlock();
#endif
				}
				/* why not just reset the timeout, eh */
				//timeout_end = time(0) + server_config.auto_time;
				continue;
			case SIGTERM:
				//LOG(LOG_INFO, "Received a SIGTERM");
				exit_server(0);
				continue;
			case SIGHUP:
				//LOG(LOG_INFO, "Received a SIGHUP");
				// re-create socket pair
				// Mason Yu. signal123
				socketpair(AF_UNIX, SOCK_STREAM, 0, signal_pipe);
				continue;

			case SIGALRM:
				/* Check whether there are expired leases. */
#ifdef DHCPD_MULTI_THREAD_SUPPORT
				thread_mutex_lock();
#endif
				lease_timer();
#ifdef DHCPD_MULTI_THREAD_SUPPORT
				thread_mutex_unlock();
#endif
				continue;
			// Mason Yu. try123
			case SIGUSR2:
				// Mason Yu. resync handler of interface's protocol
				//refresh_itf();
				discover_interfaces (DISCOVER_RUNNING);
				dump_leases();
				continue;
			// Mason Yu. SIGRTMIN for DHCP Relay.
			case 38: // It is a real time signal(SIGRTMIN). It should not be considered constants from userland.
				// Mason Yu. resync handler of interface's protocol
				discover_interfaces (DISCOVER_RELAY);
				continue;
			}
		}
#if defined(CONFIG_CMCC_AP) || defined(CONFIG_CU_AP) || defined(CONFIG_CTC_AP)
		if(FD_ISSET(dhcp_usock_entry.sock, &r)){
			char buff[256] = {0};
			int write_leases_done=0;
			while(unixsock_recv(&dhcp_usock_entry, buff, sizeof(buff) - 1, 0) > 0){
				if(!strcmp(buff, "get_DhcpClientLeases")){	//copy from SIGUSR1
					//printf("Received usock message for get_DhcpClientLeases\n");
					if (( g_dhcp_mode == LAN_SERVER || g_dhcp_mode == LAN_SERVER_AND_RELAY ) && !write_leases_done){
						write_leases_done=1;
#ifdef DHCPD_MULTI_THREAD_SUPPORT
						thread_mutex_lock();
#endif
						write_leases();
#ifdef DHCPD_MULTI_THREAD_SUPPORT
						thread_mutex_unlock();
#endif
					}
				}
			}
		}
#endif
		// Kaohj, for dhcrelay to resync the dynamic interfaces
		// Note, I don't check for other processes (ie. dhcp server, client ...)
	      	// Mason Yu. resync handler of interface's protocol
#if 0
		if (resync_itf) {
			/* Discover all the network interfaces. */
			discover_interfaces (DISCOVER_RUNNING);
			resync_itf = 0;
			continue;
		}
#endif

		/* Not likely to be transitory... */
		if (count < 0)
			error ("select: %m");

		for (l = protocols; l; l = l -> next) {
			if (!FD_ISSET (l -> fd, &r))
				continue;
			if (l -> handler)
				(*(l -> handler)) (l);
		}
	} while (1);
}
#endif

static void _got_one(struct protocol *l, struct dhcpMessage *packet, struct sockaddr_in *from, int pkt_len)
{
	struct interface_info *ip = l -> local;
	struct iaddr ifrom;

	//store server_config pool for eash thread
	struct server_config_t *server_config_p = &server_config;
	
#ifdef _PRMT_X_CT_COM_LANBINDING_CONFIG_
	unsigned char *state;
	int lan_binding_flag = 0;
	mib_get_s(PROVINCE_SICHUAN_MIB_LAN_BINDING_CONFIG_ENABLE, &lan_binding_flag, sizeof(lan_binding_flag));
	
	if(lan_binding_flag)
	{
		int i = 0, ret=0;
		unsigned char vendorclass[256] = {0};
		unsigned char *tmpstr = NULL;
		unsigned char len = 0;
		if ((state = get_option(packet, DHCP_MESSAGE_TYPE)) == NULL) {
			printf("couldn't get option from packet, ignoring\n");
			return 0;
		}
		if(state[0] == DHCPDISCOVER || state[0] == DHCPREQUEST)
		{
			tmpstr = get_option(packet, DHCP_VENDOR);
			if(tmpstr != NULL)
			{
				len=*(unsigned char*)(tmpstr-OPT_LEN);
			    if (len >= sizeof(vendorclass)) {
					fprintf(stderr, "Error: Data length %d exceeds buffer size %zu!\n", len, sizeof(vendorclass));
					len = sizeof(vendorclass) - 1;
				}

				memcpy(vendorclass,tmpstr,len);
				vendorclass[len]='\0';
				
				strtoupper(vendorclass);
				if(strstr(vendorclass, "SCITV") || strstr(vendorclass, "@ITV")
					|| ((*(vendorclass+2)==0x1f)&&(*(vendorclass+4)==0x01)))
				{
					//RTK_RG_acl_Add_Lan_Binding(packet->chaddr);// mac binding to other bridge
					ret = check_lan_binding_port_by_mac(packet->chaddr);
					send_packet_to_other_bridge(packet); //Resend duplicated packet to wan interface
					return;
				}
				else
					printf("nomorl dhcp %s\n", state[0] == DHCPDISCOVER ? "DISCOVER" : "REQUEST");
			}
		}
	}
#endif

	// Mason Yu. Process packet by Server or relay
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	if ( g_dhcp_mode == LAN_SERVER || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {
		find_match_serving_pool(packet, &server_config_p);
#ifndef DHCPD_MULTI_THREAD_SUPPORT
		memcpy(&server_config, server_config_p, sizeof(struct server_config_t));
#endif
	}
#endif

	if ( g_dhcp_mode == LAN_SERVER_AND_RELAY ) {
		if( server_config.locallyserved==0 ) {
			// By Relay
			//LOG(LOG_INFO, "should be handled by relay, skip this DHCP message(0x%x)!", server_config.dhcpserver );
			if (bootp_packet_handler) {
				servers->to.sin_addr.s_addr = server_config.dhcpserver;

				ifrom.len = sizeof(struct in_addr);

				if (ifrom.len > sizeof(ifrom.iabuf)) {
					fprintf(stderr, "Length exceeds buffer size, truncating to fit\n");
					ifrom.len = sizeof(ifrom.iabuf);
				}
				memcpy(ifrom.iabuf, &from->sin_addr, ifrom.len);

				(*bootp_packet_handler) (ip, (struct dhcp_packet *)packet, pkt_len);
			}
		}
		else { // By Server
			server_func(packet, server_config_p);
		}
	} else if ( g_dhcp_mode == LAN_SERVER ) {
		server_func(packet, server_config_p);
	} else if ( g_dhcp_mode == LAN_RELAY ) {
		if (bootp_packet_handler) {
			//printf("servers->to.sin_addr.s_addr=0x%x\n", servers->to.sin_addr.s_addr);
			ifrom.len = sizeof(struct in_addr);

			if (ifrom.len > sizeof(ifrom.iabuf)) {
				fprintf(stderr, "Length exceeds buffer size, truncating to fit\n");
				ifrom.len = sizeof(ifrom.iabuf);
			}
			memcpy(ifrom.iabuf, &from->sin_addr, ifrom.len);

			(*bootp_packet_handler) (ip, (struct dhcp_packet *)packet, pkt_len);
		}
	}
}

void got_one (l)
	struct protocol *l;
{
	struct sockaddr_in from;	
	int result;
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	union {
		//unsigned char packbuf [4096];
		/* Packet input buffer.
					 	 Must be as large as largest
						 possible MTU. */ /*MN - make smaller*/
		unsigned char packbuf [1500];
		struct dhcp_packet packet;
		// Mason Yu. try123
		struct dhcpMessage packet_server;
	} u, *pU;
	int ret, newThread = 1;
#else
	union {
		//unsigned char packbuf [4096];
		/* Packet input buffer.
					 	 Must be as large as largest
						 possible MTU. */ /*MN - make smaller*/
		unsigned char packbuf [1500];
		struct dhcp_packet packet;
		// Mason Yu. try123
		struct dhcpMessage packet_server;
	} u;
#endif
	struct interface_info *ip = l -> local;

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	pid_t dhcpd_pid = getpid(); 
	int thread_count=0;
	thread_count= count_threads_num(dhcpd_pid);
	if(thread_count >= MAX_DHCPD_THREAD_NUM){
		LOG(LOG_DEBUG, "%s:dhcpd thread num is %d >= limited num %d, dhcpd_pid=%d",__func__,thread_count, MAX_DHCPD_THREAD_NUM, dhcpd_pid);
		newThread = 0;
		pU = &u;
	}
	else{
		pU = malloc(1500);
		if (NULL == pU)
		{
			printf("%s %d fatal error, malloc fail\n", __func__, __LINE__);
			newThread = 0;
			pU = &u;
		}
		else
		{
			newThread = 1;
		}
	}
	result = receive_packet (ip, pU->packbuf, sizeof u, &from);
#else
	result = receive_packet (ip, u.packbuf, sizeof u, &from);
#endif
	//DEBUG(LOG_DEBUG, "%s tid=%d\n", __func__, pthread_self());
	if (result < 0) {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
		warn ("receive_packet failed on %s: %m", ip -> name);
		if (newThread)
			free(pU);
#endif
		return;
	}
	if (result == 0)
	{
#ifdef DHCPD_MULTI_THREAD_SUPPORT
		if (newThread)
			free(pU);
#endif
		return;
	}

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	if (newThread)
	{
		struct thread_para_st *tp = malloc(sizeof(struct thread_para_st));
		pthread_t ptID;

		if (tp == NULL)
		{
			printf("%s %d fatal error, malloc fail\n", __func__, __LINE__);
			free(pU);
			return;
		}
		tp->l = l;
		tp->packet = &pU->packet_server;
		tp->from = from;
		tp->pkt_len = result;
		ret = pthread_create(&ptID, NULL, server_rx_process, (void *)tp);
		if (ret != 0)
		{
			printf("%s create pthread fail, %s\n", __func__, strerror(errno));
			
			_got_one(l, &pU->packet_server, &from, result);
			
			free(pU);
			free(tp);
		}
		else
		{
			ret = pthread_detach(ptID);
			if (ret != 0)
			{
				printf("%s detach pthread fail, %s\n", __func__, strerror(errno));
				return;
			}
		}
		usleep(1000);
	}
	else
		_got_one(l, &pU->packet_server, &from, result);
#else
	_got_one(l, &u.packet_server, &from, result);
#endif

}


/* Use the SIOCGIFCONF ioctl to get a list of all the attached interfaces.
   For each interface that's of type INET and not the loopback interface,
   register that interface with the network I/O software, figure out what
   subnet it's on, and add it to the list of interfaces. */

void discover_interfaces (state)
	int state;
{
	struct interface_info *tmp;
	struct interface_info *last, *next;
	/*MN - to big
	char buf [8192];
	*/
	char buf [4096];
	struct ifconf ic;
	struct ifreq ifr;
	int i;
	int sock;
	struct subnet *subnet;
	struct shared_network *share;
	struct sockaddr_in foo;
	int ir;
	struct ifreq *tif;
#ifdef ALIAS_NAMES_PERMUTED
	char *s;
#endif

	// We should free intefaces and potocols
	while (interfaces !=NULL) {
		struct interface_info *p = interfaces;
		interfaces = p ->next;
		if (p->rfdesc != 0)
			close(p->rfdesc);
		if (p->wfdesc != 0)
			close(p->wfdesc);
		if (p->ifp !=0)
			free(p->ifp);
		free(p);
	}

	while (protocols !=NULL) {
		struct protocol *p = protocols;
		protocols = p ->next;
		free(p);
	}

	/* Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. */
	if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		error ("Can't create addrlist socket");

	/* Get the interface configuration information... */
	ic.ifc_len = sizeof buf;
	ic.ifc_ifcu.ifcu_buf = (caddr_t)buf;
	i = ioctl(sock, SIOCGIFCONF, &ic);

	if (i < 0)
		error ("ioctl: SIOCGIFCONF: %m");

	/* If we already have a list of interfaces, and we're running as
	   a DHCP server, the interfaces were requested. */
	if (interfaces && (state == DISCOVER_SERVER ||
			   state == DISCOVER_RELAY ||
			   state == DISCOVER_REQUESTED))
		ir = 0;
	else if (state == DISCOVER_UNCONFIGURED)
		ir = INTERFACE_REQUESTED | INTERFACE_AUTOMATIC;
	else
		ir = INTERFACE_REQUESTED;

	/* Cycle through the list of interfaces looking for IP addresses. */
	for (i = 0; i < ic.ifc_len;) {
		struct ifreq *ifp = (struct ifreq *)((caddr_t)ic.ifc_req + i);
#ifdef HAVE_SA_LEN
		if (ifp -> ifr_addr.sa_len > sizeof (struct sockaddr))
			i += (sizeof ifp -> ifr_name) + ifp -> ifr_addr.sa_len;
		else
#endif
			i += sizeof *ifp;

#ifdef ALIAS_NAMES_PERMUTED
		if ((s = strrchr (ifp -> ifr_name, ':'))) {
			*s = 0;
		}
#endif

#ifdef SKIP_DUMMY_INTERFACES
		if (!strncmp (ifp -> ifr_name, "dummy", 5))
			continue;
#endif

		if(g_dhcp_mode != LAN_RELAY
			&& strcmp(server_config.interface, ifp->ifr_name) != 0)
		{
			continue;
		}

		/* See if this is the sort of interface we want to
		   deal with. */
		// Mason Yu Test
		//note("interfaces=%d ifp->ifr_name=%s\n", interfaces, ifp -> ifr_name);
		strcpy (ifr.ifr_name, ifp -> ifr_name);
		if (ioctl (sock, SIOCGIFFLAGS, &ifr) < 0)
			error ("Can't get interface flags for %s: %m",
			       ifr.ifr_name);

		/* Skip loopback, point-to-point and down interfaces,
		   except don't skip down interfaces if we're trying to
		   get a list of configurable interfaces. */
		if ((ifr.ifr_flags & IFF_LOOPBACK) ||
#ifdef HAVE_IFF_POINTOPOINT
		    // Marked by Mason Yu. For suport PPPoE
		    //(ifr.ifr_flags & IFF_POINTOPOINT) ||
#endif
		    (!(ifr.ifr_flags & IFF_UP) &&
		     state != DISCOVER_UNCONFIGURED))
			continue;

		/* See if we've seen an interface that matches this one. */
		for (tmp = interfaces; tmp; tmp = tmp -> next)
			if (!strcmp (tmp -> name, ifp -> ifr_name))
				break;

		/* If there isn't already an interface by this name,
		   allocate one. */
		if (!tmp) {
			tmp = ((struct interface_info *)
			       dmalloc (sizeof *tmp, "discover_interfaces"));
			if (!tmp)
				error ("Insufficient memory to %s %s",
				       "record interface", ifp -> ifr_name);
			memset (tmp, 0, sizeof *tmp);
			strcpy (tmp -> name, ifp -> ifr_name);
			tmp -> next = interfaces;
			tmp -> flags = ir;
			// Kaohj
			//tmp->rfdesc=tmp->wfdesc=0;
			interfaces = tmp;
		}

		/* If we have the capability, extract link information
		   and record it in a linked list. */
#ifdef HAVE_AF_LINK
		if (ifp -> ifr_addr.sa_family == AF_LINK) {
			struct sockaddr_dl *foo = ((struct sockaddr_dl *)
						   (&ifp -> ifr_addr));
			tmp -> hw_address.hlen = foo -> sdl_alen;
			tmp -> hw_address.htype = HTYPE_ETHER; /* XXX */
			memcpy (tmp -> hw_address.haddr,
				LLADDR (foo), foo -> sdl_alen);
		} else
#endif /* AF_LINK */

		if (ifp -> ifr_addr.sa_family == AF_INET) {
			struct iaddr addr;

			/* Get a pointer to the address... */
			memcpy (&foo, &ifp -> ifr_addr,
				sizeof ifp -> ifr_addr);

			/* We don't want the loopback interface. */
			if (foo.sin_addr.s_addr == htonl (INADDR_LOOPBACK))
				continue;


			/* If this is the first real IP address we've
			   found, keep a pointer to ifreq structure in
			   which we found it. */
			if (!tmp -> ifp) {
#ifdef HAVE_SA_LEN
				int len = ((sizeof ifp -> ifr_name) +
					   ifp -> ifr_addr.sa_len);
#else
				int len = sizeof *ifp;
#endif
				tif = (struct ifreq *)malloc (len);
				if (!tif)
					error ("no space to remember ifp.");
				memcpy (tif, ifp, len);
				tmp -> ifp = tif;
				tmp -> primary_address = foo.sin_addr;
			}

			/* Grab the address... */
			addr.len = 4;

			if (addr.len > sizeof(addr.iabuf))
				addr.len = sizeof(addr.iabuf);
			memcpy (addr.iabuf, &foo.sin_addr.s_addr,addr.len);

			/* If there's a registered subnet for this address,
			   connect it together... */
			if ((subnet = find_subnet (&addr))) {
				/* If this interface has multiple aliases
				   on the same subnet, ignore all but the
				   first we encounter. */
				if (!subnet -> interface) {
					subnet -> interface = tmp;
					subnet -> interface_address = addr;
				} else if (subnet -> interface != tmp) {
					warn ("Multiple %s %s: %s %s",
					      "interfaces match the",
					      "same subnet",
					      subnet -> interface -> name,
					      tmp -> name);
				}
				share = subnet -> shared_network;
				if (tmp -> shared_network &&
				    tmp -> shared_network != share) {
					warn ("Interface %s matches %s",
					      tmp -> name,
					      "multiple shared networks");
				} else {
					tmp -> shared_network = share;
				}

				if (!share -> interface) {
					share -> interface = tmp;
				} else if (share -> interface != tmp) {
					warn ("Multiple %s %s: %s %s",
					      "interfaces match the",
					      "same shared network",
					      share -> interface -> name,
					      tmp -> name);
				}
			}
		}
	}

#if defined (LINUX_SLASHPROC_DISCOVERY)
	/* On Linux, interfaces that don't have IP addresses don't show up
	   in the SIOCGIFCONF syscall.   We got away with this prior to
	   Linux 2.1 because we would give each interface an IP address of
	   0.0.0.0 before trying to boot, but that doesn't work after 2.1
	   because we're using LPF, because we can't configure interfaces
	   with IP addresses of 0.0.0.0 anymore (grumble).   This only
	   matters for the DHCP client, of course - the relay agent and
	   server should only care about interfaces that are configured
	   with IP addresses anyway.

	   The PROCDEV_DEVICE (/proc/net/dev) is a kernel-supplied file
	   that, when read, prints a human readable network status.   We
	   extract the names of the network devices by skipping the first
	   two lines (which are header) and then parsing off everything
	   up to the colon in each subsequent line - these lines start
	   with the interface name, then a colon, then a bunch of
	   statistics.   Yes, Virgina, this is a kludge, but you work
	   with what you have. */

	if (state == DISCOVER_UNCONFIGURED) {
		FILE *proc_dev;
		char buffer [256];
		int skip = 2;

		proc_dev = fopen (PROCDEV_DEVICE, "r");
		if (!proc_dev)
			error ("%s: %m", PROCDEV_DEVICE);

		while (fgets (buffer, sizeof buffer, proc_dev)) {
			char *name = buffer;
			char *sep;

			/* Skip the first two blocks, which are header
			   lines. */
			if (skip) {
				--skip;
				continue;
			}

			sep = strrchr (buffer, ':');
			if (sep)
				*sep = '\0';
			while (*name == ' ')
				name++;

			/* See if we've seen an interface that matches
			   this one. */
			for (tmp = interfaces; tmp; tmp = tmp -> next)
				if (!strcmp (tmp -> name, name))
					break;

			/* If we found one, nothing more to do.. */
			if (tmp)
				continue;

			/* Otherwise, allocate one. */
			tmp = ((struct interface_info *)
			       dmalloc (sizeof *tmp, "discover_interfaces"));
			if (!tmp)
				error ("Insufficient memory to %s %s",
				       "record interface", name);
			memset (tmp, 0, sizeof *tmp);
			strcpy (tmp -> name, name);

			tmp -> flags = ir;
			tmp -> next = interfaces;
			interfaces = tmp;
		}
		fclose (proc_dev);
	}
#endif

	/* Now cycle through all the interfaces we found, looking for
	   hardware addresses. */
#if defined (HAVE_SIOCGIFHWADDR) && !defined (HAVE_AF_LINK)
	for (tmp = interfaces; tmp; tmp = tmp -> next) {
		struct ifreq ifr;
		struct sockaddr sa;
		int b, sk;

		if (!tmp -> ifp) {
			/* Make up an ifreq structure. */
			tif = (struct ifreq *)malloc (sizeof (struct ifreq));
			if (!tif)
				error ("no space to remember ifp.");
			memset (tif, 0, sizeof (struct ifreq));
			strcpy (tif -> ifr_name, tmp -> name);
			tmp -> ifp = tif;
		}

		/* Read the hardware address from this interface. */
		ifr = *tmp -> ifp;
		if (ioctl (sock, SIOCGIFHWADDR, &ifr) < 0)
			continue;

		sa = *(struct sockaddr *)&ifr.ifr_hwaddr;

		switch (sa.sa_family) {
#ifdef HAVE_ARPHRD_TUNNEL
		      case ARPHRD_TUNNEL:
			/* ignore tunnel interfaces. */
#endif
#ifdef HAVE_ARPHRD_ROSE
		      case ARPHRD_ROSE:
#endif
#ifdef HAVE_ARPHRD_LOOPBACK
		      case ARPHRD_LOOPBACK:
			/* ignore loopback interface */
			break;
#endif

		      case ARPHRD_ETHER:
			tmp -> hw_address.hlen = 6;
			tmp -> hw_address.htype = ARPHRD_ETHER;
			memcpy (tmp -> hw_address.haddr, sa.sa_data, 6);
			break;

#ifndef HAVE_ARPHRD_IEEE802
# define ARPHRD_IEEE802 HTYPE_IEEE802
#endif
		      case ARPHRD_IEEE802:
			tmp -> hw_address.hlen = 6;
			tmp -> hw_address.htype = ARPHRD_IEEE802;
			memcpy (tmp -> hw_address.haddr, sa.sa_data, 6);
			break;

#ifndef HAVE_ARPHRD_FDDI
# define ARPHRD_FDDI HTYPE_FDDI
#endif
		      case ARPHRD_FDDI:
			tmp -> hw_address.hlen = 16;
			tmp -> hw_address.htype = HTYPE_FDDI; /* XXX */
			memcpy (tmp -> hw_address.haddr, sa.sa_data, 16);
			break;

#ifdef HAVE_ARPHRD_METRICOM
		      case ARPHRD_METRICOM:
			tmp -> hw_address.hlen = 6;
			tmp -> hw_address.htype = ARPHRD_METRICOM;
			memcpy (tmp -> hw_address.haddr, sa.sa_data, 6);
			break;
#endif

#ifdef HAVE_ARPHRD_AX25
		      case ARPHRD_AX25:
			tmp -> hw_address.hlen = 6;
			tmp -> hw_address.htype = ARPHRD_AX25;
			memcpy (tmp -> hw_address.haddr, sa.sa_data, 6);
			break;
#endif

#ifdef HAVE_ARPHRD_NETROM
		      case ARPHRD_NETROM:
			tmp -> hw_address.hlen = 6;
			tmp -> hw_address.htype = ARPHRD_NETROM;
			memcpy (tmp -> hw_address.haddr, sa.sa_data, 6);
			break;
#endif

		      default:
			warn ("%s: unknown hardware address type %d",
			       ifr.ifr_name, sa.sa_family);
			break;
		}
	}
#endif /* defined (HAVE_SIOCGIFHWADDR) && !defined (HAVE_AF_LINK) */


	/* If we're just trying to get a list of interfaces that we might
	   be able to configure, we can quit now. */
	if (state == DISCOVER_UNCONFIGURED)
	{
		close (sock);
		return;
	}
		
	/* Weed out the interfaces that did not have IP addresses. */
	last = (struct interface_info *)0;
	for (tmp = interfaces; tmp; tmp = next) {
		next = tmp -> next;
		if ((tmp -> flags & INTERFACE_AUTOMATIC) &&
		    state == DISCOVER_REQUESTED)
			tmp -> flags &= ~(INTERFACE_AUTOMATIC |
					  INTERFACE_REQUESTED);
		if (!tmp -> ifp || !(tmp -> flags & INTERFACE_REQUESTED)) {
			if ((tmp -> flags & INTERFACE_REQUESTED) != (unsigned int)ir)
				error ("%s: not found", tmp -> name);
			if (!last)
				interfaces = interfaces -> next;
			else
				last -> next = tmp -> next;

			/* Remember the interface in case we need to know
			   about it later. */
			tmp -> next = dummy_interfaces;
			dummy_interfaces = tmp;
			continue;
		}
		last = tmp;

		memcpy (&foo, &tmp -> ifp -> ifr_addr,
			sizeof tmp -> ifp -> ifr_addr);

		/* We must have a subnet declaration for each interface. */
		if (!tmp -> shared_network && (state == DISCOVER_SERVER)) {
			warn ("No subnet declaration for %s (%s).",
			      tmp -> name, inet_ntoa (foo.sin_addr));
			warn ("Please write a subnet declaration in your %s",
			      "dhcpd.conf file for the");
			error ("network segment to which interface %s %s",
			       tmp -> name, "is attached.");
		}

		/* Find subnets that don't have valid interface
		   addresses... */
		for (subnet = (tmp -> shared_network
			       ? tmp -> shared_network -> subnets
			       : (struct subnet *)0);
		     subnet; subnet = subnet -> next_sibling) {
			if (!subnet -> interface_address.len) {
				/* Set the interface address for this subnet
				   to the first address we found. */
				subnet -> interface_address.len = 4;
				memcpy (subnet -> interface_address.iabuf,
					&foo.sin_addr.s_addr, 4);
			}
		}

		/* Register the interface... */
		if_register_receive (tmp);
		if_register_send (tmp);
	}

	/* Now register all the remaining interfaces as protocols. */
	for (tmp = interfaces; tmp; tmp = tmp -> next)
		add_protocol (tmp -> name, tmp -> rfdesc, got_one, tmp);

	close (sock);

	maybe_setup_fallback ();
}





static void usage PROTO ((char *));

TIME default_lease_time = 43200; /* 12 hours... */
TIME max_lease_time = 86400; /* 24 hours... */
struct tree_cache *global_options [256];

int log_perror = 1;

/* Needed to prevent linking against conflex.c. */
int lexline;
int lexchar;
char *token_line;
char *tlname;

char *path_dhcrelay_pid = _PATH_DHCRELAY_PID;

u_int16_t local_port;
u_int16_t remote_port;
int log_priority;

//typedef enum relaymode {Dhcp_Relay, Serving_Pool} relay_mode;
//relay_mode RelayMode;

static char copyright [] =
"Copyright 1997, 1998, 1999 The Internet Software Consortium.";
static char arr [] = "All rights reserved.";
static char message [] = "Internet Software Consortium DHCP Relay Agent";
static char contrib [] = "Please contribute if you find this software useful.";
static char url [] = "For info, please visit http://www.isc.org/dhcp-contrib.html";

// Kaohj, for dynamic interface
extern int resync_itf;

void refresh_itf(void)
{
	// It should be a dynamic interface to get a new IP configuration
	// Let the main loop to do the real interface resynchronization	
	resync_itf = 1;
}

// Magician: SIGTERM handler
void dhcrelay_term_handler(void)
{
	unlink("/var/run/dhcrelay.pid");
}

/* get an option with bounds checking (warning, not aligned). */
unsigned char *GetDHCPOption(struct dhcp_packet *packet, int code)
{
	int i, length;
	unsigned char *optionptr;
	int over = 0, done = 0, curr = OPTION_FIELD;

	optionptr = packet->options;
	i = 0;
	length = DHCP_OPTION_LEN;
	while (!done) {
		if ((i+1) >= length) {
			note("bogus packet, option fields too long.");
			return NULL;
		}
		if (optionptr[i + OPT_CODE] == code) {
			if ((i + OPT_LEN>=length) || (i + 1 + optionptr[i + OPT_LEN] >= length)) {
				note("bogus packet, option fields too long.");
				return NULL;
			}
			return optionptr + i + 2;
		}
		switch (optionptr[i + OPT_CODE]) {
		case DHCP_PADDING:
			i++;
			break;
		case DHCP_OPTION_OVER:
			if ((i + 3>=length) || (i + 1 + optionptr[i + OPT_LEN] >= length)) {
				note("bogus packet, option fields too long.");
				return NULL;
			}
			over = optionptr[i + 3];
			i += optionptr[OPT_LEN] + 2;
			break;
		case DHCP_END:
			if (curr == OPTION_FIELD && over & FILE_FIELD) {
				optionptr = (unsigned char *)packet->file;
				i = 0;
				length = 128;
				curr = FILE_FIELD;
			} else if (curr == FILE_FIELD && over & SNAME_FIELD) {
				optionptr = (unsigned char *)packet->sname;
				i = 0;
				length = 64;
				curr = SNAME_FIELD;
			} else done = 1;
			break;
		default:
			if(OPT_LEN + i>=length)
				return NULL;
			i += optionptr[OPT_LEN + i] + 2;
		}
	}
	return NULL;
}

// Mason Yu. signal123.
int listen_from_relay(void)
// try123
//	int argc;
//	char **argv, **envp;
{
	
	struct servent *ent;
	struct server_list *sp = (struct server_list *)0; 	
	char *s;
	//DHCPS_SERVING_POOL_T *pDHCPSPEntry, DHCPSPEntry;

	unsigned char value[32];
	
	s = "listen_from_relay";
#if 0 // try123
	s = strrchr (argv [0], '/');
	if (!s)
		s = argv [0];
	else
		s++;

	pDHCPSPEntry = &DHCPSPEntry;
	servers = NULL;
	RelayMode = Dhcp_Relay;

	/* Initially, log errors to stderr as well as to syslogd. */
#ifdef SYSLOG_4_2
	openlog (s, LOG_NDELAY);
	log_priority = DHCPD_LOG_FACILITY;
#else
	openlog (s, LOG_NDELAY, DHCPD_LOG_FACILITY);
#endif

#if !(defined (DEBUG) || defined (SYSLOG_4_2))
	setlogmask (LOG_UPTO (LOG_INFO));
#endif

	for (i = 1; i < argc; i++) {
		if (!strcmp (argv [i], "-p")) {
			if (++i == argc)
				usage (s);
			local_port = htons (atoi (argv [i]));
			debug ("binding to user-specified port %d", ntohs (local_port));
		} else if (!strcmp (argv [i], "-pf")) {
			if (++i == argc)
				usage (s);
			path_dhcrelay_pid = argv [i];
		} else if (!strcmp (argv [i], "-d")) {
			no_daemon = 1;
 		} else if (!strcmp (argv [i], "-i")) {
			struct interface_info *tmp =
				((struct interface_info *)
				 dmalloc (sizeof *tmp, "specified_interface"));
			if (!tmp)
				error ("Insufficient memory to %s %s",
				       "record interface", argv [i]);
			if (++i == argc) {
				usage (s);
			}
			memset (tmp, 0, sizeof *tmp);
			strcpy (tmp -> name, argv [i]);
			tmp -> next = interfaces;
			tmp -> flags = INTERFACE_REQUESTED;
			interfaces = tmp;
		} else if (!strcmp (argv [i], "-q")) {
			quiet = 1;
			quiet_interface_discovery = 1;
 		}
 		else if(!strcmp (argv [i], "-sprelay"))
 		{  // Magician: Get server list from mib chain table MIB_DHCPS_SERVING_POOL_TBL.
 			RelayMode = Serving_Pool;

			num = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
			for( j = 0; j < num; j++ )
			{
				if( !mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL, j, (void*)pDHCPSPEntry) )
					continue;

				sp = (struct server_list *)malloc(sizeof *sp);

				if (!sp)
					error ("no memory for server.\n");

				sp->next = NULL;

				if(!servers)
				{
					servers = sp;
					cur_sp = sp;
				}
				else
				{
					cur_sp->next = sp;
					cur_sp = sp;
				}

				memcpy(&sp->to.sin_addr, pDHCPSPEntry->dhcprelayip, sizeof(pDHCPSPEntry->dhcprelayip));
				// debug("Locally Served=%d", pDHCPSPEntry->localserved);
			}
 		}
		else if (argv [i][0] == '-') {
 		    usage (s);
 		} else {
			struct hostent *he;
			struct in_addr ia, *iap = (struct in_addr *)0;
			if (inet_aton (argv [i], &ia)) {
				iap = &ia;
			} else {
				he = gethostbyname (argv [i]);
				if (!he) {
					warn ("%s: host unknown", argv [i]);
				} else {
					iap = ((struct in_addr *)	he->h_addr_list [0]);
				}
			}
			if (iap) {
				sp = (struct server_list *)malloc (sizeof *sp);
				if (!sp)
					error ("no memory for server.\n");
				sp -> next = servers;
				servers = sp;
				memcpy (&sp->to.sin_addr, iap, sizeof *iap);
			}
 		}
	}

	if (!quiet) {
		note ("%s %s", message, DHCP_VERSION);
		note (copyright);
		note (arr);
		note ("");
		note (contrib);
		note (url);
		note ("");
	} else
		log_perror = 0;
#endif  //try123

	// Mason Yu. Set default server's IP for Relay
	if ( g_dhcp_mode == LAN_RELAY || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {
		struct in_addr ia, *iap = (struct in_addr *)0;
		servers = NULL;

		if ( g_server_ip != NULL ) {
			inet_aton (g_server_ip, &ia);
		} else {
			mib_get_s(MIB_ADSL_WAN_DHCPS,(void *)value, sizeof(value));
			//ia.s_addr = ((struct in_addr *)value)->s_addr;
			memcpy(&ia.s_addr, value, 4);
		}
		//printf("Default Server IP is 0x%x for Relay\n", ia.s_addr);
		iap = &ia;

		sp = (struct server_list *)malloc (sizeof *sp);
		if (!sp)
			error ("no memory for server.\n");
		sp -> next = servers;
		servers = sp;

		memcpy (&sp -> to.sin_addr,
			iap, sizeof *iap);
	} // if ( g_dhcp_mode == LAN_RELAY || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {

	/* Default to the DHCP/BOOTP port. */
	if (!local_port) {
		ent = getservbyname ("dhcps", "udp");
		if (!ent)
			local_port = htons (67);
		else
			local_port = ent -> s_port;
		endservent ();
	}
	remote_port = htons (ntohs (local_port) + 1);

	if ( g_dhcp_mode == LAN_RELAY || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {
		/* We need at least one server. */
		if (!sp) {
			usage (s);
		}

		/* Set up the server sockaddrs. */
		for (sp = servers; sp; sp = sp -> next) {
			sp -> to.sin_port = local_port;
			sp -> to.sin_family = AF_INET;
	#ifdef HAVE_SA_LEN
			sp -> to.sin_len = sizeof sp -> to;
	#endif
		}
	} // if ( g_dhcp_mode == LAN_RELAY || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {

	/* Discover all the network interfaces. */
	discover_interfaces (DISCOVER_RELAY);

	if ( g_dhcp_mode == LAN_RELAY || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {
		/* Set up the bootp packet handler... */
		bootp_packet_handler = relay;
	}

	// Mason Yu. try123
	#if 0
	/* Become a daemon... */
	if (!no_daemon) {
		int pid;
		FILE *pf;
		int pfdesc;

		log_perror = 0;

		if ((pid = vfork()) < 0)
			error ("can't fork daemon: %m");
		else if (pid)
			exit (0);

		pfdesc = open (path_dhcrelay_pid, O_CREAT | O_TRUNC | O_WRONLY, 0644);

		if (pfdesc < 0) {
			warn ("Can't create %s: %m", path_dhcrelay_pid);
		} else {
			pf = fdopen (pfdesc, "w");
			if (!pf)
				warn ("Can't fdopen %s: %m", path_dhcrelay_pid);
			else {
				fprintf (pf, "%ld\n", (long)getpid ());
				fclose (pf);
			}
		}

		close (0);
		close (1);
		close (2);
		pid = setsid ();
	}
	#endif

	/* Start dispatching packets and timeouts... */
	// Mason Yu. signal123.
	dispatch ();

	/*NOTREACHED*/
	return 0;
}

void relay (ip, packet, length)
	struct interface_info *ip;
	struct dhcp_packet *packet;
	int length;	
{
	struct server_list *sp;
	struct sockaddr_in to;
	struct interface_info *out;
	struct hardware hto;
	struct interface_info *tmp_ifinfo=NULL;
	
	// Mason Yu try123
	//struct dhcpMessage packet_server
	//server(*((struct dhcpMessage *)packet) );	 // try ok too.	
#ifdef DHCP_OPTION_82_ENABLE
	unsigned char opt82field[256];

	opt82field[OPT_CODE] = 82;
	opt82field[OPT_LEN] = 0;
	OPT82_AddSubOpt(opt82field, 1, OPT82_CIRCUIT_ID, strlen(OPT82_CIRCUIT_ID));
	OPT82_AddSubOpt(opt82field, 2, OPT82_REMOTE_ID, strlen(OPT82_REMOTE_ID));
	add_option_string(packet->options, opt82field);
	length += opt82field[OPT_LEN]+2;
#endif

	if (packet -> hlen > sizeof packet -> chaddr) {
		note ("Discarding packet with invalid hlen.");
		return;
	}

	/* If it's a bootreply, forward it to the client. */
	if (packet -> op == BOOTREPLY) {
		if (!(packet->flags & htons (BOOTP_BROADCAST))) {
			if (packet -> ciaddr.s_addr)
				to.sin_addr = packet -> ciaddr;
			else
				to.sin_addr = packet -> yiaddr;
			to.sin_port = remote_port;
		} else {
			to.sin_addr.s_addr = htonl (INADDR_BROADCAST);
			to.sin_port = remote_port;
		}
		to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
		to.sin_len = sizeof to;
#endif

		/* Set up the hardware destination address. */
		hto.hlen = packet -> hlen;
		if (hto.hlen > sizeof hto.haddr)
			hto.hlen = sizeof hto.haddr;
		if (!(packet->flags & htons (BOOTP_BROADCAST)))   		// unicast
			memcpy (hto.haddr, packet -> chaddr, hto.hlen);
		else													// broadcast
			memcpy (hto.haddr, MAC_BCAST_ADDR, strlen(MAC_BCAST_ADDR));
		hto.htype = packet -> htype;

#ifdef RELAY_AGENT_ON_WAN
		// Mason Yu. set original giaddr(Relay agent IP) to packet.
		packet -> giaddr = packet -> inaddr;
#endif

		/* Find the interface that corresponds to the giaddr in the packet. */
		for (out = interfaces; out; out = out -> next) {
			if (!memcmp(&out->primary_address, &packet->giaddr, sizeof packet->giaddr))
				break;
		}
		if (!out) {
			warn("packet to bogus giaddr %s.\n", inet_ntoa (packet->giaddr));
			return;
		}
		
		// Should we reset the giaddr to 0.0.0.0 ?
		// It can refer to RFC2131(Table 3:  Fields and options used by DHCP servers).
		// BOOTP client MUST set the 'giaddr' field to zero (0.0.0.0) in all BOOTREQUEST messages it generates(RFC 1542(3.4)
		//packet -> giaddr.s_addr = 0x0;
		if (send_packet_toLAN (out, packet, &to, &hto) > 0)
			DEBUG(LOG_INFO, "forwarded BOOTREPLY for %s to %s", print_hw_addr (packet->htype, packet->hlen, packet->chaddr), inet_ntoa (to.sin_addr));

		return;
	}

	/* If giaddr is set on a BOOTREQUEST, ignore it - it's already been gatewayed. */
	if (packet -> giaddr.s_addr) {
		note ("ignoring BOOTREQUEST with giaddr of %s\n", inet_ntoa (packet->giaddr));
		return;
	}

	/* Set the giaddr so the server can figure out what net it's from and so that we can later forward the response to the correct net. */
	// Mason Yu. Save the IP address of incoming interface into packet.
#ifdef RELAY_AGENT_ON_WAN
	packet -> inaddr = ip -> primary_address;
#else
	packet->giaddr = ip->primary_address;
#endif

	packet->hops = packet->hops + 1;

	/* Otherwise, it's a BOOTREQUEST, so forward it to all the servers. */
	// Mason Yu.
	//if( RelayMode == Dhcp_Relay )
	for(tmp_ifinfo=interfaces;tmp_ifinfo;tmp_ifinfo=tmp_ifinfo->next)	
	{
		for (sp = servers; sp; sp = sp->next)
		{
		// Mason Yu. Change Relay agent IP from Incoming Interface to Outgoing Interface on packet.
#ifdef RELAY_AGENT_ON_WAN
		if ( fallback_interface )
			packet -> giaddr.s_addr = fallback_interface->primary_address.s_addr;
		else
			packet -> giaddr.s_addr = interfaces->primary_address.s_addr;
#endif

			if (!memcmp(&ip->primary_address, &tmp_ifinfo->primary_address, sizeof ip->primary_address)) {
				//printf("Don't send to %s\n", tmp_ifinfo->name);
				continue;
			}
			
			if(send_packet((fallback_interface? fallback_interface: tmp_ifinfo), packet, length, &sp->to) > 0)
				DEBUG(LOG_INFO, "forwarded BOOTREQUEST for %s to %s", print_hw_addr(packet->htype, packet->hlen, packet->chaddr), inet_ntoa(sp->to.sin_addr));
		}
	}
	// Mason Yu.
	#if 0
	else if( RelayMode == Serving_Pool ) // Magician: TR-069 Serving Pool
	{
		unsigned char *classVendor;
		unsigned char *classVendorStr[256] = {0};
		int len = 0, entry_num, i;
		DHCPS_SERVING_POOL_T *pDHCPSPEntry, DHCPSPEntry;

		pDHCPSPEntry = &DHCPSPEntry;

		if(classVendor = GetDHCPOption(packet, DHCP_VENDOR))
		{
			/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
			len=*(unsigned char*)(classVendor-OPT_LEN);
			memcpy(classVendorStr,classVendor,len);
			classVendorStr[len]=0;

			sp = servers;

			entry_num = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
			for( i = 0; i < entry_num; i++ )
			{
				if( !mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL, i, (void*)pDHCPSPEntry) )
					continue;

				if(strstr(classVendorStr, pDHCPSPEntry->vendorclass))
				{
					if(pDHCPSPEntry->localserved)  // Locally Served enabled, so drop this packet.
						break;

				#ifdef RELAY_AGENT_ON_WAN
					if ( fallback_interface )
						packet->giaddr.s_addr = fallback_interface->primary_address.s_addr;
					else
						packet->giaddr.s_addr = interfaces->primary_address.s_addr;
				#endif

					if(send_packet((fallback_interface? fallback_interface: interfaces), packet, length, &sp->to) > 0)
						DEBUG(LOG_INFO, "forwarded BOOTREQUEST for %s to %s", print_hw_addr(packet->htype, packet->hlen, packet->chaddr), inet_ntoa(sp->to.sin_addr));
				}

				sp = sp->next;
			}
		}
		else
			note("Get option 60 failed!");
	}
	else
		note("RelayMode has not been set!");
	#endif

}

static void usage (appname)
	char *appname;
{
	note (message);
	note (copyright);
	note (arr);
	note ("");
	note (contrib);
	note (url);
	note ("");

	warn ("Usage: %s [-i] [-d] [-i if0] [...-i ifN] [-p <port>]", appname);
	error ("      [-pf pidfilename] [server1 [... serverN]]");
}

void cleanup ()
{
}

int write_lease (void)
{
	return 1;
}

int commit_leases ()
{
	return 1;
}

void bootp (void)
{
}

void dhcp (void)
{
}

#ifdef _PRMT_X_CT_COM_LANBINDING_CONFIG_
unsigned int getIfIndex(unsigned int type)
{
	MIB_CE_ATM_VC_T entry;
	int i, total;
	char ifname[16] = {0};

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;
		if ((entry.applicationtype & type) && entry.enable == 1)
		{
			ifGetName(entry.ifIndex, ifname, sizeof(ifname));
			return if_nametoindex(ifname);
		}
	}

	return DUMMY_IFINDEX;
}

void send_packet_to_other_bridge(struct dhcpMessage *raw)
{
	unsigned int out_if_index = -1;
	int ret = 0;
	if((out_if_index = getIfIndex(X_CT_SRV_OTHER)) == DUMMY_IFINDEX)
	{
		printf("Outgoing interface is not specified\n");
		return;
	}

	printf("out_if_index=%u \n",out_if_index);
	//ret = raw_packet(raw, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
	//				SERVER_PORT, MAC_BCAST_ADDR, out_if_index);
	ret = l2_raw_packet(raw, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
					SERVER_PORT, MAC_BCAST_ADDR, out_if_index);

	return;
}

#endif

