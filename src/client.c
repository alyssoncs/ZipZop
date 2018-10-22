#include "client.h"

/**
 * @brief Struct representing a connect client in the server.
 */
struct client {
	const char *name; 	/**< Client name */
	int sockfd; 		/**< Socket that holds the connection with this client */
	pthread_t thread; 	/**< The server thread responsible to listen to this client's messages */
};

/**
 * @brief Create a client instance.
 *
 * Both parameters will be copied into the message, so the user is free to @c free() 
 * the parameters passed to this function if necessary.
 *
 * @param[in] name The client name.
 * @param[in] sockfd The socket connected to this client.
 *
 * @return A pointer to the client in case of success, NULL otherwise.
 * The client must be freed, using client_destroy().
 * 
 * @see client_destroy
 */
struct client *client_create(const char *name, int sockfd)
{
	struct client *c = malloc(sizeof(struct client));
	if (c) {
		if (name) {
			char *tmp_name = malloc(sizeof(char) * (strlen(name)+1));
			if (tmp_name) {
				strcpy(tmp_name, name);
				client_set_name(c, tmp_name);
			}
		}
		client_set_socket(c, sockfd);
	}

	return c;
}

/**
 * Destroys a client.
 *
 * @param[in] c A pointer to the client.
 */
void client_destroy(struct client *c)
{
	if (c) {
		char *tmp_name = (char *)client_get_name(c);
		free(tmp_name);
		free(c);
	}
}

/**
 * @brief Get the client name.
 *
 * @param[in] c The client.
 *
 * @return The client name.
 */
const char *client_get_name(struct client *c)
{
	if (c) {
		return c->name;
	}
	
	return NULL;
}

/**
 * @brief Get the client socket.
 *
 * @param[in] c The client.
 *
 * @return The client socket.
 */
int client_get_socket(struct client *c)
{
	if (c) {
		return c->sockfd;
	}
	
	return -1;
}

/**
 * @brief Get the client thread.
 *
 * @param[in] c The client.
 *
 * @return An Address of the client thread.
 *
 * @warning This function returns the address of the actual thread stored in the client. Do not try to free this address.
 */
pthread_t *client_get_thread(struct client *c)
{
	if (c) {
		return &c->thread;
	}
	
	return NULL;
}

/**
 * @brief Set the client name.
 *
 * @param[in] c The client.
 * @param[in] name The client name.
 */
void client_set_name(struct client *c, const char *name)
{
	if (c) {
		c->name = name;
	}
}

/**
 * @brief Set the client socket.
 *
 * @param[in] c The client.
 * @param[in] sockfd The client socket.
 */
void client_set_socket(struct client *c, int sockfd)
{
	if (c) {
		c->sockfd = sockfd;
	}
}

/**
 * @brief Set the client thread.
 *
 * @param[in] c The client.
 * @param[in] thread The client thread.
 */
void client_set_thread(struct client *c, pthread_t thread)
{
	if (c) {
		c->thread = thread;
	}
}
