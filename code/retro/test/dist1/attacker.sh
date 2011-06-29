#! /usr/bin/expect -f

set host [lindex $argv 0]

spawn ssh alice@$host -p 2022 -F /dev/null \
    -o "PreferredAuthentications password" \
    -o "StrictHostKeyChecking no" \
    -o "UserKnownHostsFile /dev/null"

expect "password:"
send "alice\n"

expect "\\$"
send "cd /tmp/dist1\n"

# overwrite personal webpage
# expect "\\$"
# send "wget google.com -o index.html\n"

# create trojan
expect "\\$"
send "cp /bin/ls xxx\n"

# install trojan
expect "\\$"
send "echo ./xxx >> ./alice.sh\n"

expect "\\$"
send "exit\n"
expect eof
