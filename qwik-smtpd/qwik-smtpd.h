// include files
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>

// state values for alarm() timeouts
#define CONNECT  0  // client just connected; 220 sent
#define GREETING 1  // after HELO/EHLO has been issued
#define MAILFROM 2  // after MAIL has been issued
#define RCPTTO   3  // after RCPT has been issued
#define DATA     4  // after DATA has been issued
