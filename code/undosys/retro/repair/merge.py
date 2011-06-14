#!/usr/bin/python

import optparse
import atexit

import ctrl
import mgrapi
import dbg
import fsmgr
import runopts
import osloader
import retroctl
import util

import pdb
import code

util.install_pdb()

def main():
  atexit.register(retroctl.disable)

  parser = optparse.OptionParser(usage="%prog [options] LOG-DIRECTORY")
  parser.add_option("-d", "--dry", default=False, action="store_true",
        dest="dryrun", help="dry run for repair")
  parser.add_option("-p", "--profile", default=False, action="store_true",
        dest="profile", help="profiling execution time")

  (opts, args) = parser.parse_args()

  if opts.profile:
    runopts.set_profile()

  if len(args) != 2:
    parser.print_usage()
    exit(1)

  osloader1 = osloader.OsLoader(args[0])

  # Ensure that all syscalls have been loaded, hence connected
  osloader1.all()
  nodes1 = mgrapi.RegisteredObject.all()

  # Only load second executions's objects after we've safely stowed the
  # relevant part of the first's graph
  osloader2 = osloader.OsLoader(args[1])


  d = {}
  filenodes = [node for node in nodes1 if isinstance(node, fsmgr.FileDataNode)]
  for node in filenodes:
    try:
      chkpt = min(node.checkpoints)
    except AttributeError:
      dbg.error('skipping rollback due to lack of checkpoints')
      continue
    else:
      assert chkpt
      d[node] = chkpt

  runopts.set_dryrun(opts.dryrun)
  ctrl.repair2(d)

def print_tics_tacs():
  for o in mgrapi.RegisteredObject.all():
    try: print ('object %s\n    tic %s tac %s' % (o, o.tic, o.tac))
    except AttributeError: print 'no tic or tac'

if __name__ == "__main__":
  main()
