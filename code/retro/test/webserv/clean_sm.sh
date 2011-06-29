#!/bin/sh
echo "Cleaning up apache's shared semaphore"
ipcs -s | grep $USER | perl -e 'while (<STDIN>) { @a=split(/\s+/); print `ipcrm sem $a[1]`}'