#!/usr/bin/env python3
"""Authenticate whole current/fresh TypeQuery contexts before private binding."""
import argparse,hashlib,json,os,re,subprocess,sys
from pathlib import Path
from specialize_native_operands import specialize
from validate_native_packed_rotation_generation_tu15 import dispatcher_map
from validate_native_object_type_query_source_frame_tu15 import PINS
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def one(source,pattern,label):
    values=re.findall(pattern,source,re.M|re.S)
    if len(values)!=1:raise ValueError('Missing/ambiguous complete '+label)
    return values[0]
def parts(source):
    return dict(
        body=one(source,r'^static void run_10de40\(WorldState &s\) \{.*?^\}','TypeQuery body'),
        resolver=one(source,r'^std::uint32_t selected\([^\n]+','resolver'),
        dispatcher=one(source,r'^static void dispatch\([^\n]+','dispatcher'),
        state=one(source,r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;','State'),
        sign_extend=one(source,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}','sign extension'),
        public_entry=one(source,r'^void source_run_physics_live_support_tu15\(.*?^\}','public entry'),
        owns_entry=one(source,r'^bool source_physics_live_support_tu15_owns_entry\([^\n]+','public ownership'))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(UNIT,'tools/generate_physics_live_support_tu15.py','tools/audit_native_object_type_query_source_owners.py',
           'tools/specialize_native_operands.py','tools/native_flight_property_query_replacements.py',
           'tools/native_object_record_ratio_replacements.py',
           'tools/native_object_type_query_replacements.py')
    pins={name:sha(ROOT/name)for name in names}
    report=dict(passed=False,production_bound=False,scope=__doc__,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        target=scratch/'fresh.cpp'
        command=[sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(target),'--manifest',str(scratch/'generation.json')]
        done=subprocess.run(command,capture_output=True,text=True)
        if done.returncode:raise ValueError('Genuine whole Primary generation failed '+(done.stdout+done.stderr)[-7000:])
        from native_object_type_query_replacements import remove_object_type_query
        from native_flight_property_query_replacements import remove_flight_property_query
        from native_object_record_ratio_replacements import remove_object_record_ratio
        current=remove_object_type_query(remove_flight_property_query(remove_object_record_ratio((ROOT/UNIT).read_text())))
        fresh=remove_object_type_query(remove_flight_property_query(remove_object_record_ratio(target.read_text())))
        raw=[current,fresh];sources=raw+[specialize(value)[0]for value in raw]
        labels=('current','fresh','current-prepared','fresh-prepared');rows=[]
        callback_owners=(0xbec50,0xc9070,0xe0900,0xf9270,0x100e70,0x1046c0,0x10be20,0x11e490)
        external_owners={0x100e70,0x10be20,0x11e490}
        for label,source in zip(labels,sources,strict=True):
            values=parts(source)
            original=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',values['body'])
            if digest(original)!=PINS[0x10de40]:raise ValueError('Complete TypeQuery body changed beyond callback projection')
            sites=[int(value,16)for value in re.findall(r'(?:s\.services\.instruction|s\.instruction_literal)\(0x([0-9a-f]+)u\)',values['body'])]
            if len(sites)!=43 or len(set(sites))!=43:raise ValueError('Complete TypeQuery site set differs')
            # Raw dispatcher grammars authenticate every target and fallback.
            # Prepared shims are recorded whole and each real child clause
            # must remain explicit, without claiming its dependency closure.
            if 'prepared'not in label:
                mapping=dispatcher_map(values['resolver'],values['dispatcher'])
                if mapping.get(0x10de40)!='10de40':raise ValueError('Actual TypeQuery dispatcher identity differs')
                for entry in callback_owners:
                    target=mapping.get(entry)
                    if target is None and entry in external_owners:continue
                    if target!=format(entry,'x'):raise ValueError('Inherited callback dispatcher identity differs '+hex(entry))
            else:
                for entry in(0x10de40,*callback_owners):
                    count=values['dispatcher'].count('run_'+format(entry,'x')+'(s);return;')
                    if count==0 and entry in external_owners:continue
                    if count!=1:raise ValueError('Prepared real callback clause differs '+hex(entry))
            if values['dispatcher'].count('s.required(site,target,tail);')!=1:
                raise ValueError('Actual external callback fallback changed')
            path=scratch/(label+'.cpp');path.write_text(source);checks=[]
            for compiler in('g++','clang++'):
                syntax=[compiler,'-std=c++20','-Wno-ignored-attributes','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)]
                done=subprocess.run(syntax,capture_output=True,text=True)
                if done.returncode:raise ValueError('Whole TypeQuery context syntax failed '+done.stderr[-7000:])
                checks.append(dict(compiler=compiler,command=syntax,passed=True))
            local=[entry for entry in callback_owners if 'run_'+format(entry,'x')+'(s);return;'in values['dispatcher']]
            rows.append(dict(variant=label,complete_context_sha256={key:digest(value)for key,value in values.items()},whole_source_sha256=digest(source),whole_cpp_syntax=checks,canonical_body_sha256=digest(original),local_child_targets=[hex(entry)for entry in local],external_required_targets=[hex(entry)for entry in callback_owners if entry not in local]))
        report.update(passed=True,contexts=rows,genuine_generation_command=command,child_execution_qualification='Only earlier E0900 finite-path proof; mapping other seven targets is provenance, not execution closure')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in names}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
