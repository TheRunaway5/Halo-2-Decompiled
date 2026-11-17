#!/usr/bin/env python3
"""Owned native parameter replay, sensitivity controls and scoped sanitizers."""
import argparse,gzip,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'src/campaign/ai/native_campaign_ai_movement_link_parameters_tu15.cpp'
CHILD=ROOT/'src/campaign/ai/native_campaign_ai_character_property_tu15.cpp'
DRIVER=ROOT/'tests/native_campaign_ai_movement_link_parameters_tu15_tests.cpp'
MUTANTS=(
    ('force_threshold','U(count>=7)','U(count>7)'),
    ('initial_force','0x200356u,g[7],0','0x200356u,g[7],1'),
    ('speed_clamp','count<6?U(std::int32_t(count)):6u','count<5?U(std::int32_t(count)):5u'),
    ('scenario_count','std::bit_cast<std::int32_t>(g[2])>0','std::bit_cast<std::int32_t>(g[2])>=0'),
    ('scenario_stride','g[2]+g[1]*4u+0x80u','g[2]+g[1]*8u+0x80u'),
    ('body_fallback_flag','g[3]+0xaau','g[3]+0xabu'),
    ('kind_upper_bound','kind>=0 && kind<6','kind>=0 && kind<=6'),
)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--baseline-only',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();fixture=scratch/'fixture.json';fixture.write_bytes(gzip.decompress(a.fixture.read_bytes()))
    report=dict(passed=False,source_sha256={str(p.relative_to(ROOT)):sha(p)for p in(SOURCE,CHILD,DRIVER,Path(__file__))},fixture_sha256=sha(a.fixture),commands=[],results=[],owned_scratch=str(scratch),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True)
    def save():a.report.write_text(json.dumps(report,indent=2)+'\n')
    def run(command,env=None):
        report['commands'].append(command);r=subprocess.run(command,text=True,capture_output=True,env=env)
        return r
    def require(command):
        r=run(command)
        if r.returncode:report['failure']=dict(command=command,returncode=r.returncode,stderr=r.stderr[-5000:]);save();raise RuntimeError(r.stderr[-5000:])
    original=SOURCE.read_text();variants=[('baseline',None,None)]
    if not a.baseline_only:variants+=list(MUTANTS)+[('asan_ubsan',None,None)]
    for name,old,new in variants:
        options=['-fsanitize=address,undefined','-fno-omit-frame-pointer']if name=='asan_ubsan'else[]
        common=['c++','-std=c++20','-O1'if options else'-O2','-g0','-fmax-errors=1','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),*options]
        driver=scratch/('sanitized-driver.o'if options else'driver.o');child=scratch/('sanitized-child.o'if options else'child.o')
        for target,source in((driver,DRIVER),(child,CHILD)):
            if not target.exists():require([*common,'-c',str(source),'-o',str(target)])
        changed=original
        if old:assert original.count(old)==1,(name,original.count(old));changed=original.replace(old,new)
        source=scratch/(name+'.cpp');obj=scratch/(name+'.o');probe=scratch/(name+'.probe');source.write_text(changed)
        require([*common,'-c',str(source),'-o',str(obj)]);require(['c++',str(driver),str(obj),str(child),'-o',str(probe),*options])
        r=run([str(probe),str(fixture)],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
        good=bool(r.returncode)if old else r.returncode==0
        report['results'].append(dict(name=name,passed=good,rejected=bool(old and r.returncode),returncode=r.returncode,stdout=r.stdout[-2000:],stderr=r.stderr[-2000:]));save();print(json.dumps(report['results'][-1]),flush=True)
        for generated in(source,obj,probe):generated.unlink()
        if not good:raise RuntimeError(name+' failed qualification')
    assert sha(SOURCE)==report['source_sha256'][str(SOURCE.relative_to(ROOT))]
    report['passed']=True;save()
if __name__=='__main__':main()
