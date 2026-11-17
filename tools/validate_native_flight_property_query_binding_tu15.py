#!/usr/bin/env python3
"""Authenticate and syntax-check the four exact E0900 profiles of one owner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

import native_flight_property_query_replacements as primary
from specialize_native_operands import specialize

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
GEN='tools/generate_physics_live_support_tu15.py'
DEATH='src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
NEW_DEFAULT='src/gameplay/objects/native_tu15_flight_property_query_source_owner.hpp'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def reject(function,*args):
    try:
        function(*args)
    except ValueError:
        return
    raise ValueError('Accepted a foreign or changed E0900 binding')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',default='c++')
    p.add_argument('--require-binding',action='store_true');p.add_argument('--death-cycle',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    a.output.mkdir(parents=True,exist_ok=False)
    binding=primary;replace=primary.replace_flight_property_query;remove=primary.remove_flight_property_query
    unit,gen,foreign=UNIT,GEN,DEATH
    receipt_key='native_flight_property_query_entries'
    extra=()
    if a.death_cycle:
        import native_death_cycle_flight_query_replacements as binding
        replace=binding.replace_death_cycle_flight_query;remove=binding.remove_death_cycle_flight_query
        unit,gen,foreign=DEATH,'tools/generate_death_cycle_runtime_tu15.py',UNIT
        receipt_key='native_death_cycle_flight_query_entries'
        extra=(gen,'tools/native_death_cycle_flight_query_replacements.py',
               'src/gameplay/objects/native_tu15_death_cycle_flight_query_source_owner.hpp')
    files=(UNIT,GEN,DEATH,NEW_DEFAULT,'tools/native_flight_property_query_replacements.py',
           'tools/native_object_type_query_replacements.py','tools/native_object_record_ratio_replacements.py','tools/specialize_native_operands.py',
           str(Path(__file__).resolve().relative_to(ROOT)),*extra)
    before={name:sha(ROOT/name)for name in files}
    headers={path.resolve():sha(path)for tree in ('include','src')for path in (ROOT/tree).rglob('*')
             if path.suffix in ('.hpp','.h','.inc')}
    report=dict(passed=False,owned_scratch=str(scratch),source_before=before,contexts=[],retained_large_artifacts=[])
    try:
        fresh=scratch/'fresh.cpp'
        manifest=scratch/'generation.json'
        generator=[sys.executable,'-B',str(ROOT/gen),'--output',str(fresh)]
        if a.death_cycle:
            manifest=scratch/'generation/generation.json';generator+=['--directory',str(manifest.parent)]
        else:generator+=['--manifest',str(manifest)]
        ran=subprocess.run(generator,cwd=ROOT,capture_output=True,text=True,timeout=600)
        if ran.returncode:raise ValueError('Actual E0900 owning generation failed: '+ran.stderr[-8000:])
        report['generation']=dict(command=generator,manifest_sha256=sha(manifest))
        current=(ROOT/unit).read_text()
        report['production_bound']=binding.authenticate(current)[3]is not None
        for fresh_flag,source in enumerate((current,fresh.read_text())):
            if a.require_binding:binding.installed_entries(source)
            if not a.death_cycle:
                from native_object_record_ratio_replacements import remove_object_record_ratio
                source=remove_object_record_ratio(source)
            raw=remove(source)
            raw_candidate=replace(raw)
            prepared,retag_receipt=specialize(raw_candidate)
            real_retag=retag_receipt.get(receipt_key)==['e0900']
            if a.require_binding and not real_retag:raise ValueError('Installed specializer missed E0900')
            if not real_retag:prepared=binding.retag_specialized(raw_candidate,prepared)
            for literal,base in ((False,raw),(True,specialize(raw)[0])):
                candidate=replace(base)
                if remove(candidate)!=base or replace(candidate)!=candidate:
                    raise ValueError('E0900 overlay does not preserve every sibling byte')
                if literal and candidate!=prepared:raise ValueError('E0900 specializer retag is not exact')
                match,body,profile,installed=binding.authenticate(candidate)
                if profile!=(literal,bool(fresh_flag)) or profile!=installed:
                    raise ValueError('Wrong E0900 exact profile')
                prefix=binding.PREFIXES[profile]
                controls={
                    'no-include':candidate.replace(binding.INCLUDE,'',1),
                    'duplicate-include':binding.INCLUDE+candidate,
                    'include-moved':binding.INCLUDE+candidate.replace(binding.INCLUDE,'',1),
                    'foreign-include-basename':candidate+f'\n#include <{binding.BASENAME}>\n',
                    'wrong-tag':candidate.replace(prefix,binding.PREFIXES[not literal,bool(fresh_flag)],1),
                    'wrong-spill':candidate.replace(prefix,binding.PREFIXES[literal,not bool(fresh_flag)],1),
                    'moved-prefix':candidate.replace(match[0],body[:-2]+prefix+'\n}',1),
                    'body-drift':candidate.replace(match[0],match[0].replace('0x70000c9u','0x70000c8u',1),1),
                    'foreign-namespace-alias':candidate+f'\nnamespace alt = halo2::{binding.OWNER};\n',
                    'foreign-core-using':candidate+f'\nusing namespace halo2::{primary.CORE};\n',
                    'outside-bridge':candidate+f'\n// halo2::{binding.OWNER}::bridge\n',
                    'source-namespace-shift':candidate.replace('namespace {','namespace elsewhere {',1),
                    'resolver-drift':candidate.replace('std::uint32_t selected(','std::uint32_t other_selected(',1),
                    'public-entry-drift':candidate.replace('void source_run_'+
                        ('death_cycle_runtime' if a.death_cycle else 'physics_live_support')+'_tu15(',
                        'void changed_e0900_public_entry(',1),
                }
                if fresh_flag or a.death_cycle:
                    helper=primary.HELPER.search(candidate)[0]
                    controls['spill-helper-drift']=candidate.replace(helper,helper.replace('0x200u','0x100u',1),1)
                for label,mutated in controls.items():
                    if mutated==candidate:raise ValueError('Missing E0900 binding defect seam: '+label)
                    reject(binding.installed_entries,mutated)
                profile_name=('fresh'if fresh_flag else'current')+('-prepared'if literal else'-raw')
                cpp=scratch/(profile_name+'.cpp');dep=cpp.with_suffix('.d');cpp.write_text(candidate)
                command=[a.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
                         '-MD','-MF',str(dep),str(cpp)]
                ran=subprocess.run(command,cwd=ROOT,text=True,capture_output=True,timeout=600)
                if ran.returncode:raise ValueError('Whole E0900 owning compile failed: '+ran.stderr[-8000:])
                for item in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                    path=Path(item).resolve()
                    if path in headers:before[str(path.relative_to(ROOT))]=headers[path]
                report['contexts'].append(dict(name=profile_name,sha256=sha(cpp),cpp_syntax=True,
                    actual_retag=real_retag,strict_refusals=len(controls),command=command))
        other=(ROOT/foreign).read_text()
        if replace(other)!=other:raise ValueError('E0900 overlay took the other source copy')
        reject(binding.authenticate,binding.INCLUDE+other)
        report.update(passed=True,other_copy_changed=False,orphan_refused=True,source_owner='DeathCycle' if a.death_cycle else 'Primary')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in before}
        if report['source_after']!=before:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
