#!/bin/bash
diff /mnt/retro/trunk/tmp/simplest/safe.txt $1<(echo -e line-1\\nline-2\\nline-3)
