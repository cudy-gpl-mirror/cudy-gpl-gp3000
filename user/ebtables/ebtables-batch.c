#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include "include/ebtables_u.h"

#define LOAD_EXTENSION_DIR "/usr/local/lib/ebtables"
#define EXTENSION_PREFIX "libebt_"
static int load_extensions(const char *search_path, const char *prefix);

static struct ebt_u_replace replace;
void ebt_early_init_once();

#define EBTABLES_PROGNAME "ebtables"
#define SOCKET_PATH  "/var/run/ebtablesFastApply.usock"

#define MAX_CONNECTIONS 50

//#define DEBUG

static char* errstr = NULL;

static unsigned current_line = 0;
static int server_sockfd=0;

struct iptables_info_st {
	unsigned char arg[64];
};

struct iptables_msg_st{
	long msg_type;
	int nargv;
	struct iptables_info_st argvs[32];
};

#ifdef DEBUG
static void
dumpargv(struct iptables_msg_st *qbuf)
{
	char *buffer_p = NULL;
    int i=0;
	char buff[2048]={0};

	buffer_p = &buff[0];
	// Calculate the total length of the combined string
    for (i = 0; i < qbuf->nargv; i++) {
        buffer_p += sprintf(buffer_p, "%s ", qbuf->argvs[i].arg);
    }

    *(buffer_p - 1) = '\0';

	printf("%s\n", buff);
}
#endif

static void process_msg(int client_sockfd)
{
	struct iptables_msg_st qbuf;
	int ret = 1;
	char *_argvs[64];
	int i=0;
	
	// Read data from the client
	ssize_t num_bytes_read = read(client_sockfd, &qbuf, sizeof(qbuf));
	// Close the client socket
    close(client_sockfd);
	if (num_bytes_read <= 0) {
		perror("read");
		close(client_sockfd);
		return;
	}

		
#ifdef DEBUG
	dumpargv(&qbuf);
#endif
	for(i=0; i < qbuf.nargv; i++){
		_argvs[i]= &qbuf.argvs[i].arg[0];
	}
	_argvs[i]=NULL;
		
	switch (fork()) {
	case 0:
		ebt_silent = 0;
		//init_extensions();
		//load_extensions(LOAD_EXTENSION_DIR, EXTENSION_PREFIX);
		//ebt_early_init_once();
		strcpy(replace.name, "filter");
		do_command(qbuf.nargv, _argvs, EXEC_STYLE_PRG, &replace);
		/* Not usually reached */
		exit(0);
	case -1:
		fprintf(stderr, "[%s] fork failure\n", __func__);
		return;
	default: /* Parent */
		wait(NULL);
	}

	return;
}

int main(int argc, char *argv[])
{
	int client_sockfd=0;
	socklen_t clilen;
	struct sockaddr_un server_addr, client_addr;
	char buffer[BUFSIZ];
	int c;
	
	// Remove the previous socket file (cleanup)
	unlink(SOCKET_PATH);

	// Create a server socket
	server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_sockfd == -1) {
		perror("socket");
		return -1;
	}

	// Set the SO_REUSEADDR option
	int optval = 1;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		perror("setsockopt");
		close(server_sockfd);
		return -1;
	}

	// Set the server address
	memset(&server_addr, 0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

	// Bind the server socket to the address
	if (bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un)) == -1) {
		perror("bind");
		close(server_sockfd);
		return -1;
	}

	// Listen for connections
	if (listen(server_sockfd, MAX_CONNECTIONS) == -1) {
		perror("listen");
		close(server_sockfd);
		return -1;
	}

	printf("[%s]TCP server started. Listening for connections...\n", __FUNCTION__);

	ebt_silent = 0;
	init_extensions();
	load_extensions(LOAD_EXTENSION_DIR, EXTENSION_PREFIX);
	ebt_early_init_once();

	// Accept connections from clients
	while (1) {
		clilen = sizeof(struct sockaddr_un);
		client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &clilen);
		if (client_sockfd == -1) {
			if (errno == EINTR) {
				// Interrupted by signal, continue to accept new connections
				continue;
			}else{
				perror("accept");
				continue;
			}
		}

		process_msg(client_sockfd);
	}

	// Close the server socket
	close(server_sockfd);

	// Remove the socket file (cleanup)
	unlink(SOCKET_PATH);
	return 0;
}

static int load_extensions(const char *search_path, const char *prefix)
{
	DIR *dir;
	void *handle;
	struct dirent **nameList;
	int total, i;
	void (*initFunc)(void);
	struct stat sb;
	char path[256];
	
	if(!search_path || (dir = opendir(search_path)) == NULL ) {
		//ebt_print_error("Error to open directory (%s)\n", (search_path)?search_path:"NULL");
		return 1;
	}
	else {
		closedir(dir);
		total = scandir(search_path, &nameList ,0,alphasort);
		if(total < 0) {
			//ebt_print_error("Fail to scandir...\n");
			return 1;
		}
		else {
			for(i = 0; i < total; i++) {
				if(nameList[i]->d_name != NULL && 
					!strncmp(nameList[i]->d_name, prefix, strlen(prefix)))
				{
					sprintf(path, "%s/%s", search_path, nameList[i]->d_name);
					if(stat(path, &sb) == 0 && S_ISREG(sb.st_mode)) {
						handle = dlopen(path, RTLD_NOW|RTLD_GLOBAL);
						if(handle) {
							if((initFunc = dlsym(handle, "ebt_init"))) {
								initFunc();
							}
							else {
								dlclose(handle);
							}
						}
					}
				}
				free(nameList[i]);
			}
			free(nameList);
		}
	}

	return NULL;
}
