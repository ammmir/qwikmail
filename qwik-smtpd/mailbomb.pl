#!/usr/bin/perl

use Net::SMTP;

print <<_EOF;

This script will attempt to trash your mail server with mail.
It is meant as a test of the QwikMail SMTP server, to see how
much mail it can actually handle.

_EOF

for($i = 0; $i < 100; $i++) {
  my $smtp = Net::SMTP->new('localhost');

  $smtp->mail('who@cares.dom');
  $smtp->to('amir');

  $smtp->data();
  $smtp->datasend("To: amir\n");
  $smtp->datasend("From: Who Cares <who\@cares.dom>\n");
  $smtp->datasend("Subject: test message; number $i\n");
  $smtp->datasend("\n");
  $smtp->datasend("A simple test message...\n");
  $smtp->dataend();

  $smtp->quit;
  print "$i ";
}

print "\n\nSent $i messages!\n";
