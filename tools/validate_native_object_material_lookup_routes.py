#!/usr/bin/env python3
"""Private material-owner candidates using the qualified lookup transport harness."""
import hashlib,json,os,re,subprocess,sys
from pathlib import Path
import native_object_material_lookup_replacements as binding
import validate_native_object_body_routes_tu15 as baseline
from audit_native_object_material_lookup import UNIT,GENERATOR,REFERENCE,parts
from specialize_native_operands import specialize
ROOT=Path(__file__).resolve().parents[1]
def prepare(scratch,require_binding=False):
    baseline.prepare_reference(scratch)
    fresh=scratch/'fresh-material.cpp'
    command=[sys.executable,'-B',str(ROOT/GENERATOR),'--reference',str(ROOT/REFERENCE),
        '--output',str(fresh),'--directory',str(scratch/'material-generation')]
    done=subprocess.run(command,capture_output=True,text=True)
    if done.returncode:raise ValueError('Material actual-owner generation failed: '+done.stderr[-7000:])
    sources=[(ROOT/UNIT).read_text(),fresh.read_text()]
    if require_binding:
        for source in sources:
            if binding.replace_object_material_lookup(source)!=source:raise ValueError('Material default is not installed')
            binding.installed_entries(source)
    values=[specialize(source)[0]for source in sources]
    blocks=['static std::uint64_t literals{};',parts(values[0])['sign_extend']]
    for n,value in enumerate(values):
        candidate=binding.replace_object_material_lookup(value)
        if binding.remove_object_material_lookup(candidate)!=binding.remove_object_material_lookup(value):
            raise ValueError('Exact material source-route reconstruction failed')
        body=parts(candidate)['body'].replace('run_bcdf0(',f'candidate_{n}(',1)
        body=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'++literals;s.instruction_literal(\1);',body)
        blocks.append(body)
    blocks.append('static void route(WorldState&s,unsigned n){switch(n){case 0u:candidate_0(s);return;case 1u:candidate_1(s);return;default:throw std::runtime_error("Invalid private material route");}}')
    (scratch/'object_body_candidates.inc').write_text('\n'.join(blocks)+'\n')
    original=(ROOT/baseline.TEST).read_text()
    if baseline.sha(ROOT/baseline.TEST)!=baseline.TEST_PIN:raise ValueError('Qualified source harness drift')
    seam='''            out.resume=native_object_body_detail::run_source_frame(state,continuation);
            if(out.resume)source_reference::retained(state,out.resume);'''
    if original.count(seam)!=1 or original.count('namespace {')!=1:raise ValueError('Material source harness seam drift')
    original=original.replace(seam,'''            if(route_selector>=0)candidate_source::route(state,unsigned(route_selector));
            else {'''+seam+'''\n            }''',1)
    anchor='namespace candidate_source {\n#include "object_body_candidates.inc"\n}\nstatic int route_selector=-1;\nnamespace {'
    original=original.replace('namespace {',anchor,1).replace('int main(int argc,char**argv)','int baseline_probe_main(int argc,char**argv)',1)
    harness=scratch/'material-routes.cpp'
    harness.write_text('#include "gameplay/objects/native_tu15_object_material_lookup_source_owner.hpp"\n'+original+'\n'+baseline.MAIN)
    return harness,dict(genuine_generation_command=command,source_contexts=2,
        exact_baseline_harness_sha256=baseline.TEST_PIN,prepared_only_runtime_scope=True,
        shared_transport_reference='exact identity-only full body projection matches qualified UnitView owner',
        material_admission_and_default_installed=False)
def main():
    # The shared driver owns compilation, provider mutations, genuine suffixes,
    # reference controls and all guard defects. Only its source preparation is
    # replaced here; expected values and test results are never substituted.
    names=[UNIT,GENERATOR,'tools/native_object_material_lookup_replacements.py',
        'src/gameplay/objects/native_tu15_object_material_lookup_source_owner.hpp',
        'tools/validate_native_object_material_lookup_routes.py']
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names}
    baseline.prepare=prepare;baseline.UNIT=UNIT;baseline.GENERATOR=GENERATOR
    output=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
    result=1
    try:result=baseline.main()
    finally:
        if output.exists():
            report=json.loads(output.read_text());after={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names}
            report.update(material_source_before=pins,material_source_after=after,material_source_bound='--require-binding'in sys.argv)
            if after!=pins:
                report.update(passed=False,material_source_drift=True)
                result=1
            output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return result
if __name__=='__main__':raise SystemExit(main())
