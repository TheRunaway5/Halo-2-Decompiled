#!/usr/bin/env python3
"""Original weapon cycles with an authored player representation and biped.

The matg representation row selects the actual bipd definition; player+88 is
the explicit representation input. Constructor/input/update/postpasses and
representation-based graph lookup execute unchanged. Inherited raw equipped
storage, animation/world services and failed allocations remain declared;
this is not a complete biped/world or rendered-species execution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
from oracle_smg_firing_1_1 import Cache,ROOT,Trace,UNIT,PLAYER_DATA


def main():
    parser=argparse.ArgumentParser(description=__doc__,add_help=False)
    parser.add_argument('--kind',choices=('reload','energy'),required=True)
    parser.add_argument('--representation',type=int,choices=(2,3),required=True)
    parser.add_argument('--output',type=Path,required=True)
    args,remaining=parser.parse_known_args()
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    globals_=next(tag for tag in cache.tags.values() if tag['group']=='matg')
    rows=cache.block(globals_['offset']+0x138,0xbc)
    row=rows[args.representation]
    unit=cache.tags[cache.u(row+0xb4)]
    assert unit['group']=='bipd'
    representation_hex=bytes(cache.data[row:row+0xbc]).hex()
    cache.data.close();cache.stream.close()
    if args.kind=='reload':import oracle_weapon_reload_cycles_1_1 as parent
    else:import oracle_weapon_energy_cycles_1_1 as parent
    class RepresentedTrace(Trace):
        def __init__(self,*positional,**keywords):
            super().__init__(*positional,**keywords)
            self.w(UNIT,int(unit['datum'],16))
            self.g.mem_write(PLAYER_DATA+0x88,bytes([args.representation]))
            self.authored['represented_unit']=unit
            self.authored['representation_selector']=args.representation
    parent.Trace=RepresentedTrace
    parent.__doc__=__doc__+f'\nCycle family: {args.kind}; representation: {args.representation}.\n'
    sys.argv=[sys.argv[0],'--output',str(args.output),*remaining]
    parent.main()
    path=args.output/'report.json';report=json.loads(path.read_text())
    report.update(representation=args.representation,unit=unit,representation_row_hex=representation_hex,
        generator_tool_sha256=report['tool_sha256'],
        represented_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')


if __name__=='__main__':main()
