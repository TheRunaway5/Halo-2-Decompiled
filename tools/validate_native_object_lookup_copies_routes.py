#!/usr/bin/env python3
"""Execute six genuine prepared lookup copies through the shared native transport."""
import hashlib, json, re, sys
from pathlib import Path
import native_object_lookup_copies_replacements as binding
import validate_native_object_body_routes_tu15 as baseline
from audit_native_object_lookup_copies import OWNERS, generate, parts, project
from specialize_native_operands import specialize

ROOT = Path(__file__).resolve().parents[1]

def prepare(scratch, require_binding=False):
    baseline.prepare_reference(scratch)
    blocks = ['static std::uint64_t literals{};']
    contexts = []
    for owner in OWNERS:
        sources, command = generate(scratch, owner)
        if require_binding:
            for source in sources:
                if binding.replace_object_lookup_copies(source) != source:
                    raise ValueError('Actual lookup copy default missing: ' + owner)
                binding.installed_entries(source)
        for variant, source in zip(('current', 'fresh'), sources):
            value = specialize(source)[0]
            candidate = binding.replace_object_lookup_copies(value)
            if binding.remove_object_lookup_copies(candidate) != binding.remove_object_lookup_copies(value):
                raise ValueError('Lookup copy exact route reconstruction drift: ' + owner)
            context = parts(candidate, owner)
            project(binding.original_body(context['body']), context['resolver'])
            if not contexts:
                blocks.append(context['sign_extend'])
            index = len(contexts)
            body = context['body'].replace('run_bcdf0(', f'candidate_{index}(', 1)
            body = re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);', r'++literals;s.instruction_literal(\1);', body)
            blocks.append(body)
            contexts.append(dict(owner=owner, variant=variant, generation_command=command, whole_prepared_sha256=hashlib.sha256(value.encode()).hexdigest()))
    blocks.append('static void route(WorldState&s,unsigned n){switch(n){' + ''.join(f'case {index}u:candidate_{index}(s);return;' for index in range(len(contexts))) + 'default:throw std::runtime_error("Invalid lookup copy route");}}')
    (scratch / 'object_body_candidates.inc').write_text('\n'.join(blocks) + '\n')
    original = (ROOT / baseline.TEST).read_text()
    if baseline.sha(ROOT / baseline.TEST) != baseline.TEST_PIN:
        raise ValueError('Qualified source lookup transport harness drift')
    seam = '''            out.resume=native_object_body_detail::run_source_frame(state,continuation);
            if(out.resume)source_reference::retained(state,out.resume);'''
    if original.count(seam) != 1 or original.count('namespace {') != 1:
        raise ValueError('Exact lookup transport harness seam drift')
    original = original.replace(seam, '''            if(route_selector>=0)candidate_source::route(state,unsigned(route_selector));
            else {''' + seam + '\n            }', 1)
    anchor = 'namespace candidate_source {\n#include "object_body_candidates.inc"\n}\nstatic int route_selector=-1;\nnamespace {'
    original = original.replace('namespace {', anchor, 1).replace('int main(int argc,char**argv)', 'int baseline_probe_main(int argc,char**argv)', 1)
    if baseline.MAIN.count('variant<2u') != 1 or baseline.MAIN.count('{"source_variants",2}') != 1:
        raise ValueError('Lookup copy route count seam drift')
    count = 2 * len(OWNERS)
    main = baseline.MAIN.replace('variant<2u', f'variant<{count}u', 1).replace('{"source_variants",2}', f'{{"source_variants",{count}}}', 1)
    harness = scratch / 'lookup-copies-routes.cpp'
    harness.write_text('#include "gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp"\n' + original + '\n' + main)
    return harness, dict(source_contexts=contexts, exact_baseline_harness_sha256=baseline.TEST_PIN, prepared_only_runtime_scope=True, supplied_results=False, full_parent_closure=False)

def main():
    names = [name for pair in OWNERS.values() for name in pair]
    names += ['tools/native_object_lookup_copies_replacements.py', 'tools/audit_native_object_lookup_copies.py', 'src/gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp', 'tools/validate_native_object_lookup_copies_routes.py']
    pins = {name:hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in names}
    baseline.prepare = prepare
    output = Path(sys.argv[sys.argv.index('--output') + 1]) / 'report.json'
    result = 1
    try:
        result = baseline.main()
    finally:
        if output.exists():
            report = json.loads(output.read_text())
            after = {name:hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in names}
            report.update(lookup_copies_source_before=pins, lookup_copies_source_after=after, lookup_copies_bound='--require-binding' in sys.argv)
            if after != pins:
                report.update(passed=False, lookup_copies_source_drift=True)
                result = 1
            output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    return result

if __name__ == '__main__':
    raise SystemExit(main())
