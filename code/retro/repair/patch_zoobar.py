#!/usr/bin/python

## See retro/test/webserv/README for details on how to run this

import sys, util, ctrl, mgrapi, mgrutil, zoobarmgr

def patch_php():
    if len(sys.argv) != 3:
        print "Usage: %s LOG-DIRECTORY PHP-SCRIPT-NAME" % sys.argv[0]
        exit(1)

    logdir = sys.argv[1]
    php_fn = sys.argv[2]
    
    zoobarmgr.set_logdir(sys.argv[1])
    zoobarmgr.load(None, None)
    patched_node = None
    for o in mgrapi.RegisteredObject.all():
        if isinstance(o, zoobarmgr.PhpScript) and \
           o.name[-1].find(php_fn) != -1:
            patched_node = o
            
    if patched_node is None:
        print "nothing to repair: script %s not in the history graph" % php_fn
        exit(0)

    ## Get the first action that depends on patched_node
    an = min(patched_node.readers)

    ## set the timestamp of update node to before that of the first action
    update_node = zoobarmgr.PhpScriptUpdate(('phpupdate', php_fn), patched_node)
    update_node.tic = update_node.tac = an.tic
    update_node.connect()

    print "repair after patching script", patched_node
    cp = iter(patched_node.checkpts).next()
    ctrl.repair(patched_node, cp)

if __name__ == "__main__":
    patch_php()
