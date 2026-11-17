#!/usr/bin/env python3
"""Bounded semantic controls over an owned CMake refresh replay build.

The CMake build must itself belong to HALO2_TEST_SCRATCH. All replacement
objects, executables and expanded reference DATA remain in that lifetime.
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
SOURCE=ROOT/'src/campaign/ai/native_campaign_ai_biped_refresh_tu15.cpp'

def main():
    p=argparse.ArgumentParser();p.add_argument('--build',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();build=a.build.resolve();assert build.is_relative_to(scratch)
    work=scratch/'refresh-controls';work.mkdir(exist_ok=True)
    fixture=work/'fixture.json';fixture.write_bytes(gzip.decompress((ROOT/'tests/fixtures/native_campaign_ai_biped_refresh_tu15.json.gz').read_bytes()))
    commands=json.loads((build/'compile_commands.json').read_text())
    compile=next(c for c in commands if c['file']==str(SOURCE) and 'native_campaign_ai_biped_refresh_tu15_tests.dir/' in c['command'])
    original_object=compile['output'];link=shlex.split((build/'CMakeFiles/native_campaign_ai_biped_refresh_tu15_tests.dir/link.txt').read_text())
    text=SOURCE.read_text();variants={'control':text}
    for name,needle,replacement in (
        ('false_publication','0xe229du,sector,none','0xe229du,sector,0'),
        ('fall_state','if(p.r8(biped+0x3dcu)==5u)','if(p.r8(biped+0x3dcu)==6u)'),
        ('contact_bit','p.r16(biped+0x348u)>>12u','p.r16(biped+0x348u)>>11u'),
        ('actor_mode','==0x50000cbu','==0x50000ccu'),
        ('origin_bias','epsilon=p.r32(0x46a240u)','epsilon=0u'),
        ('hit_field','{0x94,0x70,0xe1f33}','{0x94,0x58,0xe1f33}')):
        assert needle in text;variants[name]=text.replace(needle,replacement,1)
    results=[]
    for name,source in variants.items():
        unit=work/(name+'.cpp');unit.write_text(source);obj=work/(name+'.o');binary=work/name
        cmd=shlex.split(compile['command']);cmd[cmd.index('-o')+1]=str(obj);cmd[-1]=str(unit);cmd+=['-I'+str(SOURCE.parent)]
        c=subprocess.run(cmd,cwd=build,capture_output=True,text=True);assert c.returncode==0,c.stderr
        cmd=[str(obj) if part==original_object else part for part in link if not part.startswith('-Wl,--dependency-file=')]
        cmd[cmd.index('-o')+1]=str(binary)
        c=subprocess.run(cmd,cwd=build,capture_output=True,text=True);assert c.returncode==0,c.stderr
        c=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
        results.append(dict(name=name,exit_code=c.returncode,output=(c.stdout+c.stderr)[-4000:],source_sha256=hashlib.sha256(source.encode()).hexdigest()))
        print(name,c.returncode,(c.stdout+c.stderr).strip(),flush=True)
    passed=results[0]['exit_code']==0 and all(r['exit_code']!=0 for r in results[1:])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(passed=passed,scratch=str(scratch),results=results,retained_large_artifacts=[]),indent=2)+'\n')
    return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
