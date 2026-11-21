#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <ctype.h>
#ifdef EMBED
#include <rtk/utility.h>
#ifdef CONFIG_CU_SECURITY_FUNC
#include <rtk/security_func.h>
#endif
#endif
#include "signal.h"

#ifdef EMBED
#define SERVER_FIFO_NAME "/tmp/ppp_serv_fifo"
#define CLIENT_FIFO_NAME "/tmp/ppp_cli_fifo"
#else
#define SERVER_FIFO_NAME "ppp_serv_fifo"
#define BUFFER_SIZE	256

struct data_to_pass_st {
	int	id;
	char data[BUFFER_SIZE];
};
#endif


int main(int argc, char **argv)
{
	int server_fifo_fd;
	struct data_to_pass_st msg = {0};
	int i;
	int read_res;
	char *buf;
	int rest_len;
	
	server_fifo_fd = open(SERVER_FIFO_NAME, O_WRONLY|O_NONBLOCK);
	if(server_fifo_fd == -1) {
		fprintf(stderr, "Sorry, no server\n");
		exit(EXIT_FAILURE);
	}

	msg.id = 2684;
	buf = msg.data;
	rest_len=sizeof(msg.data);
	for(i=0; i<argc; i++) {
		snprintf(buf, rest_len, "%s", argv[i]);
		buf[strlen(argv[i])] = 0x20;
		buf += (strlen(argv[i])+1);
		rest_len=rest_len-strlen(argv[i])-1;
	}
	if(i>0)
	{
		buf--;
		*buf = '\0';
	}
	write(server_fifo_fd, &msg, sizeof(msg));
	close(server_fifo_fd);
	exit(EXIT_SUCCESS);
}

