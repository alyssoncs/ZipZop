#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#include "errcodes.h"
#include "message.h"
#include "client.h"

#define PORT "1234"
#define MESSAGE_LEN 2000 

bool check_args(int argc)
{
	if (argc == 3)
		return true;
	return false;
}

void print_usage(const char *name)
{
	printf("usage: %s <server addr> <username>\n", name);
}

void show_message(struct message *m)
{
	if (m) {
		printf("[%s]: ", message_get_sender(m));
		printf("%s\n", message_get_content(m));
	}
}

void *listen_thread(void *client)
{
	struct client *c = (struct client *)client;
	char msg[MESSAGE_LEN];

	ssize_t numbytes;
	while ((numbytes = recv(client_get_socket(c), msg, MESSAGE_LEN - 1, 0)) > 0) {
		msg[numbytes] = '\0';
		struct message *m = message_unpack(msg);
		show_message(m);
		message_destroy(m);
	}

	return NULL;
}

void *speak_thread(void *client)
{
	struct client *c = (struct client *)client;
	char msg[MESSAGE_LEN];

	while (true) {
		fgets(msg, MESSAGE_LEN, stdin);
		char *nl = strchr(msg, '\n');
		if (nl) {
			*nl = '\0';
		}

		int rv = send(client_get_socket(c), msg, strlen(msg) + 1, 0);
		if (rv == -1) {
			perror("send()");
		}
	}

	return NULL;
}

struct addrinfo *get_server_addr(const char * server_name)
{
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_UNSPEC;
	hints.ai_socktype 	= SOCK_STREAM;

	int rv;
	if ((rv = getaddrinfo(server_name, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(E_GETADDRINFO);
	}

	return servinfo;
}

int create_and_connect(struct addrinfo *addr)
{
	int sockfd;

	if ((sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
		return -1;
	}

	if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) == -1) {
		return -1;
	}

	return sockfd;
}

void server_introduction(struct client *c)
{
	int sockfd 			= client_get_socket(c);
	const char *name 	= client_get_name(c);
	int len 			= strlen(name) + 1;

	int rv = send(sockfd, name, len, 0);
	if (rv == -1) {
		perror("send()");
	}
}

void communicate(const char *user_name, int sockfd)
{
	struct client *c = client_create(user_name, sockfd);

	if (c) {
		server_introduction(c);
		if (pthread_create(client_get_thread(c), NULL, listen_thread, c)) {
			exit(E_PTHREAD_CREATE);
		}
		speak_thread(c);
	}
}

int main(int argc, char **argv)
{
	if (check_args(argc) == false) {
		print_usage(argv[0]);
		return E_BAD_ARGS;
	}

	const char *server_name 	= argv[1];
	const char *user_name 		= argv[2];

	struct addrinfo *servinfo = get_server_addr(server_name);
	
	struct addrinfo *p;
	int sockfd;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = create_and_connect(p)) == -1) {
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo);

	if (!p) {
		fprintf(stderr, "failed to connect\n");
		exit(E_CONNECT);
	}

	communicate(user_name, sockfd);
	return 0;
}
