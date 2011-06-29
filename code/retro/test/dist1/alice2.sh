#!/usr/bin/expect -f

set host [lindex $argv 0]

# real alice
spawn ssh alice@$host -p 2022 -F /dev/null \
    -o "PreferredAuthentications password" \
    -o "StrictHostKeyChecking no" \
    -o "UserKnownHostsFile /dev/null"

expect "password:"
send "alice\n"

expect "\\$"
send "cd /tmp/dist1\n"

expect "\\$"
send "echo line-2 >> safe-client.txt\n"

expect "\\$"
send "./alice.sh\n"

expect "\\$"
send "exit\n"
expect eof
