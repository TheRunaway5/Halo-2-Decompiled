#!/usr/bin/env python3
"""Authenticate the complete solver and real constraint production call chain."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
import research_mp_sandbox_update_functions as base
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    parent=ROOT/'analysis/multiplayer/sandbox-source/physics-solver-20261011/correspondence-verified/complete-report.json'
    jac=ROOT/'analysis/multiplayer/sandbox-source/physics-solver-20261011/jacobian-correspondence/complete-report.json'
    assert sha(parent)=='b6793a21f1780bee254eabafd229d39810f26e715369ba22eba16aeb2c98809d'
    assert sha(jac)=='3142cf933d178165e5b29978076974eb8861d5eb954b13de214f2aef1c298277'
    r=json.loads(parent.read_text());j=json.loads(jac.read_text())
    assert j['extension_tool_sha256']==sha(ROOT/'tools/research_physics_jacobian_20261011.py')
    mapping={int(k,16):v for k,v in (r['entries']|j['entries']).items()}
    oldp=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(oldp)==PIN and sha(base.NEW_DEFAULT)==base.NEW_PIN
    old,new=Xbe(oldp),Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
    for target,site in ((0x31b810,0x31bd33),(0x31b880,0x31bdf9),(0x31b9d0,0x31bc2e),(0x1d2d70,0x31bc01)):
        oi=oc.at(site);ni=nc.at(j['instruction_map'][f'{site:08x}'])
        assert oi.mnemonic==ni.mnemonic and oi.operands[0].imm==target
        mapping[target]=ni.operands[0].imm
    assert len(mapping)==36
    directory=a.output/'instructions';directory.mkdir();rows=[];addresses={};sites={}
    for entry,target in sorted(mapping.items()):
        oi=[oc.at(p) for p in sorted(_trace(oc,entry,set(mapping)).code)]
        ni=[nc.at(p) for p in sorted(_trace(nc,target,set(mapping.values())).code)]
        assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(entry),hex(target))
        for x,y in zip(oi,ni):
            sites[f'{x.address:08x}']=y.address
            for xo,yo in zip(x.operands,y.operands):
                pairs=[]
                if xo.type==yo.type==3:pairs.append((xo.mem.disp,yo.mem.disp))
                elif xo.type==yo.type==2:pairs.append((xo.imm,yo.imm))
                for p,q in pairs:
                    if base.containing(old,p) and base.containing(new,q):
                        key=f'{p:08x}';assert key not in addresses or addresses[key]==q
                        addresses[key]=q
            if x.mnemonic=='call' and x.operands[0].type==2 and x.operands[0].imm in mapping:
                assert mapping[x.operands[0].imm]==y.operands[0].imm
        row=dict(disc_1_1_entry=f'{entry:08x}',later_entry=f'{target:08x}',instructions=len(oi),complete_paired_body=True)
        for label,ins in (('disc',oi),('later',ni)):
            p=directory/f'{entry:08x}-{label}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            row[label+'_listing']=dict(file=p.name,sha256=sha(p))
        rows.append(row)
    r.update(extension_tool_sha256=sha(Path(__file__)),parent_correspondence_sha256=sha(parent),
        jacobian_correspondence_sha256=sha(jac),complete_bodies=len(mapping),routines=rows,
        entries={f'{p:08x}':q for p,q in mapping.items()},instruction_map=sites,address_map=addresses,
        original_virtual_slot_evidence=j['virtual_slot_evidence'])
    p=a.output/'complete-report.json';p.write_text(json.dumps(r,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(complete_bodies=len(mapping),report_sha256=sha(p))))
if __name__=='__main__':main()
