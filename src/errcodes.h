#ifndef ERRCODES_H
#define ERRCODES_H

/**
 * @brief Possible error codes in the project.
 */
enum errcodes {
	E_SUCCESS,          /**< Success value */
	E_GETADDRINFO,      /**< Error code if getaddrinfo() fails */
	E_BIND,             /**< Error code if it was not possible to bind() in the specified port */
	E_LISTEN,           /**< Error code if listen() fails */
	E_BAD_ARGS,         /**< Error code if the user gave a bad input */
	E_CONNECT,          /**< Error code if connect() fails */
	E_PTHREAD_CREATE    /**< Error code if it was not possible to create a new thread */
};

#endif
