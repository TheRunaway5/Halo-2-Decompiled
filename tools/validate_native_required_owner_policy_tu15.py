#!/usr/bin/env python3
"""Execute the exact World required-owner certificate in a disposable context."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
WORLD='src/physics/physics_live_world_tu15.cpp'
HELPER='src/physics/runtime/native_tu15_required_owner_policy.hpp'
FILES=(WORLD,HELPER,'include/halo2/physics_contact_runtime.hpp','tests/native_tu15_required_owner_policy_tests.cpp','tools/validate_native_required_owner_policy_tu15.py')
def hashes():return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.report.parent.mkdir(parents=True,exist_ok=True);report=dict(passed=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        world=(ROOT/WORLD).read_text();match=re.search(r'^    struct WorldRequiredOwner \{.*?^    \};',world,re.M|re.S)
        if not match or world.count('s.required_owner=WorldRequiredOwner{this};')!=1 or world.count('s.required_owner_policy={&s.required_owner.target_type(),main_thread,this,&WorldRequiredOwner::matches};')!=1:raise ValueError('Actual World required-owner producer drift')
        body=match[0];(scratch/'world_required_owner.inc').write_text(body+'\n');outcomes=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;command=[compiler,'-std=c++20','-O2','-pthread','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_required_owner_policy_tests.cpp'),'-o',str(binary)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            compiled=subprocess.run(command,capture_output=True,text=True);assert compiled.returncode==0,compiled.stderr[-5000:]
            run=subprocess.run([str(binary)],capture_output=True,text=True);assert run.returncode==0,run.stdout+run.stderr
            mutants=[]
            if a.semantic_mutants:
                helper=(ROOT/HELPER).read_text()
                defects=(('wrong_callback_type','helper','services.required_owner.target_type()==*policy.callback_type&&',''),
                    ('wrong_world','world','body->world==owner&&',''),
                    ('active_trace','world','!body->world->first_person_phase_active&&!body->world->weapon_effect_phase_active','true'))
                for name,kind,needle,replacement in defects:
                    source=helper if kind=='helper'else body
                    if source.count(needle)!=1:raise ValueError('Exact owned-policy defect seam drift '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);overlay.mkdir()
                    target=overlay/HELPER if kind=='helper'else overlay/'world_required_owner.inc';target.parent.mkdir(parents=True,exist_ok=True);target.write_text(source.replace(needle,replacement,1))
                    cmd=list(command);cmd[cmd.index('-o')+1]=str(overlay/name);cmd[1:1]=['-I'+str(overlay),'-I'+str(overlay/'src')]
                    compiled=subprocess.run(cmd,capture_output=True,text=True);assert compiled.returncode==0,compiled.stderr[-5000:]
                    failed=subprocess.run([str(overlay/name)],capture_output=True,text=True)
                    expected={'wrong_callback_type':'Wrong callback type admitted','wrong_world':'Different owner admitted','active_trace':'Active trace admitted'}[name]
                    if failed.returncode!=1 or expected not in failed.stderr:raise ValueError('Owned-policy actual defect not rejected '+name+' '+failed.stderr[-2000:])
                    mutants.append(dict(name=name,rejected=True,diagnostic=failed.stderr.strip()))
            outcomes.append(dict(compiler=compiler,result=json.loads(run.stdout),compiled_mutants=mutants))
        report.update(passed=True,world_callable_sha256=hashlib.sha256(body.encode()).hexdigest(),compilers=outcomes,
            scope='Exact actual World callable/matcher and producer wiring; captured-owner/executor/trace/metadata/copy/wrapper controls, transparent reference/exception forwarding; no numerical whole-World child claim')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        a.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
