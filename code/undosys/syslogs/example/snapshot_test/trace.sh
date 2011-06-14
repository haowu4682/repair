#!/bin/sh

echo x1 >> foo.txt
./attacker.sh
echo x2 >> foo.txt
echo x3 >> foo.txt
echo bar >> bar.txt

# echo x2 >> foo.txt
# ./attacker.sh
# ./append.sh

#
# foo.txt
# 
# x1            : by setup.sh
# x2            : by trace.sh
# attack        : by attacker.sh
# x3            : by append.sh
# 
