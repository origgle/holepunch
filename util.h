#ifndef UTIL_H
#define UTIL_H

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>	//For strcasecmp
#include <stdlib.h>	
#include <stdarg.h>

#define die(S) do { perror((S)); exit(1); } while (0);

struct socket_option {
	int level;
	int optname;
	const void *optval;
	socklen_t optlen;
};

int serve(const char *port, ...);
int dial(const char *host, const char *port, ...);

char *random_password(char *buf, int count);
const char *get_local_ip(char *buf, int size);

void tokenate(char *buf, const char *delim, ...);

typedef struct {
	fd_set fds;
	int highest;
} rd_set;

#define RD_ZERO(S) do { \
	FD_ZERO(&(S)->fds); \
	(S)->highest = -1; \
} while (0)

#define RD_SET(F, S) do { \
		FD_SET((F), &(S)->fds); \
		if ((F) > (S)->highest) \
			(S)->highest = (F); \
		} while (0)

#define RD_ISSET(F, S) (FD_ISSET((F), &(S)->fds))

#define RD_SELECT(S, T) (select((S)->highest+1, &(S)->fds, NULL, NULL, (T)))

#endif
