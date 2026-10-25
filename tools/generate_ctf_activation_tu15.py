#!/usr/bin/env python3
"""Compile complete authenticated TU1.5 shared CTF/Assault activation owners.

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
from objective_native_tu15_injection import inject_native_objective_owners

ENTRIES=(
    0x249460,0x24ce00,0x24cef0, # Whole initializer and authored marker/region owners.
    0x249590,0x24b5c0,0x24b500,0x24cd70, # Slot12 activation, reset and real factory caller.
    0x164930,0x15b3b0, # Source team rotation and retained spawn-bank mapping.
    0x160a90,0x160da0,0x160e50,0x160f10, # Missing generic objective registry lifecycle.
    0x24a2e0,0x24cd10,0x24a2f0,0xb1a10, # Slot21/23 and match dirty wrapper.
    0x15afb0,0x15a560,0x15a500,0x15d750,0x160680,0x1f1a60,0x15b840,0x15f5e0,
    0xa6780, # Complete shared +C initializer callback selected by the original vtable.
    0x24a810, # Complete shared +40 actual player/equipment/flag admission callback.
    0x106350, # Complete12-site classifier over the real weapon's admitted tag.
    0x1a87c0, # Complete11-site event-request constructor reached by flag pickup.
    0x160d40, # Complete26-site actual carrier-player registry refresh.
    0x24a4b0, # Complete86-site selected CTF actual objective-drop notification.
    0x24a070,0x24dae0,0x24b6e0, # Complete source tick and real objective-bank validation producers.
    0x24b4c0,0x249b20,0x24d5f0,0x165610,0x249a40,0x24b950,0x24c490,
    # Complete home-marker/proximity/timer/return/score-state/overtime owners.
)
SPAWN_ENTRIES=(0x249600,0x24a6f0)


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path)
    p.add_argument('--directory',type=Path,required=True)
    p.add_argument('--spawn',action='store_true',help='Regenerate the separately owned player-spawn and spawn-filter callbacks')
    a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    entries=SPAWN_ENTRIES if a.spawn else ENTRIES
    suffix='ctf_activation_spawn_tu15' if a.spawn else 'ctf_activation_tu15'
    if a.output is None:a.output=ROOT/f'src/gameplay/multiplayer/source_{suffix}_generated.cpp'
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}|set(entries)
    a.directory.mkdir(parents=True,exist_ok=True)
    sites={};instructions={};addresses={};routines=[];listings=[]
    for entry in entries:
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
        entries={f'{p:08x}':p for p in entries},body_sites=sites,
        instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=routines)
    correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,complete_listings=True,code_override=code,
        runtime_header=f'halo2/source_{suffix}.hpp',runtime_function=f'source_run_{suffix}')
    source=a.output.read_text().replace('source_physics_live_support_tu15_owns_entry',f'source_{suffix}_owns_entry').replace(
        'source_run_physics_live_support_tu15',f'source_run_{suffix}')
    a.output.write_text(inject_native_objective_owners(source))
    report=json.loads(manifest.read_text())
    report.update(ctf_activation_generator_sha256=sha(__file__),generated_cpp_sha256=sha(a.output),
        instruction_listings=listings,
        existing_owner_reuse=dict(marker_query='001a8850',object_request='000b9960',object_factory='000b9b70',
            object_publication='000b1a70',entity_dirty='000b7e50',registry_remove='00160b70',
            registry_acquire='00160c90',registry_release='00160ea0'),
        unqualified_mode_paths='Real raw4/5 entity bank callbacks and later-round15C840 remain required native owners; no state/result is supplied to skip them.',
        typed_initializer_files_changed=False,gameplay_admission_changed=False,
        scope='Complete individually compiled original owners, not yet a composed native MCC object-birth or full game-mode acceptance.')
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(bodies=len(entries),instructions=len(instructions),generated_cpp=str(a.output),
        generated_cpp_sha256=sha(a.output),manifest_sha256=sha(manifest))),flush=True)


if __name__=='__main__':main()
