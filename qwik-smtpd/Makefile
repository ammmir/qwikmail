# Makefile for QwikMail SMTP server (qwik-smtpd)

RM	= /bin/rm
CP	= /bin/cp
MV	= /bin/mv
MKDIR	= /bin/mkdir
CAT	= /bin/cat
SLEEP	= /bin/sleep
CHMOD	= /bin/chmod
GCC	= gcc

default_target: dist

clean:
	@echo "Cleaning up..."
	${RM} -f qwik-smtpd
	${CHMOD} -x qwik-deliver.pl mailbomb.pl myip.sh checkpassword
	@${SLEEP} 0

install: dist
	${MKDIR} /etc/qwik-smtpd
	${CP} -u checkpassword /var/spool/qwik-smtpd
	${MKDIR} /var/spool/qwik-smtpd
	${MKDIR} /var/spool/qwik-smtpd/control
	${MKDIR} /var/spool/qwik-smtpd/messages
	@echo `sh myip.sh` > /etc/qwik-smtpd/localip
	@echo `hostname` > /etc/qwik-smtpd/localhost
	@echo `hostname` > /etc/qwik-smtpd/smtpgreeting 
	@echo ""
	@echo "Congratulations! QwikMail SMTP got installed!"
	@echo "I have created the spool directories under:"
	@echo "/var/spool/qwik-smtpd"
	@echo ""
	@echo "Configuration files reside in: /etc/qwik-smtpd"
	@echo "I have copied some sample config file there..."
	@echo ""
	@echo "Now you will need to modify qwik-deliver.pl to"
	@echo "suit your needs (read INSTALL for more details."
	@echo ""
	@echo "Run qwik-deliver like so: ./qwik-deliver.pl"
	@echo ""
	@echo "Have a nice time getting this working! :-)"
	@echo ""
	@echo "http://qwikmail.sourceforge.net/smtpd/"
	@echo ""

dist: clean
	@echo "Compiling..."
	${GCC} -O2 -o qwik-smtpd qwik-smtpd.c
	@echo ""
	${CHMOD} +x qwik-deliver.pl mailbomb.pl myip.sh checkpassword
	@echo ""
	@echo "Congrats! qwik-smtpd compiled successfully."
	@echo ""
	@echo "Now do a 'make install' to create the queue directories"
	@echo "and to copy the config files and binaries."
	@echo ""
	@echo "Be careful not to install on top of a running queue!"
	@echo ""
