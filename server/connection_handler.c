#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include "sigaction.h"
#include "connection_handler.h"

// Number of bytes used to store the data sent from remote, via sockets
#define BUFF_LEN_BYTES 8194*4

int handle_connection(int new_sockfd, int fd) {
    size_t buf_len = BUFF_LEN_BYTES;
    char* writestr = (char*)malloc(buf_len); // used to write bytes to file
    char* readstr = (char*)malloc(buf_len); // used to read bytes from file
    if (writestr == NULL || readstr == NULL) {
        perror("malloc");
        free(writestr);
        free(readstr);
        return -1;
    }

    bool do_serve = true;
    bool is_packet_received = false;
    ssize_t nb_rcvd = 0; // used as an offset to know how many bytes are in a packet.
    ssize_t nb_read;
    ssize_t nb_sent;
    while (do_serve && !(caught_sigint || caught_sigterm)) {
        // e. Receives data over the connection and appends to file.
        ssize_t n = recv(new_sockfd, (char*)(writestr + nb_rcvd), buf_len - nb_rcvd, 0);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // no data available, try again later.
                continue;
            }
            perror("recv");
            do_serve = false;
            break;
        }
        else if (n == 0) {
            do_serve = false;
            break;
        }
        else {
            nb_rcvd += n;
        }

        // e. use a newline to separate data packets received.
        if (writestr[nb_rcvd-1] == '\n') {
                // e. each newline should result in an append to the /var/tmp/aesdsocketdata file
                if (write(fd, writestr, nb_rcvd) == -1) {
                    perror("write");
                    do_serve = false;
                    is_packet_received = false;
                } else {
                    is_packet_received = true;
                }
                // always reset buffer before reading again.
                memset(writestr, 0, buf_len);
                // reset the offset of received bytes.
                nb_rcvd = 0;
        }

        // f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as
        //  the received data packet completes.
        if (is_packet_received) {
                    lseek(fd, 0, SEEK_SET);

                    while ((nb_read = read(fd, readstr, buf_len)) > 0) {
                            if ((nb_sent = send(new_sockfd, readstr, nb_read, 0)) == -1) {
                                perror("send");
                                do_serve = false;
                                break;
                            }
                    }
                    if (nb_read == -1) {
                        perror("read");
                        do_serve = false;
                        break;
                    }
                    is_packet_received = false; // reset the flag.
                    memset(readstr, 0, buf_len);
        }
    }

    free(writestr);
    free(readstr);

    return 0;
}
