#include "client.h"

struct client {
	const char *name;
	int sockfd;
	pthread_t thread;
};

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

void client_destroy(struct client *c)
{
	if (c) {
		char *tmp_name = (char *)client_get_name(c);
		free(tmp_name);
		free(c);
	}
}

const char *client_get_name(struct client *c)
{
	if (c) {
		return c->name;
	}
	
	return NULL;
}

int client_get_socket(struct client *c)
{
	if (c) {
		return c->sockfd;
	}
	
	return -1;
}

pthread_t *client_get_thread(struct client *c)
{
	if (c) {
		return &c->thread;
	}
	
	return NULL;
}

void client_set_name(struct client *c, const char *name)
{
	if (c) {
		c->name = name;
	}
}
void client_set_socket(struct client *c, int sockfd)
{
	if (c) {
		c->sockfd = sockfd;
	}
}
void client_set_thread(struct client *c, pthread_t thread)
{
	if (c) {
		c->thread = thread;
	}
}
