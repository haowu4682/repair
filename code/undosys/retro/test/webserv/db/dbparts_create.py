#!/usr/bin/python

parts = {
    'PERSON':
    {
        'column' : 'USERNAME',
        'type'   : 'str',
        'ranges' : ['a', 'b', 'c', 'd', 'e', 'f', 'k', 'l', 'm', 'n']
    }
}

import json
open('/tmp/retro/db_partitions.txt', 'w').write(json.dumps(parts))
