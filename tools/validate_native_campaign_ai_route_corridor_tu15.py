#!/usr/bin/env python3
"""Borrow frozen build inputs read-only; compile mutation controls in owned scratch.
All replacement objects, executables and expanded DATA are removed by run_test.py.
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
SOURCE=ROOT/'src/campaign/ai/native_campaign_ai_route_corridor_tu15.cpp'

def main():
    p=argparse.ArgumentParser();p.add_argument('--build',type=Path);p.add_argument('--repeat-original',action='store_true');p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if a.repeat_original:
        results=[]
        for suffix,script,extra in (
            ('', 'oracle_native_campaign_ai_route_corridor_tu15.py', ['--baseline']),
            ('_extended','oracle_native_campaign_ai_route_corridor_tu15.py',['--select','append_m(10|11|14|15|16|17)|zero_direction|margin_']),
            ('_prefix','oracle_native_campaign_ai_route_corridor_prefix_tu15.py',[])):
            expected=ROOT/('tests/fixtures/native_campaign_ai_route_corridor'+suffix+'_tu15.json.gz')
            actual=scratch/expected.name;report=scratch/(suffix+'-original.json')
            cmd=[str(ROOT/'.tools/python/bin/python'),'-B',str(ROOT/'tools'/script),'--output',str(actual),'--report',str(report),*extra]
            owned=scratch/(suffix or "base");owned.mkdir()
            env=dict(os.environ,HALO2_TEST_SCRATCH=str(owned),TMPDIR=str(owned),TMP=str(owned),TEMP=str(owned))
            c=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True)
            assert not c.returncode,c.stdout+c.stderr
            same=actual.read_bytes()==expected.read_bytes()
            results.append(dict(fixture=str(expected),bytes=expected.stat().st_size,identical=same,sha256=hashlib.sha256(actual.read_bytes()).hexdigest(),command=cmd))
            print(expected.name,'byte-identical',same,flush=True)
        passed=all(r['identical'] for r in results)
        a.report.write_text(json.dumps(dict(passed=passed,scratch=str(scratch),results=results,retained_large_artifacts=[]),indent=2)+'\n')
        return 0 if passed else 1
    assert a.build is not None
    build=a.build.resolve();assert build.is_dir()  # Borrowed immutable build inputs; every generated artifact is owned below.
    work=scratch/'refresh-controls';work.mkdir(exist_ok=True)
    fixture=work/'fixture.json';fixture.write_bytes(gzip.decompress((ROOT/'tests/fixtures/native_campaign_ai_route_corridor_tu15.json.gz').read_bytes()))
    commands=json.loads((build/'compile_commands.json').read_text())
    compile=next(c for c in commands if c['file'].endswith('/'+SOURCE.name) and 'native_campaign_ai_route_corridor_tu15_tests.dir/' in c['command'])
    original_object=compile['output'];link=shlex.split((build/'CMakeFiles/native_campaign_ai_route_corridor_tu15_tests.dir/link.txt').read_text())
    text=SOURCE.read_text();variants={'control':text}
    for name,needle,replacement in (
        ('completion_flag','0x2cdf01u,c+0x28u,1,1','0x2cdf01u,c+0x28u,0,1'),
        ('candidate_cost','auto total=scalar.add(distance,r(entry+0x14u))','auto total=distance'),
        ('heap_direction','if(!scalar.above(r(node(c,pi)+0x24u),r(node(c,ci)+0x24u)))break','if(scalar.above(r(node(c,pi)+0x24u),r(node(c,ci)+0x24u)))break'),
        ('tangent_side','w(0x2cab45u,first,x)','w(0x2cab45u,first,x^0x80000000u)'),
        ('candidate_side','if(r(previous+0x1cu,1)!=side)return none','if(r(previous+0x1cu,1)==side)return none'),
        ('goal_distance','w(0x2cd73au,c+0x24u,r(out+0x14u))','w(0x2cd73au,c+0x24u,0)')):
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
