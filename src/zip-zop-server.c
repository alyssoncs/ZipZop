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
 * @brief Carry out mutual exclusion and insert the new client on the list.
 *
 * This function locks the @c CLIENT_LIST_MUTEX and inserts the client on the list, then unlocks the mutex.
 *
 * @param[in] c The client.
 *
 * @see CLIENT_LIST
 * @see CLIENT_LIST_MUTEX
 */
void insert_client_concurrent(struct client *c)
{
	pthread_mutex_lock(&CLIENT_LIST_MUTEX);
	sll_insert_last(&CLIENT_LIST, c);
	pthread_mutex_unlock(&CLIENT_LIST_MUTEX);
}

/**
 * @brief Carry out mutual exclusion and remove the new client on the list.
 * 
 * This function locks the @c CLIENT_LIST_MUTEX and inserts the client on the list, then unlocks the mutex.
 *
 * @param[in] c The client.
 *
 * @return The client just removed. NULL otherwise.
 *
 * @see CLIENT_LIST
 * @see CLIENT_LIST_MUTEX
 */
struct client *remove_client_concurrent(struct client *c)
{
	pthread_mutex_lock(&CLIENT_LIST_MUTEX);
	void *key = sll_remove_elm(&CLIENT_LIST, c);
	pthread_mutex_unlock(&CLIENT_LIST_MUTEX);

	return (struct client *)key;
}

/**
 * @brief Sends a message from one client to all clients.
 *
 * The message will be sent as a packet version of a struct message.
 *
 * @param[in] c The client that sent the message.
 * @param[in] msg The message content.
 *
 * @see message_pack
 */
void broadcast_client_message(struct client *c, const char *msg)
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
 * @brief Sends a message from the server to all clients.
 *
 * The message will be sent as a packet version of a struct message.
 *
 * @param[in] msg The message content.
 *
 * @see message_pack
 */
void broadcast_server_message(const char *msg)
{
	/* The serialized message will be stored here */
	char *pack = "";

	struct message *m = message_create(msg, "server");
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
		void *key = remove_client_concurrent(c);

		if (key) {
			close(client_get_socket(c));
			client_destroy(c);
		}
	}

}

/**
 * @brief Kill all connected clients.
 *
 * Removes all clients from the @c CLIENT_LIST, destroy them and closes the connection.
 *
 * @see CLIENT_LIST
 */
void kill_all_clients(void)
{
	pthread_mutex_lock(&CLIENT_LIST_MUTEX);

	struct client *c;
	while ((c = sll_remove_first(&CLIENT_LIST))) {
		pthread_cancel(*client_get_thread(c));
		close(client_get_socket(c));
		client_destroy(c);
	}

	pthread_mutex_unlock(&CLIENT_LIST_MUTEX);
}

/**
 * @brief Keeps listening to client messages.
 *
 * This function will be executed by a thread that is responsable for 
 * keep checking if there is a new message from the client.
 *
 * If there is an new message, the thread will execute the broadcast_client_message().
 *
 * @param[in] client A pointer to the client.
 *
 * @see broadcast_client_message
 */
void *listen_to_client_thread(void *client)
{
	struct client *c = (struct client *)client;
	char msg[MESSAGE_LEN];
	
	ssize_t numbytes;
	while ((numbytes = recv(client_get_socket(c), msg, MESSAGE_LEN - 1, 0)) > 0) {
		msg[numbytes] = '\0';
		broadcast_client_message(c, msg);
	}

	perror("listen_to_client_thread -> recv():");
	sprintf(msg, "%s has exit the room", client_get_name(c));

	kill_client(c);

	broadcast_server_message(msg);

	return NULL;
}

/**
 * @brief Keeps listening commands from stdin.
 *
 * This function will be executed by a thread responsible for listen to user commands.
 *
 * @param arg An adress to the accept_clients_thread() thread, so it can cancel the 
 * thread when the server administrator executes the @c /shutdown command.
 *
 * @see accept_clients_thread
 */
void *listen_to_commands_thread(void *arg)
{
	/* accept_clients_thread() thread */
	pthread_t accept_thread = *(pthread_t *)arg;

	char cmd[MESSAGE_LEN];

	while (fgets(cmd, MESSAGE_LEN, stdin)) {
		char *tok = strtok(cmd, " \n\t");

		if (tok) {
			if (strcmp(tok, "/shutdown") == 0) {
				unsigned time = 10;
				while(time--) {
					char goodbye_message[50];
					sprintf(goodbye_message, "Server shutting down in %.2d seconds.", time+1);
					broadcast_server_message(goodbye_message);
					sleep(1);
				} 

				kill_all_clients();
				pthread_cancel(accept_thread);
				break;
			}
		}
		
	}

	return arg;
}

/**
 * @brief Create a new client and add it in the @c CLIENT_LIST.
 *
 * Also broadcast everyone that the new client has entered the room.
 *
 * @param[in] sockfd The socket created in accept_clients_thread(),  and that 
 * is used to communicate with the client that will be created.
 *
 * @see accept_clients_thread
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
		insert_client_concurrent(c);

		/* 
		 * Create a a new thread for that client, this thread 
		 * will execute the listen_to_client_thread() function 
		 */
		if (pthread_create(client_get_thread(c), NULL, listen_to_client_thread, c)) {
			exit(E_PTHREAD_CREATE);
		}

		char welcome_message[MESSAGE_LEN];

		snprintf(welcome_message, MESSAGE_LEN, "%s entered the room", client_get_name(c));
		broadcast_server_message(welcome_message);
	}
}

/**
 * @brief Keeps on accepting new clients connections.
 *
 * Keeps listening for incoming connections, wen a new one
 * arrives accepts it and instantiates a new client.
 *
 * @param[in] sock Adress to the socket used to listen to new connections.
 */
void *accept_clients_thread(void *sock)
{
	int sockfd = *(int *)sock;

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
 * @return The socket in case os success. @c -1 otherwise.
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

	/* Tries to bind to a given address and port */
	if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == -1) {
		return -1;
	}

	return sockfd;
}

/**
 * @brief This function is responsible to make the initial 
 * configuration, so that this program can run as a server.
 *
 * @return A socket in passive mode, that has the localhost address asigned to it.
 * The user should be able to call accept() in this socket.
 */
int configure_as_server(void)
{
	struct addrinfo *servinfo = get_internet_addr();
	
	int sockfd;
	/* Iterate in the list of internet addresses, trying to bind in the PORT with it */
	for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = create_and_bind(p)) != -1) {
			break;
		}
		perror("socket()");
	}

	freeaddrinfo(servinfo);

	/* if sockfd is -1, that means that we could not find an address in wich we managed to bind to a port */
	if (sockfd == -1) {
		fprintf(stderr, "failed to bind\n");
		exit(E_BIND);
	}

	/* Mark the sockfd as passive, so it will be able to listen for incoming connection */
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(E_LISTEN);
	}

	return sockfd;
}

/**
 * @brief The zip-zop-server. 
 *
 * A TCP server that will accept connections from zip-zop-clients, 
 * hear its messages and broadcast them to all connected clients. Working as a chatroom.
 */
int main(void)
{
	int sockfd = configure_as_server();

	pthread_t accept_thread;
	if (pthread_create(&accept_thread, NULL, accept_clients_thread, &sockfd)) {
		exit(E_PTHREAD_CREATE);
	}

	listen_to_commands_thread(&accept_thread);

	return 0;
}

