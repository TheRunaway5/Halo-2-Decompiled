#!/usr/bin/env python3
"""Compare the actual VehicleEnvironment MathState lookup with its raw native view."""
import argparse,hashlib,json,os,platform,re,shlex,subprocess
from pathlib import Path
from validate_native_object_body_frame_tu15 import corpus
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/vehicles/physics_vehicle_environment_state_tu15_generated.cpp'
FRAME='src/gameplay/objects/native_tu15_object_body_raw_frame.hpp'
TEST='tests/native_tu15_vehicle_object_lookup_tests.cpp'
BODY_PIN='0b00de7a5443ca306ccdc00399f92a82e90f9ce380177cf2468010b578fc4355'
PATTERN=re.compile(r'^void run_bcdf0\(MathState &s\) \{.*?^\}',re.M|re.S)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(scratch):
    import native_vehicle_object_lookup_replacements as vehicle
    current=vehicle.remove_vehicle_object_lookup((ROOT/UNIT).read_text());bodies=PATTERN.findall(current)
    if len(bodies)!=1 or hashlib.sha256(bodies[0].encode()).hexdigest()!=BODY_PIN:
        raise ValueError('Actual whole VehicleEnvironment lookup changed')
    helper=re.findall(r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}',current,re.M|re.S)
    if len(helper)!=1:raise ValueError('Actual sign extension helper changed')
    pcs=re.findall(r'if\(s.services.instruction\)s.services.instruction\((0x[0-9a-f]+)u\);',bodies[0])
    if len(pcs)!=35 or len(set(pcs))!=35:raise ValueError('Actual raw callback boundary changed')
    import native_object_body_replacements as binding
    from validate_native_object_body_source_frame_tu15 import UNIT as SIMPLE
    sibling=binding.remove_object_body((ROOT/SIMPLE).read_text())
    sibling=re.findall(r'^static void run_bcdf0\(WorldState &s\) \{.*?^\}',sibling,re.M|re.S)
    if len(sibling)!=1:raise ValueError('Qualified lookup sibling changed')
    def projection(text):
        text=text.replace('static void run_bcdf0(WorldState &s)','void run_bcdf0(MathState &s)')
        text=re.sub(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',r'if(s.services.instruction)s.services.instruction(\1);',text)
        return re.sub(r'throw std::runtime_error\("[^"]+"\);','throw std::runtime_error("SOURCE_OWNER_ERROR");',text)
    if projection(sibling[0])!=projection(bodies[0]):raise ValueError('Vehicle source differs from qualified sibling operations')
    (scratch/'vehicle_object_lookup_reference.inc').write_text(helper[0]+'\n'+bodies[0]+'\n')
    return dict(actual_body_sha256=BODY_PIN,raw_callback_sites=pcs,
        actual_math_state=True,retained_semantic_operations_changed=False,
        source_projection='Complete sibling differs only in state type, callback braces, linkage and final unreachable error')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(UNIT,FRAME,TEST,'include/halo2/native_tu15_object_body_kernel.hpp',
        'src/physics/vehicles/physics_vehicle_motion_math_internal.hpp','src/physics/vehicles/physics_vehicle_motion_internal.hpp',
        'src/physics/vehicles/physics_vehicle_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp',
        'src/physics/runtime/physics_contact_runtime_internal.hpp','tools/validate_native_vehicle_object_lookup_tu15.py')
    pins={name:sha(ROOT/name)for name in names};report=dict(passed=False,production_bound=False,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    headers={path.resolve():sha(path)for tree in('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc','.cpp')}
    try:
        report['qualification']=prepare(scratch)
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(corpus(),separators=(',',':')))
        results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True,timeout=600)
            if built.returncode:raise ValueError('Vehicle raw lookup compile failed '+built.stderr[:3000]+built.stderr[-3000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1800)
            if done.returncode:raise ValueError('Vehicle raw lookup comparison failed '+(done.stdout+done.stderr)[-7000:])
            mutations=[]
            if a.semantic_mutants:
                original=(ROOT/FRAME).read_text()
                definitions=(
                    ('skip-raw-callback','if(s_.services.instruction)s_.services.instruction(pc);','if(false)s_.services.instruction(pc);'),
                    ('cache-datum-across-callback','bool in_bounds(std::uint32_t,std::uint32_t){\n        step(0xbce04u);s_.set(1,s_.reg(6,4,0),4,0);','bool in_bounds(std::uint32_t datum,std::uint32_t){\n        step(0xbce04u);s_.set(1,datum,4,0);'),
                    ('skip-branch-callback','step(0xbce29u);return s_.flags.zf;','return s_.flags.zf;'),
                    ('wrong-mask-shift','s_.flags.shift(s_.reg(2,4,0),s_.reg(1,1,0),4)','s_.flags.shift(s_.reg(2,4,0),s_.reg(0,1,0),4)'),
                    ('wrong-restored-edi','step(0xbce46u);s_.set(7,s_.pop(),4,0);','step(0xbce46u);s_.set(6,s_.pop(),4,0);'))
                for label,old,new in definitions:
                    if original.count(old)!=1:raise ValueError('Vehicle raw defect seam changed '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/FRAME
                    target.parent.mkdir(parents=True);target.write_text(original.replace(old,new,1))
                    changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(overlay/'bad')
                    bad_build=subprocess.run(changed,capture_output=True,text=True,timeout=600)
                    if bad_build.returncode:raise ValueError('Vehicle raw defect compile failed '+label+': '+bad_build.stderr[-5000:])
                    bad=subprocess.run([str(overlay/'bad'),str(fixture)],capture_output=True,text=True,timeout=1800)
                    diagnostic=(bad.stdout+bad.stderr)[-5000:]
                    if not bad.returncode or 'Vehicle raw lookup differs'not in diagnostic:raise ValueError('Vehicle raw defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
