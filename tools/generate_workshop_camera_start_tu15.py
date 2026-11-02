"""Lower the complete unchanged original nonrelative camera HS wrapper.

Generated native operations belong to owned scratch until qualified. This
does not route the owner, admit an MCC declaration or supply camera results.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body, generate


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(owner)and a.directory.resolve().is_relative_to(owner)
    assert not a.output.exists()and not a.directory.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    assert image.read(0x458e90,18).hex()=='04000000b0272b0000000000020024000b00'
    code=_Code(image,'.text');body=complete_body(code,0x2b27b0)
    assert len(body.code)==26 and not body.tables and not getattr(body,'byte_tables',[])
    calls={pc:code.at(pc).operands[0].imm for pc in body.code if code.at(pc).mnemonic=='call'}
    assert calls=={0x2b27d0:0x212010,0x2b27e3:0x16edd0,0x2b27ec:0x211da0}
    instructions={};addresses=set()
    for pc in sorted(body.code):
        row=code.at(pc);assert bytes(row.bytes)==image.read(pc,row.size)
        instructions[f'{pc:08x}']=pc;addresses.update((pc,pc+row.size))
        for operand in row.operands:
            if operand.type==2:addresses.add(operand.imm&0xffffffff)
            elif operand.type==3:addresses.add(operand.mem.disp&0xffffffff)
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={'002b27b0':0x2b27b0},
        body_sites={'002b27b0':sorted(body.code)},instruction_map=instructions,
        address_map={f'{v:08x}':v for v in sorted(addresses)},
        routines=[dict(entry=0x2b27b0,tables=[],byte_tables=[])])
    a.directory.mkdir(parents=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    manifest=a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,code_override=code)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_workshop_camera_start_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_workshop_camera_start_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_workshop_camera_start_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_workshop_camera_start_tu15_owns_entry')
    a.output.write_text(source);report=json.loads(manifest.read_text())
    report.update(module_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest(),
        original_bytes=image.read(0x2b27b0,0x46).hex(),descriptor=image.read(0x458e90,18).hex(),
        required_owners=calls,engine_consumers_qualified=False,runtime_enabled=False)
    manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(entries=1,sites=26,bytes=a.output.stat().st_size)))


if __name__=='__main__':main()
