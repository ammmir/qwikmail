#!/bin/sh

# change eth0 here to whatever is your network interface
/sbin/ifconfig eth0 | grep 'inet addr' | awk '{print $2}' | sed -e 's/.*://'
