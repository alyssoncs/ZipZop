#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>
#include <string.h>

#include <pthread.h>

struct client;

struct client *client_create(const char *name, int sockfd);
void client_destroy(struct client *c);
const char *client_get_name(struct client *c);
int client_get_socket(struct client *c);
pthread_t *client_get_thread(struct client *c);
void client_set_name(struct client *c, const char *name);
void client_set_socket(struct client *c, int sockfd);
void client_set_thread(struct client *c, pthread_t thread);

#endif
