#!/bin/bash
diff /mnt/retro/trunk/tmp/rerun1/mybashrc $1<(echo -e E=/bin/cat)
diff /mnt/retro/trunk/tmp/rerun1/safe.txt $1<(echo -e line1\\nline2)
diff /mnt/retro/trunk/tmp/rerun1/foo.txt  $1<(echo -e foo\\nline1)