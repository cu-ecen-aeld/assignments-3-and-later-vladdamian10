CC := gcc

ifneq (,$(findstring aarch64-none-linux-gnu-, $(CROSS_COMPILE)))
CC := aarch64-none-linux-gnu-gcc
endif

all:
	$(CC) writer.c -o writer

clean:
	rm -f writer writer.o
	

