#!/usr/bin/env python3
"""Qualify exact TypeQuery candidate overlays in whole current/fresh contexts."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
import native_object_type_query_replacements as binding
from specialize_native_operands import specialize
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(UNIT,'tools/generate_physics_live_support_tu15.py','tools/specialize_native_operands.py',
        'tools/native_object_type_query_replacements.py','tools/audit_native_object_type_query_candidates.py')
    pins={name:sha(ROOT/name)for name in names}
    report=dict(passed=False,production_bound=a.require_binding,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        fresh=scratch/'fresh.cpp'
        command=[sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'generation.json')]
        done=subprocess.run(command,capture_output=True,text=True,timeout=1200)
        if done.returncode:raise ValueError('Whole Primary generation failed '+(done.stdout+done.stderr)[-6000:])
        actual=[(ROOT/UNIT).read_text(),fresh.read_text()]
        if a.require_binding:
            for value in actual:binding.installed_entries(value)
        raw=[binding.remove_object_type_query(value)for value in actual]
        sources=raw+[specialize(value)[0]for value in raw]
        labels=('current-raw','fresh-raw','current-prepared','fresh-prepared');contexts=[]
        for label,source in zip(labels,sources,strict=True):
            parts=binding.parts(source);candidate=binding.replace_object_type_query(source)
            if binding.remove_object_type_query(candidate)!=source or binding.replace_object_type_query(candidate)!=candidate:
                raise ValueError('TypeQuery exact inverse/idempotence failed')
            binding.installed_entries(candidate)
            negatives=[]
            def refused(name,bad):
                try:binding.replace_object_type_query(bad)
                except ValueError as error:negatives.append(dict(name=name,error=str(error)));return
                raise ValueError('TypeQuery candidate accepted '+name)
            body=binding.parts(candidate)['body'];prefix=binding.entry_prefix(body)
            refused('wrong-format-tag',candidate.replace(prefix,binding.prefix(not binding.callback_format(body)),1))
            refused('external-prefix',candidate+'\n'+prefix+'\n')
            refused('duplicate-include',binding.INCLUDE+candidate)
            refused('moved-prefix',candidate.replace(prefix,'',1).replace('s.gp',prefix+'s.gp',1))
            first=f'NativeObjectTypeQuery{binding.FRONTIERS[0]:x}: '
            refused('missing-label',candidate.replace(first,'',1))
            refused('external-label',candidate+'\n'+first+'\n')
            refused('missing-include',candidate.replace(binding.INCLUDE,'',1))
            refused('wrong-root',candidate.replace('0x4ecbd0u','0x4ecbd4u',1))
            for field in('resolver','dispatcher','state','sign_extend','public_entry','owns_entry'):
                changed=parts[field].replace('{','{ ',1)if '{'in parts[field]else parts[field]+' '
                refused('changed-'+field,candidate.replace(parts[field],changed,1))
            # Real current/fresh contexts may share one complete field. Reject
            # every actual mixed tuple outside the authenticated profile set.
            for other in sources:
                values=binding.parts(other)
                for field in('dispatcher','state','public_entry','owns_entry'):
                    if values[field]==parts[field]:continue
                    mixed=candidate.replace(parts[field],values[field],1)
                    try:binding.authenticate(mixed)
                    except ValueError:refused('mixed-'+field+'-'+binding.digest(values[field])[:12],mixed)
            checks=[];path=scratch/(label+'.cpp');path.write_text(candidate)
            for compiler in('g++','clang++'):
                syntax=[compiler,'-std=c++20','-Wno-ignored-attributes','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)]
                built=subprocess.run(syntax,capture_output=True,text=True,timeout=600)
                if built.returncode:raise ValueError('Whole TypeQuery candidate syntax failed '+built.stderr[-6000:])
                checks.append(dict(compiler=compiler,passed=True,command=syntax))
            if not binding.callback_format(parts['body']):
                prepared,_=specialize(candidate)
                # Before integration the private specializer needs the exact
                # late retag; installed specialization already performs it.
                if binding.entry_prefix(binding.parts(prepared)['body'],False)==binding.prefix(False):
                    prepared=binding.retag_specialized(candidate,prepared)
                else:binding.installed_entries(prepared)
                if not binding.callback_format(binding.parts(prepared)['body']):raise ValueError('TypeQuery prepared format missing')
                binding.installed_entries(prepared)
                if binding.remove_object_type_query(prepared)!=specialize(source)[0]:raise ValueError('TypeQuery retag changed unrelated prepared source')
            contexts.append(dict(variant=label,original_context_sha256={k:binding.digest(v)for k,v in parts.items()},candidate_sha256=binding.digest(candidate),exact_inverse=True,idempotent=True,whole_cpp_syntax=checks,strict_refusals=negatives))
        report.update(passed=True,contexts=contexts,genuine_generation_command=command,
            child_execution_qualification='Only earlier finite E0900 paths; seven further callback owners remain open')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in names}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
