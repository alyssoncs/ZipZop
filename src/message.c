#include "message.h"

struct message {
	const char *content;
	const char *sender_name;
};

struct message *message_create(const char *content, const char *sender_name)
{
	int content_len = 0;
	int sender_name_len = 0;

	if (content)
		content_len = strlen(content) + 1;
	
	if (sender_name)
		sender_name_len = strlen(sender_name) + 1;

	struct message *m = malloc(sizeof(struct message));
	if (m) {
		m->content 		= malloc(sizeof(char) * content_len);
		m->sender_name 	= malloc(sizeof(char) * sender_name_len);
	}

	if (m->content && m->sender_name) {
		strcpy((char *)m->content, content);
		strcpy((char *)m->sender_name, sender_name);
	}

	return m;
}

void message_destroy(struct message *m)
{
	char *content 		= (char *)message_get_content(m);
	char *sender_name 	= (char *)message_get_sender(m);

	free(content);
	free(sender_name);
	free(m);
}

const char *message_get_content(struct message *m)
{
	if (m)
		return m->content;

	return NULL;
}

const char *message_get_sender(struct message *m)
{
	if (m)
		return m->sender_name;

	return NULL;
}

char *message_pack(struct message *m)
{
	char *pack = NULL;
	if (m) {
		char *content = (char *)message_get_content(m);
		char *sender  = (char *)message_get_sender(m);

		int size = strlen(content) + strlen(sender) + 2;
		pack = malloc(sizeof(char) * size);
		if (pack) {
			strcpy(pack, content);
			strcpy(pack + strlen(pack) + 1, sender);
		}
	}

	return pack;
}

struct message *message_unpack(char *pack)
{
	struct message *m = NULL;
	if (pack) {
		char *content = pack;
		char *sender  = strchr(pack, '\0') + 1;

		m = message_create(content, sender);
	}

	return m;
}

