# Makefile for QwikMail SMTP server (qwik-smtpd)
# configuration
BINARIES=/usr/local/bin
QUEUE=/var/spool/qwik-smtpd
CONFIG=/etc/qwik-smtpd
# end configuration

CP	= /bin/cp
MV	= /bin/mv
MKDIR	= /bin/mkdir
INSTALL = /usr/bin/install
CC = gcc
CFLAGS = -O2

SOURCES = qwik-smtpd.h qwik-smtpd.c config.h
OBJS = qwik-smtpd.o
TARGET = qwik-smtpd

${TARGET}: ${OBJS}
	@echo "Compiling..."
	${CC} ${CFLAGS} -o ${TARGET} ${OBJS}
	@echo ""
	@echo "Congrats! qwik-smtpd compiled successfully."
	@echo ""
	@echo "Proceed with a 'make install' to install the following items:"
	@echo " - binaries into ${BINARIES}"
	@echo " - queue structure into ${QUEUE}"
	@echo " - configuration files into ${CONFIG}"
	@echo ""
	@echo "Be careful not to install on top of a running queue!"
	@echo "Thank you for using QwikMail."
	@echo ""

${OBJS}: ${SOURCES}
	${CC} ${CFLAGS} -c $*.c

clean:
	@echo "Cleaning up..."
	rm -f ${OBJS} ${TARGET}

test: ${TARGET}
	./qwik-smtpd

# default_target: dist
install: ${TARGET}
	${MKDIR} ${CONFIG}
	${INSTALL} -d -o qwikmail -g qwikmail -m 750 ${CONFIG}
	${INSTALL} -d -o qwikmail -g qwikmail -m 750 ${QUEUE}
	${INSTALL} -d -o qwikmail -g qwikmail -m 750 ${QUEUE}/control
	${INSTALL} -d -o qwikmail -g qwikmail -m 750 ${QUEUE}/messages
	${INSTALL} -o qwikmail -g qwikmail -m 750 -p checkpassword ${QUEUE}
	${INSTALL} -o qwikmail -g qwikmail -m 750 -p -s qwik-smtpd ${BINARIES}
	${INSTALL} -o qwikmail -g qwikmail -m 750 -p qwik-deliver.pl ${BINARIES}/qwik-deliver
	@echo `sh myip.sh` > ${CONFIG}/localip
	@echo `hostname` > ${CONFIG}/localhost
	@echo `hostname` > ${CONFIG}/smtpgreeting 
	@echo ""
	@echo "Congratulations! QwikMail SMTP got installed!"
	@echo "I have created the spool directories under:"
	@echo "${QUEUE}"
	@echo ""
	@echo "Configuration files reside in: ${CONFIG}"
	@echo "I have copied some sample config file there..."
	@echo ""
	@echo "Binaries have been installed under ${BINARIES}"
	@echo ""
	@echo "Now you will need to modify qwik-deliver.pl to"
	@echo "suit your needs (read INSTALL for more details."
	@echo ""
	@echo "Run qwik-deliver like so: ${BINARIES}/qwik-deliver"
	@echo ""
	@echo "http://qwikmail.sourceforge.net/smtpd/"
	@echo ""
