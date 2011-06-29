#!/bin/bash
diff -ur /mnt/retro/trunk/tmp/dist0/client-safe.txt $1<(echo -e line-1\\nline-2\\nline-3)
