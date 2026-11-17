#!/usr/bin/env python3
"""Check four genuine 10BE20 source binding profiles and exact siblings."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

import native_device_property_query_replacements as binding
from specialize_native_operands import specialize

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/runtime/source_map_world_support_tu15_generated.cpp'
GEN='tools/generate_map_world_support_tu15.py'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--require-binding',action='store_true')
    a=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    a.output.mkdir(parents=True,exist_ok=False)
    names=(UNIT,GEN,'tools/generate_physics_live_support_tu15.py',
           'tools/specialize_native_operands.py','tools/native_device_property_query_replacements.py',
           'tools/native_map_world_property_query_replacements.py',str(Path(__file__).relative_to(ROOT)))
    pins={n:sha(ROOT/n)for n in names}
    headers={p.resolve():sha(p)for d in ('include','src')for p in (ROOT/d).rglob('*')if p.suffix in ('.hpp','.h','.inc')}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],contexts=[])
    try:
        fresh=scratch/'fresh.cpp'
        command=[sys.executable,'-B',str(ROOT/GEN),'--output',str(fresh),'--manifest',str(scratch/'manifest.json')]
        ran=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=600)
        if ran.returncode:raise ValueError('Actual MapWorld generation failed: '+ran.stderr[-8000:])
        report['generator']=command
        current=(ROOT/UNIT).read_text()
        report['production_bound']=binding.authenticate(current)[3]is not None
        for fresh_flag,source in enumerate((current,fresh.read_text())):
            if a.require_binding:binding.installed_entries(source)
            raw=binding.remove_device_property_query(source)
            installed=binding.replace_device_property_query(raw)
            prepared,receipt=specialize(installed)
            actual_retag=receipt.get('native_device_property_query_entries')==['10be20']
            if a.require_binding and not actual_retag:raise ValueError('Actual specializer missed 10BE20')
            if not actual_retag:prepared=binding.retag_specialized(installed,prepared)
            for literal,base in ((False,raw),(True,specialize(raw)[0])):
                candidate=binding.replace_device_property_query(base)
                if binding.remove_device_property_query(candidate)!=base or binding.replace_device_property_query(candidate)!=candidate:
                    raise ValueError('10BE20 exact byte inverse changed')
                if literal and prepared!=candidate:raise ValueError('10BE20 exact prepared retag changed')
                if binding.pair.installed_entries(candidate)!=list(binding.pair.ENTRIES):
                    raise ValueError('Other installed MapWorld property owners changed')
                match,body,profile,key=binding.authenticate(candidate)
                if profile!=key or key!=(literal,bool(fresh_flag)):raise ValueError('Wrong 10BE20 exact profile')
                prefix=binding.PREFIXES[profile]
                controls={
                    'missing-include':candidate.replace(binding.INCLUDE,'',1),
                    'duplicate-include':binding.INCLUDE+candidate,
                    'moved-include':binding.INCLUDE+candidate.replace(binding.INCLUDE,'',1),
                    'alternate-include':candidate+f'\n#include <{binding.BASENAME}>\n',
                    'wrong-tag':candidate.replace(prefix,binding.prefix(not literal,bool(fresh_flag)),1),
                    'wrong-spill':candidate.replace(prefix,binding.prefix(literal,not bool(fresh_flag)),1),
                    'moved-bridge':candidate.replace(match[0],body[:-2]+prefix+'\n}',1),
                    'fixed-selector-drift':candidate.replace(match[0],match[0].replace('0x5000081u','0x5000082u',1),1),
                    'outside-bridge':candidate+f'\nusing namespace halo2::{binding.OWNER};\n',
                    'outside-core':candidate+f'\nnamespace wrong=halo2::{binding.CORE};\n',
                    'removed-sibling':candidate.replace(binding.pair.INCLUDE,'',1),
                    'scope-moved':candidate.replace('namespace {','namespace unintended {',1),
                    'public-drift':candidate.replace('void source_run_map_world_support_tu15(','void not_map_world_support(',1),
                }
                if fresh_flag:
                    helper=binding.HELPER.search(candidate)[0]
                    controls['spill-helper-drift']=candidate.replace(helper,helper.replace('0x200u','0x100u',1),1)
                for label,bad in controls.items():
                    if bad==candidate:raise ValueError('Missing 10BE20 control '+label)
                    try:binding.installed_entries(bad)
                    except ValueError:pass
                    else:raise ValueError('Accepted 10BE20 control '+label)
                label=('fresh'if fresh_flag else'current')+('-prepared'if literal else'-raw')
                cpp=scratch/(label+'.cpp');dep=cpp.with_suffix('.d');cpp.write_text(candidate)
                cmd=[a.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
                     '-MD','-MF',str(dep),str(cpp)]
                checked=subprocess.run(cmd,capture_output=True,text=True,timeout=600)
                if checked.returncode:raise ValueError('Whole MapWorld compile failed: '+checked.stderr[-8000:])
                for path in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                    resolved=Path(path).resolve()
                    if resolved in headers:pins[str(resolved.relative_to(ROOT))]=headers[resolved]
                report['contexts'].append(dict(name=label,strict_refusals=len(controls),cpp_sha256=sha(cpp),
                    exact_reversal=True,partner_preserved=True,actual_retag=actual_retag))
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_before']=pins
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
