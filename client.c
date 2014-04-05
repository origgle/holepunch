
#include "util.h"
#include "set.h"

int clientnum, listener;
FILE *to_server, *to_peer;
char connection_password[16];


struct client_connection_request {
	int clientnum;
	char pass[16];
	char priv_ip[64];
	char pub_ip[64];
};

set_t connection_requests;

int handshake(FILE *f) {
	char buf[256];
	char *cmd, *cnum;
	fprintf(f, "HELLO %s\n", get_local_ip(buf, 256));
	fgets(buf, 256, f);
	tokenate(buf, " \n", &cmd, &cnum, NULL);
	return atoi(cnum);	
}

void peer_connect(const char *priv_ip, const char *pub_ip) {	
	int i, highest_fd, to_priv, to_pub;
	struct sockaddr priv_addr, pub_addr;
	fd_set rfds, wfds;
	socklen_t len = sizeof(int);

	i = 1;
	//nonblock flag supported since Linux 2.6.27
	to_priv = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	setsockopt(to_priv, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	inet_pton(AF_INET, priv_ip, &priv_addr);
	connect(to_priv, &priv_addr, sizeof(priv_addr));

	to_pub = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	setsockopt(to_pub, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	inet_pton(AF_INET, pub_ip, &pub_addr);
	connect(to_pub, &pub_addr, sizeof(pub_addr));

	highest_fd = listener;
	if (to_priv > highest_fd) highest_fd = to_priv;
	if (to_pub > highest_fd) highest_fd = to_pub;

	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(listener, &rfds);
		
		FD_ZERO(&wfds);
		FD_SET(to_priv, &wfds);
		FD_SET(to_pub, &wfds);
	
		select(highest_fd + 1, &rfds, &wfds, NULL, NULL);
	
		if (FD_ISSET(listener, &rfds)) {
			to_peer = fdopen(accept(listener, NULL, NULL), "r+");
			break;
		} else if (FD_ISSET(to_priv, &wfds)) {
			getsockopt(to_priv, SOL_SOCKET, SO_ERROR, &i, &len);
			if (i == 0) { to_peer = fdopen(to_priv, "r+"); break; }
			else {
				usleep(1);
				connect(to_priv, &priv_addr, sizeof(priv_addr));
			}
		} else if (FD_ISSET(to_pub, &wfds)) {
			getsockopt(to_pub, SOL_SOCKET, SO_ERROR, &i, &len);
			if (i == 0) { to_peer = fdopen(to_pub, "r+"); break; }
			else {
				usleep(1);
				connect(to_pub, &pub_addr, sizeof(pub_addr));
			}
		}
	}
}

void keyboard_input() {
	char buf[256];
	char pass[16];
	char *cmd, *cnum;
	int i;
	const struct client_connection_request *request;
	fgets(buf, 256, stdin);
	if (strcasecmp(cmd, "CONNECT") == 0) {
		fprintf(to_server, "CONNECT %s %s\n", cnum, random_password(pass, 10));
	} else if (strcasecmp(cmd, "ACCEPT") == 0 || strcasecmp(cmd, "REJECT") == 0) {
		i = atoi(cnum);
		request = set_find(&connection_requests, &i);
		if (request == NULL) {
			fprintf(stdout, "Client %d never sent a connection request\n", i);
			return;
		}
		fprintf(to_server, "%s %d %s\n", cmd, i, request->pass);
		if (strcasecmp(cmd, "ACCEPT") == 0)
			peer_connect(request->priv_ip, request->pub_ip);
	} else if (strcasecmp(cmd, "DISCONNECT") == 0) {
		fputs("DISCONNECT\n", to_server);
		fclose(to_server);
	}
}

void server_message() {
	char buf[256];
	struct client_connection_request request;
	char *cmd, *cnum, *pass, *priv_ip, *pub_ip;
	fgets(buf, 256, to_server);
	tokenate(buf, " \n", &cmd, &cnum, &pass, &priv_ip, &pub_ip, NULL);
	if (strcasecmp(cmd, "CONNECT") == 0) {
		request.clientnum= atoi(cnum);
		strcpy(request.pass, pass);
		strcpy(request.priv_ip, priv_ip);
		strcpy(request.pub_ip, pub_ip);
		set_insert(&connection_requests, &request, sizeof(request));
		fprintf(stdout, "Connection request from client %d\n", request.clientnum);
	} else if (strcasecmp(cmd, "REJECT") == 0) {
		if (strcmp(pass, connection_password) == 0) {
			fprintf(stdout, "Client %s rejected your connection request\n", cnum);
		}
	} else if (strcasecmp(cmd, "ACCEPT") == 0) {
		if (strcmp(pass, connection_password) == 0) {
			fprintf(stdout, "Client %s accepted your connection request\n", cnum);
			peer_connect(priv_ip, pub_ip);	
		}
	}
	
}

int main(int argc, const char **argv) {
	int i;
	struct socket_option opt;
	fd_set rfds;
	
	if (argc != 3) {
		fprintf(
			stdout,
			"Outernet client\n"
			"Usage: %s [server] [port]\n",
			argv[0]
		);
		return 1;
	}

	connection_password[0] = 0;
	set_init(&connection_requests, int_cmp);

	opt.level = SOL_SOCKET;
	opt.optname = SO_REUSEADDR;
	opt.optval = &i;
	opt.optlen = sizeof(i);
	i = 1;
	
	listener = serve(argv[2], &opt, NULL);
	to_server = fdopen( dial(argv[1], argv[2], &opt, NULL), "r+" );

	clientnum = handshake(to_server);
	fprintf(stdout, "Your client number is %d\n", clientnum);
	
	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		FD_SET(fileno(to_server), &rfds);
	
		select(fileno(to_server) + 1, &rfds, NULL, NULL, NULL);	
		
		if (FD_ISSET(0, &rfds))
			keyboard_input();
		else if (FD_ISSET(fileno(to_server), &rfds))
			server_message();
	
	}	
}
