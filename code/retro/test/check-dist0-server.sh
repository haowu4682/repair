#!/bin/bash
diff -ur /mnt/retro/trunk/tmp/dist0/server-safe.txt $1<(echo -e line-1\\nline-2)
