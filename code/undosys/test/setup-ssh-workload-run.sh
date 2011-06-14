#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

echo "#!/bin/sh
echo IHTFP > foo.txt
" > /mnt/undofs/d/foo.sh
chmod 755 /mnt/undofs/d/foo.sh

echo "#!/bin/sh
./attacker.sh
./foo.sh
./ssh-server.sh
" > /mnt/undofs/d/trace.sh
chmod 755 /mnt/undofs/d/trace.sh

echo '#!/bin/sh
echo "date >> foo.txt" >> foo.sh
echo "date > bar.txt" >> foo.sh
' > /mnt/undofs/d/attacker.sh
chmod 755 /mnt/undofs/d/attacker.sh

cat >/mnt/undofs/d/ssh-server.sh <<EOM
#!/bin/sh
while test -f /mnt/undofs/d/restart-sshd; do
  LD_PRELOAD=/lib/libundowrap.so chroot /mnt/undofs/d /usr/sbin/sshd -d -p 2022
done
EOM
chmod 755 /mnt/undofs/d/ssh-server.sh
touch /mnt/undofs/d/restart-sshd

D=`pwd`
cp $D/../test/apr-1.3.3.tar.gz /mnt/undofs/d/tmp
( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -q -f -o /tmp/record.log ./trace.sh)

