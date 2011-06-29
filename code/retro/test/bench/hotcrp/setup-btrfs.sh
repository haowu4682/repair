#! /bin/sh

sudo cp -rf hotcrp-2.26-orig /mnt/retro/trunk/tmp
sudo mkdir -p /mnt/retro/trunk/tmp/hotcrp-2.26-benchmark
sudo cp hotcrp-2.26-benchmark/bench-btrfs.sh /mnt/retro/trunk/tmp/hotcrp-2.26-benchmark
sudo cp hotcrp-2.26-benchmark/bench.py /mnt/retro/trunk/tmp/hotcrp-2.26-benchmark
sudo cp hotcrp-2.26-benchmark/create_users.php /mnt/retro/trunk/tmp/hotcrp-2.26-benchmark

cd ..
sudo ./clean-btrfs.sh
