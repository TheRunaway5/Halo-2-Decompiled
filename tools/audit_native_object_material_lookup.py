#!/usr/bin/env python3
"""Private next-owner discovery for the exact object-material BCDF0 helper."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from specialize_native_operands import specialize
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/gameplay/objects/source_object_material_tu15_generated.cpp'
GENERATOR='tools/generate_object_material_tu15.py'
REFERENCE='analysis/weapons-tu15-goal-20261013/hunter-public-contact-original-v1'
BODY_PIN='85928a501bfc9101c77ec90ea28bad7f671178173909f4a52220e06e3fc3320c'
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def one(source,pattern):
    matches=re.findall(pattern,source,re.M|re.S)
    if len(matches)!=1:raise ValueError('Missing/ambiguous complete object-material context')
    return matches[0]
def parts(source):
    return {
        'body':one(source,r'^static void run_bcdf0\(WorldState &s\) \{.*?^\}'),
        'resolver':one(source,r'^std::uint32_t selected\([^\n]+'),
        'dispatcher':one(source,r'^static void dispatch\([^\n]+'),
        'state':one(source,r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;'),
        'sign_extend':one(source,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}'),
        'public_entry':one(source,r'^void source_run_object_material_tu15\(.*?^\}'),
        'owns_entry':one(source,r'^bool source_object_material_tu15_owns_entry\([^\n]+'),
    }
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--candidate',action='store_true');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    files=[UNIT,GENERATOR,'tools/generate_physics_live_support_tu15.py',
        'tools/specialize_native_operands.py','tools/audit_native_object_material_lookup.py',
        REFERENCE+'/report.json',REFERENCE+'/hardware/report.json']
    if a.candidate:files+=['tools/native_object_material_lookup_replacements.py','src/gameplay/objects/native_tu15_object_material_lookup_source_owner.hpp','tools/native_object_body_replacements.py']
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in files}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),
        source_before=pins,retained_large_artifacts=[],scope=__doc__)
    try:
        if pins[REFERENCE+'/report.json']!='5055f0507759fba420c7df96ba2c0b72db67002c362e62dbe3c26f54b7ef7451':
            raise ValueError('Object-material actual-caller reference changed')
        fresh=scratch/'fresh.cpp';command=[os.sys.executable,'-B',str(ROOT/GENERATOR),
            '--reference',str(ROOT/REFERENCE),'--output',str(fresh),'--directory',str(scratch/'generation')]
        generated=subprocess.run(command,capture_output=True,text=True)
        if generated.returncode:raise ValueError('Genuine object-material generation failed: '+generated.stderr[-7000:])
        import native_object_material_lookup_replacements as binding
        actual_sources=[(ROOT/UNIT).read_text(),fresh.read_text()]
        if a.require_binding:
            for value in actual_sources:
                if binding.replace_object_material_lookup(value)!=value:raise ValueError('Current/fresh material lookup default missing')
                binding.installed_entries(value)
        values=[binding.remove_object_material_lookup(value)for value in actual_sources]
        values+=[specialize(v)[0]for v in values];contexts=[]
        names=['current','fresh','current-prepared','fresh-prepared']
        if a.candidate:
            proposals=[]
            for value in values:
                proposed=binding.replace_object_material_lookup(value)
                if proposed==value or binding.replace_object_material_lookup(proposed)!=proposed or binding.remove_object_material_lookup(proposed)!=value:
                    raise ValueError('Private material lookup reconstruction failed')
                proposals.append(proposed)
            values+=proposals;names+=[name+'-candidate'for name in names]
        projections=[]
        for n,value in enumerate(values):
            owner=parts(value);body=binding.canonical_body(owner['body'])if binding else owner['body']
            body=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',body)
            if n==0 and digest(body)!=BODY_PIN:raise ValueError('Complete current object-material lookup body drift')
            used=set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)',body))
            resolver=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',owner['resolver']))
            for operand in used:
                if resolver.get(operand)!=operand:raise ValueError('Nonidentity object-material operand '+operand)
            projected=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',body)
            if projections and projected!=projections[0]:raise ValueError('Complete identity-only material body projection differs in context '+str(n))
            projections.append(projected)
            refusals={}
            if a.candidate:
                for name,part in owner.items():
                    at=part.index('{')+1;changed=value.replace(part,part[:at]+'/* context drift */'+part[at:],1)
                    try:binding.authenticate(changed)
                    except ValueError as error:refusals[name]=str(error)
                    else:raise ValueError('Material context drift accepted '+name)
                if binding.entry_prefix(owner['body']) is not None:
                    for pc in binding.FRONTIERS:
                        label=f'NativeObjectBody{pc:x}: '
                        entry='s.instruction_literal(0xbcdf0u);'if 's.instruction_literal(0xbcdf0u);'in owner['body']else'if(s.services.instruction){s.services.instruction(0xbcdf0u);}'
                        changed=value.replace(label,'',1).replace(entry,label+entry,1)
                        try:binding.authenticate(changed)
                        except ValueError as error:refusals['relocated_'+f'{pc:x}']=str(error)
                        else:raise ValueError('Material continuation relocation accepted')
            target=scratch/f'context-{n}.cpp';target.write_text(value);checks=[]
            for compiler in('g++','clang++'):
                syntax=[compiler,'-std=c++20','-Wno-ignored-attributes','-fsyntax-only',
                    '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(target)]
                checked=subprocess.run(syntax,capture_output=True,text=True)
                if checked.returncode:raise ValueError('Complete object-material syntax failed: '+checked.stderr[-7000:])
                checks.append(dict(compiler=compiler,command=syntax,passed=True))
            contexts.append(dict(variant=names[n],
                whole_source_sha256=digest(value),complete_context_sha256={key:digest(part)for key,part in owner.items()},
                identity_only_body_projection_sha256=digest(projected),exact_identity_operands=sorted(used),
                whole_cpp_syntax=checks,strict_context_refusals=refusals))
        report.update(passed=True,contexts=contexts,genuine_generation_command=command,
            original_caller='d1878',lookup_actual_entry='bcdf0',public_parent_entry='18b4c0',
            actual_source_sha256=[digest(value)for value in actual_sources],production_bound=a.require_binding,
            admission_and_binding_qualified=False)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in files}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
