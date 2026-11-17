#!/usr/bin/env python3
"""Check the whole CombatAction 165590 candidate and its Projectile sibling."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from generated_routine_ownership import function_bodies
import native_object_definition_mode_query_replacements as binding
from research_mp_sandbox_update_functions import NEW_DEFAULT
from specialize_native_operands import specialize
from validate_native_object_definition_mode_query_source_tu15 import ROOT, UNIT, prepare
from validate_native_node_progress_query_source_tu15 import sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if not NEW_DEFAULT.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True,exist_ok=False)
    names=(UNIT,'tools/generate_combat_action_tu15.py','tools/specialize_native_operands.py',
        'tools/native_object_definition_mode_query_replacements.py','tools/native_projectile_function_query_replacements.py',
        'tools/native_object_table_ratio_replacements.py',
        'tools/validate_native_object_definition_mode_query_source_tu15.py',str(Path(__file__).relative_to(ROOT)),
        'src/gameplay/objects/native_tu15_object_definition_mode_query_source_owner.hpp',
        'src/gameplay/objects/native_tu15_object_definition_mode_query_source_frame.hpp',
        'include/halo2/native_tu15_object_definition_mode_query_kernel.hpp')
    before={name:sha(ROOT/name) for name in names}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),
                retained_large_artifacts=[],contexts=[])
    try:
        report['authority']=prepare(scratch,args.require_binding)
        from native_object_table_ratio_replacements import remove_object_table_ratio
        current=(ROOT/UNIT).read_text();fresh=(scratch/'fresh.cpp').read_text()
        for label,source in (('current',current),('fresh',fresh)):
            source=remove_object_table_ratio(source)
            if args.require_binding:binding.installed_entries(source)
            raw=binding.remove_object_definition_mode_query(source)
            installed=binding.replace_object_definition_mode_query(raw)
            prepared,receipt=specialize(installed)
            official=receipt.get('native_object_definition_mode_query_entries')==['165590']
            if args.require_binding and not official:raise ValueError('Actual 165590 specialization missing')
            if not official:prepared=binding.retag_specialized(installed,prepared)
            for literal,candidate,previous in ((False,installed,raw),(True,prepared,specialize(raw)[0])):
                if binding.remove_object_definition_mode_query(candidate)!=previous:
                    raise ValueError('Complete 165590 inverse changed')
                if binding.replace_object_definition_mode_query(candidate)!=candidate:
                    raise ValueError('Complete 165590 installer is not idempotent')
                if binding.projectile.installed_entries(candidate)!=['100e70']:
                    raise ValueError('Genuine Projectile sibling lost')
                old={s[1]:body for s,body in function_bodies(previous)}
                new={s[1]:body for s,body in function_bodies(candidate)}
                if old.keys()!=new.keys() or [entry for entry in old if old[entry]!=new[entry]]!=['165590']:
                    raise ValueError('An unrelated CombatAction body changed')
                target=new['165590']
                controls={
                    'missing-include':candidate.replace(binding.INCLUDE,'',1),
                    'moved-include':binding.INCLUDE+candidate.replace(binding.INCLUDE,'',1),
                    'duplicate-include':candidate+binding.INCLUDE,
                    'wrong-tag':candidate.replace(binding.prefix(literal),binding.prefix(not literal),1),
                    'foreign-owner':candidate+'\nusing namespace halo2::'+binding.OWNER+';\n',
                    'foreign-helper':candidate+'\nnamespace wrong=halo2::'+binding.DETAIL+';\n',
                    'namespace-drift':candidate.replace('namespace {','namespace changed {',1),
                    'removed-projectile':candidate.replace(binding.ANCHOR,'',1),
                    'definition-word-drift':candidate.replace(target,target.replace('0x290u','0x292u',1),1),
                    'wrong-export':candidate.replace('bool source_combat_action_tu15_owns_entry(',
                                                       'bool unrelated_owns_entry(',1),
                }
                for name,changed in controls.items():
                    if changed==candidate:raise ValueError('Missing 165590 rejection control '+name)
                    try:binding.installed_entries(changed)
                    except ValueError:continue
                    raise ValueError('Accepted 165590 rejection control '+name)
                cpp=scratch/(label+('-prepared' if literal else '-raw')+'.cpp');cpp.write_text(candidate)
                compiled=subprocess.run([args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),
                     '-I'+str(ROOT/'src'),str(cpp)],capture_output=True,text=True,timeout=1200)
                if compiled.returncode:raise ValueError('Whole CombatAction compile failed: '+compiled.stderr[-7000:])
                report['contexts'].append(dict(label=cpp.stem,source_sha256=sha(cpp),
                    strict_refusals=len(controls),neighboring_bodies_preserved=len(old)-1,official_retag=official))
        report.update(passed=True,production_bound=args.require_binding)
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        report['source_before']=before
        report['source_after']={name:sha(ROOT/name) for name in before}
        if report['source_after']!=before:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
