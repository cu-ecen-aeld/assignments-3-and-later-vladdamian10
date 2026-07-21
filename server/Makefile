CC := gcc
CFLAGS += -Wall -Werror -g

ifneq (,$(findstring aarch64-none-linux-gnu-, $(CROSS_COMPILE)))
CC := aarch64-none-linux-gnu-gcc
endif

all:
	$(CC) aesdsocket.c sigaction.c socket_utils.c $(CFLAGS) -o aesdsocket

clean:
	rm -f aesdsocket.o sigaction.o socket_utils.o aesdsocket

