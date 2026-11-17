#!/usr/bin/env python3
"""Disposable physical-fixture replay, old-profile regression and mutation controls."""
import argparse,gzip,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ARC=Path('src/campaign/ai/native_campaign_ai_jump_arc_tu15.cpp')
ADAPTER=Path('src/campaign/ai/native_campaign_ai_movement_jump_tu15.cpp')
COMMON=[ARC,Path('src/campaign/ai/native_campaign_ai_jump_fallback_tu15.cpp'),Path('src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp')]
MUTATIONS=[
 ('missing_minimum',ARC,'if(publish_minimum)publish_minimum(minimum_speed);','if(false&&publish_minimum)publish_minimum(minimum_speed);'),
 ('minimum_bits',ARC,'publish_minimum(minimum_speed);','publish_minimum(minimum_speed^1u);'),
 ('vertical_velocity',ARC,'result.vertical_velocity=bits(vz);','result.vertical_velocity=result.horizontal_speed;'),
 ('horizontal_speed',ARC,'result.horizontal_speed=spill_single();','result.horizontal_speed=spill_single()^1u;'),
 ('high_arc_sign',ARC,'const std::int32_t high_arc_sign=1;','const std::int32_t high_arc_sign=-1;'),
 ('minimum_store_pc',ARC,'store(0xff2e1u,minimum_speed,value);','store(0xff2e2u,minimum_speed,value);'),
 ('caller_cleanup',ADAPTER,'abi.gp[4]=g[4]+0x30u;','abi.gp[4]=g[4]+0x2cu;')]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--baseline-only',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);results=[];commands=[]
    fixtures={}
    for kind in ('movement_jump','jump_arc'):
        dest=scratch/(kind+'.json');dest.write_bytes(gzip.decompress((ROOT/'tests/fixtures'/f'native_campaign_ai_{kind}_tu15.json.gz').read_bytes()));fixtures[kind]=dest
    configs=[('baseline','movement_jump',None,False),('old_profile','jump_arc',None,False)]
    if not a.baseline_only:configs += [(m[0],'movement_jump',m,False)for m in MUTATIONS]+[('sanitizer','movement_jump',None,True),('old_profile_sanitizer','jump_arc',None,True)]
    passed=True
    for name,kind,mutation,sanitize in configs:
        exe=scratch/name;replacement=None;sources=[ROOT/s for s in COMMON]+([ROOT/ADAPTER]if kind=='movement_jump'else[])
        if mutation:
            _,target,old,new=mutation;text=(ROOT/target).read_text();assert text.count(old)==1,(name,text.count(old));replacement=scratch/(name+'.cpp');replacement.write_text(text.replace(old,new));sources=[replacement if s==ROOT/target else s for s in sources]
        cmd=['c++','-std=c++20','-O1'if sanitize else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),str(ROOT/'tests'/f'native_campaign_ai_{kind}_tu15_tests.cpp'),*[str(s)for s in sources],'-o',str(exe)]
        if sanitize:cmd[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        commands.append(cmd);c=subprocess.run(cmd,text=True,capture_output=True)
        env=os.environ.copy();env.update(ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        run=subprocess.run([str(exe),str(fixtures[kind])],text=True,capture_output=True,env=env)if c.returncode==0 else None
        okay=c.returncode==0 and run is not None and (run.returncode!=0 if mutation else run.returncode==0)
        results.append(dict(name=name,passed=okay,compile_returncode=c.returncode,compile_stderr=c.stderr[-4000:],returncode=run.returncode if run else None,stdout=run.stdout[-1500:]if run else'',stderr=run.stderr[-3000:]if run else''))
        print(json.dumps(results[-1]),flush=True)
        exe.unlink(missing_ok=True)
        if replacement:replacement.unlink()
        if not okay:passed=False;break
    report=dict(passed=passed,results=results,commands=commands,owned_scratch=str(scratch),retained_large_artifacts=[],source_sha256={str(s):hashlib.sha256((ROOT/s).read_bytes()).hexdigest()for s in [ADAPTER,*COMMON]},sanitizer_scope='New adapter, shared arc/fallback/vector owners and replay drivers instrumented; leak sanitizer disabled.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');return int(not passed)
if __name__=='__main__':raise SystemExit(main())
