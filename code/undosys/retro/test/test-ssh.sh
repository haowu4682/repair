#! /usr/bin/expect

spawn ssh alice@18.26.4.156 -p 2022 -F /dev/null \
    -o "PreferredAuthentications password" \
    -o "StrictHostKeyChecking no" \
    -o "UserKnownHostsFile /dev/null"

expect "password:"
send "alice\n"

expect "\\$"
send "cd /tmp\n"

expect "\\$"
send "ls\n"

expect "\\$"
send "exit\n"
expect eof
