CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt

all:
	$(CC) aesdsocket.c sigaction.c socket_utils.c daemon.c connection_handler.c thread_list.c thread_data.c timestamp_thread.c thread_client.c $(CFLAGS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)
