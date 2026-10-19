#!/usr/bin/env python3
"""Compile complete authenticated TU1.5 shared CTF/Assault configuration owners.

Existing native marker queries, object factories, dirty/entity/codec owners and
already-owned registry operations are mandatory external native callees. No
binary/opcode interpreter, replayed program or successful service result is
emitted. The generated initializer preserves the original full caller ABI while
the independently verified typed bank API remains a separate supported view.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES=(0x24b390,0x249440,0x24a6c0)


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=ROOT/'src/source_ctf_configuration_tu15_generated.cpp')
    p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}|set(ENTRIES)
    a.directory.mkdir(parents=True,exist_ok=True)
    sites={};instructions={};addresses={};routines=[];listings=[]
    for entry in ENTRIES:
        body=_trace(code,entry,starts)
        assert body.code and all(width==4 for _,width,_ in body.tables),(hex(entry),body.tables)
        rows=[];raw=bytearray()
        for pc in sorted(body.code):
            i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
            rows.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
            raw.extend(pc.to_bytes(4,'little')+bytes(i.bytes))
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for o in i.operands:
                if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
                elif o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
        listing=a.directory/f'{entry:08x}.asm';listing.write_text(''.join(rows))
        listings.append(dict(entry=f'{entry:08x}',file=listing.name,sha256=sha(listing),bytes=listing.stat().st_size,
            instructions=len(rows),instruction_bytes_sha256=hashlib.sha256(raw).hexdigest(),
            direct_callees=[f'{q:08x}'for q in sorted(body.calls)],tail_targets=[f'{q:08x}'for q in sorted(body.tail_jumps)]))
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=q,count=n)for q,_,n in body.tables],byte_tables=[]))
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,
        instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=routines)
    correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,complete_listings=True,code_override=code,
        runtime_header='halo2/source_ctf_configuration_tu15.hpp',runtime_function='source_run_ctf_configuration_tu15')
    source=a.output.read_text().replace('source_physics_live_support_tu15_owns_entry','source_ctf_configuration_tu15_owns_entry').replace(
        'source_run_physics_live_support_tu15','source_run_ctf_configuration_tu15')
    a.output.write_text(source)
    report=json.loads(manifest.read_text())
    report.update(ctf_configuration_generator_sha256=sha(__file__),generated_cpp_sha256=sha(a.output),
        instruction_listings=listings,
        existing_owner_reuse=dict(retained_team_rotation='00164930'),
        unqualified_mode_paths='Only whole slot4 configuration predicate, slot0 source-kind getter and slot26 team selection. Real164930 remains an external native owner; no callbacks, bank results or mode are supplied.',
        typed_initializer_files_changed=False,gameplay_admission_changed=False,
        scope='Complete individually compiled original owners, not yet a composed native MCC object-birth or full game-mode acceptance.')
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions),generated_cpp=str(a.output),
        generated_cpp_sha256=sha(a.output),manifest_sha256=sha(manifest))),flush=True)


if __name__=='__main__':main()
