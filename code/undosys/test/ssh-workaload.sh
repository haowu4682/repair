#!/usr/bin/expect

set timeout -1

spawn ssh alice@localhost -p 2022 -o "PreferredAuthentications password" -o "StrictHostKeyChecking no" -o "UserKnownHostsFile /dev/null"
expect "password:"
send "alice\n"

expect "\\$"
send "wget http://www.opensource.apple.com/source/autoconf/autoconf-8/autoconf/autoconf.texi?txt\n"

expect "\\$"
send "mv autoconf.texi\?txt autoconf.texi\n"

expect "\\$"
send "texi2pdf -b autoconf.texi\n"

expect "\\$"
send "exit\n"

wait
