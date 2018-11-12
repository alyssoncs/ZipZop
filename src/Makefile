CC=gcc
CFLAGS=-Wall -Wextra -fno-strict-aliasing -lpthread
OBJSERV=zip-zop-server.o client.o sllist.o message.o
OBJCLIE=zip-zop-client.o client.o message.o

start: zip-zop-server zip-zop-client

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

zip-zop-server: $(OBJSERV)
	$(CC) $(CFLAGS) $^ -o $@

zip-zop-client: $(OBJCLIE)
	$(CC) $(CFLAGS) $^ -o $@

clean: 
	rm *.o
