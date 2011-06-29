#!/bin/bash
diff -ur /mnt/retro/trunk/tmp/rerun0/mybashrc $1<(echo -e E=/bin/echo)
diff -ur /mnt/retro/trunk/tmp/rerun0/safe.txt $1<(echo -e line1\\nline2)
diff -ur /mnt/retro/trunk/tmp/rerun0/foo.txt  $1<(echo -e foo\\nsafe.txt)