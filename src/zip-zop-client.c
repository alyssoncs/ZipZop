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

/** @brief The port where this application will be running */
#define PORT "1234"

/** @brief Maximum length of a client message */
#define MESSAGE_LEN 2000 

/**
 * @brief Checks if the user enter the arguments in the correct manner.
 *
 * @param[in] argc Number of arguments.
 *
 * @return @c true if the arguments are correct, @c false otherwise.
 */
bool check_args(int argc)
{
	if (argc == 3)
		return true;
	return false;
}

/**
 * @brief Prints the correct usage of the program.
 *
 * @param[in] name The name of this program.
 */
void print_usage(const char *name)
{
	printf("usage: %s <server addr> <username>\n", name);
}

/**
 * @brief Displays a message in the screen.
 *
 * @param[in] m The message.
 */
void show_message(struct message *m)
{
	if (m) {
		printf("[%s]: ", message_get_sender(m));
		printf("%s\n", message_get_content(m));
	}
}

/**
 * @brief Keeps listening to server messages.
 *
 * This function will be executed by a thread that is responsable for 
 * keep checking if there is a new message from the server.
 *
 * If there is an new message, the thread will display the message.
 *
 * @param[in] client A pointer to the client.
 *
 * @see show_message
 */
void *listen_to_server_thread(void *client)
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

	perror("listen_to_server_thread -> recv():");

	pthread_cancel(*client_get_thread(c));
	close(client_get_socket(c));
	client_destroy(c);

	return NULL;
}

/**
 * @brief Keeps reading messages from @c stdin and send them to server.
 *
 * The message will be sent as a packet version of a struct message.
 *
 * @param[in] c The client that sent the message.
 *
 * @see message_pack
 */
void *speak_thread(void *client)
{
	struct client *c = (struct client *)client;
	char msg[MESSAGE_LEN];

	while (fgets(msg, MESSAGE_LEN, stdin)) {
		char *nl = strchr(msg, '\n');
		if (nl) {
			*nl = '\0';
		}

		char cmd[MESSAGE_LEN];
		strcpy(cmd, msg);
		char *tok = strtok(cmd, " \n\t");

		if (tok) {
			if (strcmp(tok, "/exit") == 0) {
				exit(0);
			}
		}

		int rv = send(client_get_socket(c), msg, strlen(msg) + 1, 0);
		if (rv <= 0) {
			perror("send()");
		}
	}

	return NULL;
}

/**
 * @brief Gets the internet address of the server.
 *
 * Given the server name, this function will try to find an internet address to this server.
 *
 * @param[in] server_name The server name.
 *
 * @return A pointer to a list of possibly valid server internet addresses.
 */
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

/**
 * @brief Attempts to create a socket to an internet address 
 * and connect to it in to a port.
 *
 * @param[in] addr The internet address.
 *
 * @return The socket in case os success. @c -1 otherwise.
 */
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

/**
 * @brief Presents the client to the server.
 *
 * This function sends everything that is needed to introduce the client to the server.
 *
 * In this case only the client name is sent to the server.
 *
 * @param[in] c The client.
 */
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

/**
 * @brief Manages the connection with a user and a server.
 *
 * Given a username and a socket connected with the server, manages the connection, 
 * creating a thread to listen to incomming messages from the server, and another to 
 * read messages from the user and send them to the server.
 *
 * @param[in] user_name The username.
 * @param[in] sockfd The socket connected to the server.
 */
void communicate(const char *user_name, int sockfd)
{
	struct client *c = client_create(user_name, sockfd);

	if (c) {
		server_introduction(c);
		if (pthread_create(client_get_thread(c), NULL, speak_thread, c)) {
			exit(E_PTHREAD_CREATE);
		}
		listen_to_server_thread(c);
	}
}

/**
 * @brief The zip-zop-client.
 *
 * A TCP client that will connect with an instance of the zip-zop-server.
 *
 * @param[in] argc Number of arguments given by the user.
 * @param[in] argc An array of strings representing the arguments given by the user
 * 
 * @note Usage: ./zip-zop-client <server_addr> <username>
 */
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
	/* Iterate through all the list of server addresses, and try to connect to one of them */
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
