#
# utilities
#

import sys

#
# trigger pdb when error
#
def install_pdb() :
    def info(type, value, tb):
        if hasattr(sys, 'ps1') or not sys.stderr.isatty():
            # You are in interactive mode or don't have a tty-like
            # device, so call the default hook
            sys.__execthook__(type, value, tb)
        else:
            import traceback, pdb
            # You are not in interactive mode; print the exception
            traceback.print_exception(type, value, tb)
            print
            # ... then star the debugger in post-mortem mode
            pdb.pm()

    sys.excepthook = info

# singleton
def The( singleton, *args ) :
    if not hasattr( singleton, "__instance" ) :
        singleton.__instance = singleton( *args )

    return singleton.__instance

def unique( ele ) :
    return list(set(ele))

def static( obj, var, init ) :
    if not hasattr( obj, var ) :
        setattr( obj, var, init )
    return getattr( obj, var )

def union( l1, l2 ):
    return list(set(l1)|set(l2))