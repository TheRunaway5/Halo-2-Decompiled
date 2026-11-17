#!/usr/bin/env python3
"""Validate the exact paired CombatAction bindings and their real local call."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_object_table_ratio_replacements as binding
from specialize_native_operands import specialize
from validate_native_flight_property_query_source_tu15 import ROOT, digest
from validate_native_object_definition_mode_query_source_tu15 import GENERATOR, UNIT

FUNCTIONS=re.compile(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{.*?^\}',re.M|re.S)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_bound=args.require_binding,owned_scratch=str(scratch),
                contexts=[],retained_large_artifacts=[])
    try:
        fresh=scratch/'fresh.cpp'
        cmd=[sys.executable,'-B',str(ROOT/GENERATOR),'--output',str(fresh),'--directory',str(scratch/'generation')]
        generated=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=900)
        if generated.returncode:
            raise ValueError('Genuine CombatAction generation failed: '+generated.stderr[-8000:])
        report['generation']=cmd
        for origin,actual in (('current',(ROOT/UNIT).read_text()),('fresh',fresh.read_text())):
            baseline=binding.remove_object_table_ratio(actual)
            raw=binding.replace_object_table_ratio(baseline)
            if args.require_binding and raw!=actual:
                raise ValueError('Actual installed CombatAction pair does not reproduce')
            prepared=specialize(baseline)[0]
            candidate_prepared,receipt=specialize(raw)
            if not receipt.get('native_object_table_ratio_entries'):
                if args.require_binding:
                    raise ValueError('Installed pair specialization receipt missing')
                candidate_prepared=binding.retag_specialized(raw,candidate_prepared)
            if candidate_prepared!=binding.replace_object_table_ratio(prepared):
                raise ValueError('Raw/prepared table-ratio proposals differ')
            for literal,reference,candidate in ((False,baseline,raw),(True,prepared,candidate_prepared)):
                if (binding.remove_object_table_ratio(candidate)!=reference or
                    binding.replace_object_table_ratio(candidate)!=candidate or
                    binding.installed_entries(candidate)!=list(binding.ENTRIES)):
                    raise ValueError('Complete CombatAction table-ratio inverse differs')
                old,new=({m[1]:m[0] for m in FUNCTIONS.finditer(text)}for text in (reference,candidate))
                if old.keys()!=new.keys() or {key for key in old if old[key]!=new[key]}!=set(binding.ENTRIES):
                    raise ValueError('Neighboring complete CombatAction body changed')
                parent=new[binding.PARENT];leaf=new[binding.LEAF]
                ptag=binding.prefix(binding.PARENT,literal);ltag=binding.prefix(binding.LEAF,literal)
                controls={
                    'parent-target':candidate.replace(parent,parent.replace('run_24b430(s);','run_165590(s);',1),1),
                    'native-parent-target':candidate.replace('run_24b430(child);','run_165590(child);',1),
                    'missing-leaf-tag':candidate.replace(ltag,'',1),
                    'missing-parent-tag':candidate.replace(ptag,'',1),
                    'mixed-tag':candidate.replace(ptag,binding.prefix(binding.PARENT,not literal),1),
                    'leaf-tag-position':candidate.replace(ltag,'\n    ;'+ltag,1),
                    'leaf-read':candidate.replace(leaf,leaf.replace('0x17eu','0x17cu',1),1),
                    'missing-include':candidate.replace(binding.INCLUDE,'',1),
                    'include-position':binding.INCLUDE+candidate.replace(binding.INCLUDE,'',1),
                    'foreign-marker':candidate+'\n// '+binding.OWNER+'\n',
                    'wrong-owner':candidate.replace('void source_run_combat_action_tu15(',
                                                    'void source_run_wrong_owner(',1),
                    'prior-165590':candidate.replace(binding.definition.INCLUDE,'',1),
                }
                for label,text in controls.items():
                    if text==candidate:
                        raise ValueError('Missing table-ratio defect seam: '+label)
                    for check in (binding.replace_object_table_ratio,binding.remove_object_table_ratio,binding.installed_entries):
                        try:
                            check(text)
                        except ValueError:
                            continue
                        raise ValueError('Unrejected table-ratio defect: '+label)
                tag='prepared' if literal else 'raw'
                cpp=scratch/(origin+'-'+tag+'.cpp');cpp.write_text(candidate)
                syntax=[]
                for compiler in args.compiler or ('g++','clang++'):
                    command=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(cpp)]
                    checked=subprocess.run(command,capture_output=True,text=True,timeout=1200)
                    if checked.returncode:
                        raise ValueError('Whole CombatAction table-ratio syntax failed: '+checked.stderr[-8000:])
                    syntax.append(command)
                report['contexts'].append(dict(origin=origin,format=tag,syntax=syntax,
                    candidate_sha256=digest(candidate),baseline_sha256=digest(reference),
                    other_complete_bodies=len(old)-len(binding.ENTRIES),structural_refusals=sorted(controls)))
                print(origin,tag,'passed',flush=True)
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
