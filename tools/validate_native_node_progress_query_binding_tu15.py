#!/usr/bin/env python3
"""Prove the installed 1CF0E0 owner in current and freshly generated AIFrame."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from audit_native_object_lookup_copies import generate, GENERATION_PROVENANCE
from generated_routine_ownership import function_bodies
import native_node_progress_query_replacements as binding
from research_mp_sandbox_update_functions import NEW_DEFAULT
from specialize_native_operands import specialize
from validate_native_node_progress_query_source_tu15 import ROOT, UNIT, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', default='c++')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not NEW_DEFAULT.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    names = (UNIT, 'tools/generate_ai_frame_tu15.py', 'tools/specialize_native_operands.py',
             'tools/native_node_progress_query_replacements.py', str(Path(__file__).relative_to(ROOT)),
             'src/gameplay/animation/native_tu15_node_progress_query_source_owner.hpp',
             'src/gameplay/animation/native_tu15_node_progress_query_source_frame.hpp',
             'include/halo2/native_tu15_node_progress_query_kernel.hpp')
    before = {name:sha(ROOT/name) for name in names}
    report = dict(passed=False, production_bound=False, owned_scratch=str(scratch),
                  retained_large_artifacts=[], contexts=[])
    try:
        values, command = generate(scratch,'ai_frame')
        report.update(generation=command, generation_provenance=GENERATION_PROVENANCE['ai_frame'])
        for label,raw in zip(('current','fresh'),values,strict=True):
            if binding.installed_entries(raw) != ['1cf0e0']:
                raise ValueError('Actual AIFrame owner missing')
            prepared, receipt = specialize(raw)
            if receipt['native_node_progress_query_entries'] != ['1cf0e0']:
                raise ValueError('Actual AIFrame specializer missed 1CF0E0')
            if binding.remove_node_progress_query(prepared) != specialize(binding.remove_node_progress_query(raw))[0]:
                raise ValueError('AIFrame 1CF0E0 prepared inverse changed')
            for literal,text in ((False,raw),(True,prepared)):
                original = binding.remove_node_progress_query(text)
                if binding.replace_node_progress_query(original) != text or binding.replace_node_progress_query(text) != text:
                    raise ValueError('Exact AIFrame reversal or idempotence changed')
                prior = {signature[1]:body for signature,body in function_bodies(original)}
                current = {signature[1]:body for signature,body in function_bodies(text)}
                if prior.keys()!=current.keys() or [entry for entry in prior if prior[entry]!=current[entry]]!=['1cf0e0']:
                    raise ValueError('An unrelated complete AIFrame body changed')
                target = current['1cf0e0']
                parent = current['1d63c0']
                controls = {
                    'missing-include':text.replace(binding.INCLUDE,'',1),
                    'moved-include':binding.INCLUDE+text.replace(binding.INCLUDE,'',1),
                    'duplicate-include':text+binding.INCLUDE,
                    'wrong-callback-tag':text.replace(binding.prefix(literal),binding.prefix(not literal),1),
                    'foreign-alias':text+'\nusing namespace halo2::'+binding.OWNER+';\n',
                    'namespace-drift':text.replace('namespace {','namespace wrong {',1),
                    'wrong-native-child':text.replace(target,target.replace('run_1e3150(state);','run_1e5c80(state);',1),1),
                    'wrong-reference-child':text.replace(target,target.replace('run_1e3150(s);','run_1e5c80(s);',1),1),
                    'local-caller-drift':text.replace(parent,parent.replace('run_1cf0e0(s);','run_1e3150(s);',1),1),
                    'live-admission-drift':text.replace('return p==0x1d0670u ||','return p==0x1cf0e0u || p==0x1d0670u ||',1),
                }
                for name,changed in controls.items():
                    if changed==text:
                        raise ValueError('Missing 1CF0E0 rejection control: '+name)
                    try:
                        binding.installed_entries(changed)
                    except ValueError:
                        continue
                    raise ValueError('Accepted 1CF0E0 rejection control: '+name)
                cpp = scratch/(label+('-prepared' if literal else '-raw')+'.cpp')
                cpp.write_text(text)
                checked = subprocess.run([args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),
                                          '-I'+str(ROOT/'src'),str(cpp)],capture_output=True,text=True,timeout=1200)
                if checked.returncode:
                    raise ValueError('Whole AIFrame syntax failed: '+checked.stderr[-8000:])
                report['contexts'].append(dict(label=cpp.stem,strict_refusals=len(controls),
                                               source_sha256=sha(cpp),neighboring_bodies_preserved=len(prior)-1))
        report.update(passed=True,production_bound=True,external_live_admission_changed=False)
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        report['source_before'] = before
        report['source_after'] = {name:sha(ROOT/name) for name in before}
        if report['source_after'] != before:
            report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
