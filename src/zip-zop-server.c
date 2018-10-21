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
#define BACKLOG 10

struct addrinfo *get_internet_addr(void)
{
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_UNSPEC;
	hints.ai_socktype 	= SOCK_STREAM;
	hints.ai_flags 		= AI_PASSIVE;

	int rv;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(E_GETADDRINFO);
	}

	return servinfo;
}

int create_and_bind(struct addrinfo *addr)
{
	int yes = 1;
	int sockfd;

	if ((sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
		return -1;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		return -1;
	}

	if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == -1) {
		return -1;
	}

	return sockfd;
}

int accept_clients(int sockfd)
{
	struct sockaddr_storage client_addr;
	socklen_t addrlen = sizeof(client_addr);
	int client_sockfd;

	while (true) {
		client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);
		if (client_sockfd == -1) {
			perror("accept");
			continue;
		}

		int numbytes = 100;
		char msg[100];
		recv(client_sockfd, msg, numbytes, 0);
		printf("cliente: %s\n", msg);

		close(client_sockfd);

	}

	return 0;
}

int main(void)
{
	struct addrinfo *servinfo = get_internet_addr();
	
	struct addrinfo *p;
	int sockfd;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = create_and_bind(p)) == -1) {
			perror("socket()");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo);

	if (!p) {
		fprintf(stderr, "failed to bind\n");
		exit(E_BIND);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(E_LISTEN);
	}

	accept_clients(sockfd);

	return 0;
}
