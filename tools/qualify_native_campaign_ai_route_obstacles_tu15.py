#!/usr/bin/env python3
"""Owned compiler/mutation proof using a currently owned CMake build's closure.

The supplied build must remain owned by tools/run_test.py/run_tests.py. New
objects and executables stay under this invocation's HALO2_TEST_SCRATCH.
"""
import argparse,gzip,hashlib,json,os,shlex,subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SOURCE=Path('src/campaign/ai/native_campaign_ai_route_obstacles_tu15.cpp')
DRIVER=Path('tests/native_campaign_ai_route_obstacles_tu15_tests.cpp')
MUTANTS=(
 ('ignored_object','if(object==ignore0||object==ignore1)return;','if(object==ignore1)return;'),
 ('exclusive_step','if(step>=obstacle_class)return;','if(step>obstacle_class)return;'),
 ('marker_class','flags|=0x40u;special=true;','flags|=0x20u;special=true;'),
 ('moving_flag','moving=1;','moving=0;'),
 ('shape_flag','extra=8u;','extra=0u;'),
 ('vertical_weight','s.mul(s.mul(z,z),p.r32(0x46a120u))','s.mul(z,z)'),
 ('special_output','const auto output=special?secondary:primary;','const auto output=special?primary:primary;'),
 ('callee_pop','abi.gp[4]=entry+44u;','abi.gp[4]=entry+40u;'),
)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();build=a.build.resolve()
    assert (build.parent/'source').exists()or '.local/test-scratch/'in str(build),'Only an owned build may supply dependencies'
    commands=[]
    def run(command,expected=0,env=None):
        commands.append(command);r=subprocess.run(command,cwd=ROOT,text=True,capture_output=True,env=env)
        if expected==0:assert r.returncode==0,r.stdout[-8000:]+r.stderr[-8000:]
        return r
    fixture=scratch/'fixture.json';fixture.write_bytes(gzip.decompress(a.fixture.read_bytes()))
    link=shlex.split((build/'CMakeFiles/native_campaign_ai_route_obstacles_tu15_tests.dir/link.txt').read_text())
    link=[x for x in link if not x.startswith('-Wl,--dependency-file=')]
    for n,x in enumerate(link):
        if not x.startswith('-')and not Path(x).is_absolute()and x.endswith(('.o','.a')):link[n]=str(build/x)
    source_object=next(n for n,x in enumerate(link)if x.endswith('/'+str(SOURCE)+'.o'))
    driver_object=next(n for n,x in enumerate(link)if x.endswith('/'+str(DRIVER)+'.o'))
    output_index=link.index('-o')+1
    original=(ROOT/SOURCE).read_text();results=[]
    for name,old,new in [('baseline',None,None),*MUTANTS,('asan_ubsan',None,None)]:
        changed=original
        if old is not None:assert original.count(old)==1,(name,original.count(old));changed=original.replace(old,new)
        source=scratch/(name+'.cpp');obj=scratch/(name+'.o');probe=scratch/(name+'.probe');source.write_text(changed)
        sanitizer=name=='asan_ubsan';options=['-fsanitize=address,undefined','-fno-omit-frame-pointer']if sanitizer else[]
        run(['c++','-std=c++20','-O1'if sanitizer else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/campaign/ai'),*options,'-c',str(source),'-o',str(obj)])
        command=link.copy();command[source_object]=str(obj);command[output_index]=str(probe)
        if sanitizer:
            driver=scratch/'sanitized-driver.o';run(['c++','-std=c++20','-O1','-I'+str(ROOT/'include'),*options,'-c',str(ROOT/DRIVER),'-o',str(driver)])
            command[driver_object]=str(driver);command+=options
        run(command)
        r=run([str(probe),str(fixture)],expected=1 if old else 0,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
        if old:assert r.returncode!=0,name+' survived'
        results.append(dict(name=name,passed=True,rejected=bool(old),returncode=r.returncode,stdout=r.stdout[-2000:],stderr=r.stderr[-2000:]))
        print(json.dumps(results[-1]),flush=True)
    report=dict(passed=True,results=results,commands=commands,source_sha256={str(p):sha(ROOT/p)for p in(SOURCE,DRIVER,Path(__file__).relative_to(ROOT))},fixture_sha256=sha(a.fixture),owned_scratch=str(scratch),borrowed_owned_build=str(build),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
