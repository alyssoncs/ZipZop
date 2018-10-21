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
#include "sllist.h"

#define PORT "1234"
#define BACKLOG 10
#define CLIENT_NAME_LEN 100
#define MESSAGE_LEN 2000

struct sllist *CLIENT_LIST = SLL_INIT();
pthread_mutex_t CLIENT_LIST_MUTEX;

void client_thread_broadcast(struct client *c, const char *msg)
{
	char *pack = "";

	struct message *m = message_create(msg, client_get_name(c));
	if (m) {
		int len;
		pack = message_pack(m, &len);
		if (pack) {
			pthread_mutex_lock(&CLIENT_LIST_MUTEX);

			for (struct sllist *p = CLIENT_LIST; p; p = sll_get_next(&p)) {
				struct client *current_client = (struct client *)sll_get_key(p);
				int rv = send(client_get_socket(current_client), pack, len, 0);
				if (rv == -1) {
					perror("send()");
				}
			}

			pthread_mutex_unlock(&CLIENT_LIST_MUTEX);

			free(pack);
		}
		message_destroy(m);
	}
}

void kill_client(struct client *c)
{
	if (c) {
		pthread_mutex_lock(&CLIENT_LIST_MUTEX);
		void *key = sll_remove_elm(&CLIENT_LIST, c);
		pthread_mutex_unlock(&CLIENT_LIST_MUTEX);

		if (key) {
			client_destroy(c);
		}
	}

}
void *client_thread_listen(void *client)
{
	struct client *c = (struct client *)client;
	char msg[MESSAGE_LEN];
	
	ssize_t numbytes;
	while ((numbytes = recv(client_get_socket(c), msg, MESSAGE_LEN - 1, 0)) > 0) {
		msg[numbytes] = '\0';
		client_thread_broadcast(c, msg);
	}
	perror("client_thread_listen -> recv():");
	kill_client(c);

	return NULL;
}

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

void create_new_client(int sockfd)
{
	char client_name[CLIENT_NAME_LEN];
	ssize_t numbytes = recv(sockfd, client_name, CLIENT_NAME_LEN - 1, 0);
	client_name[numbytes] = '\0';

	struct client *c = client_create(client_name, sockfd);
	if (c) {
		pthread_mutex_lock(&CLIENT_LIST_MUTEX);
		sll_insert_last(&CLIENT_LIST, c);
		pthread_mutex_unlock(&CLIENT_LIST_MUTEX);

		if (pthread_create(client_get_thread(c), NULL, client_thread_listen, c)) {
			exit(E_PTHREAD_CREATE);
		}
	}
}

int accept_clients(int sockfd)
{
	pthread_mutex_init(&CLIENT_LIST_MUTEX, NULL);

	struct sockaddr_storage client_addr;
	socklen_t addrlen = sizeof(client_addr);
	int client_sockfd;

	while (true) {
		client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);
		if (client_sockfd == -1) {
			perror("accept");
			continue;
		}

		create_new_client(client_sockfd);
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
