CC := gcc
CFLAGS += -Wall -Werror -g

ifneq (,$(findstring aarch64-none-linux-gnu-, $(CROSS_COMPILE)))
CC := aarch64-none-linux-gnu-gcc
endif

all:
	$(CC) aesdsocket.c sigaction.c $(CFLAGS) -o aesdsocket

clean:
	rm -f aesdsocket.o sigaction.o aesdsocket

