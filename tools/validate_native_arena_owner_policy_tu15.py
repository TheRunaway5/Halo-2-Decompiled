#!/usr/bin/env python3
"""Qualify a private actual Arena producer copy; production remains unchanged."""
import argparse,hashlib,json,os,platform,re,shlex,subprocess,sys
from pathlib import Path
from native_arena_owner_policy_prototype import prototype,prototype_header,SERVICES_PIN,HEADER_PIN,RUN_PIN,OLD_CALL,PUBLISHED_CALL
from validate_native_arena_initialize_owner_tu15 import SOURCES
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--source-routes',action='store_true');p.add_argument('--uncontrolled-reference-controls',action='store_true');p.add_argument('--memory-revocation-control',action='store_true');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if a.require_binding and not a.source_routes:p.error('Installed binding requires genuine source routes')
    if a.uncontrolled_reference_controls and not a.source_routes:p.error('The scheduling control requires genuine source routes')
    if a.memory_revocation_control and not a.source_routes:p.error('The bank guard control requires genuine source routes')
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);driver='tests/native_tu15_arena_owner_policy_tests.cpp';files=(driver,'tools/native_arena_owner_policy_prototype.py','tools/validate_native_arena_owner_policy_tu15.py','tools/validate_native_arena_initialize_owner_tu15.py','tools/validate_native_arena_initialize_frame_tu15.py','src/physics/runtime/native_tu15_arena_initialize_owner.hpp','src/physics/runtime/native_tu15_arena_publication.hpp','include/halo2/physics_game_arena_native.hpp',*SOURCES[1:]);pins={name:sha(ROOT/name)for name in files};headers={x.resolve():sha(x)for folder in('include','src')for x in(ROOT/folder).rglob('*')if x.suffix in('.h','.hpp','.inc')}
    report=dict(passed=False,private_unbound=True,production_policy_issued=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        import native_arena_initialize_replacements as binding
        pins['tools/native_arena_initialize_replacements.py']=sha(ROOT/'tools/native_arena_initialize_replacements.py')
        name='src/physics/physics_game_arena_native.cpp';current_owner=(ROOT/name).read_text();original=binding.canonical_owner(current_owner);candidate=prototype(original)
        if a.require_binding:
            current_generated=(ROOT/'src/physics/physics_game_arena_runtime_generated.cpp').read_text();current_bootstrap=(ROOT/'src/physics/native_tu15_arena_bootstrap.cpp').read_text()
            if binding.replace_arena_owner(current_owner)!=current_owner or binding.replace_arena_initialize(current_generated)!=current_generated or binding.replace_arena_bootstrap(current_bootstrap)!=current_bootstrap:raise ValueError('Required installed Arena producer/source/bootstrap missing')
        from native_arena_owner_policy_prototype import services,INCLUDES
        restored=candidate;start=restored.index('  struct RequiredOwner {');end=restored.index('  bool platform(',start)
        restored=restored[:start]+services(original)+restored[end:]
        restored=restored.replace('  const std::thread::id parent_executor=std::this_thread::get_id();\n','',1)
        restored=restored.replace(PUBLISHED_CALL,OLD_CALL,1)
        if not restored.startswith(INCLUDES)or restored[len(INCLUDES):]!=original:raise ValueError('Private producer changed unrelated actual owner bytes')
        auth=[]
        for label,old,new in(('bank-guard','require(&m==&shared,','require(true,'),('target','if(call.target==0x21b920u)','if(call.target==0x21b921u)'),('lower-forward','lower.required_owner(m,call,abi,o);','lower.required_owner(m,call,abi,{});')):
            if original.count(old)!=1:raise ValueError('Missing strict producer drift anchor '+label)
            try:prototype(original.replace(old,new,1))
            except ValueError:auth.append(dict(name=label,rejected=True))
            else:raise ValueError('Changed complete actual required body accepted '+label)
        try:prototype(original.replace('  SourcePhysicsGameArenaConfiguration config;\n','  SourcePhysicsGameArenaConfiguration config;\n  SourcePhysicsGameArenaConfiguration config;\n',1))
        except ValueError:auth.append(dict(name='ambiguous-field',rejected=True))
        else:raise ValueError('Ambiguous producer field accepted')
        if original.count('impl_->services(),impl_->observe);')!=1:raise ValueError('Exact synchronous call mutation anchor differs')
        try:prototype(original.replace('impl_->services(),impl_->observe);','impl_->services(std::stop_token{}),impl_->observe);',1))
        except ValueError:auth.append(dict(name='synchronous-publication-caller',rejected=True))
        else:raise ValueError('Changed synchronous issuer call scope accepted')
        public=scratch/'include/halo2/physics_game_arena_native.hpp';public.parent.mkdir(parents=True);public.write_text(prototype_header((ROOT/'include/halo2/physics_game_arena_native.hpp').read_text()))
        declaration='namespace halo2 {void native_arena_policy_probe(const SourcePhysicsWorldRuntimeServices&,const void*);}\n'
        include='#include "halo2/physics_game_arena_native.hpp"\n'
        candidate=candidate.replace(include,include+declaration,1);needle='    joined.required_owner_policy={&joined.required_owner.target_type(),parent_executor,this,&RequiredOwner::matches};\n'
        if candidate.count(needle)!=1:raise ValueError('Private actual producer probe boundary drift')
        candidate=candidate.replace(needle,needle+'    native_arena_policy_probe(joined,this);\n',1)
        generated=None
        if a.source_routes:
            from validate_native_arena_initialize_frame_tu15 import body,UNITS
            source_name='src/physics/physics_game_arena_runtime_generated.cpp';raw=binding.remove_arena_initialize((ROOT/source_name).read_text());parent=body(raw)
            if hashlib.sha256(parent.encode()).hexdigest()!=UNITS[source_name]:raise ValueError('Complete actual parent body drift')
            exported=raw[raw.index('void source_run_physics_game_arena_runtime('):]
            if hashlib.sha256(exported.encode()).hexdigest()!='4543981d9d679e199c1635f1b1f5d1d5e29d6db26dec69d92b603effb527dd4a':raise ValueError('Complete synchronous generated export drift')
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',raw,re.S)[0]
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',raw)[0]
            if hashlib.sha256(resolver.encode()).hexdigest()!='ff495ade2e6eb33688d00bae80af91364ff47d20914d31c36d283928ae73bbae'or hashlib.sha256(dispatcher.encode()).hexdigest()!='151bef387336d665aa7ba909bf0996dabc061036415e327616763d104f0b3a6d':raise ValueError('Whole original parent dispatch/resolver drift')
            measured,count=re.subn(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',r'halo2::native_arena_policy_retained(\1);if(s.services.instruction){s.services.instruction(\1);}',parent)
            if count!=45:raise ValueError('Complete parent statement counter scope differs')
            repeated='if(s.services.instruction)s.services.instruction(0x1252b8u);'
            if measured.count(repeated)!=1:raise ValueError('Authentic repeated statement counter differs')
            measured=measured.replace(repeated,'halo2::native_arena_policy_retained(0x1252b8u);'+repeated,1)
            opening=measured.index('{')+1;prefix='\n  halo2::native_arena_policy_before_parent(s.services);\n  if(halo2::native_arena_initialize_owner_detail::bridge(s,[](auto&current,auto target,auto site,bool tail){dispatch(current,target,site,tail);}))return;'
            measured=measured[:opening]+prefix+measured[opening:]
            generated=scratch/'arena-source-prototype.cpp';generated.write_text('#include "physics/runtime/native_tu15_arena_initialize_owner.hpp"\nnamespace halo2 {void native_arena_policy_before_parent(const SourcePhysicsWorldRuntimeServices&);void native_arena_policy_retained(std::uint32_t);}\n'+raw.replace(parent,measured,1))
            candidate=candidate.replace(declaration,declaration+'namespace halo2 {void native_arena_policy_io(bool);}\n',1)
            for signature,write in(('    void read(std::uint32_t p, std::span<std::byte> b) const override {\n',False),('    void write(std::uint32_t p, std::span<const std::byte> b) override {\n',True)):
                if candidate.count(signature)!=1:raise ValueError('Exact SharedMemory counter boundary differs')
                candidate=candidate.replace(signature,signature+'      native_arena_policy_io('+str(write).lower()+');\n',1)
        owned=scratch/'arena-owner-prototype.cpp';owned.write_text(candidate);results=[]
        for compiler in a.compiler or('g++','clang++'):
            folder=scratch/Path(compiler).name;folder.mkdir();flags=['-std=c++20','-O1'if a.sanitizers else'-O2','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(scratch/'include'),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src')]
            if a.sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            if a.source_routes:flags+=['-DHALO2_ARENA_SOURCE_ROUTES=1']
            objects=[];commands=[]
            for index,source in enumerate((driver,*SOURCES[1:])):
                unit=owned if source==name else generated if generated and source=='src/physics/physics_game_arena_runtime_generated.cpp'else ROOT/source;target=folder/(str(index)+'.o');dep=target.with_suffix('.d');command=[compiler,*flags,'-c',str(unit),'-o',str(target),'-MD','-MF',str(dep)];commands.append(command);done=subprocess.run(command,capture_output=True,text=True)
                if done.returncode:raise ValueError('Actual producer compile failed: '+done.stderr[-8000:])
                for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(value).resolve()
                    if path.is_relative_to(ROOT)and not path.is_relative_to(scratch)and path.is_file():pins[str(path.relative_to(ROOT))]=headers.get(path,pins.get(str(path.relative_to(ROOT)),sha(path)))
                objects.append(target)
            binary=folder/'policy-test';command=[compiler,*flags,*map(str,objects),'-Wl,--gc-sections','-pthread','-lcrypto','-o',str(binary)];commands.append(command);done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Actual producer link failed: '+done.stderr[-8000:])
            native_env=os.environ.copy();native_env.pop('HALO2_REFERENCE_ARENA_INITIALIZE',None)
            if a.memory_revocation_control:
                native_env['HALO2_ARENA_MEMORY_POLICY_REVOCATION']='1';reference_env=native_env.copy();reference_env['HALO2_REFERENCE_ARENA_INITIALIZE']='1'
                checked=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120,env=reference_env)
                report['memory_revocation_original_control']=dict(exit_code=checked.returncode,diagnostic=checked.stderr[-4000:],result=json.loads(checked.stdout)if checked.returncode==0 else None)
                if checked.returncode:raise ValueError('Genuine memory-revocation reference failed: '+checked.stderr[-4000:])
            done=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120,env=native_env)
            if done.returncode:raise ValueError('Actual producer mismatch: '+(done.stdout+done.stderr)[-8000:])
            result=dict(compiler=compiler,commands=commands,result=json.loads(done.stdout),diagnostic=done.stderr[-4000:],compiled_mutants=[])
            if a.source_routes:
                env=os.environ.copy();env['HALO2_REFERENCE_ARENA_INITIALIZE']='1';reference=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120,env=env)
                if reference.returncode:raise ValueError('Actual source reference failed: '+(reference.stdout+reference.stderr)[-8000:])
                reference_data=json.loads(reference.stdout);left=dict(result['result']['actual_cold_source']);right=dict(reference_data['actual_cold_source'])
                result['positive_reference']=reference_data;result['reference_diagnostic']=reference.stderr[-4000:];report['compilers']=results+[result]
                for ignored in('native_default','retained_parent_statements'):left.pop(ignored);right.pop(ignored)
                if left!=right:
                    report['cold_differences']={key:dict(native=left[key],reference=right[key])for key in left if left[key]!=right[key]}
                    differences=[]
                    for bank in left['banks']:
                        address=bank['address'];one=(scratch/'source-native-banks'/str(address)).read_bytes();other_path=scratch/'source-reference-banks'/str(address)
                        if not other_path.exists():continue
                        other=other_path.read_bytes()
                        for n in range(0,min(len(one),len(other)),4):
                            if one[n:n+4]!=other[n:n+4]:differences.append(dict(address=hex(address+n),native=one[n:n+4].hex(),reference=other[n:n+4].hex()))
                            if len(differences)>=64:break
                        if len(differences)>=64:break
                    report['cold_word_differences_first64']=differences
                    raise ValueError('Genuine native/reference cold resources or full ABI differ: '+repr(list(report['cold_differences'])))
                result['positive_reference']=reference_data;result['cold_native_reference_equal']=True
                if a.uncontrolled_reference_controls:
                    runs=[];identities=set();control_env=env.copy();control_env['HALO2_ARENA_UNCONTROLLED_COLD']='1'
                    for ordinal in range(4):
                        control=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120,env=control_env)
                        if control.returncode:raise ValueError('Genuine uncontrolled reference failed: '+(control.stdout+control.stderr)[-8000:])
                        value=json.loads(control.stdout)['actual_cold_source'];identity=hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest();identities.add(identity);runs.append(dict(run=ordinal,state_sha256=identity,actual_reference=value))
                        if len(identities)>1:break
                    result['uncontrolled_reference_schedule_control']=dict(runs=runs,unique_states=len(identities),variability_observed=len(identities)>1,scope='Complete original parent with empty schedule hooks. Every actual state/hash is retained; no differing queue words are excluded from the controlled native/reference comparison.')
            if a.semantic_mutants:
                definitions=(('unqualified-lower','native_required_owner_policy_detail::preserves_instruction_policy(lower)','true','Actual Arena producer policy domain differs'),('unqualified-schedule','native_arena_schedule_detail::preserves_parent_policy(config.schedule)','true','Actual Arena producer policy domain differs'),('foreign-capture','actual->owner==expected && actual->owner &&\n          actual->owner->preserves_parent_instruction_policy()','actual->owner && actual->owner->preserves_parent_instruction_policy()','Foreign Arena capture accepted'),('inherited-lower-certificate','joined.required_owner_policy={&joined.required_owner.target_type(),parent_executor,this,&RequiredOwner::matches};','joined.required_owner_policy=lower.required_owner_policy;','Actual Arena producer policy domain differs'))
                owner_index=(driver,*SOURCES[1:]).index(name)
                for label,old,new,witness in definitions:
                    if candidate.count(old)!=1:raise ValueError('Missing private producer mutation anchor '+label)
                    unit=folder/(label+'.cpp');unit.write_text(candidate.replace(old,new,1));target=unit.with_suffix('.o');command=[compiler,*flags,'-c',str(unit),'-o',str(target)];checked=subprocess.run(command,capture_output=True,text=True)
                    if checked.returncode:raise ValueError('Producer mutant failed to compile '+label+': '+checked.stderr[-5000:])
                    mutant_objects=list(objects);mutant_objects[owner_index]=target;binary=folder/(label+'-test');command=[compiler,*flags,*map(str,mutant_objects),'-Wl,--gc-sections','-pthread','-lcrypto','-o',str(binary)];checked=subprocess.run(command,capture_output=True,text=True)
                    if checked.returncode:raise ValueError('Producer mutant failed to link '+label+': '+checked.stderr[-5000:])
                    checked=subprocess.run([str(binary)],capture_output=True,text=True,timeout=30);diagnostic=(checked.stdout+checked.stderr)[-4000:]
                    if checked.returncode==0 or witness not in diagnostic:raise ValueError('Producer mutation survived/unrecognized '+label+': '+diagnostic)
                    result['compiled_mutants'].append(dict(name=label,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic))
                if a.source_routes:
                    relative='src/physics/runtime/native_tu15_arena_publication.hpp';header=(ROOT/relative).read_text()
                    definitions=(('foreign-memory','current->memory_==&memory','true','Foreign Arena memory accepted in live scope'),('copied-publication','current->services_==&services','true','Copied Arena table accepted in live scope'))
                    for label,old,new,witness in definitions:
                        if header.count(old)!=1:raise ValueError('Missing private publication mutation anchor '+label)
                        overlay=folder/label;target_header=overlay/relative;target_header.parent.mkdir(parents=True);target_header.write_text(header.replace(old,new,1));mutant_objects=list(objects)
                        for source in(driver,name,'src/physics/physics_game_arena_runtime_generated.cpp'):
                            index=(driver,*SOURCES[1:]).index(source);unit=owned if source==name else generated if source.endswith('runtime_generated.cpp')else ROOT/source;target=overlay/(str(index)+'.o');command=[compiler,'-I'+str(overlay/'src'),*flags,'-c',str(unit),'-o',str(target)];checked=subprocess.run(command,capture_output=True,text=True)
                            if checked.returncode:raise ValueError('Publication mutant failed to compile '+label+': '+checked.stderr[-5000:])
                            mutant_objects[index]=target
                        binary=overlay/'test';command=[compiler,*flags,*map(str,mutant_objects),'-Wl,--gc-sections','-pthread','-lcrypto','-o',str(binary)];checked=subprocess.run(command,capture_output=True,text=True)
                        if checked.returncode:raise ValueError('Publication mutant failed to link '+label+': '+checked.stderr[-5000:])
                        checked=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120);diagnostic=(checked.stdout+checked.stderr)[-4000:]
                        if checked.returncode==0 or witness not in diagnostic:raise ValueError('Publication mutation survived/unrecognized '+label+': '+diagnostic)
                        result['compiled_mutants'].append(dict(name=label,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic,consistent_header_overlay=True))
                    relative='src/physics/runtime/native_tu15_arena_initialize_owner.hpp';header=(ROOT/relative).read_text()
                    definitions=(('missing-owned-memory','!native_arena_publication_detail::Borrow::qualified(s.memory,s.services)||','false||'),)
                    source_index=(driver,*SOURCES[1:]).index('src/physics/physics_game_arena_runtime_generated.cpp')
                    for label,old,new in definitions:
                        if header.count(old)!=1:raise ValueError('Missing source admission mutation '+label)
                        overlay=folder/label;target_header=overlay/relative;target_header.parent.mkdir(parents=True);target_header.write_text(header.replace(old,new,1));target=overlay/'source.o';command=[compiler,'-I'+str(overlay/'src'),*flags,'-c',str(generated),'-o',str(target)];checked=subprocess.run(command,capture_output=True,text=True)
                        if checked.returncode:raise ValueError('Source admission mutant failed to compile '+label+': '+checked.stderr[-5000:])
                        mutant_objects=list(objects);mutant_objects[source_index]=target;binary=overlay/'test';command=[compiler,*flags,*map(str,mutant_objects),'-Wl,--gc-sections','-pthread','-lcrypto','-o',str(binary)];checked=subprocess.run(command,capture_output=True,text=True)
                        if checked.returncode:raise ValueError('Source admission mutant failed to link '+label+': '+checked.stderr[-5000:])
                        checked=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120);diagnostic=(checked.stdout+checked.stderr)[-4000:]
                        if checked.returncode==0 or'Source fallback skipped first retained statement'not in diagnostic:raise ValueError('Actual source admission mutant survived/unrecognized '+label+': '+diagnostic)
                        result['compiled_mutants'].append(dict(name=label,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic,actual_first_retained_fallback_checked=True))
            results.append(result)
        report.update(passed=True,private_unbound=not a.require_binding,installed_binding_required=a.require_binding,compilers=results,strict_auth_controls=auth,unrelated_bytes_preserved=True,original_services_body_sha256=SERVICES_PIN,original_synchronous_call_scope_sha256=RUN_PIN,original_public_declaration_sha256=HEADER_PIN,private_public_declaration_sha256=sha(public),private_compiled_source_sha256=sha(owned),issuer_scope_evidence=dict(const_local_table=True,private_constructor_friends_only_actual_owner=True,public_header_unchanged=sha(public)==HEADER_PIN,no_reverse_platform_library_symbol=True,exact_table_address_required=True,copy_rejected=True,nested_restore_and_exception_retirement_checked=a.source_routes,dispatcher_captures_only_immutable_impl_and_stop=True,generated_synchronous_export_sha256='4543981d9d679e199c1635f1b1f5d1d5e29d6db26dec69d92b603effb527dd4a'),scope='Owned disposable exact Arena producer and actual compiled source route, const scoped Services publication and captured SharedMemory, callback-free initialization, genuine cold default/reference resources and bank/frame refusals. Callback-bearing World, wider native child dependencies and engine/runtime closure remain open.')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
