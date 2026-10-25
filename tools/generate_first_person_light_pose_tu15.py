#!/usr/bin/env python3
"""Compile complete TU1.5 first-person light-pose owners, without call substitution."""
import argparse,json,os,shutil
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--directory',type=Path)
    parser.add_argument('--report-output',type=Path)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.directory is None:args.directory=scratch/'first-person-light-pose-generation'
    assert args.directory.resolve().is_relative_to(scratch)
    args.directory.mkdir(parents=True,exist_ok=False)
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries=(0x31b30,0x3d2b0)
    sites={};instructions={};addresses={};routines=[]
    for entry in entries:
        body=_trace(code,entry,set(entries));assert body.code and not body.tables
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            ins=code.at(pc);assert image.read(pc,ins.size)==bytes(ins.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            for op in ins.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in entries},body_sites=sites,instruction_map=instructions,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    correspondence=args.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=args.directory/'generation.json'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired)
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_first_person_light_pose_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_first_person_light_pose_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_first_person_light_pose_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_first_person_light_pose_tu15_owns_entry')
    args.output.write_text(source)
    report=json.loads(manifest.read_text());report.update(generated_cpp_sha256=sha(args.output),
        module_generator_sha256=sha(Path(__file__)),complete_instructions=len(instructions),
        source_calls_or_child_results_changed=False,
        scope='Complete3D2B0 first-person cache and31B30 up/forward orthogonalization; downstream original owners remain required.')
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    if args.report_output:
        args.report_output.mkdir(parents=True,exist_ok=False)
        for name in ('bodies.json','generation.json'):
            path=args.directory/name;assert path.stat().st_size<256*1024
            shutil.copyfile(path,args.report_output/name)
    print(json.dumps(dict(complete_instructions=len(instructions),generated_sha256=sha(args.output))),flush=True)
if __name__=='__main__':main()
