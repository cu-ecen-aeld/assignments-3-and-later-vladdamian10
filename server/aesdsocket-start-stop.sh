#!/bin/sh

DAEMON="aesdsocket"
case "$1" in
    start)
        echo "Starting aesdsocket"
        start-stop-daemon -S -n "${DAEMON}" -a /usr/bin/"${DAEMON}" -- -d
        ;;
    stop)
        echo "Stopping aesdsocket"
        start-stop-daemon -K -n "${DAEMON}" --signal TERM
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
esac

exit 0

