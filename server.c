
#include "set.h"
#include "util.h"

struct client_info {
	int clientnum;
	FILE *client;
	char private_ip[64];
	char public_ip[64];
};

set_t client_set;

void* client_handler(void *arg) {
	const struct client_info *info, *info2;
	char buf[256];
	char *cmd, *cnum, *pass;
	int i, clientnum;

	info = (const struct client_info*)arg;

	if (fgets(buf, 256, info->client) == NULL)
		goto close_connection;
	tokenate(buf, " \n", &cmd, &cnum, &pass, NULL);
	if (strcmp(cmd, "DISCONNECT") == 0)
		goto close_connection;
	if (
		strcmp(cmd, "CONNECT") &&
		strcmp(cmd, "ACCEPT") &&
		strcmp(cmd, "REJECT")
	) goto close_connection;
	
	if (cnum == NULL) goto close_connection;
	sscanf(cnum, "%d", &clientnum);

	info2 = set_find(&client_set, &clientnum);
	if (info2 == NULL) goto close_connection;
	
	if (strcmp(cmd, "REJECT") == 0) {
		if (pass == NULL) goto close_connection;
		fprintf(info2->client, "%s %d %s\n", cmd, info->clientnum, pass);
	} else {
		fprintf(info2->client, "%s %d %s %s %s\n",
			cmd, info->clientnum, pass, info->private_ip, info->public_ip);
	}
close_connection:
	fclose(info->client);
	set_remove(&client_set, &info->clientnum);
	pthread_exit(NULL);
}

int accept_client(int server, struct client_info *info) {
	int i, client;
	struct sockaddr addr;
	socklen_t addrlen;
	char *s;
	
	addrlen = sizeof(addr);
	client = accept(server, &addr, &addrlen);
	if (client == -1) return -1;
	inet_ntop(AF_INET, &addr, info->public_ip, 64);
	info->clientnum = client;
	info->client = fdopen(client, "r+");
	setbuf(info->client, NULL);

	fgets(info->private_ip, 64, info->client);
	if (strncmp(info->private_ip, "HELLO", 6)) {
		close(client);
		return -1;
	}

	memmove(info->private_ip, info->private_ip + 6, strlen(info->private_ip + 6));
	s = strpbrk(info->private_ip, "\r\n");
	if (s) *s = 0;
	
	fprintf(info->client, "HELLO %d\n", info->clientnum);
	
	return 0;	
}

int main(int argc, const char **argv) {
	int server, client;
	pthread_t thread;
	pthread_attr_t attr;
	char *s;
	struct client_info info;

	if (argc != 2) {
		fprintf(stdout,
			"outernet rondezvous server\n"
			"Usage: %s PORT\n", argv[0]);
		return 1;
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	set_init(&client_set, int_cmp);
	server = serve(argv[1], NULL);

	for (;;) {
		if (accept_client(server, &info)) continue;
		pthread_create(&thread, &attr, client_handler,
			(void*)set_insert(&client_set, &info, sizeof(info)));
	}
}
