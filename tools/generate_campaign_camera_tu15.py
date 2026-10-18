#!/usr/bin/env python3
"""Lower complete authenticated TU1.5 scripted-camera actions/consumers.

Includes every branch of each body. Untaken controller modes retain required
external owners; no camera, animation, resource, BSP leaf or PVS is supplied.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
ENTRIES=(0x3f440,0x11e720,0x144ff0,0x157010,0x157220,0x158340,0x158560,0x158900,0x16edd0,0x16f110,0x16f150,0x16f550,0x16f5a0,0x16f6a0,0x16fd40,0x16fe00,0x171a50,0x1720e0,0x1723d0,0x172cf0,0x172d70,0x173050,0x173250,0x173490,0x173a10,0x173a60,0x173bd0,0x173e30,0x174690,0x174b60)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists()or a.directory.exists():raise ValueError('Preserve generated evidence')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected original English TU1.5')
    x=Xbe(NEW_DEFAULT);c=_Code(x,'.text')
    calls={0x2b2745:0x16f150,0x2b26fe:0x158560,0x2b283a:0x16edd0,0x16fbfc:0x16fe00,0x13b6c0:0x16fd40}
    for pc,target in calls.items():
        i=c.at(pc)
        if i.mnemonic!='call'or i.operands[0].imm!=target:raise ValueError('Original call correspondence differs')
    sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        b=_trace(c,entry,set(ENTRIES))
        if not b.code or any(w not in (1,4)for _,w,_ in b.tables):raise ValueError('Original CFG unsupported')
        sites[f'{entry:08x}']=sorted(b.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n)for p,w,n in b.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=x.read(p,w*n).hex())for p,w,n in b.tables if w==1]))
        for pc in sorted(b.code):
            i=c.at(pc)
            if x.read(pc,i.size)!=bytes(i.bytes):raise ValueError('Original instruction differs')
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True);manifest=a.directory/'bodies.json'
    manifest.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=manifest,additional_entries=Path(__file__)),paired)
    s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_campaign_camera_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_campaign_camera_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_campaign_camera_tu15').replace('source_physics_live_support_tu15_owns_entry','source_campaign_camera_tu15_owns_entry')
    a.output.write_text(s);path=a.directory/'generation.json';r=json.loads(path.read_text())
    r.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        original_call_sites={f'{p:08x}':f'{q:08x}'for p,q in calls.items()},scope=__doc__)
    path.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
    print(len(ENTRIES),'complete original camera bodies;',len(instructions),'instructions')

if __name__=='__main__':main()
