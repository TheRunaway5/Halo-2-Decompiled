#!/usr/bin/env python3
"""Check complete Sandbox dispatch preservation and the actual owner ABI."""
import argparse,hashlib,json,os,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import native_sandbox_dispatch_preservation as preserve
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    files=('tools/native_sandbox_dispatch_preservation.py','tools/generate_sandbox_special_tu15.py','tools/generate_physics_live_support_tu15.py','tests/native_sandbox_dispatch_tu15_tests.cpp','src/gameplay/objects/source_sandbox_special_tu15_generated.cpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','tools/native_model_marker_lookup_replacements.py','tools/validate_native_sandbox_dispatch_tu15.py')
    pins={name:digest(ROOT/name)for name in files};report=dict(passed=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    headers={path.resolve():digest(path)for tree in('src','include')for path in(ROOT/tree).rglob('*')if path.suffix in('.h','.hpp','.inc')}
    try:
        canonical=(ROOT/files[4]).read_text();generated=scratch/'sandbox.cpp';command=[sys.executable,'-B',str(ROOT/files[1]),'--output',str(generated),'--directory',str(scratch/'generation')]
        done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-5000:]
        source=generated.read_text();old=preserve.dispatcher(canonical)[0];fixed=preserve.dispatcher(source)[0]
        from native_selected_operand_binding import require_selected_identity
        require_selected_identity(canonical,old,{preserve.RESOLVER_PIN})
        assert preserve.mappings(old)==preserve.mappings(fixed)and preserve.preserve(source)==source and preserve.preserve(canonical)==canonical
        raw=fixed.replace(preserve.PRESERVED_CASE,preserve.RAW_CASE,1);assert preserve.digest(raw)==preserve.RAW_PIN
        labels=sorted(preserve.mappings(fixed));stubs=''.join(f'static void run_{target:x}(WorldState&s){{throw CalleeStop{{0x{target:x}u,snapshot(s)}};}}\n'for target in labels)
        from native_model_marker_lookup_replacements import function
        body_pins={}
        for target in labels:
            name=f'run_{target:x}';before=function(canonical,name);after=function(source,name)
            require_selected_identity(canonical,before,{preserve.RESOLVER_PIN})
            projected=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',before)
            if projected!=after:raise ValueError('Fresh Sandbox complete target body changed '+name)
            body_pins[name]=dict(canonical=preserve.digest(before),literal=preserve.digest(after))
        fragment_sites=((0x108080,0x1088e5),(0x15e1e0,0x15e326),(0x15f8b0,0x15fdf4),(0x15f8b0,0x1600ad))
        fragments=[]
        for index,(entry,pc)in enumerate(fragment_sites):
            before=function(canonical,f'run_{entry:x}');after=function(source,f'run_{entry:x}')
            before=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',before)
            old_line=next(line for line in before.splitlines()if line.startswith(f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}'))
            new_line=next(line for line in after.splitlines()if line.startswith(f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}'))
            historical=new_line
            for old_text,new_text in preserve.BODY_CHANGES[f'{entry:x}']:
                if new_text in historical:historical=historical.replace(new_text,old_text,1)
            fragments.append((index,old_line,new_line,historical))
        # Preserve the failed old switch as a distinct provenance input. Only
        # its exact missing owner branch is accepted by the repair.
        rejected=0
        mutations=(
            (raw,raw.replace(preserve.RAW_CASE,'case 0xcabc0u:run_10eeb0(s);return;',1)),
            (raw,raw.replace('default:s.required(site,target,tail);','case 0xdead1234u:run_10eeb0(s);return;default:s.required(site,target,tail);',1)),
            (raw,raw.replace('default:s.required(site,target,tail);','default:s.required(site+1u,target,tail);',1)),
            (fixed,fixed.replace(preserve.PRESERVED_CASE,preserve.RAW_CASE+preserve.RAW_CASE,1)),
            (old,old.replace('s.required(site,target,tail);','s.required(site,target,false);',1)),
        )
        for original,changed in mutations:
            if changed==original:raise ValueError('Sandbox drift control did not mutate its witness')
            trial=source.replace(fixed,changed,1)if original!=old else canonical.replace(old,changed,1)
            try:preserve.preserve(trial)
            except ValueError:rejected+=1
            else:raise ValueError('Changed complete Sandbox dispatch was accepted')
        resolver_text=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        trial=source.replace(resolver_text,resolver_text.replace('return p;','return p+1u;',1),1)
        if trial==source:raise ValueError('Sandbox resolver mutation did not mutate its witness')
        try:preserve.preserve(trial)
        except ValueError:rejected+=1
        else:raise ValueError('Changed complete Sandbox resolver was accepted')
        for name in preserve.BODY_CONTEXTS:
            body=function(source,'run_'+name);changed=body.replace('s.services.instruction','s.services.instruction_changed',1)
            if changed==body:raise ValueError('Sandbox target body mutation witness missing')
            try:preserve.preserve(source.replace(body,changed,1))
            except ValueError:rejected+=1
            else:raise ValueError('Changed complete Sandbox target body accepted '+name)
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',canonical,re.S)[0]
        fixture='namespace canonical {\n'+resolver+'\n'+stubs+old+'\n}\nnamespace preserved {\n'+stubs+fixed+'\n}\nnamespace raw {\n'+stubs+raw+'\n}\n'
        for variant in range(3):
            fixture+=f'namespace direct{variant} {{\nusing canonical::run_cabc0;\nusing canonical::dispatch;\n'
            fixture+=''.join(f'static void call_{index}(WorldState&s){{{rows[variant]}}}\n'for index,*rows in fragments)
            fixture+='}\n'
        (scratch/'sandbox_dispatchers.inc').write_text(fixture)
        outcomes=[]
        syntax=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/(Path(compiler).name+'-routing')
            compile_dep=scratch/(Path(compiler).name+'-routing.d')
            compile=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(compile_dep),str(ROOT/files[3]),'-o',str(binary)]
            if args.sanitizers:compile+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(compile,capture_output=True,text=True);assert done.returncode==0,done.stderr[-7000:]
            done=subprocess.run([str(binary)],capture_output=True,text=True);assert done.returncode==0,(done.stdout+done.stderr)[-7000:]
            outcomes.append(dict(compiler=compiler,actual=json.loads(done.stdout)))
            syntax_dep=scratch/(Path(compiler).name+'-syntax.d')
            command_syntax=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(syntax_dep),str(generated)]
            done=subprocess.run(command_syntax,capture_output=True,text=True,timeout=300);assert done.returncode==0,done.stderr[-7000:]
            for dependency_file in(compile_dep,syntax_dep):
                for value in shlex.split(dependency_file.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(value).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            syntax.append(dict(compiler=compiler,whole_generated_unit=True))
        report.update(passed=True,unique_targets=len(labels),complete_dispatch_clauses_equal=True,complete_target_bodies_equal_after_authenticated_identity_projection=body_pins,fallback_arguments_equal=True,strict_dispatch_resolver_mutants_rejected=rejected,canonical_dispatcher=preserve.CANONICAL_PIN,historical_raw_semantic_drift=preserve.RAW_PIN,preserved_dispatcher=preserve.PRESERVED_PIN,compilers=outcomes,standalone_syntax=syntax,scope='Complete routing map/body projection and real State.required entry ABI; callee algorithms/outcomes and physical fault behavior are not qualified by this regression')
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['source_after']={name:digest(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
