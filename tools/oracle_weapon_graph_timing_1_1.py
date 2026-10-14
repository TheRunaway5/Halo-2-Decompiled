#!/usr/bin/env python3
"""Unchanged original representation/dual graph resolution and timing consumer.

Actual Lockout weapon/player metadata, raw object/player/time ABI storage and
literal actions/sections are inputs. The complete169F3B/16A06A and their graph
callees execute without supplied results or instruction patches. This isolated
query does not execute a weapon/world update or establish rendered poses.
The no-model control changes only the selected authored record's model datum.
"""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import Trace, Cache, ROOT, WEAPON, UNIT, WI, PLAYER_DATA
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX

ACTIONS=(0x0400000c,0x06000006,0x05000024,0x0c000064,0x0b000065,0x0d000021,
  0x0e000607,0x0e000608,0x0e000609,0x0e00060a,0x0c000073,0x0c000075,
  0x0c000077,0x11000074,0x11000076,0x0a0005bb,0x0b0005b2,0x130005bc,
  0x140005b3,0x0500000a,0x0b00006d,0x0a000066,0x08000071,0x0c0005ae,
  0x150005af,0x140005b0,0x0b0005b1,0x08000025)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    cache=Cache(ROOT/'assets/disc/maps/lockout.map');trace=Trace(1)
    reached={}
    trace.g.hook_add(4,lambda g,address,size,data:reached.setdefault(address,bytes(g.mem_read(address,size)).hex()))
    lines=['# datum representation dual no_model activity section timing16'];counts={};weapons=[]
    for tag in cache.tags.values():
        if tag['group']!='weap':continue
        trace.w(WEAPON,int(tag['datum'],16));weapons.append(tag)
        records=cache.block(tag['offset']+0x2a8,16)
        for representation in (2,3):
            trace.g.mem_write(PLAYER_DATA+0x88,bytes([representation]))
            for dual in (0,1):
                trace.g.mem_write(UNIT+0x212,bytes([0,1 if dual else 255]))
                for no_model in (0,1):
                    preferred=1 if representation==3 else 0
                    record=records[preferred if len(records)>preferred else 0] if records else None
                    if no_model and record is not None:trace.w(record+cache.magic+4,0xffffffff)
                    for action in ACTIONS:
                        for section in (0,1,2,3):
                            trace.events.clear();trace.entry_counts.clear()
                            trace.run(0x16a06a,(action,section),((UC_X86_REG_ECX,WI),))
                            result=trace.g.reg_read(UC_X86_REG_EAX)&65535
                            lines.append(' '.join(f'{v:08x}' for v in (int(tag['datum'],16),representation,dual,no_model,action,section,result)))
                            counts['cases']=counts.get('cases',0)+1
                            counts['nonzero']=counts.get('nonzero',0)+int(result!=0)
                    if no_model and record is not None:trace.w(record+cache.magic+4,cache.u(record+4))
        print(tag['name'],flush=True)
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
    report=dict(scope=__doc__,counts=counts,weapons=weapons,actions=[f'{v:08x}' for v in ACTIONS],
      xbe_sha256=hashlib.sha256(trace.image.data).hexdigest(),cache_sha256=hashlib.sha256(cache.data).hexdigest(),
      tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
      inherited_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),
      fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),instruction_patches=[],supplied_results=[],
      reached_instruction_bytes={f'{a:08x}':raw for a,raw in sorted(reached.items())})
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(counts),flush=True)


if __name__=='__main__':main()
