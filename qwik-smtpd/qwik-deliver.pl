#!/usr/bin/perl -w

# qwik-deliver.pl - delivery agent for QwikMail smtpd
# Part of the Qwik SQL Webmail Server project, part of qwik-smtpd:
# http://qwikmail.sourceforge.net/smtpd/
# (C) Copyright 2000-2001 by Amir Malik

use strict;
use DBI;
use Fcntl ':flock';
use POSIX qw(setsid);
use vars qw($dbh $killed $exp $pid);

# catch these signals
$SIG{TERM} = sub { $killed = 1; };
$SIG{HUP} = sub { $killed = 1; };
$SIG{INT} = sub { $killed = 1; };
$SIG{QUIT} = sub { $killed = 1; };
$SIG{STOP} = sub { $killed = 1; };
$SIG{CHLD} = sub { wait };

# CONFIGURATION OPTIONS

my $domain = 'virusexperts.com';

my $hostname = 'hosting';

my $queuedir = '/var/spool/qwik-smtpd'; # MODIFY THIS

my $delivery = 2; # 0 = none (???), 1 = mbox, 2 = maildir

my $maildir = 'Maildir'; # name of maildir under user's homedir

my $varspool = '/var/spool/mail';

my $homedirs = '/home';

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
      open(STDERR, ">>/tmp/qsmtpd.log");
      if(!$<) {
        my($x,$uid,$gid);
        ($x,$x,$x,$x,$uid,$gid) = stat("$homedirs/$localuser");
        (!$uid || !$gid) && die "file is root-owned";
        $> = $uid || die "can't set effuid";
        $) = $gid || die "can't set effgid";
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
          open(MSG,">tmp/$time.$$.$hostname");
          print MSG $message;
          close(MSG);
          link("tmp/$time.$$.$hostname","new/$time.$$.$hostname");
        }
      } else { # no deliver; ignore! :-O
        # ???
      }

      if(!$localuser) {
        if(open(INJECT,"| /usr/sbin/sendmail \"$to\"")) {
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
    exit(0);
  } else {
    sleep(60);
  }
}
