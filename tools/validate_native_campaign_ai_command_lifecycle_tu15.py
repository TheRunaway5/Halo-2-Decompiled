#!/usr/bin/env python3
"""Compile lifecycle controls from borrowed immutable build inputs in owned scratch."""
import argparse,gzip,hashlib,json,os,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'src/campaign/ai/native_campaign_ai_command_lifecycle_tu15.cpp'
TEST=ROOT/'tests/native_campaign_ai_command_lifecycle_tu15_tests.cpp'
def main():
    p=argparse.ArgumentParser();p.add_argument('--build',type=Path);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--fixture',type=Path,action='append',required=True);p.add_argument('--mutants',action='store_true');p.add_argument('--repeat-original',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    if a.repeat_original:
        results=[]
        for n,fixture in enumerate(a.fixture):
            owned=scratch/str(n);owned.mkdir();output=owned/'reference.json.gz';report=owned/'original.json'
            flag=next(('--'+kind for kind in ('extra','identity','composed') if '_'+kind+'_' in fixture.name),None)
            cmd=[str(ROOT/'.tools/python/bin/python'),'-B',str(ROOT/'tools/oracle_native_campaign_ai_command_lifecycle_tu15.py'),
                '--output',str(output),'--report',str(report),'--verify',str(fixture.resolve())]+([flag] if flag else [])
            env=dict(os.environ,HALO2_TEST_SCRATCH=str(owned),TMPDIR=str(owned),TMP=str(owned),TEMP=str(owned))
            r=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True)
            results.append(dict(fixture=str(fixture),bytes=fixture.stat().st_size,exit_code=r.returncode,
                identical=r.returncode==0,output=(r.stdout+r.stderr)[-4000:]));print(fixture.name,r.returncode,flush=True)
        passed=all(r['identical'] for r in results)
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(passed=passed,scratch=str(scratch),results=results,retained_large_artifacts=[]),indent=2)+'\n')
        return 0 if passed else 1
    assert a.build is not None
    build=a.build.resolve();commands=json.loads((build/'compile_commands.json').read_text())
    target='native_campaign_ai_command_movement_tu15_tests'
    cc=next(c for c in commands if c['file'].endswith('/'+SOURCE.name) and target+'.dir/' in c['command'])
    tc=next(c for c in commands if c['file'].endswith('/native_campaign_ai_command_movement_tu15_tests.cpp'))
    link=shlex.split((build/('CMakeFiles/'+target+'.dir/link.txt')).read_text())
    def compile(row,src,obj):
        cmd=shlex.split(row['command']);cmd[cmd.index('-o')+1]=str(obj);cmd[-1]=str(src)
        cmd[1:1]=['-I'+str(ROOT/'include'),'-I'+str(ROOT/'src')]
        r=subprocess.run(cmd,cwd=build,capture_output=True,text=True);assert not r.returncode,r.stderr
    testobj=scratch/'test.o';compile(tc,TEST,testobj)
    shared=ROOT/'src/campaign/scripting/native_campaign_script_ai_tu15.cpp'
    sc=next(c for c in commands if c['file'].endswith('/'+shared.name))
    sharedobj=scratch/'shared.o';compile(sc,shared,sharedobj)
    fixtures=[]
    for n,f in enumerate(a.fixture):
        out=scratch/f'{n}.json';out.write_bytes(gzip.decompress(f.read_bytes()));fixtures.append(out)
    source=SOURCE.read_text();variants={'control':source}
    if a.mutants:
        for name,old,new in (
            ('head_association','body+0x858u,id','body+0x858u,none'),
            ('script_ordinal','c+0x34u,script','c+0x34u,script+1u'),
            ('thread_retirement','retire_thread(0x2622a8u,thread,gp)','retire_thread(0x2622a8u,thread+1u,gp)'),
            ('command_state','c+0x30u,1,2','c+0x30u,0,2'),
            ('root_stack','top+4u,expression','top+4u,expression+1u'),
            ('current_script','const U id=r(body+0x85cu)','const U id=r(body+0x858u)'),
            ('chain_identity','{actor.datum,in[7]}','{(r(actor.row,2)<<16u)|(actor.datum&0xffffu),in[7]}')):
            assert old in source,old;variants[name]=source.replace(old,new,1)
    results=[]
    for name,text in variants.items():
        unit=scratch/(name+'.cpp');unit.write_text(text);obj=scratch/(name+'.o');binary=scratch/name;compile(cc,unit,obj)
        cmd=[str(obj) if x==cc['output'] else str(testobj) if x==tc['output'] else x for x in link if not x.startswith('-Wl,--dependency-file=')]
        cmd.insert(1,str(sharedobj))
        cmd[cmd.index('-o')+1]=str(binary);r=subprocess.run(cmd,cwd=build,capture_output=True,text=True);assert not r.returncode,r.stderr
        cases=[]
        for fixture in fixtures:
            r=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);cases.append(dict(exit_code=r.returncode,output=(r.stdout+r.stderr)[-5000:]));print(name,cases[-1],flush=True)
        results.append(dict(name=name,runs=cases,source_sha256=hashlib.sha256(text.encode()).hexdigest()))
    passed=all(c['exit_code']==0 for c in results[0]['runs']) and all(any(c['exit_code']!=0 for c in r['runs']) for r in results[1:])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(passed=passed,scratch=str(scratch),results=results,retained_large_artifacts=[]),indent=2)+'\n')
    return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
