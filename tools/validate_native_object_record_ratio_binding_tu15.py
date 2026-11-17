#!/usr/bin/env python3
"""Validate the private exact Primary 107140 binding and adjacent caller."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_object_record_ratio_replacements as binding
from specialize_native_operands import specialize
from validate_native_object_record_ratio_source_tu15 import GEN, ROOT, UNIT, digest

FUNCTIONS = re.compile(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{.*?^\}',re.M|re.S)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),
                retained_large_artifacts=[],contexts=[])
    try:
        fresh=scratch/'fresh.cpp'
        generation=[sys.executable,'-B',ROOT/GEN,'--output',fresh,'--manifest',scratch/'generation.json']
        generated=subprocess.run(generation,cwd=ROOT,capture_output=True,text=True,timeout=600)
        if generated.returncode:
            raise ValueError('Genuine Primary generation failed: '+generated.stderr[-8000:])
        report['generation']=[str(arg) for arg in generation]
        for origin,actual in (('current',(ROOT/UNIT).read_text()),('fresh',fresh.read_text())):
            if args.require_binding:
                binding.installed_entries(actual)
            canonical=binding.remove_object_record_ratio(actual)
            raw=binding.replace_object_record_ratio(canonical)
            if args.require_binding and raw!=actual:
                raise ValueError('Installed 107140 does not reproduce the exact proposal')
            prepared=specialize(canonical)[0]
            candidate_prepared,receipt=specialize(raw)
            if args.require_binding:
                if receipt.get('native_object_record_ratio_entries')!=[binding.ENTRY]:
                    raise ValueError('Installed 107140 specialization receipt missing')
            elif not receipt.get('native_object_record_ratio_entries'):
                candidate_prepared=binding.retag_specialized(raw,candidate_prepared)
            if candidate_prepared!=binding.replace_object_record_ratio(prepared):
                raise ValueError('107140 raw specialization differs from prepared binding')
            for format_name,baseline,candidate in (('raw',canonical,raw),('prepared',prepared,candidate_prepared)):
                if (binding.remove_object_record_ratio(candidate)!=baseline or
                    binding.replace_object_record_ratio(candidate)!=candidate or
                    binding.installed_entries(candidate)!=[binding.ENTRY]):
                    raise ValueError('Exact 107140 whole source inverse changed')
                old,new=({match[1]:match[0] for match in FUNCTIONS.finditer(text)} for text in (baseline,candidate))
                if old.keys()!=new.keys() or [key for key in old if old[key]!=new[key]]!=[binding.ENTRY]:
                    raise ValueError('A neighboring complete Primary body changed')
                tag=binding.prefix(format_name=='prepared')
                leaf=new[binding.ENTRY]
                controls={
                    'leaf':candidate.replace(leaf,leaf.replace('0x2c8u+s.gp[2]','0x2c4u+s.gp[2]',1),1),
                    'parent':candidate.replace('s.push(0x104815u,0x10481au);run_107140(s);',
                                               's.push(0x104815u,0x104819u);run_107140(s);',1),
                    'wrong-tag':candidate.replace(tag,binding.prefix(format_name=='raw'),1),
                    'missing-tag':candidate.replace(tag,'',1),
                    'tag-position':candidate.replace(tag,'\n    ;'+tag,1),
                    'missing-include':candidate.replace(binding.INCLUDE,'',1),
                    'include-position':binding.INCLUDE+candidate.replace(binding.INCLUDE,'',1),
                    'missing-ordered-shift4':candidate.replace(binding.ORDERED_SHIFTS[0][1],binding.ORDERED_SHIFTS[0][0],1),
                    'missing-ordered-shift6':candidate.replace(binding.ORDERED_SHIFTS[1][1],binding.ORDERED_SHIFTS[1][0],1),
                    'external-marker':candidate+'\n// '+binding.OWNER+'\n',
                    'extra-marker':candidate.replace(tag,tag+tag,1),
                    'other-owner':candidate.replace('void source_run_physics_live_support_tu15(',
                                                    'void source_run_wrong_owner(',1),
                }
                for label,text in controls.items():
                    if text==candidate:
                        raise ValueError('Missing 107140 defect seam: '+label)
                    for check in (binding.replace_object_record_ratio,binding.remove_object_record_ratio,binding.installed_entries):
                        try:
                            check(text)
                        except ValueError:
                            continue
                        raise ValueError('Unrejected 107140 structural defect: '+label)
                cpp=scratch/(origin+'-'+format_name+'.cpp')
                cpp.write_text(candidate)
                syntax=[]
                for compiler in args.compiler or ('g++','clang++'):
                    command=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),
                             '-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/physics'),str(cpp)]
                    checked=subprocess.run(command,capture_output=True,text=True,timeout=1200)
                    if checked.returncode:
                        raise ValueError('Whole Primary 107140 syntax failed: '+checked.stderr[-8000:])
                    syntax.append(command)
                report['contexts'].append(dict(origin=origin,format=format_name,syntax=syntax,
                    candidate_sha256=digest(candidate),baseline_sha256=digest(baseline),
                    neighboring_bodies=len(old)-1,structural_refusals=sorted(controls)))
                print(origin,format_name,'passed',flush=True)
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        (output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
