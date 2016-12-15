#include "util.h"

int serve(const char *port, ...) {
	int i, sockfd;
	struct addrinfo hints, *ai;
	struct socket_option *opt;
	va_list v;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	if (i = getaddrinfo(NULL, port, &hints, &ai)) {
		fprintf(stderr, "serve: getaddrinfo: %s\n", gai_strerror(i));
		exit(1);
	}
	sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sockfd == -1) die("socket");
	va_start(v, port);
	for (
		opt = va_arg(v, struct socket_option*);
		opt != NULL;
		opt = va_arg(v, struct socket_option*)
	) {
		if (setsockopt(sockfd,
			opt->level, opt->optname,
			opt->optval, opt->optlen)) die("setsockopt");
	}
	va_end(v);
	if (bind(sockfd, ai->ai_addr, ai->ai_addrlen)) die("bind");
	if (listen(sockfd, 4)) die("listen");
	freeaddrinfo(ai);
	return sockfd;
}

int dial(const char *host, const char *port, ...) {
	int i, sockfd;
	struct addrinfo hints, *ai;
	struct socket_option *opt;
	va_list v;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	if (i = getaddrinfo(host, port, &hints, &ai)) {
		fprintf(stderr, "dial: getaddrinfo: %s\n", gai_strerror(i));
		exit(1);
	}
	sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sockfd == -1) die("socket");
	va_start(v, port);
	for (
		opt = va_arg(v, struct socket_option*);
		opt != NULL;
		opt = va_arg(v, struct socket_option*)
	) {
		if (setsockopt(sockfd, opt->level,
				opt->optname, opt->optval, opt->optlen))
					die("setsockopt");
	}
	va_end(v);
	if (connect(sockfd, ai->ai_addr, ai->ai_addrlen)) die("connect");
	freeaddrinfo(ai);
	return sockfd;	
}

char *random_password(char *buf, int count) {
	int i;
	char *s = buf;
	srand(time(NULL));
	for (i = 0; i != count; ++i, ++s) {
		*s = rand() % 25 + 'A';
	}	
	*s = 0;
	return buf;
}

const char *get_local_ip(char *buf, int size) {
	//const char *res;
	struct ifaddrs *ifa;
	if (getifaddrs(&ifa)) die("getifaddrs");
	for (; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) continue;
		if (ifa->ifa_addr->sa_family != AF_INET) continue;
		if (strcmp(ifa->ifa_name, "eth0") && strcmp(ifa->ifa_name, "wlan0"))
			continue;
		int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), buf, size, NULL, 0, NI_NUMERICHOST);
		if(s == 0) {
			fprintf(stdout, "get_local_ip[%s] = %s\n", ifa->ifa_name, buf);
			break;
		}
	}
	fprintf(stdout, "TODO: FREE ME\n");
//	freeifaddrs(ifa);
	return &buf[0];
}

void tokenate(char *buf, const char *delim, ...) {
	va_list v;
	char **s, *token, *save;

	va_start(v, delim);
	s = va_arg(v, char**);	
	for (
		token = strtok_r(buf, delim, &save);
		token != NULL;
		token = strtok_r(NULL, delim, &save)
	) {
		if (s == NULL) break;
		*s = token;
		s = va_arg(v, char**);	
	}
	while (s != NULL) {
		*s = NULL;
		s = va_arg(v, char**);
	}
	va_end(v);
}

int ask(const char *fmt, ...) {
	int res;
	va_list v;
	va_start(v, fmt);
	vfprintf(stdout, fmt, v);
	va_end(v);
	res = fgetc(stdin);
	while (fgetc(stdin) != '\n');
	return res;	
}
