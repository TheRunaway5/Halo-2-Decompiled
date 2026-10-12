#!/usr/bin/env python3
"""Authenticate complete solver, initialization and pointer-advance bodies."""
import argparse, hashlib, json
from pathlib import Path
import research_mp_sandbox_update_functions as base
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    parent=ROOT/'analysis/multiplayer/sandbox-source/physics-solver-motion-20261011/correspondence-verified/complete-report.json'
    assert sha(parent)=='aa3b9dad059a64a9ba52eac5e8807572da1d1c8db7b0f255c17a18e7a59f1c0f'
    r=json.loads(parent.read_text());mapping={int(k,16):v for k,v in r['entries'].items()}
    old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path)==PIN and sha(base.NEW_DEFAULT)==base.NEW_PIN
    old,new=Xbe(old_path),Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
    # Tiny pointer-advance routines are identified by complete paired caller
    # edges, not the prefix searcher's minimum candidate length.
    for entry,callsite in ((0x322ad0,0x322d6f),(0x322ab0,0x324b5e),(0x322ac0,0x324aea)):
        oi=oc.at(callsite);ni=nc.at(r['instruction_map'][f'{callsite:08x}'])
        assert oi.mnemonic==ni.mnemonic=='call' and oi.operands[0].imm==entry
        mapping[entry]=ni.operands[0].imm
    rows=[];instruction_map={};addresses={};directory=a.output/'instructions';directory.mkdir()
    for entry,target in sorted(mapping.items()):
        oi=[oc.at(p) for p in sorted(_trace(oc,entry,set(mapping)).code)]
        ni=[nc.at(p) for p in sorted(_trace(nc,target,set(mapping.values())).code)]
        assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(entry),hex(target))
        row=dict(disc_1_1_entry=f'{entry:08x}',later_entry=f'{target:08x}',instructions=len(oi),complete_paired_body=True)
        for x,y in zip(oi,ni):
            instruction_map[f'{x.address:08x}']=y.address
            for xo,yo in zip(x.operands,y.operands):
                pairs=[]
                if xo.type==yo.type==3:pairs.append((xo.mem.disp,yo.mem.disp))
                elif xo.type==yo.type==2:pairs.append((xo.imm,yo.imm))
                for p,q in pairs:
                    if base.containing(old,p) and base.containing(new,q):
                        key=f'{p:08x}';assert key not in addresses or addresses[key]==q
                        addresses[key]=q
        for label,ins in (('disc',oi),('later',ni)):
            p=directory/f'{entry:08x}-{label}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            row[label+'_listing']=dict(file=p.name,sha256=sha(p))
        rows.append(row)
    r.update(extension_tool_sha256=sha(Path(__file__)),parent_correspondence_sha256=sha(parent),
        complete_bodies=len(mapping),routines=rows,entries={f'{p:08x}':q for p,q in mapping.items()},
        instruction_map=instruction_map,address_map=addresses,
        solver_entries=[f'{x:08x}' for x in (0x322ed0,0x322d60,0x322ad0,0x322ab0,0x322ac0)])
    p=a.output/'complete-report.json';p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(complete_bodies=len(mapping),solver_correspondence_sha256=sha(p))))
if __name__=='__main__':main()
