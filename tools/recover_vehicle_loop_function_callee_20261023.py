#!/usr/bin/env python3
"""Recover the complete seven-instruction vehicle bit query reached by F9270.

Only two original call edges change. All other existing native instruction
bodies remain byte-identical, including the qualified namespace adaptation.
"""
import hashlib,json,os,re
from pathlib import Path
from types import SimpleNamespace
from generate_physics_live_support_tu15 import generate
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/combat-presentation-audit-20261022/repair-object-function-symbols'
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=OUT.parent/'repair-followup-20261023';pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    image=Xbe(NEW_DEFAULT);assert sha(image.data)==NEW_PIN;code=_Code(image,'.text');entry=0xfa5d0
    body=_trace(code,entry,{entry});assert len(body.code)==7 and not body.tables
    sites={};instructions={};addresses={}
    for pc in body.code:
        i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
        instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
        for op in i.operands:
            if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
            elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{entry:08x}':entry},
        body_sites={f'{entry:08x}':sorted(body.code)},instruction_map=instructions,
        address_map={f'{pc:08x}':v for pc,v in addresses.items()},routines=[dict(entry=entry,tables=[],byte_tables=[])])
    directory=owner/'vehicle-loop-bit-query';directory.mkdir(exist_ok=False)
    correspondence=directory/'body.json';correspondence.write_text(json.dumps(paired))
    output=directory/'generated.cpp'
    generate(SimpleNamespace(output=output,manifest=directory/'manifest.json',correspondence=correspondence,
        additional_entries=ROOT/'tools/generate_physics_live_support_tu15.py'),paired)
    pattern=re.compile(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{.*?^\}',re.M|re.S)
    recovered={int(m[1],16):m[0] for m in pattern.finditer(output.read_text())};assert set(recovered)=={entry}
    rel='src/physics/physics_live_support_tu15_generated.cpp';path=ROOT/rel;before=path.read_text()
    assert before==(source/rel).read_text(),'Physics source drift must be reconciled before recovery'
    text=before
    old_bodies={int(m[1],16):m[0] for m in pattern.finditer(before)}
    declaration='static void run_f9270(WorldState &);';assert text.count(declaration)==1
    text=text.replace(declaration,'static void run_fa5d0(WorldState &);\n'+declaration)
    start='static void run_f9270(WorldState &s) {';assert text.count(start)==1
    text=text.replace(start,recovered[entry]+'\n\n'+start)
    for pc in (0xf9621,0xf963c):
        i=code.at(pc);assert i.mnemonic=='call' and i.operands[0].imm==entry
        old=f'{{const auto target=0xfa5d0u;s.push(0x{pc:x}u,0x{pc+5:x}u);dispatch(s,target,0x{pc:x}u,false);}}'
        assert text.count(old)==1;text=text.replace(old,f's.push(0x{pc:x}u,0x{pc+5:x}u);run_fa5d0(s);')
    case='case 0xf9270u:run_f9270(s);break;';assert text.count(case)==1
    text=text.replace(case,case+'\ncase 0xfa5d0u:run_fa5d0(s);break;')
    owns='case 0xf9270u:case ';assert text.count(owns)==1
    text=text.replace(owns,'case 0xfa5d0u:'+owns)
    new_bodies={int(m[1],16):m[0] for m in pattern.finditer(text)}
    assert set(new_bodies)==set(old_bodies)|{entry}
    assert [pc for pc in old_bodies if old_bodies[pc]!=new_bodies[pc]]==[0xf9270]
    (directory/'before.cpp').write_text(before)
    assert path.read_text()==before;path.write_text(text);(source/rel).write_text(text)
    updated=source_hashes(source);assert {p for p in pins if pins[p]!=updated[p]}=={rel}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    receipt=dict(scope=__doc__,path=rel,before_sha256=sha(before.encode()),after_sha256=sha(text.encode()),
        original_xbe_sha256=NEW_PIN,original_instructions={pc:code.at(int(pc,16)).bytes.hex() for pc in instructions},
        existing_bodies=len(old_bodies),new_entry=entry,changed_original_calls=[0xf9621,0xf963c],
        other_existing_bodies_identical=True,source_results_supplied=False,runtime_installed=False)
    (OUT/'vehicle-loop-callee-recovery.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()
