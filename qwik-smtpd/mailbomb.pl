#!/usr/bin/perl
# (C) 2002 by Amir Malik.
# Part of qwik-smtpd, http://qwikmail.sourceforge.net/smtpd/

my $TOTAL = 100_000; # 10,000 should be adequae :)
my $RCPT_TO = 'null'; # must be a valid account/recipient
my $SERVER = '127.0.0.1:2555'; # preferably on a development machine/port

$| = 1;
use Net::SMTP;

print <<_EOF;
qwik-smtpd : mailbomb.pl
========================

This script will attempt to trash your mail server with mail.
It is meant as a test of the QwikMail SMTP server, to see how
much mail it can actually handle.

Configured to send $TOTAL emails.
For best results, fork off a couple of these scripts, and then
measure the results... :-)

Server: $SERVER
RCPT_TO: $RCPT_TO
Total: $TOTAL

_EOF

die("You need to set SERVER, RCPT_TO, and TOTAL for this to work!\nTo actually run it, type: $0 do it\n") if(!$SERVER || !$RCPT_TO || !$TOTAL || !$ARGV[0] || !$ARGV[1]);

my $start = time();

for($i = 1; $i <= $TOTAL; $i++) {
  my $smtp = Net::SMTP->new($SERVER);
  $smtp->mail('user@domain.ext');
  $smtp->to($RCPT_TO);
  $smtp->data();
  $smtp->datasend("To: $RCPT_TO\n");
  $smtp->datasend("From: user\@domain.ext\n");
  $smtp->datasend("Subject: mailbomb.pl -- number $i\n");
  $smtp->datasend("\n");
  $smtp->datasend("A simple test message...\n$i out of a total $TOTAL\n");
  $smtp->dataend();
  $smtp->quit;
  print "$i/$TOTAL = " . int(0.5 + 100*$i/$TOTAL) . "%\n";
}

my $now = time();
my $elapsed = $now - $start;
my $el;
if($elapsed > 60) {
  $el = "$elapsed s = ";
  $elapsed = $elapsed / 60;
  my $s = $elapsed;
  $elapsed = int(0.5 + $elapsed);
  $s =~ m/(.*)\.(.*)/;
  $s = "0.$2";
  $s *= 60;
  $s = int(0.5 + $s);
  $el .= "$elapsed m $s s";
} else {
  $el = "$elapsed s";
}

print "\n\nSent $i messages!\n";
print "Start time: $start\nEnd time: $now\nElapsed time: $el\n";
