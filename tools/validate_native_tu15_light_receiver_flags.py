#!/usr/bin/env python3
"""Whole unchanged31650 versus native memory and typed sphere flag kernels.

Use run_test.py. Compare every persistent byte, original read/write chronology,
store PCs/before/after bytes and live argument cells. No instruction patch or
original service outcome is supplied. The unreachable byte0 store is a prefix.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base
from native_tu15_light_receiver_flag_cases import cases,SECTIONS_MAP,SP

ROOT=Path(__file__).resolve().parents[1]


def outside_stack(store):return not 0x62000000<=store['address']<0x62020000 or store['address']>=SP


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    if not args.xbe.is_file():return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=base.PIN:raise ValueError('Wrong original TU1.5 executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);base.SECTIONS_MAP=SECTIONS_MAP
    metadata=base.prepare(image,scratch,(0x31650,));rows=cases(image)
    (scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    run=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=300)
    if run.returncode:raise ValueError((run.stdout+run.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());digest=hashlib.sha256();summary=[]
    with (scratch/'native.log').open('w+')as errors:
        p=subprocess.Popen([str(args.native.resolve())],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,proof['cases'],strict=True):
                if not gold['completed']:raise ValueError(row['name']+' original did not complete')
                variants=[dict(row,observe=False),dict(row,observe=True)]
                if row['typed']:variants.append(dict(row,projection='typed'))
                for variant in variants:
                    typed=variant.get('projection')=='typed';name=row['name']+(' typed'if typed else' observed'if variant['observe']else' plain')
                    p.stdin.write(json.dumps(variant,separators=(',',':'))+'\n');p.stdin.flush();line=p.stdout.readline()
                    if not line:errors.seek(0);raise ValueError(errors.read()[-3000:])
                    actual=json.loads(line)
                    for a,b in zip(actual['banks'],gold['banks'],strict=True):
                        if a['address']!=b['address']:raise ValueError(name+' bank topology differs')
                        offset=2*(SP-a['address']+(8 if typed else 0))if a['address']==0x62000000 else 0
                        if a['after'][offset:]!=b['after'][offset:]:
                            first=next(i for i,(x,y)in enumerate(zip(a['after'][offset:],b['after'][offset:]))if x!=y)
                            raise ValueError(name+' bytes differ at '+hex(a['address']+offset//2+first//2))
                    if not typed:
                        expected=[{k:s[k]for k in('address','before','written')}for s in gold['writes']if outside_stack(s)]
                        writes=[s for s in actual['writes']if outside_stack(s)]
                        if writes!=expected:raise ValueError(name+' original write history differs')
                        expected_reads=[[s['address'],s['bytes']]for s in gold['reads']if not 0x62000000<=s['address']<0x62020000]
                        reads=[s for s in actual['reads']if not 0x62000000<=s[0]<0x62020000]
                        if reads!=expected_reads:
                            i=next((i for i,(a,b)in enumerate(zip(reads,expected_reads))if a!=b),min(len(reads),len(expected_reads)))
                            raise ValueError(name+' original read history differs at '+str(i)+': '+str(reads[i:i+2])+' vs '+str(expected_reads[i:i+2]))
                        if variant['observe']and actual['stores']!=[s for s in gold['writes']if outside_stack(s)]:raise ValueError(name+' original observed store PCs/bytes differ')
                    if actual['stopped']!=row.get('expect_zero_fault',False):raise ValueError(name+' fault prefix differs')
                    if gold['after_fp']!=row['before_fp']:raise ValueError(name+' original31650 changed FP controls/status')
                    digest.update(json.dumps(actual,sort_keys=True,separators=(',',':')).encode())
                    summary.append(dict(id=row['id'],projection='typed'if typed else'observed'if variant['observe']else'plain',passed=True,result=actual['result']))
            p.stdin.close()
            if p.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if p.poll()is None:p.terminate();p.wait(timeout=10)
    report=dict(passed=True,physical_cases=len(rows),comparisons=len(summary),original_instructions=proof['original_instructions'],
        original_xbe_sha256=base.PIN,owners=metadata['owners'],comparison_sha256=digest.hexdigest(),results=summary,
        instruction_patches=proof['instruction_patches'],supplied_original_outcomes=proof['supplied_original_outcomes'],
        helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),owned_scratch=str(scratch),retained_large_artifacts=[],scope=__doc__)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',len(rows),'whole original31650 input histories,',len(summary),'native/typed comparisons,',proof['original_instructions'],'unchanged instructions')
    return 0


if __name__=='__main__':raise SystemExit(main())
