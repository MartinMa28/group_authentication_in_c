#! /usr/bin/expect -f
set timeout -1
spawn ./scp_bash.sh
match_max 100000
expect -exact "unix_martin@192.168.0.3's password: "
send -- "unix_martin\r"
expect eof