#!/usr/bin/env python3
"""Execute genuine prepared damage/vitality lookup candidates and exact suffixes."""
import hashlib,json,re,sys
from pathlib import Path
import native_object_lookup_canonical_replacements as binding
import validate_native_object_body_routes_tu15 as baseline
from audit_native_object_lookup_canonical_owners import OWNERS,generate,mapping,parts,project
from specialize_native_operands import specialize
ROOT=Path(__file__).resolve().parents[1]
def prepare(scratch,require_binding=False):
    sites,_=mapping();contexts=[];blocks=['static std::uint64_t literals{};'];reference=None
    for owner in OWNERS:
        sources,command=generate(scratch,owner)
        for label,source in zip(('current','fresh'),sources,strict=True):
            if require_binding and binding.installed_entries(source)!=['bbcb0']:raise ValueError('Canonical lookup current/fresh default missing')
            original=binding.remove_canonical_object_lookup(source);prepared=specialize(original)[0]
            context=parts(prepared,owner);project(context,sites)
            if reference is None:
                reference=context['body'];blocks.append(context['sign_extend'])
            elif context['body']!=reference:raise ValueError('Qualified canonical retained source bodies differ')
            candidate=binding.replace_canonical_object_lookup(prepared)
            if binding.remove_canonical_object_lookup(candidate)!=prepared or binding.replace_canonical_object_lookup(candidate)!=candidate:raise ValueError('Canonical lookup complete candidate reversal drift')
            index=len(contexts);body=parts(candidate,owner)['body'].replace('run_bbcb0(',f'candidate_{index}(',1)
            body=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'++literals;s.instruction_literal(\1);',body);blocks.append(body)
            contexts.append(dict(owner=owner,variant=label,genuine_generation_command=command,prepared_source_sha256=binding.digest(prepared)))
    blocks.append('static void route(WorldState&s,unsigned n){switch(n){'+''.join(f'case {index}u:candidate_{index}(s);return;'for index in range(len(contexts)))+'default:throw std::runtime_error("Invalid canonical object lookup route");}}')
    (scratch/'object_body_candidates.inc').write_text('\n'.join(blocks)+'\n')
    # Keep every retained operation and callback at its canonical site. The
    # only additions select authenticated next-instruction suffixes and count
    # coverage, exactly as in the already-qualified actual-owner fixture.
    at=reference.index('{')+1
    switch='\nswitch(resume){case 0u:break;'+''.join(f'case 0x{pc:x}u:goto N{pc:x};'for pc in binding.FRONTIERS)+'default:throw std::runtime_error("Invalid canonical retained lookup suffix");}\n'
    for pc in binding.FRONTIERS:
        needle=f's.instruction_literal(0x{pc:x}u);'
        if reference.count(needle)!=1:raise ValueError('Canonical retained frontier missing')
        reference=reference.replace(needle,f'N{pc:x}: '+needle,1)
    reference=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'coverage.insert(\1);s.instruction_literal(\1);',reference)
    reference=reference[:at]+switch+reference[at:];reference=reference.replace('static void run_bbcb0(WorldState &s)','static void retained(WorldState &s,std::uint32_t resume)',1)
    (scratch/'object_body_source_reference.inc').write_text('static std::set<std::uint32_t>coverage;\n'+context['sign_extend']+'\n'+reference+'\n')
    original=(ROOT/baseline.TEST).read_text()
    if baseline.sha(ROOT/baseline.TEST)!=baseline.TEST_PIN:raise ValueError('Qualified actual lookup harness drift')
    seam='''            out.resume=native_object_body_detail::run_source_frame(state,continuation);
            if(out.resume)source_reference::retained(state,out.resume);'''
    if original.count(seam)!=1 or original.count('namespace {')!=1:raise ValueError('Canonical lookup harness seam drift')
    mapped='''            out.resume=native_object_body_detail::run_source_frame(state,continuation,native_object_body_detail::Noop{},native_object_body_detail::CanonicalSites{});
            if(out.resume)source_reference::retained(state,out.resume);'''
    original=original.replace(seam,'''            if(route_selector>=0)candidate_source::route(state,unsigned(route_selector));
            else {'''+mapped+'\n            }',1)
    anchor='namespace candidate_source {\n#include "object_body_candidates.inc"\n}\nstatic int route_selector=-1;\nnamespace {'
    original=original.replace('namespace {',anchor,1).replace('int main(int argc,char**argv)','int baseline_probe_main(int argc,char**argv)',1)
    main=baseline.MAIN.replace('variant<2u','variant<4u',1).replace('{"source_variants",2}','{"source_variants",4}',1)
    main=main.replace('halo2::native_object_body_owner_detail::bridge(state)','halo2::native_object_body_canonical_owner_detail::bridge(state)')
    harness=scratch/'canonical-object-lookup-routes.cpp';harness.write_text('#include "gameplay/objects/native_tu15_object_body_canonical_source_owner.hpp"\n'+original+'\n'+main)
    return harness,dict(source_contexts=contexts,exact_baseline_harness_sha256=baseline.TEST_PIN,prepared_only_runtime_scope=True,canonical_observation_sites=True,actual_tu15_root='4ecbd0',full_parent_closure=False)
def main():
    names=[name for pair in OWNERS.values()for name in pair]+['tools/native_object_lookup_canonical_replacements.py','tools/audit_native_object_lookup_canonical_owners.py','tools/validate_native_object_lookup_canonical_routes.py','src/gameplay/objects/native_tu15_object_body_canonical_source_owner.hpp','src/gameplay/objects/native_tu15_object_body_canonical_sites.hpp']
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names};baseline.prepare=prepare
    output=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';result=1
    try:result=baseline.main()
    finally:
        if output.exists():
            report=json.loads(output.read_text());after={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names}
            report.update(canonical_source_before=pins,canonical_source_after=after,canonical_source_bound='--require-binding'in sys.argv)
            if after!=pins:report.update(passed=False,canonical_source_drift=True);result=1
            output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return result
if __name__=='__main__':raise SystemExit(main())
