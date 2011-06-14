#!/usr/bin/python

import util, ctrl, mgrapi, mgrutil, copy, optparse
import zoobarmgr

util.install_pdb()

def load_graph(logdir):
    zoobarmgr.set_logdir(logdir)
    zoobarmgr.load(None, None)

def pick_attack_nodes():
    ckpts = {}
    for o in mgrapi.RegisteredObject.all():
	if not isinstance(o, mgrutil.BufferNode) or \
	   o.name[-1] != 'htargs' or o.data is None or \
	   'hint=attack' not in o.data['env'].get('QUERY_STRING', ''):
            continue
        
        attack_req = mgrapi.RegisteredObject.by_name_load(o.name)
        nd = copy.deepcopy(attack_req.data)
        nd['env']['REQUEST_METHOD'] = 'GET'
        nd['env']['QUERY_STRING'] = ''
        nd['post'] = ''
        tr = min(attack_req.readers).tic
        tw = min(attack_req.writers).tic
        attack_data = mgrutil.BufferCheckpoint(o.name+('repair_cp',), min(tr, tw), nd)

        ckpts[attack_req] = attack_data

    return ckpts

if __name__ == "__main__":
    parser = optparse.OptionParser(usage="%prog [options] LOG-DIRECTORY")
    parser.add_option("-g", "--gui", default=False, action="store_true",
                      dest="gui", help="gui-based repair")

    (opts, args) = parser.parse_args()

    if len(args) != 1:
        parser.error("incorrect arguments")

    load_graph(args[0])
    ckpts = pick_attack_nodes()
    ctrl.repair2(ckpts, opts.gui)
