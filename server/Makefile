CC := gcc
CFLAGS += -Wall -Werror -g

ifneq (,$(findstring aarch64-none-linux-gnu-, $(CROSS_COMPILE)))
CC := aarch64-none-linux-gnu-gcc
endif

all:
	$(CC) aesdsocket.c sigaction.c socket_utils.c daemon.c connection_handler.c thread_list.c $(CFLAGS) -o aesdsocket -lpthread

clean:
	rm -f aesdsocket.o sigaction.o socket_utils.o daemon.o connection_handler.o thread_list.o aesdsocket

