#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "errcodes.h"

#define PORT "1234"

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

	send(sockfd, "hello, world!", 13, 0);
	close(sockfd);

	return 0;
}
