#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdlib.h>
#include <string.h>

struct message;

struct message *message_create(const char *content, const char *sender_name);
void message_destroy(struct message *m);
const char *message_get_content(struct message *m);
const char *message_get_sender(struct message *m);
char *message_pack(struct message *m, int *len);
struct message *message_unpack(char *pack);

#endif
