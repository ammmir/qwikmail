#!/usr/bin/perl -w
# qwik-deliver.pl - delivery agent for QwikMail smtpd
# Part of the Qwik SQL Webmail Server project, part of qwik-smtpd:
# http://qwikmail.sourceforge.net/smtpd/
# (C) Copyright 2000-2002 by Amir Malik
# $Date: 2002-04-28 22:15:18 $
# $Revision: 1.7 $

# Assumptions: (please grep this file for MODIFY THIS and change)
# installing in /home/qwik-smtpd
# spool is in /var/spool/qwik-smtpd
# Maildir structure by default ie... /home/USERNAME/Maildir/SUBDIR
# pid file for this daemon in /home/qwik-smtpd/qwik-deliver.pid
# THIS DAEMON RUNS AS root! YOU HAVE BEEN WARNED.
# THE CODE SHOULD BE SECURE, BUT BE SURE TO LOOK OVER WHAT THIS DAEMON
# EXECUTES, HOW, AND WHAT ARGUMENTS IT PASSES TO THE SHELL!!!
# works fine for me :-) -- heck, sendmail's smtpd runs as root... hmm..

use strict;
use DBI;
use Fcntl ':flock';
use POSIX qw(setsid);
use vars qw($dbh $killed $exp $pid);

# set a good umask
umask(077);

# write pid
my $pidfile = "/home/qwik-smtpd/qwik-deliver.pid"; # MODIFY THIS
open(PID,">$pidfile");
print PID "$$\n";
close(PID);

# catch these signals
$SIG{TERM} = sub { $killed = 1; };
$SIG{HUP} = sub { $killed = 1; };
$SIG{INT} = sub { $killed = 1; };
$SIG{QUIT} = sub { $killed = 1; };
$SIG{STOP} = sub { $killed = 1; };
$SIG{CHLD} = sub { wait };

# MODIFY THS BELOW

my $domain = getConfig('localhost');
my $hostname = getConfig('localhost');
my $queuedir = getConfig('queuedir') || '/var/spool/qwik-smtpd';
my $delivery = getConfig('delivery') || 2;
my $maildir = getConfig('homemaildir') || 'Maildir'; # maildir under user's dir
my $varspool = getConfig('spooldir') || '/var/spool/mail';
my $homedirs = '/home';

if(!$domain || !$hostname || !$queuedir) {
  print "Error: configuration is incomplete\n";
  exit(0);
}

#######################

$pid = 0;
$killed = 0;
$exp = '([^":\s<>()/;]*@[^":\s<>()/;]*)'; # funky regexp for email addr

# don't let UNIX buffering spoil your day! :-)
$| = 1;

chdir '/' or die "Can't chdir to /: $!";
open(STDIN, '/dev/null') or die "Can't read /dev/null: $!";
open(STDOUT, '>>/dev/null') or die "Can't write to /dev/null: $!";
open(STDERR, '>>/dev/null') or die "Can't write to /dev/null: $!";
defined(my $pid = fork()) or die "Can't fork: $!";
exit() if($pid);
setsid() or die "Can't start a new session: $!";
umask(0);

# our infinite loop
while(1) {
  if($killed) {
    exit(0);
  }
  opendir(DIR,"$queuedir/control");
  my @files = readdir(DIR);
  closedir(DIR);
  foreach my $file (@files) {
    next if($file eq '.' || $file eq '..');
    my $message;
    open(MSG,"$queuedir/messages/$file");
    while(<MSG>) { $message .= $_; }
    close(MSG);
    open(CONTROL,"$queuedir/control/$file");
    # loop through the control file and inject the message for each user
    while(<CONTROL>) {
      chop;
      my $to = $_;
      $to =~ /$exp/smg; # make it into a valid email address
      my $localuser;
      if($to =~ /(.*)\@$domain/i) {
        $localuser = $1;
        $localuser =~ s/<//;
      }

    unless($pid = fork) { # start of fork()
      open(STDERR, ">>/var/spool/qwik-deliver.log"); # MODIFY THIS
      if($to =~ /(.*)\@$domain/i && !$<) {
        if(-e "$homedirs/$localuser/") {
          my($x,$uid,$gid);
          ($x,$x,$x,$x,$uid,$gid) = stat("$homedirs/$localuser");
          (!$uid || !$gid) && die "file is root-owned";
          $> = $uid || die "can't set effuid";
          $) = $gid || die "can't set effgid";
        } else {
          $delivery = 3;
        }
      }

      if($delivery == 1) { # mbox
        $localuser = undef if(!-e "$varspool/mail/$localuser");
        if($localuser) {
          open(MBOX,">>$varspool/$localuser");
          flock(MBOX,LOCK_EX);
          print MBOX $message;
          flock(MBOX,LOCK_UN);
          close(MBOX);
        }
      } elsif($delivery == 2) { # maildir
        $localuser = undef if(!-e "$homedirs/$localuser/$maildir");
        if($localuser) {
          chdir("$homedirs/$localuser/$maildir");
          my $time;
          $time = time();
          my $e;
          $e = stat("tmp/$time.$$.$hostname");
          if(!$e) {
            sleep(2);
            $time = time();
            $e = stat("tmp/$time.$$.$hostname");
          }
          $message =~ m/Message-ID: <(.*)>/;
          my $msgid = $1;
          Log("DELIVERY $msgid local $localuser $e");
          open(MSG,">tmp/$time.$$.$hostname");
          print MSG $message;
          close(MSG);
          link("tmp/$time.$$.$hostname","new/$time.$$.$hostname");
        }
      } else { # no deliver; ignore! :-O
        # ???
      }

      if(!$localuser || $localuser =~ /(.*)\@$domain/i) {
        my $from;
        # qwik-smtpd hack via SquirrelMail
        $message =~ m/X-LocalUser: (.*)/;
        $from = $1;
        $message =~ m/Message-ID: <(.+?)>/;
        my $msgid = $1;
        $from =~ s/(\/|\\|\.\.|\'|\")//g;

	# MODIFY THIS BELOW; this is just a dummy log...
	open(Z,">>/tmp/sm.log");
	print Z "\nFROM = $from\nMSG-ID: $msgid\nTo = $to\n";
	close(Z);

	if($from =~ /\@$domain/i) {
          $from = "MAILER-DAEMON@" . $domain;
        } else {
          $from .= "@$domain";
        }
        if(open(INJECT,"| /usr/sbin/sendmail -f\"$from\" \"$to\"")) {
          Log("DELIVERY $msgid sendmail $to $from");
          print INJECT $message;
          close(INJECT);
        }
      }

      exit(0);
      } # end of fork()

    }
    close(CONTROL);
    # delete the control and message files from the queue
    unlink("$queuedir/control/$file");
    unlink("$queuedir/messages/$file");
    $message = undef;
  }

  # if we got a SIGnal, then disconnect from db and die, else wait 1 minute
  # and then re-run the queue
  if($killed) {
    unlink($pidfile);
    exit(0);
  } else {
    sleep(30);
  }
}

sub getConfig {
  my($name) = @_;
  my $value;
  if(open(CONFIG,"</etc/qwik-smtpd/$name")) {
    chomp($value = <CONFIG>);
    close(CONFIG);
  }
  return $value;
}

sub Log {
  print STDERR "$_[0]\n";
}
