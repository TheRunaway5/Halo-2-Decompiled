#!/usr/bin/env python3
"""Replay object-navigation semantic controls using an existing owned build.

Run via run_test.py. Read the supplied CMake build without changing it; all
compiled source copies, objects, executables and expanded fixtures stay in
HALO2_TEST_SCRATCH. Build with CMAKE_EXPORT_COMPILE_COMMANDS=ON first.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

ROOT=Path(__file__).resolve().parents[1]
TARGET='native_campaign_ai_area_perform_tu15_tests'
RELATIVE='src/campaign/ai/native_campaign_ai_object_navigation_tu15.cpp'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    build=args.build.resolve(strict=True);source=ROOT/RELATIVE
    text=source.read_text();source_hash=sha(source)
    fixture=ROOT/'tests/fixtures/native_campaign_ai_object_navigation_tu15.json.gz'
    fixture_hash=sha(fixture);expanded=scratch/'reference.json'
    expanded.write_bytes(gzip.decompress(fixture.read_bytes()))
    commands=json.loads((build/'compile_commands.json').read_text())
    unit=next(row for row in commands if Path(row['file'])==source and f'{TARGET}.dir' in row['command'])
    compile_command=shlex.split(unit['command'])
    original_object=compile_command[compile_command.index('-o')+1]
    link=shlex.split((build/f'CMakeFiles/{TARGET}.dir/link.txt').read_text())
    assert original_object in link
    report=dict(passed=False,scratch=str(scratch),input_build=str(build),
        source_sha256=source_hash,fixture_sha256=fixture_hash,controls=[],retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    def save():args.report.write_text(json.dumps(report,indent=2)+'\n')
    controls=(
        ('control',None,None),
        ('unit_inverse_bit','allowed=(~p.r8(abi.gp[0]+0x348u))&1u;',
            'allowed=p.r8(abi.gp[0]+0x348u)&1u;'),
        ('device_bit_position','p.r8(abi.gp[0]+0x12cu)>>7u','p.r8(abi.gp[0]+0x12cu)>>6u'),
        ('parent_root','} while (abi.gp[0]!=none);','} while (false);'),
        ('optional_index_store','if (abi.gp[0]) p.w32(0x275e1bu,abi.gp[0],abi.gp[7]);',
            'if (false) p.w32(0x275e1bu,abi.gp[0],abi.gp[7]);'),
        ('biped_dead_state','if(state==2u)','if(state==3u)'),
        ('biped_update_tick','std::bit_cast<i32>(abi.gp[1])>std::bit_cast<i32>(p.r32(body+0x350u))',
            'std::bit_cast<i32>(abi.gp[1])>=std::bit_cast<i32>(p.r32(body+0x350u))'),
        ('biped_dynamic_flag','p.r8(abi.gp[1]+abi.gp[2]*8u+4u)&1u',
            'p.r8(abi.gp[1]+abi.gp[2]*8u+4u)&2u'),
        ('biped_reset_tick','{0xe22f6u,0x350u}','{0xe22f6u,0x354u}'),
        ('biped_point_y','p.w32(0xe2494u,abi.gp[6]+4u,abi.gp[1]);',
            'p.w32(0xe2494u,abi.gp[6]+4u,abi.gp[2]);'),
        ('biped_world_point_saved_word','abi.gp[4],abi.gp[7],fp,p.observe)',
            'abi.gp[4],0u,fp,p.observe)'))
    try:
        save()
        for name,before,after in controls:
            if before is not None:assert text.count(before)==1,name
            cpp=scratch/(name+'.cpp');obj=scratch/(name+'.o');exe=scratch/name
            cpp.write_text(text if before is None else text.replace(before,after,1))
            compile_args=[str(cpp) if value==str(source) else value for value in compile_command]
            compile_args[compile_args.index('-o')+1]=str(obj)
            if '-MF' in compile_args:
                compile_args[compile_args.index('-MF')+1]=str(scratch/(name+'.d'))
            if '-MT' in compile_args:
                compile_args[compile_args.index('-MT')+1]=str(obj)
            assert not any(value.startswith('-Wp,-M') for value in compile_args)
            compile_args+=['-I',str(source.parent)]
            link_args=[str(obj) if value==original_object else value for value in link]
            link_args[link_args.index('-o')+1]=str(exe)
            for command in (compile_args,link_args):
                result=subprocess.run(command,cwd=build,text=True,capture_output=True,timeout=180)
                if result.returncode:raise RuntimeError(name+' compilation failed: '+result.stderr[-4000:])
            result=subprocess.run([str(exe),str(expanded)],cwd=ROOT,text=True,capture_output=True,timeout=60)
            # A semantic assertion exits1; a signal or timeout is not a useful
            # rejection witness for these four defined arithmetic mutations.
            passed=result.returncode==(0 if before is None else 1)
            report['controls'].append(dict(name=name,passed=passed,exit_code=result.returncode,
                source_sha256=sha(cpp),diagnostic=(result.stdout+result.stderr)[-2000:]))
            save();print(name,'passed' if passed else 'FAILED',flush=True)
            for path in (cpp,obj,exe):path.unlink()
            if not passed:raise RuntimeError(name+' semantic control failed')
        report['source_unchanged']=sha(source)==source_hash
        report['fixture_unchanged']=sha(fixture)==fixture_hash
        report['passed']=report['source_unchanged'] and report['fixture_unchanged']
        return 0 if report['passed'] else 1
    finally:save()

if __name__=='__main__':raise SystemExit(main())
