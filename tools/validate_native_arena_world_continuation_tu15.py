#!/usr/bin/env python3
"""Private whole-cold Arena with pinned World callback arms and real owners."""
import argparse,hashlib,json,os,platform,re,shlex,subprocess
from pathlib import Path
import native_arena_initialize_continuation_replacements as binding
import native_arena_world_producer_inputs as world
from validate_native_arena_initialize_owner_tu15 import SOURCES
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(scratch):
    evidence=world.prepare(ROOT,scratch)
    (scratch/'arena_world_bridge.hpp').write_text('''#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_arena_initialize_source_frame.hpp"
namespace halo2 {
using ArenaWorldState=physics_world_runtime_detail::WorldState;
bool arena_world_reference();bool arena_world_eligible(const ArenaWorldState&);
void arena_world_parent(bool,ArenaWorldState&);void arena_world_statement(std::uint32_t);
void arena_world_native(bool);
void arena_world_continuation(std::uint32_t);
void arena_world_child(bool,std::uint32_t,std::uint32_t,const SourcePhysicsWorldRuntimeAbi&);
struct ArenaWorldParentScope{ArenaWorldState&state;explicit ArenaWorldParentScope(ArenaWorldState&s):state(s){arena_world_parent(true,state);}~ArenaWorldParentScope(){arena_world_parent(false,state);}};
template<class Call>void arena_world_call(ArenaWorldState&s,std::uint32_t target,std::uint32_t site,Call call){
    arena_world_child(true,target,site,s.save());
    try{call();}catch(...){const auto original=std::current_exception();arena_world_child(false,target,site,s.save());std::rethrow_exception(original);}
    arena_world_child(false,target,site,s.save());
}
template<class Call>std::uint32_t arena_world_entry(ArenaWorldState&s,Call call){
    if(arena_world_reference()||!arena_world_eligible(s))return 1u;
    arena_world_native(false);
    const auto child=[&](auto&current,std::uint32_t target,std::uint32_t site,bool tail){arena_world_call(current,target,site,[&]{call(current,target,site,tail);});};
    const auto next=native_arena_initialize_source_detail::run_source(s,arena_world_eligible,child);
    if(next){arena_world_continuation(next);arena_world_native(true);}return next;
}
}
''')
    source=(ROOT/'src/physics/physics_game_arena_runtime_generated.cpp').read_text();raw,bound=binding.body(source)
    canonical=binding.original.remove_arena_initialize(source)
    private_prefix=binding.PREFIX.replace('native_arena_continuation_entry(s)','halo2::arena_world_entry(s,[](auto&current,auto target,auto site,bool tail){dispatch(current,target,site,tail);})',1).replace('case 0u:return;','case 0u:return;case 1u:break;',1)
    bound=bound.replace(binding.PREFIX,private_prefix,1)
    for old,new in(
        ('dispatch(s,target,0x1252a4u,false);','halo2::arena_world_call(s,target,0x1252a4u,[&]{dispatch(s,target,0x1252a4u,false);});'),
        ('run_217090(s);','halo2::arena_world_call(s,selected(0x217090u,s.image),0x1252bau,[&]{run_217090(s);});'),
        ('s.push(0x1252edu,selected(0x1252f2u,s.image));run_1656a0(s);','s.push(0x1252edu,selected(0x1252f2u,s.image));halo2::arena_world_call(s,selected(0x1656a0u,s.image),0x1252edu,[&]{run_1656a0(s);});'),
        ('s.push(0x125321u,selected(0x125326u,s.image));run_1656a0(s);','s.push(0x125321u,selected(0x125326u,s.image));halo2::arena_world_call(s,selected(0x1656a0u,s.image),0x125321u,[&]{run_1656a0(s);});')):
        if bound.count(old)!=1:raise ValueError('Exact actual Arena child call instrumentation drift')
        bound=bound.replace(old,new,1)
    opening=bound.index('{')+1;bound=bound[:opening]+'\n halo2::ArenaWorldParentScope actual_parent{s};'+bound[opening:]
    bound,n=re.subn(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',r'halo2::arena_world_statement(\1);if(s.services.instruction){s.services.instruction(\1);}',bound)
    if n!=45 or bound.count(binding.REPEAT)!=1:raise ValueError('Actual parent/repeated statement counting drift')
    bound=bound.replace(binding.REPEAT,'halo2::arena_world_statement(0x1252b8u);'+binding.REPEAT,1)
    target=scratch/'arena-world-runtime.cpp';target.write_text('#include "arena_world_bridge.hpp"\n'+canonical.replace(raw,bound,1))
    evidence.update(original_parent_body_sha256=binding.digest(raw),private_measured_parent_sha256=binding.digest(bound),all_seventeen_owner_source_sha256=binding.digest(canonical),supplied_children=False)
    return target,evidence
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true');p.add_argument('--frontiers',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);driver='tests/native_tu15_arena_world_continuation_tests.cpp'
    sources=(driver,*SOURCES[1:],'src/runtime/source_execution_inputs.cpp')
    names=(*sources,world.WORLD,'tools/native_arena_world_producer_inputs.py','tools/validate_native_arena_world_continuation_tu15.py','tools/native_arena_initialize_continuation_replacements.py','src/physics/runtime/native_tu15_arena_initialize_source_frame.hpp')
    pins={n:sha(ROOT/n)for n in names};headers={x.resolve():sha(x)for folder in('include','src')for x in(ROOT/folder).rglob('*')if x.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        generated,report['actual_source_provenance']=prepare(scratch);results=[]
        for compiler in a.compiler or('g++','clang++'):
            folder=scratch/Path(compiler).name;folder.mkdir();flags=['-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch)]
            if a.sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            objects=[];commands=[]
            for index,name in enumerate(sources):
                unit=generated if name=='src/physics/physics_game_arena_runtime_generated.cpp'else ROOT/name;target=folder/(str(index)+'.o');dep=target.with_suffix('.d');command=[compiler,*flags,'-c',str(unit),'-o',str(target),'-MD','-MF',str(dep)];commands.append(command);done=subprocess.run(command,capture_output=True,text=True)
                if done.returncode:raise ValueError('Actual World Arena source compile failed '+name+': '+done.stderr[-10000:])
                for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(value).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
                objects.append(target)
            binary=folder/'world-arena';command=[compiler,*flags,*map(str,objects),'-Wl,--gc-sections','-pthread','-lcrypto','-o',str(binary)];commands.append(command);done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Actual World Arena link failed '+done.stderr[-10000:])
            rows=[]
            for feedback in((True,)if a.smoke else(True,False)):
                for policy in((6,7,8,9,10)if a.frontiers else(0,1)if a.smoke else(0,1,2,3,4,5)):
                    pair=[]
                    for reference in(False,True):
                        env=os.environ.copy()
                        for key in('HALO2_INSTRUCTION_CENSUS','HALO2_REFERENCE_INSTRUCTION_CALLBACKS','HALO2_REFERENCE_LITERAL_INSTRUCTIONS','HALO2_ARENA_CONTINUATION_REFERENCE','HALO2_ARENA_DECLARED_FEEDBACK'):env.pop(key,None)
                        if feedback:env['HALO2_ARENA_DECLARED_FEEDBACK']='1'
                        if reference:env['HALO2_ARENA_CONTINUATION_REFERENCE']='1'
                        env['HALO2_ARENA_WORLD_POLICY']=str(policy)
                        done=subprocess.run([str(binary)],capture_output=True,text=True,timeout=180,env=env)
                        if done.returncode:raise ValueError('Whole-cold actual World Arena run failed '+str((compiler,feedback,policy,reference))+': '+(done.stdout+done.stderr)[-8000:])
                        pair.append(json.loads(done.stdout))
                    native,reference=pair;left=dict(native);right=dict(reference)
                    for key in('parent_native_calls','parent_handoffs','handoff_pc','parent_retained_statements'):left.pop(key);right.pop(key)
                    exact=left==right;differences=[key for key in left if left[key]!=right[key]]
                    record=dict(declared_feedback_schedule=feedback,policy=policy,native=native,reference=reference,full_exact_equality=exact,differing_fields=differences);rows.append(record);report['compilers']=results+[dict(compiler=compiler,commands=commands,rows=rows)]
                    if feedback and not exact:raise ValueError('Controlled actual World Arena differs in '+repr(differences))
                    if not feedback:
                        for key in('escaped_abi','files','real_worker_retired','last_instruction','actual_guard_changes','wider_required_calls'):
                            if left[key]!=right[key]:raise ValueError('Actual empty-schedule World Arena invariant differs '+key)
            results.append(dict(compiler=compiler,commands=commands,rows=rows))
        report.update(passed=True,compilers=results,scope='Pinned actual World callback/cursor/caller arms in a declared receiver; complete Arena cold runs with genuine existing physical/file/worker/CRC owners. Controlled schedule pairs compare every bank/ABI/trace/resource field. Empty schedule pairs retain all actual outcomes and explicitly record variation; no whole-World/cache construction, production issuer, all-native children or runtime claim.')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={n:sha(ROOT/n)for n in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        a.report.parent.mkdir(parents=True,exist_ok=True);data=(json.dumps(report,indent=2)+'\n').encode()
        if len(data)>1_048_576:
            from native_metadata import retain_bytes
            receipt=retain_bytes(data,a.report,a.report.parent)
            (a.report.parent/'storage.json').write_text(json.dumps(dict(passed=True,lossless_original_bytes_preserved=True,receipt=receipt,retained_large_artifacts=[]),indent=2)+'\n')
        else:a.report.write_bytes(data)
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
