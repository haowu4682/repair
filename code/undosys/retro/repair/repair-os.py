#!/usr/bin/python

import optparse
import atexit

import ctrl
import mgrapi
import attacker
import dbg
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
  parser.add_option("-a", "--attk"  , dest="attack",
        metavar="NAME"  , default="pick_attack_execve",
        help="algorithm to pick attacker's node")
  parser.add_option("-p", "--profile", default=False, action="store_true",
        dest="profile", help="profiling execution time")

  # hints
  parser.add_option("-i", "--inode" , dest="inode_hint",
        metavar="FILE"  , default=None,
        help="attacker's binary")

  (opts, args) = parser.parse_args()

  if opts.profile:
    runopts.set_profile()

  if len(args) != 1 or not opts.inode_hint:
    parser.print_usage()
    exit(1)

  # pick attack
  attack = attacker.find_attack_node(args[0], opts.attack, opts.inode_hint)

  osloader.set_logdir(args[0])
  attack_node = mgrapi.RegisteredObject.by_name_load(attack)
  print_tics_tacs()
  if attack_node is None:
    raise Exception('missing attack node', attack)

  chkpt = max(c for c in attack_node.checkpoints if c < min(attack_node.actions))
  assert chkpt

  assert len(attack_node.actions) == 1
  #code.interact(local=locals())
  #pdb.set_trace()

  # cancel exec syscall actor
  for a in attack_node.actions:
    dbg.info("cancel: %s" % a.argsnode)
    a.argsnode.data = None
    a.cancel = True

  dbg.info("pick:", chkpt)

  runopts.set_dryrun(opts.dryrun)

  ctrl.repair(attack_node, chkpt)

def print_tics_tacs():
  for o in mgrapi.RegisteredObject.all():
    try: print ('object %s\n    tic %s tac %s' % (o, o.tic, o.tac))
    except AttributeError: print 'no tic or tac'

if __name__ == "__main__":
  main()
