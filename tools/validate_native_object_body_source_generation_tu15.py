#!/usr/bin/env python3
"""Private current/fresh UnitView lookup contexts from the unchanged TU1.5 image."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from specialize_native_operands import specialize
from validate_native_object_body_source_frame_tu15 import UNIT,BODY_PIN
ROOT=Path(__file__).resolve().parents[1]
GENERATOR='tools/generate_source_unit_view_reset_tu15.py'
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def one(source,pattern,name):
    values=re.findall(pattern,source,re.M|re.S)
    if len(values)!=1:raise ValueError('Missing/ambiguous complete UnitView '+name)
    return values[0]
def parts(source):
    return {
        'body':one(source,r'^static void run_bcdf0\(WorldState &s\) \{.*?^\}','lookup'),
        'resolver':one(source,r'^std::uint32_t selected\([^\n]+','resolver'),
        'dispatcher':one(source,r'^static void dispatch\([^\n]+','dispatcher'),
        'state':one(source,r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;','state'),
        'sign_extend':one(source,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}','sign extension'),
        'public_entry':one(source,r'^void source_run_unit_view_reset_tu15\(.*?^\}','public entry'),
        'owns_entry':one(source,r'^bool source_unit_view_reset_tu15_owns_entry\([^\n]+','public ownership'),
    }
def canonical_body(value):
    return re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',
        r'if(s.services.instruction){s.services.instruction(\1);}',value)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--candidate',action='store_true')
    p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    files=[UNIT,GENERATOR,'tools/generate_physics_live_support_tu15.py',
        'tools/specialize_native_operands.py','tools/validate_native_object_body_source_generation_tu15.py',
        'tools/validate_native_object_body_source_frame_tu15.py']
    if args.candidate:files+=['tools/native_object_body_replacements.py','src/gameplay/objects/native_tu15_object_body_source_owner.hpp']
    pins={name:digest((ROOT/name).read_text())for name in files}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),
        source_before=pins,retained_large_artifacts=[],scope=__doc__)
    try:
        current=(ROOT/UNIT).read_text();fresh=scratch/'fresh.cpp';manifest=scratch/'fresh.json'
        command=[os.sys.executable,'-B',str(ROOT/GENERATOR),'--output',str(fresh),'--manifest',str(manifest)]
        done=subprocess.run(command,capture_output=True,text=True)
        if done.returncode:raise ValueError('Genuine UnitView regeneration failed: '+done.stderr[-7000:])
        generation=json.loads(manifest.read_text())
        if generation['original_sha256']!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
            raise ValueError('Fresh UnitView image identity mismatch')
        import native_object_body_replacements as binding
        actual_sources=[current,fresh.read_text()]
        if args.require_binding:
            for value in actual_sources:
                if binding.replace_object_body(value)!=value:raise ValueError('Actual current/fresh object lookup default is not installed')
                binding.installed_entries(value)
        values=[binding.remove_object_body(value)for value in actual_sources];values+=[specialize(value)[0]for value in values]
        labels=['current','fresh','current-prepared','fresh-prepared']
        if args.candidate:
            proposals=[]
            for value in values:
                proposed=binding.replace_object_body(value)
                if proposed==value or binding.replace_object_body(proposed)!=proposed or binding.remove_object_body(proposed)!=value:
                    raise ValueError('Private complete object lookup candidate reconstruction failed')
                proposals.append(proposed)
            values+=proposals;labels+=[name+'-candidate'for name in labels]
        contexts=[]
        isolated_helper=scratch/'isolated-object-body-binding.py'
        if args.candidate:isolated_helper.write_text((ROOT/'tools/native_object_body_replacements.py').read_text())
        for n,value in enumerate(values):
            owner=parts(value)
            body=binding.original_body(owner['body'])if binding else owner['body']
            if digest(canonical_body(body))!=BODY_PIN:
                raise ValueError('Complete current/fresh lookup semantic body drift')
            # The genuine target dispatcher must own this actual TU1.5 entry.
            if 'case 0xbcdf0u:run_bcdf0(s);return;'not in owner['dispatcher']:
                raise ValueError('Actual UnitView dispatcher ownership changed')
            if specialize(value)[0]!=value and n%4>=2:raise ValueError('Repeated preparation drift')
            controls={}
            if args.candidate:
                for name,part in owner.items():
                    at=part.index('{')+1
                    changed=value.replace(part,part[:at]+'/* exact context drift */'+part[at:],1)
                    try:binding.authenticate(changed)
                    except ValueError as error:controls[name]=str(error)
                    else:raise ValueError('Private object lookup context drift accepted: '+name)
                if binding.entry_prefix(owner['body']) is not None:
                    for pc in binding.FRONTIERS:
                        label=f'NativeObjectBody{pc:x}: '
                        entry='s.instruction_literal(0xbcdf0u);'if 's.instruction_literal(0xbcdf0u);'in owner['body']else'if(s.services.instruction){s.services.instruction(0xbcdf0u);}'
                        changed=value.replace(label,'',1).replace(entry,label+entry,1)
                        try:binding.authenticate(changed)
                        except ValueError as error:controls['relocated_'+f'{pc:x}']=str(error)
                        else:raise ValueError('Relocated object lookup continuation accepted: '+hex(pc))
            target=scratch/f'context-{n}.cpp';target.write_text(value)
            if args.candidate:
                isolated_output=scratch/f'isolated-context-{n}.cpp'
                script='import importlib.util,sys;from pathlib import Path;s=importlib.util.spec_from_file_location("isolated_binding",sys.argv[1]);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);Path(sys.argv[3]).write_text(m.replace_object_body(Path(sys.argv[2]).read_text()))'
                copied=subprocess.run([os.sys.executable,'-I','-c',script,str(isolated_helper),str(target),str(isolated_output)],cwd=scratch,capture_output=True,text=True)
                if copied.returncode or isolated_output.read_text()!=binding.replace_object_body(value):
                    raise ValueError('Copied isolated source binder is not self-contained/idempotent: '+copied.stderr[-5000:])
            compilers=[]
            for compiler in args.compiler or('g++','clang++'):
                syntax=[compiler,'-std=c++20','-Wno-ignored-attributes','-fsyntax-only',
                    '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(target)]
                checked=subprocess.run(syntax,capture_output=True,text=True)
                if checked.returncode:raise ValueError('Complete UnitView syntax failed: '+checked.stderr[-7000:])
                compilers.append(dict(compiler=compiler,command=syntax,passed=True))
            contexts.append(dict(variant=labels[n],
                whole_source_sha256=digest(value),complete_context_sha256={key:digest(part)for key,part in owner.items()},
                whole_cpp_syntax=compilers,strict_context_refusals=controls,
                isolated_copied_binder_passed=args.candidate))
        report.update(passed=True,contexts=contexts,genuine_generation_command=command,
            generator_manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest(),
            original_sha256=generation['original_sha256'],complete_source_contexts=len(values),
            actual_installed_sources_sha256=[digest(value)for value in actual_sources],
            installed_binding_required=args.require_binding,
            authority_limit='Exact body/context identities and full syntax; no source admission/default install or original return supplied')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:digest((ROOT/name).read_text())for name in files}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
