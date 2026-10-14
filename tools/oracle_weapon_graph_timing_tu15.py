#!/usr/bin/env python3
"""Execute the complete TU1.5 graph/timing query against frozen TU1.1 inputs.

Raw object/player/time storage and actual Lockout metadata are inputs. The four
root globals are the literal TU1.5 object/tag/player/time ABI pointers. Neither
graph resolution nor its timing/event/CRT callees supply service results or
patch instructions. This is an isolated query, not an original rendered world.
"""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import (Trace,Cache,ROOT,POOL,TAGS,PLAYERS,TIME,WI,
    WEAPON,UNIT,PLAYER_DATA)
from weapon_reference_versions import images
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX


class Original(Trace):
    def __init__(self):
        self.reached={}
        super().__init__(1)
        self.image=next(image for label,image,_ in images() if label=='TU1.5')
        for section in self.image.sections:self.g.mem_write(section.va,self.image.section_bytes(section))
        self.globals={0x4ecbd0:POOL,0x4f0414:TAGS,0x4f54f4:PLAYERS,0x51df54:TIME}
        for address,value in self.globals.items():self.w(address,value)

    def hook(self,g,address,size,user):
        self.recent.append(address)
        if address not in self.reached:self.reached[address]=bytes(g.mem_read(address,size)).hex()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();trace=Original();cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    count=nonzero=0
    for line in args.reference.read_text().splitlines():
        if not line or line[0]=='#':continue
        datum,representation,dual,no_model,action,section,expected=(int(v,16) for v in line.split())
        tag=cache.tags[datum];records=cache.block(tag['offset']+0x2a8,16)
        preferred=1 if representation==3 else 0
        record=records[preferred if len(records)>preferred else 0] if records else None
        trace.w(WEAPON,datum);trace.g.mem_write(PLAYER_DATA+0x88,bytes([representation]))
        trace.g.mem_write(UNIT+0x212,bytes([0,1 if dual else 255]))
        if record is not None:trace.w(record+cache.magic+4,0xffffffff if no_model else cache.u(record+4))
        trace.run(0x16b44a,(action,section),((UC_X86_REG_ECX,WI),))
        actual=trace.g.reg_read(UC_X86_REG_EAX)&65535
        assert actual==expected,(tag['name'],representation,dual,no_model,hex(action),section,expected,actual)
        count+=1;nonzero+=actual!=0
    assert (count,nonzero) in ((28512,4720),(29568,5344),(38016,5776))
    for address,raw in trace.reached.items():assert trace.image.read(address,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
    result=dict(scope=__doc__,cases=count,nonzero=nonzero,instruction_patches=[],supplied_results=[],
        globals={f'{a:08x}':f'{v:08x}' for a,v in trace.globals.items()},
        xbe_sha256=hashlib.sha256(trace.image.data).hexdigest(),cache_sha256=hashlib.sha256(cache.data).hexdigest(),
        fixture_sha256=hashlib.sha256(args.reference.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inherited_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),
        reached_instruction_bytes={f'{a:08x}':raw for a,raw in sorted(trace.reached.items())})
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=count,nonzero=nonzero,original_instruction_rows=len(trace.reached))),flush=True)


if __name__=='__main__':main()
