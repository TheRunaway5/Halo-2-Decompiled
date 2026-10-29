#!/usr/bin/env python3
"""Compile the semantic AI leaf test and reject actual algorithm mutations.

Run through tools/run_test.py; every object, executable and mutated source is
private scratch. The retained report includes commands and tested source hashes.
This is a component proof, not a full campaign or installed runtime claim.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
CORE=Path('src/campaign/ai/native_campaign_ai_queries_tu15.cpp')
BINDING=Path('src/campaign/ai/native_campaign_ai_queries_tu15_binding.cpp')
TEST=Path('tests/native_campaign_ai_queries_tu15_tests.cpp')
FIXTURE=Path('tests/fixtures/native_campaign_ai_queries_tu15_original.json')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',default='g++');parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);steps=[]
    flags=['-std=c++23','-O1','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include')]
    if args.sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    def run(command,expected=0):
        process=subprocess.run([str(x) for x in command],cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        steps.append(dict(command=[str(x) for x in command],returncode=process.returncode,output=process.stdout[-3000:]))
        assert (process.returncode==0) if expected==0 else (process.returncode!=0),steps[-1]
        return process.stdout
    test_object=scratch/'test.o';binding_object=scratch/'binding.o'
    run([args.compiler,*flags,'-c',ROOT/TEST,'-o',test_object]);run([args.compiler,*flags,'-c',ROOT/BINDING,'-o',binding_object])
    source=(ROOT/CORE).read_text()
    variants={'native':source,
        'score_inversion':source.replace('return records.query_blocked() ? 0u : 3u;','return records.query_blocked() ? 3u : 0u;'),
        'reset_mark_inversion':source.replace('!preserve_unmarked || record.marked()','!preserve_unmarked || !record.marked()')}
    results={}
    for name,text in variants.items():
        if name!='native':assert text!=source
        path=scratch/(name+'.cpp');path.write_text(text);object_path=scratch/(name+'.o');binary=scratch/name
        run([args.compiler,*flags,'-c',path,'-o',object_path])
        run([args.compiler,*flags,test_object,binding_object,object_path,'-o',binary])
        output=run([binary,ROOT/FIXTURE],0 if name=='native' else 1)
        if name!='native':assert any(message in output for message in (
            'semantic behavior score differs','ordered persistent stores differ',
            'lazy ordered semantic reads differ')),output
        results[name]=dict(expected_failure=name!='native',result=output.strip())
    paths=[CORE,BINDING,TEST,FIXTURE,Path('include/halo2/native_campaign_ai_queries_tu15.hpp'),
           Path('include/halo2/native_campaign_ai_queries_tu15_binding.hpp')]
    report=dict(passed=True,scope=__doc__,compiler=args.compiler,sanitizers=args.sanitizers,
        source_sha256={str(path):hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in paths},
        results=results,steps=steps,retained_large_artifacts=[],scratch=str(scratch),
        production_instruction_decoder=False,production_original_execution=False,
        full_campaign_or_selected_delivery_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('steps','source_sha256','scope')}))

if __name__=='__main__':main()
