/* Do the following:
    - change QUEUE_DIR to point to your queue dir (eg. /var/spool/qwik-smtpd)
    - change MAX_RECIPIENTS if you want to
    - change RFC1123FMT to point to your timezone; eg. PDT to EST

   And that's it!

   Description:
    QUEUE_DIR - root path of the spool directory for control and message files
    CONFIG_DIR - where the configuration files are kept (aka qmail's control/)
    RFC1123FMT - time format for your timezone, in RFC1123 time format
    MAX_RECIPIENTS - the max number of recipients a message can have
    MAX_SMTP_ERRORS - the max number of errors in an SMTP transaction before
                      the server disconnects the client
*/

// where the root of the mail queue directory is
#define QUEUE_DIR "/var/spool/qwik-smtpd"

// where the qwik-smtpd configuration files reside
#define CONFIG_DIR "/etc/qwik-smtpd"

/* change the -0800 and PDT according to your timezone; the time format
   below is _supposed_ to be RFC 1123 compliant, but it really isn't! */
#define RFC1123FMT "%d %b %Y %H:%M:%S -0800 (PDT)"


/* ###################### DEFAULT VALUES ############################### */
// config files will override the following defaults

/* maximum number of recipients per message; RFC 821 says that every
   compliant server should accept at least 100 */
#define MAX_RECIPIENTS 100

/* maximum number of bad commands that can be received before the server
   shuts down the connection; DO NOT SET BELOW 2! */
#define MAX_SMTP_ERRORS 10

// maximum size of an email message, in bytes; 5MB = 5242880; 0 to disable
#define MAX_MESSAGE_SIZE 5242880

/* these are timouts in seconds; the client must issue one of these commands
   before the server commits suicide; cause it don't wanna deal with clients
   that don't do anything! :-) set to 0 if you want to disable a timeout */
#define CONNECT_TIMEOUT 300	// 5m
#define MAIL_TIMEOUT 60		// 1m
#define RCPT_TIMEOUT 60		// 1m
#define DATA_TIMEOUT 7200	// 2h
