#!/bin/sh
# use tcpserver instead of (x)inetd
# to accept incoming smtp connections on port 25
USER=qwikmail
GROUP=qwikmail
BINARY=/home/qwik-smtpd/qwik-smtpd
exec /usr/local/bin/tcpserver -v -u $USER -g $GROUP -DHRl0 0 25 $BINARY
