#!/usr/bin/env python3
"""Private exact26-function VehicleEnvironment BCDF0 public API comparison."""
import argparse,hashlib,json,os,platform,re,subprocess
from pathlib import Path
import native_vehicle_object_lookup_replacements as binding
from validate_native_object_body_frame_tu15 import corpus
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/vehicles/physics_vehicle_environment_state_tu15_generated.cpp'
TEST='tests/native_tu15_vehicle_object_lookup_public_tests.cpp'
OWNER='src/physics/vehicles/native_tu15_vehicle_object_lookup_owner.hpp'
HOOK_SITE=' FpScope scope(fp);MathState s{m,observe,bound,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);\n'
HOOK=' halo2::vehicle_lookup_public_proof::loaded(s,bound,observe);\n'
COUNTER='\n ++halo2::vehicle_lookup_public_proof::literals;'
PROOF_INCLUDE='#include "vehicle_object_lookup_public_proof.hpp"\n'
API='source_run_physics_vehicle_environment_state_runtime'
REFERENCE='source_run_physics_vehicle_environment_state_reference'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def public_defects(source):
    """Alter only real statements of the privately bound actual public API."""
    success='\n abi=s.save();fp={control_word(),_mm_getcsr()};\n'
    caught='}}catch(...) {abi=s.save();fp={control_word(),_mm_getcsr()};throw;}'
    copied=' auto bound=services;'
    publication='fp={control_word(),_mm_getcsr()};'
    if source.count(success)!=1 or source.count(caught)!=1 or source.count(copied)!=1 or source.count(publication)!=2:raise ValueError('Actual public defect statement anchors drifted')
    return {
        'omitted_success_ABI_save':source.replace(success,'\n fp={control_word(),_mm_getcsr()};\n',1),
        'omitted_throw_ABI_save':source.replace(caught,'}}catch(...) {fp={control_word(),_mm_getcsr()};throw;}',1),
        'aliased_caller_services':source.replace(copied,' auto &bound=const_cast<SourcePhysicsContactRuntimeServices&>(services);',1),
        'omitted_FP_publication':source.replace(publication,'')
    }
def prepare(scratch,require_binding=False):
    installed=(ROOT/UNIT).read_text();binding.authenticate(installed)
    bound=binding.PREFIX in installed
    if require_binding and(not bound or binding.installed_entries(installed)!=['bcdf0']):raise ValueError('Actual Vehicle public native source default is not installed')
    source=binding.remove_vehicle_object_lookup(installed)
    if binding.digest(source)!=binding.SOURCE_PIN or binding.INCLUDE in source:raise ValueError('Public probe requires exact canonical complete26-function unit')
    candidate=binding.replace_vehicle_object_lookup(source)
    if binding.remove_vehicle_object_lookup(candidate)!=source or binding.replace_vehicle_object_lookup(candidate)!=candidate or binding.installed_entries(candidate)!=['bcdf0']:raise ValueError('Vehicle candidate exact reversal/idempotence changed')
    if source.count(HOOK_SITE)!=1 or candidate.count(HOOK_SITE)!=1:raise ValueError('Unique actual public load/save/catch hook site drift')
    if len(re.findall(r'^void run_[0-9a-f]+\(MathState &s\) \{',source,re.M))!=26:raise ValueError('Complete Vehicle function count drift')
    candidate=candidate.replace(HOOK_SITE,HOOK_SITE+HOOK,1).replace(binding.HEADER+binding.PREFIX,binding.HEADER+binding.PREFIX+COUNTER,1)
    reference=source.replace(HOOK_SITE,HOOK_SITE+HOOK,1).replace(binding.HEADER,binding.HEADER+COUNTER,1).replace(API,REFERENCE)
    if binding.remove_vehicle_object_lookup(candidate.replace(HOOK,'',1).replace(COUNTER,'',1))!=source:raise ValueError('Private candidate instrumentation changed unrelated unit bytes')
    if reference.replace(REFERENCE,API).replace(HOOK,'',1).replace(COUNTER,'',1)!=source:raise ValueError('Private reference instrumentation changed actual source')
    header=r'''#pragma once
#include "halo2/physics_vehicle_environment_state.hpp"
#include "physics/vehicles/physics_vehicle_motion_math_internal.hpp"
#include "physics/vehicles/native_tu15_vehicle_object_lookup_owner.hpp"
namespace halo2 {
void source_run_physics_vehicle_environment_state_reference(SourcePhysicsContactMemory&,std::uint32_t,SourcePhysicsContactRuntimeAbi&,SourcePhysicsFactoryFloatingPoint&,const SourcePhysicsContactRuntimeServices&,const SourcePhysicsContactRecordObserver&);
namespace vehicle_lookup_public_proof {
using State=physics_vehicle_motion_math_detail::MathState;
inline thread_local std::function<void(State&,const SourcePhysicsContactRuntimeServices&,const SourcePhysicsContactRecordObserver&)>entered;
inline thread_local unsigned literals{};
inline void loaded(State&s,const SourcePhysicsContactRuntimeServices&services,const SourcePhysicsContactRecordObserver&observer){if(entered)entered(s,services,observer);}
inline void source_reference(SourcePhysicsContactMemory&m,std::uint32_t entry,SourcePhysicsContactRuntimeAbi&a,SourcePhysicsFactoryFloatingPoint&fp,const SourcePhysicsContactRuntimeServices&services,const SourcePhysicsContactRecordObserver&observer){source_run_physics_vehicle_environment_state_reference(m,entry,a,fp,services,observer);}
}
}
'''
    (scratch/'vehicle_object_lookup_public_proof.hpp').write_text(header)
    (scratch/'candidate.cpp').write_text(PROOF_INCLUDE+candidate);(scratch/'reference.cpp').write_text(PROOF_INCLUDE+reference)
    return dict(whole_source_sha256=binding.SOURCE_PIN,whole_body_sha256=binding.BODY_PIN,functions=26,all25_other_bodies_and_public_context_preserved=True,actual_default_installed=bound,native_prefix_private_only=not bound,reference_API_symbol_only_renamed=REFERENCE,private_instrumentation='One uniquely authenticated post-MathState-load pointer-only hook in both real API copies plus fallback entry counter; real services copy, FpScope/load/save/catch unchanged',hook_site_sha256=binding.digest(HOOK_SITE),proof_header_sha256=binding.digest(header),candidate_sha256=sha(scratch/'candidate.cpp'),reference_sha256=sha(scratch/'reference.cpp'))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--controls-only',action='store_true');p.add_argument('--mutations',action='store_true');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=[UNIT,TEST,OWNER,'src/gameplay/objects/native_tu15_object_body_raw_frame.hpp','include/halo2/native_tu15_object_body_kernel.hpp','tools/native_vehicle_object_lookup_replacements.py','tools/validate_native_object_body_frame_tu15.py',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for h in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((q for q in(current.parent/h,ROOT/'include'/h,ROOT/'src'/h,ROOT/'tests'/h)if q.is_file()),None)
            if target is not None:
                relative=str(target.relative_to(ROOT))
                if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        report['preparation']=prepare(scratch,a.require_binding);fixture=scratch/'inputs.json';fixture.write_text(json.dumps(corpus(),separators=(',',':')));report['corpus_sha256']=sha(fixture);report['compilers']=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name
            command=[compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/TEST),str(scratch/'candidate.cpp'),str(scratch/'reference.cpp'),'-o',str(binary)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True,timeout=600)
            if built.returncode:raise ValueError('Public Vehicle actual-unit compile failed: '+built.stderr[-12000:])
            modes={}
            for mode in('native','reference'):
                env=os.environ.copy();env.pop('HALO2_REFERENCE_VEHICLE_OBJECT_LOOKUP',None)
                if mode=='reference':env['HALO2_REFERENCE_VEHICLE_OBJECT_LOOKUP']='1'
                done=subprocess.run([str(binary),str(fixture)]+(['--reference']if mode=='reference'else[])+(['--controls-only']if a.controls_only else[]),capture_output=True,text=True,env=env,timeout=1800)
                if done.returncode:raise ValueError('Public Vehicle actual API differs '+mode+': '+(done.stdout+done.stderr)[-10000:])
                modes[mode]=json.loads(done.stdout)
            item=dict(compiler=compiler,command=command,modes=modes);report['compilers'].append(item)
            if a.mutations:
                item['compiled_public_defects']={}
                for name,text in public_defects((scratch/'candidate.cpp').read_text()).items():
                    mutated=scratch/(Path(compiler).name+'-'+name+'.cpp');mutated.write_text(text);bad=scratch/(Path(compiler).name+'-'+name)
                    bad_command=[str(mutated)if value==str(scratch/'candidate.cpp')else str(bad)if value==str(binary)else value for value in command]
                    built=subprocess.run(bad_command,capture_output=True,text=True,timeout=600)
                    if built.returncode:raise ValueError('Actual public defect failed compilation '+name+': '+built.stderr[-4000:])
                    env=os.environ.copy();env.pop('HALO2_REFERENCE_VEHICLE_OBJECT_LOOKUP',None)
                    rejected=subprocess.run([str(bad),str(fixture),'--controls-only'],capture_output=True,text=True,env=env,timeout=1800)
                    diagnostic=(rejected.stdout+rejected.stderr)[-4000:]
                    if rejected.returncode==0 or 'Vehicle public lookup differs'not in diagnostic:raise ValueError('Actual public defect did not produce an authentic comparison rejection '+name+': '+diagnostic)
                    item['compiled_public_defects'][name]=dict(rejected=True,source_sha256=binding.digest(text),diagnostic=diagnostic)
        report.update(passed=True,production_bound=report['preparation']['actual_default_installed'],scope='Exact whole-unit public API native/reference, full ABI/banks/events/callback/FP/exceptions; no supplied lookup result. Installation status authenticated separately.')
    except Exception as error:report['error']=repr(error)
    report['source_after']={name:sha(ROOT/name)for name in pins};report['sources_unchanged']=report['source_after']==pins
    if not report['sources_unchanged']:report.update(passed=False,source_drift=True)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
