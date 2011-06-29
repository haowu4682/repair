#!/bin/bash
diff -ur /mnt/retro/trunk/tmp/rerun2/mybashrc $1<(echo -e E=/bin/cat)
diff -ur /mnt/retro/trunk/tmp/rerun2/safe.txt $1<(echo -e line1\\nline2\\nline3)
diff -ur /mnt/retro/trunk/tmp/rerun2/foo.txt  $1<(echo -e foo\\nline1)