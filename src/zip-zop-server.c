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

/** @brief The port where this application will be running. */
#define PORT "1234"

/** @brief The number of clients that will be kept in the queue if the server is not ready for accepting them. */
#define BACKLOG 10

/** @brief Maximum length of a client name. */
#define CLIENT_NAME_LEN 100

/** @brief Maximum length of a client message. */
#define MESSAGE_LEN 2000

/**
 * @brief A singly linked list that will keep all the connected clients.
 *
 * @warning Mutual exclusion must be ensured before accessing this list.
 *
 * @see CLIENT_LIST_MUTEX
 */
struct sllist *CLIENT_LIST = SLL_INIT();

/**
 * @brief The @c CLIENT_LIST mutex.
 *
 * This is used to ensure mutual exclusion wen accessing the @c CLIENT_LIST, given 
 * the nature of the application where multiple threads might use the list.
 *
 * @see CLIENT_LIST
 */
pthread_mutex_t CLIENT_LIST_MUTEX;

/**
 * @brief Sends a message to all clients.
 *
 * The message will be sent as a packet version of a struct message.
 *
 * @param[in] c The client that sent the message.
 * @param[in] msg The message content.
 *
 * @see message_pack
 */
void client_thread_broadcast(struct client *c, const char *msg)
{
	/* The serialized message will be stored here */
	char *pack = "";

	struct message *m = message_create(msg, client_get_name(c));
	if (m) {
		int len;
		pack = message_pack(m, &len);
		if (pack) {
			pthread_mutex_lock(&CLIENT_LIST_MUTEX);

			/* Iterate through all the CLIENT_LIST, and send the message to all the connected clients */
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

/**
 * @brief Kill a client.
 *
 * Removes a client from the @c CLIENT_LIST, destroys it and closes the connection.
 *
 * @param[in] c The client.
 *
 * @see CLIENT_LIST
 */
void kill_client(struct client *c)
{
	if (c) {
		pthread_mutex_lock(&CLIENT_LIST_MUTEX);
		void *key = sll_remove_elm(&CLIENT_LIST, c);
		pthread_mutex_unlock(&CLIENT_LIST_MUTEX);

		if (key) {
			close(client_get_socket(c));
			client_destroy(c);
		}
	}

}

/**
 * @brief Keeps listening to client messages.
 *
 * This function will be executed by a thread that is responsable for 
 * keep checking if there is a new message from the client.
 *
 * If there is an new message, the thread will execute the client_thread_broadcast().
 *
 * @param[in] client A pointer to the client.
 *
 * @see client_thread_broadcast
 */
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

/**
 * @brief Find a set of possible internet addresses of localhost.
 *
 * @return A list of addrinfo, wich contain the adresses.
 */
struct addrinfo *get_internet_addr(void)
{
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));  /* Zero the hints variable */
	hints.ai_family 	= AF_UNSPEC;   /* Use IPv4 or IPv6 */
	hints.ai_socktype 	= SOCK_STREAM; /* Use TCP */
	hints.ai_flags 		= AI_PASSIVE;  /* Use my IP (this server will run on localhost) */

	int rv;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(E_GETADDRINFO);
	}

	return servinfo;
}

/**
 * @brief Attempts to create a socket and bind to a port with the given internet address.
 *
 * @param[in] addr The internet address.
 *
 * @return The socket in case os success. -1 otherwise.
 */
int create_and_bind(struct addrinfo *addr)
{
	int yes = 1;
	int sockfd;

	/* Tries to create a socket */
	if ((sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
		return -1;
	}

	/* Configure to reuse the same address between instantiations (prevents the "address alredy in use" error) */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		return -1;
	}

	/* Ties to bind to a given port */
	if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == -1) {
		return -1;
	}

	return sockfd;
}

/**
 * @brief Create a new client and add it in the @c CLIENT_LIST.
 *
 * @param[in] sockfd The socket created in accept_clients(),  and that 
 * is used to communicate with the client that will be created.
 *
 * @see accept_clients
 * @see CLIENT_LIST
 */
void create_new_client(int sockfd)
{
	/* 
	 * The first thing the client will do is to send its name 
	 * to the server, so we will recieve it in this function 
	 */

	/* Where the client name will be stored */
	char client_name[CLIENT_NAME_LEN];
	/* recv() the client name and store it in the client_name buffer */
	ssize_t numbytes = recv(sockfd, client_name, CLIENT_NAME_LEN - 1, 0);
	client_name[numbytes] = '\0';

	struct client *c = client_create(client_name, sockfd);
	if (c) {
		/* Carry out mutual exclusion and insert the new client on the list */
		pthread_mutex_lock(&CLIENT_LIST_MUTEX);
		sll_insert_last(&CLIENT_LIST, c);
		pthread_mutex_unlock(&CLIENT_LIST_MUTEX);

		/* 
		 * Create a a new thread for that client, this thread 
		 * will execute the client_thread_listen() function 
		 */
		if (pthread_create(client_get_thread(c), NULL, client_thread_listen, c)) {
			exit(E_PTHREAD_CREATE);
		}
	}
}

/**
 * @brief Keeps on accepting new clients connections.
 *
 * Keeps listening for incoming connections, wen a new one
 * arrives accepts it and instantiates a new client.
 *
 * @param[in] sockfd Socket used to listen to new connections.
 */
int accept_clients(int sockfd)
{
	pthread_mutex_init(&CLIENT_LIST_MUTEX, NULL);

	struct sockaddr_storage client_addr;
	socklen_t addrlen = sizeof(client_addr);
	int client_sockfd;

	while (true) {
		/* Listen and accept incoming connections, client_socket is the new connection socket */
		client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);
		if (client_sockfd == -1) {  /* If the connection failed */
			perror("accept");
			continue;
		}

		create_new_client(client_sockfd);
	}

	return 0;
}

/**
 * @brief The zip-zop-server. 
 *
 * A TCP server that will accept connections from zip-zop-clients, 
 * hear its messages and broadcast them to all connected clients. Working as a chatroom.
 */
int main(void)
{
	struct addrinfo *servinfo = get_internet_addr();
	
	struct addrinfo *p;
	int sockfd;
	/* Iterate in the list of internet addresses, trying to bind in the PORT with it */
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = create_and_bind(p)) == -1) {
			perror("socket()");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo);

	/* if p is NULL, that means that we could not find an address in wich we managed to bind to a port */
	if (!p) {
		fprintf(stderr, "failed to bind\n");
		exit(E_BIND);
	}

	/* Mark the sockfd as passive, so it will be able to listen for incoming connection */
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(E_LISTEN);
	}

	accept_clients(sockfd);

	return 0;
}
