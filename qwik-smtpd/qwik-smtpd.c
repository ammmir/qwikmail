/* qwik-smtpd.c
   Description: a very bare-bones, minimum implementation of an SMTP server,
                this server complies with RFC-821 section 4.5.1 Minimum
                Implementation -- implementing all the required functionality
                of an SMTP server
   Version: 0.3
   $Date: 2002-04-28 21:55:30 $ 
   $Revision: 1.12 $
   Author: Amir Malik
   Website: http://qwikmail.sourceforge.net/smtpd/

   (C) Copyright 2000-2002 by Amir Malik

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   See the file COPYING for the full license.
   See the file INSTALL for installation instructions.

   TODO: audit the code for security issues, performance problems, lockups,
         possible bottlenecks, etc.; stop using printf(), use a less memory-
         hogging function like puts(), etc.; get gethostbyaddr() working;
         allow relaying to a bigger subset of hosts

   All patches, improvements, criticism, suggestions, and comments are welcome
   and encouraged!
*/

// Before we begin: ``Is it possible to have an SMTP server in less than 500
//                    lines of code?''
// Update (7:39 PM, 16 June 2001): ``Yes, but not an ESMTP server!''

#include "qwik-smtpd.h"
#include "config.h"

// globals
int isGood = 0;
int badCmds = 0;
int clientState = CONNECT;
char clientMailFrom[1024];
char *clientRcptTo[100];
int clientRcpts = 0;
int messageSize = 0;
int cfgRcptHosts = 0;
char *rcpthosts[100];
int logging = 1;
char logline[1024];

// checkpassword
int good;
char pipecmd[1024];

// functions
extern int parseInput(char [],char [],char [], char []);
extern int getline(char [], int);
extern int getConfig(char []);
extern int getConfigMulti(char []);
extern int out(int, char[]);
extern int push(char[]);
extern int catch_alarm(int);

int getIP()
{
  struct sockaddr name;
  struct sockaddr_in *name_in;
  char *sockname;
  size_t namelen;
  namelen = sizeof(name);
  getpeername(0, &name, &namelen);
  name_in = (struct sockaddr_in *)&name;
  sockname = inet_ntoa(name_in->sin_addr);
  return (int) sockname;
}

int main(int argc, char* argv[])
{
  /* chars are _that_ big because of buffer overflow concerns, but we try
     to limit that by reading only the first 512 chars from stdin.
     RFC821 mentions that command lines can be no longer than 512 chars,
     including the <CRLF>, but data lines may be no more than 1000 chars */

  // real paranoid about the size of chars :-O
  char *arg;
  int i = 0;

  int found = 0;

  // load config limits
  int max_recipients = atoi(getConfig("maxrcpts"));
  int max_smtp_errors = atoi(getConfig("maxsmtperrors"));
  long double max_message_size = atoi(getConfig("databytes"));

  // load config timeouts
  int connect_timeout = atoi(getConfig("connect_timeout"));
  int mail_timeout = atoi(getConfig("mail_timeout"));
  int rcpt_timeout = atoi(getConfig("rcpt_timeout"));
  int data_timeout = atoi(getConfig("data_timeout"));

  char greeting[256];
  char inputLine[1024];
  char arg1[1024];
  char arg2[1024];
  char arg3[1024];
  char *cmd;
  char *option;
  char clientHost[32];
  char clientIP[32];
  char clientHelo[32];
  char localIP[64];
  char localHost[128];
  char userName[1024];
  char controlFile[64];
  char messageFile[64];
  char messageID[128];
  char Received[128];
  FILE *fpout;

  FILE *chk;

  FILE *Log;

  int x = 0;
  int myPid = getpid(); // get our pid for a little more randomness!
  time_t t1;
  char *s;

  time_t now;
  char timebuf[100];
  now = time( (time_t*) 0 );
  (void) strftime( timebuf, sizeof(timebuf), RFC1123FMT, localtime( &now ) );

  if(!max_recipients) max_recipients = MAX_RECIPIENTS;
  if(!max_smtp_errors) max_smtp_errors = MAX_SMTP_ERRORS;
  if(!max_message_size) max_message_size = MAX_MESSAGE_SIZE;

  if(!connect_timeout) connect_timeout = CONNECT_TIMEOUT;
  if(!mail_timeout) mail_timeout = MAIL_TIMEOUT;
  if(!rcpt_timeout) rcpt_timeout = RCPT_TIMEOUT;
  if(!data_timeout) data_timeout = DATA_TIMEOUT;

  strcpy(localIP,getConfig("localip"));
  strcpy(localHost,getConfig("localhost"));
  strcpy(clientIP,getIP());

  //get rcpthosts
  getConfigMulti("rcpthosts");

  // TODO: convert the IP address into a hostname
  strcpy(clientHost,getIP());

  // send SMTP greeting
  strcpy(greeting, getConfig("smtpgreeting"));
  if(!strcmp(greeting,"")) sprintf(greeting, "%s SMTP service", localHost);
  printf("220 %s ready\r\n", greeting);
  (void) fflush(stdout);

  signal(SIGALRM, catch_alarm);

  // TODO: use another mechanism for logging; syslog dumps core
  // TODO: maybe something like /var/log/qwik-smtpd
  // open the syslog connection for logging
  //openlog("qwik-smtpd",LOG_PID,LOG_MAIL);

  // our own logging
  //if((Log = fopen("/opt/spool/qwik-smtpd.log","a")) == NULL) logging = 0;

  alarm(connect_timeout);

  // our big getline() loop; here we go
  while(getline(inputLine,1024) != EOF)
  {
    if(clientState == DATA) // we're in the middle of the DATA command
    {
      // dot . by a line on itself; double check that it's not 2 dots in front
      // kinda "queue" the message, write the control file
      if(!strncmp(inputLine,".",1) && strncmp(inputLine,"..",2) )
      {
        fclose(fpout);
        if(max_message_size > 0 && messageSize > max_message_size)
        {
          out(552, "too much mail data");
          unlink(messageFile);
          messageSize = 0;
        }
        else
        {
          if((fpout = fopen(controlFile,"w")) == NULL)
          {
            out(500, "cannot queue message; try again later");
          }
          else
          {
            // unnecessary check for max_rec.
            if(clientRcpts < max_recipients)
            {
              for(x = 0; x < max_recipients; x++)
              {
                if(clientRcptTo[x] == NULL) break;
                fprintf(fpout,clientRcptTo[x]);
                (void) fflush(fpout);
                fprintf(fpout,"\n");
                (void) fflush(fpout);
                // TODO: add logging mechanism here
                //syslog(LOG_MAIL,"From: %s To: %s ClientIP: %s %s",
                //       clientMailFrom, clientRcptTo[x], clientIP, messageID);
                //strcpy(logline,"");
                //sprintf(logline, "%s qwik-smtpd[%s] %s %s From: %s To: %s", timebuf, myPid, clientIP, messageID, clientMailFrom, clientRcptTo[x]);
                //fprintf(Log,logline);
                //(void) fflush(Log);
              }
              fclose(fpout);
            }
            else
            {
              if(x != NULL) out(500, "too many recipients");
            }
          out(250, "message accepted for delivery");
          }
        }
        clientState = GREETING;
        alarm(connect_timeout);
      }
      else // we have a preceeding dot here (escaped); eg: ..one dot before
      {
        if(!strncmp(inputLine,".",1))
        {
          for(i = 0; i < (strlen(inputLine)+1); i++) {
            if(i > 0) {
              inputLine[i-1] = inputLine[i];
            }
            if(i == strlen(inputLine)) {
              inputLine[i] = "\0";
              break;
            }
          }
        }
        fprintf(fpout,"%s\n",inputLine);
        (void) fflush(fpout);
        (void) fflush(stdout);
      }
    }
    else if(clientState < DATA)
    {
      parseInput(inputLine,arg1,arg2,arg3);
      if(!strcasecmp(arg1,"QUIT"))
      {
        out(221,"see 'ya later!");
        exit(0);
      }
      else if(!strcasecmp(arg1,"EHLO"))
      {
        clientState = GREETING;
        strcpy(clientHelo,arg2);
        if(max_message_size > 0)
        {
          printf("250-%s\r\n250-SIZE %d\r\n250 HELP\r\n", localHost,
                 max_message_size);
        }
        else
        {
          printf("250-%s\r\n250 HELP\r\n", localHost);
        }
        (void) fflush(stdout);
      }
      else if(!strcasecmp(arg1,"HELO"))
      {
        clientState = GREETING;
        strcpy(clientHelo,arg2);
        out(250, "ok");
      }
      else if(!strcasecmp(arg1,"HELP"))
      {
        out(214, "i'm a mail server, not a psychiatrist");
      }
      else if(!strcasecmp(arg1,"MAIL") && ( !strcasecmp(arg2,"FROM") ||
           !strcasecmp(arg2,"FROM:") ) )
      {
        strcpy(clientMailFrom,arg3);
        clientState = MAILFROM;
        strcpy(userName,arg3);
        if(!strcmp(arg3,"<>") )
        {
          // set EmailFrom to MAILER-DAEMON, so we know later...
          strcpy(clientMailFrom,"MAILER-DAEMON");
          out(250, "null sender <> ok");
        }
        else
        {
          out(250, "ok");
        }
        alarm(mail_timeout);
      }
      else if(!strcasecmp(arg1,"RCPT") && (!strcasecmp(arg2,"TO") ||
           !strcasecmp(arg2,"TO:")))
      {
        if(clientState <= GREETING)
        {
	  out(503,"need MAIL first");
        }
        else
        {
          if(clientRcpts > max_recipients)
          {
            out(550, "too many recipients");
          }
          else
          {
            // stop some spammers by introducing a delay
            if(clientRcpts > MAX_FAST_RCPTS || clientRcpts > max_recipients/2) sleep(1);
            // only accept mail to localHost; ignore if no @ sign
            for(x = 0; x < cfgRcptHosts; x++)
            {
              if(rcpthosts[x] != NULL && strstr(arg3,rcpthosts[x]))
              {
                found = 1;
                break;
              }
            }

            if(found == 1)
            {
              if(strstr(arg3,"!"))
              {
                out(550, "relaying denied");
              }
              else if(!strstr(arg3,"@"))
              {
                out(553, "please use full email address");
              }
              else
              {
                // either the domain part = localHost OR there is no @ sign,
                // which means that the domain = localHost
                good = 0;
                strcpy(pipecmd,"");
                alarm(0);
                sprintf(pipecmd, "%s \"%s\"", CHECKPASSWORD, arg3);
                if( strcmp(arg3,"\\") && strcmp(arg3,"..") &&
                    strcmp(arg3,"/") && strcmp(arg3,"\"") &&
                    strcmp(arg3,"\'") && strcmp(arg3,"$") &&
                    (chk = popen(pipecmd,"r")) != NULL)
                {
                  char line[128];
                  strcpy(line,"");
                  fgets(line, sizeof(line), chk);
                  pclose(chk);
                  if(!strcmp(line,"success!")) good = 1;
                }

                if(good == 1) {
                  clientState = RCPTTO;
                  push(arg3);
                  out(250, "ok");
                  alarm(rcpt_timeout);
                } else {
                  out(550, "user not here");
                  alarm(rcpt_timeout);
                }
              }
            }
            else
            {
              if(!strcasecmp(clientIP,"127.0.0.1") ||
                 !strcasecmp(clientIP,localIP)) {
                // allow only local relaying of mail
                clientState = RCPTTO;
                push(arg3);
                out(250, "ok");
                alarm(rcpt_timeout);
              }
              else
              {
                // everyone else is denied
                out(550, "relaying denied");
              }
            }
          }
        }
      }
      else if(!strcasecmp(arg1,"DATA"))
      {
        if(clientState == RCPTTO && clientRcpts > 0)
        {
          clientState = DATA;

          // here we reset controlFile, messageFile, and messageID
          strcpy(controlFile,"");
          strcpy(messageFile,"");
          strcpy(messageID,"");
          t1 = time( (time_t *) 0 );
          (int*) s = time(&t1);

          // it's not too good to use sprintf(), but...
          sprintf(controlFile, "%s/control/%d.%d", QUEUE_DIR, s, myPid);
          sprintf(messageFile, "%s/messages/%d.%d", QUEUE_DIR, s, myPid);
          sprintf(messageID, "Message-ID: <%d.%d.qwikmail@%s>\n", s,
                  myPid, localHost);
          t1 = time( (time_t *) 0 );
          (int*) s = time(&t1);
          sprintf(Received,"Received: from %s (HELO %s) (%s) by %s with SMTP; %s\n",
                  clientHost, clientHelo, clientIP, localHost, timebuf);
          if((fpout = fopen(messageFile,"w")) == NULL)
          {
            out(500, "cannot queue message; try again later");
          }
          else
          {
            fprintf(fpout,Received);
            (void) fflush(fpout);
            fprintf(fpout,messageID);
            (void) fflush(fpout);
            out(354, "type away!");
            alarm(data_timeout);
          }
        }
        else
        {
          out(503, "need MAIL/RCPT first");
        }
      }
      else
      {
        if(!strcasecmp(arg1,"NOOP"))
        {
          // no operation; used to prevent timeouts
          out(250, "ok");
          alarm(connect_timeout);
        }
        else if(!strcasecmp(arg1,"RSET"))
        {
          // reset the current state
          badCmds = 0;
          strcpy(clientMailFrom,"");
          //strcpy(clientRcptTo,"");

          for(x = 0; x < clientRcpts; x++)
          {
            //if(clientRcptTo[x] == NULL) break;
            strcpy(clientRcptTo[x],"");
          }

          clientRcpts = 0;
          strcpy(userName,"");
          out(250, "ok");
        }
        else
        {
          badCmds += 1;
          if(badCmds > max_smtp_errors)
          {
            /* looks like we got a human here as a client who doesn't know
               how to speak SMTP; or is just a bad, bad typer; incorrect
               SMTP commands received; too many bad commands received, let's
               quit and not waste memory and cpu :-) */
            out(421, "too many bad commands; closing connection");
            exit(0);
          }
          if(!isGood) out(502, "unimplemented");
          isGood = 0;
        }
      }
    }
  }
}

int out(int err, char msg[])
{
  isGood = 1;
  printf("%d %s\r\n", err, msg);
  (void) fflush(stdout);
}

int getline(char line[], int max)
{
  int nch = 0;
  int c;
  int gotSpace = 0;
  max = max - 1; // leave room for '\0'

  while((c = getchar()) != EOF)
  {
    if(c == '\r') {
      // do nothing
    } else if(clientState < DATA && gotSpace == 1 && c == ' ') {
      // do nothing
      gotSpace = 0;
    } else {
      if(c == '\n') break;
      if(clientState < DATA && c == ':') {
        gotSpace = 1;
        c = ' ';
      }

      if(nch < max)
      {
        line[nch] = c;
        nch = nch + 1;
      }
      else if(nch >= max)
      {
        return EOF;
      }
    }

    if(clientState < DATA && nch > 512) {
      // if we're not in the DATA state, lines are limited to 512 chars
      out(501, "command line too long");
      //exit(0); should we abort here?
    }

  }

  if(clientState == DATA) messageSize = messageSize + nch;
  if(c == EOF && nch == 0) return EOF;
  line[nch] = '\0';
  return nch;
}

int parseInput(char cmd[],char arg1[],char arg2[],char arg3[]) 
{
  int n = 0;
  int j = 0;
  int i = 0;

  for(;n<1000 && !isspace(cmd[n]);n++) arg1[n]=cmd[n];
  arg1[n++] = '\0';
  for(j=0;n<1000 && !isspace(cmd[n]);n++,j++) arg2[j]=cmd[n];
  arg2[j] = '\0';
  for(j=0,n++;n<1000 && !isspace(cmd[n]);n++,j++) arg3[j]=cmd[n];
  arg3[j] = '\0';
  return 0;
}

int push(char *data)
{
  clientRcptTo[clientRcpts] = (char*) malloc(64);
  strcpy(clientRcptTo[clientRcpts],data);
  clientRcpts++;
}

int getConfig(char option[])
{
  int c;
  int i = 0;
  char line[128];
  char file[128];
  FILE *config;

  sprintf(file, "%s/%s", CONFIG_DIR, option);

  if((config = fopen(file,"r")) == NULL)
  {
    //out(500, "Configuration error");
    return "";
  }
  else
  {
    while((c = getc(config)) != EOF)
    {
      if(c == '\r' || c == '\n') break;
      line[i] = c;
      i++;
    }
    line[i] = '\0';
    fclose(config);
    return line;
  }
}

// TODO: combine this function with getConfig to a more generic one
int getConfigMulti(char option[])
{
  int c;
  int i = 0;
  char line[128];
  char file[128];
  FILE *config;

  sprintf(file, "%s/%s", CONFIG_DIR, option);

  if((config = fopen(file,"r")) == NULL)
  {
    //out(500, "Configuration error");
    return "";
  }
  else
  {
    while((c = getc(config)) != EOF)
    {
      if(c == '\n')
      {
        cfgRcptHosts++;
        line[i] = '\0';
        rcpthosts[cfgRcptHosts] = (char*) malloc(64);
        strcpy(rcpthosts[cfgRcptHosts],line);
        i = -1;
        strcpy(line,"");
      }
      else
      {
        line[i] = c;
      }
      i++;
    }
    fclose(config);
    return "";
  }
}

int catch_alarm(int sig_num)
{
  out(451, "connection timeout; closing connection");
  exit(0);
}

// that's all folks!
