"""Lower the complete original TU1.5 object-name lookup into native operations.

No image instructions execute in the generated engine. Generate only into
HALO2_TEST_SCRATCH; original/native and World consumers must qualify before
publishing the source or routing its owner.
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
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--directory',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(scratch) and args.directory.resolve().is_relative_to(scratch)
    assert not args.output.exists() and not args.directory.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    assert image.read(0xbd790,28).hex()=='6685c07c13663d80027d0d8b0da0ab4e000fbfc08b0481c383c8ffc3'
    # The existing complete212560 cast owner reaches this routine directly.
    assert image.read(0x21258f,5).hex()=='e9fcb1eaff'
    code=_Code(image,'.text');body=complete_body(code,0xbd790)
    assert len(body.code)==10 and not body.tables and not getattr(body,'byte_tables',[])
    addresses=set();instructions={}
    for pc in sorted(body.code):
        item=code.at(pc);assert bytes(item.bytes)==image.read(pc,item.size)
        assert item.mnemonic!='call'
        instructions[f'{pc:08x}']=pc;addresses.update((pc,pc+item.size))
        for operand in item.operands:
            if operand.type==2:addresses.add(operand.imm&0xffffffff)
            elif operand.type==3:addresses.add(operand.mem.disp&0xffffffff)
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={'000bd790':0xbd790},
        body_sites={'000bd790':sorted(body.code)},instruction_map=instructions,
        address_map={f'{a:08x}':a for a in sorted(addresses)},
        routines=[dict(entry=0xbd790,tables=[],byte_tables=[])])
    args.directory.mkdir(parents=True);correspondence=args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    manifest=args.directory/'generation.json'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,code_override=code)
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_source_object_name_lookup_tu15.py').replace(
        'halo2/physics_live_support_tu15.hpp','halo2/source_object_name_lookup_tu15.hpp').replace(
        'source_run_physics_live_support_tu15','source_run_object_name_lookup_tu15').replace(
        'source_physics_live_support_tu15_owns_entry','source_object_name_lookup_tu15_owns_entry')
    args.output.write_text(source)
    report=json.loads(manifest.read_text())
    report.update(module_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
        original_lookup_bytes=image.read(0xbd790,28).hex(),
        authenticated_parent_tail=dict(site='0x21258f',target='0xbd790',bytes='e9fcb1eaff'),
        engine_consumers_qualified=False,runtime_enabled=False)
    manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(entries=1,instructions=10,bytes=args.output.stat().st_size)))


if __name__=='__main__':main()
