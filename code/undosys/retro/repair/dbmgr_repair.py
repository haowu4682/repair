#!/usr/bin/python
import copy, ctrl, atexit, dbmgr, mgrapi, mgrutil, util

def main():
    dbmgr.load(None, None)
    for o in mgrapi.RegisteredObject.all():
        if not isinstance(o, mgrutil.BufferNode) or o.name[-1] != 'pargs' or 'Evil' not in o.data:
            continue

        nd = copy.deepcopy(o.data)
        nd = nd.replace('Evil', '')
        act = min(o.readers.union(o.writers))

        cp = max([x for x in o.checkpts if x < act])
        update_buf = dbmgr.UpdateBufAction(util.between(cp.tac, act.tic), nd, o)
        update_buf.connect()

        ctrl.repair(o, cp, gui=True)

if __name__ == "__main__":
    main()
