#
# dbg module
#  by Taesoo Kim
#

import sys
import os

#
# dbg configuration
#
settings = {
    "test"      : True,
    "rerun"     : True,
    "syscall"   : True,
    "ctrl"      : True,
    "info"      : True,
    "error"     : True,
    "warn"      : True,
    "load"      : False,
    "debug"     : True,
    "fsmgr"     : False,
    "ptrace"    : True,
    "sha1"      : True,
    "iget"      : False,
    "connect"   : False,
    "trace"     : True,
    "state"     : True,
    "sock"      : True,
    "remote"    : True,
    "pathid"    : True,
    }

#
# usage:
#
#    dbg.test("#B<red text#>")
#    dbg.info("this is info")
#    dbg.error("this is #R<error#>")
#

#
# <<func>>   : function name
# <<line>>   : line number
# <<file>>   : file name
# <<tag>>    : tag name
# #B<        : blue
# #R<        : red
# #G<        : green
# #Y<        : yellow
# #C<        : cyan
# #>         : end mark
#

header = "'[#B<%s %s %s#>] ' % (<<func>>, <<file>>, str(<<line>>))"

# reference color
BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RESERVED, DEFAULT = range(10)

def currentframe() :
    try :
        raise Exception
    except :
        return sys.exc_traceback.tb_frame.f_back

def formatting(msg, tag, rv) :
    h = msg

    h = h.replace("<<tag>> ", tag)
    h = h.replace("<<func>>", repr(rv[2]))
    h = h.replace("<<line>>", repr(rv[1]))
    h = h.replace("<<file>>", repr(os.path.basename(rv[0])))

    return coloring(eval(h).ljust(40))

def coloring(msg) :
    h = msg
    h = h.replace("#B<", "\033[3%dm" % BLUE)
    h = h.replace("#G<", "\033[3%dm" % GREEN)
    h = h.replace("#R<", "\033[3%dm" % RED)
    h = h.replace("#Y<", "\033[3%dm" % YELLOW)
    h = h.replace("#C<", "\033[3%dm" % CYAN)
    h = h.replace("#>" , "\033[m")
    return h

def dbg(tag, mark, *msglist):
    if not (tag in settings and settings[tag]) :
        return

    f = currentframe()

    # caller's frame
    if f is not None:
        f = f.f_back

    # look up frames
    rv = "(unknown file)", 0, "(unknown function)"
    while hasattr(f, "f_code"):
        co = f.f_code
        filename = os.path.normcase(co.co_filename)
        if filename in [__file__, "<string>"]:
            f = f.f_back
            continue
        rv = (filename, f.f_lineno, co.co_name)
        break

    msg = ' '.join(map(str, msglist))
    sys.stderr.write(("%s%s %s\n" % (formatting(header, tag, rv),
                                     coloring(mark),
                                     coloring(msg))))

# Dynamically generate the debug functions. Although exec() is a nasty
# thing to use, this actually makes some sense here, because we can
# replace dbg's we're not interested in with passes... This is
# somewhat more efficient than:
# def dbg(type, msg):
#   if settings.get(type):
#     print message
for k, v in settings.iteritems() :
    if v :
        exec("def %s(*msg) : dbg('%s',' ',*msg)" % (k, k))
        exec("def %sm(*msg): dbg('%s',*msg)"     % (k, k))
    else :
        exec("def %s(*msg) : pass" % k)
        exec("def %sm(*msg): pass" % k)

def stop():
    import pdb
    pdb.Pdb().set_trace(sys._getframe().f_back)

def interact():
    from code import interact
    interact(local=locals())

class util:
  @staticmethod
  def trace(func):
    def wrapper(*args, **kws):
      sig = ('%s(%s%s)' %
        (func.__name__,
         ', '.join([str(x) for x in args]),
         (', ' + ', '.join(['%s = %s' % (str(name), str(val)) for name, val in kws.iteritems()])
           if kws else '')))
      sys.stderr.write('> %s\n' % str(sig))
      ret = func(*args, **kws)
      sys.stderr.write('< %s = %s\n' % (str(sig), str(ret)))
      return ret
    return wrapper
