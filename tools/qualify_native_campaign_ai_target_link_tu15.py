#!/usr/bin/env python3
"""Disposable native274670 adapter replay, mutation controls and sanitizers."""
import argparse,gzip,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ADAPTER=Path('src/campaign/ai/native_campaign_ai_target_link_tu15.cpp')
ACTOR=Path('src/campaign/ai/native_campaign_ai_perception_group_actor_tu15.cpp')
SOURCES=[ADAPTER,ACTOR,Path('src/campaign/ai/native_campaign_ai_perception_group_storage_tu15.cpp'),Path('src/campaign/ai/native_campaign_object_world_point_tu15.cpp'),Path('src/core/native_tu15_data_array_allocate.cpp')]
DRIVER=Path('tests/native_campaign_ai_target_link_tu15_tests.cpp')
MUTATIONS=[
 ('caller_argument',ADAPTER,'memory.read(g[4]+4u,bytes);','memory.read(g[4]+8u,bytes);'),
 ('clump_pointer',ADAPTER,'(memory,g[0],group,fp,abi.x87_depth)','(memory,g[0]+4u,group,fp,abi.x87_depth)'),
 ('caller_cleanup',ADAPTER,'abi.gp[4]=g[4]+8u;','abi.gp[4]=g[4]+4u;'),
 ('return_fp',ADAPTER,'publish();abi.gp=g;','abi.gp=g;'),
 ('throw_fp',ADAPTER,'catch(...){publish();throw;}','catch(...){throw;}'),
 ('group_head',ACTOR,'write(m,clump+0x14u,group);','write(m,clump+0x14u,group^1u);')]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--baseline-only',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);fixture=scratch/'fixture.json';fixture.write_bytes(gzip.decompress((ROOT/'tests/fixtures/native_campaign_ai_target_link_tu15.json.gz').read_bytes()))
    commands=[];results=[];passed=True
    configs=[('baseline',None,False)]
    if not a.baseline_only:configs += [(m[0],m,False)for m in MUTATIONS]+[('sanitizers',None,True)]
    for name,mutation,sanitize in configs:
        exe=scratch/name;sources=[ROOT/s for s in SOURCES];replacement=None
        if mutation:
            _,path,old,new=mutation;original=(ROOT/path).read_text();assert original.count(old)==1,(name,original.count(old));replacement=scratch/(name+'.cpp');replacement.write_text(original.replace(old,new));sources=[replacement if s==ROOT/path else s for s in sources]
        cmd=['c++','-std=c++20','-O1'if sanitize else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),str(ROOT/DRIVER),*[str(s)for s in sources],'-o',str(exe)]
        if sanitize:cmd[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        commands.append(cmd);compiled=subprocess.run(cmd,text=True,capture_output=True)
        env=os.environ.copy();env.update(ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        run=subprocess.run([str(exe),str(fixture)],text=True,capture_output=True,env=env)if compiled.returncode==0 else None
        okay=compiled.returncode==0 and run is not None and (run.returncode!=0 if mutation else run.returncode==0)
        results.append(dict(name=name,passed=okay,compile_returncode=compiled.returncode,compile_stderr=compiled.stderr[-2000:],returncode=run.returncode if run else None,stdout=run.stdout[-1500:]if run else'',stderr=run.stderr[-3000:]if run else''));print(json.dumps(results[-1]),flush=True)
        exe.unlink(missing_ok=True)
        if replacement:replacement.unlink()
        if not okay:passed=False;break
    report=dict(passed=passed,results=results,commands=commands,owned_scratch=str(scratch),retained_large_artifacts=[],source_sha256={str(s):hashlib.sha256((ROOT/s).read_bytes()).hexdigest()for s in [*SOURCES,DRIVER]},sanitizer_scope='Adapter, existing semantic child owners, allocator and memory driver; leak sanitizer disabled.',contract='Whole-call retained DATA, ordered semantic reads/stores, saved GP/ESP, retained FP and live caller values; no volatile GP, EFLAGS, child store-PC or private frame claim.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');return int(not passed)
if __name__=='__main__':raise SystemExit(main())
