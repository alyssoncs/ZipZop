#include "message.h"

/**
 * @brief Struct representing a messege sent by some sender.
 */
struct message {
	const char *content; 		/**< The content of the message */
	const char *sender_name; 	/**< The username of the sender */
};

/**
 * @brief Creates a message.
 *
 * Both parameters will be copied into the message, so the user is free to @c free() 
 * the parameters passed to this function if necessary.
 *
 * @param[in] content The content of the message.
 * @param[in] sender_name The username of the sender.
 *
 * @return A pointer to a struct message in case of success, NULL otherwise. 
 * The message must be freed, using message_destroy(), when is not needed anymore.
 * @see message_destroy
 */
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

/**
 * @brief Destroys a message.
 *
 * @param[in] m A pointer to the message.
 *
 * @see message_create
 */
void message_destroy(struct message *m)
{
	char *content 		= (char *)message_get_content(m);
	char *sender_name 	= (char *)message_get_sender(m);

	free(content);
	free(sender_name);
	free(m);
}

/**
 * @brief Get the message content.
 *
 * @param[in] m A pointer to the message.
 *
 * @return A pointer to the message content.
 *
 * @warning The returned value should not be freed.
 */
const char *message_get_content(struct message *m)
{
	if (m)
		return m->content;

	return NULL;
}

/**
 * @brief Get the message sender.
 *
 * @param[in] m A pointer to the message.
 *
 * @return A pointer to the sender name.
 *
 * @warning The returned value should not be freed.
 */
const char *message_get_sender(struct message *m)
{
	if (m)
		return m->sender_name;

	return NULL;
}

/**
 * @brief Serialize a message.
 *
 * Pack/Serialize the struct message in a format that  
 * can be sent through the network.
 *
 * @param[in] m A pointer to the message.
 * @param[out] len A pointer to a integer where the length of the serialized message will be stored.
 *
 * @return A pointer to the serialized message. This should be freed when is not necessary anymore.
 *
 * @see message_unpack
 */
char *message_pack(struct message *m, int *len)
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

		*len = size;
	}

	return pack;
}

/**
 * @brief Deserialize a message.
 *
 * Unpack/Deserialize a string into a struct message.
 *
 * @param[in] pack The string that represent the packed message generated by message_pack().
 *
 * @return A pointer to the deserialized message. This should be freed when is not necessary anymore.
 *
 * @see message_pack
 */
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

