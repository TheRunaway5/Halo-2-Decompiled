#!/usr/bin/env python3
"""Original31650 versus its retained World binding and sphere reference control."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base
from native_tu15_light_receiver_flag_cases import cases,SECTIONS_MAP,SP,LPOOL,REC,TAGS,DEFINITION

ROOT=Path(__file__).resolve().parents[1]


def inputs(image):
    original=cases(image);rows=[]
    for row in original:
        rdata=bytes.fromhex(row['banks'][0]['before'])
        if row.get('expect_zero_fault')or rdata!=image.read(0x46a000,4096):continue
        rows.append(row)
    for index in (28,11):
        row=copy.deepcopy(original[index]);row.update(id=len(rows),name='World sphere reference '+row['name'],projection='typed-world')
        def put(at,value,size=4):
            for b in row['banks']:
                raw=bytearray.fromhex(b['before'])
                if b['address']<=at and at+size<=b['address']+len(raw):
                    raw[at-b['address']:at-b['address']+size]=value.to_bytes(size,'little');b['before']=raw.hex();return
            raise ValueError(hex(at))
        # Declared generation/header cells required by the host lifetime guard.
        # None is read by original31650; both executors receive identical bytes.
        put(LPOOL+0x24,0x110);put(LPOOL+0x38,2);put(REC,0x7701,2);put(REC+4,DEFINITION)
        put(TAGS+16*(DEFINITION&65535)+4,DEFINITION);rows.append(row)
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--map',type=Path,required=True);parser.add_argument('--native',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    if not args.xbe.is_file()or not args.map.is_file():return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=base.PIN:raise ValueError('Wrong original image')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);base.SECTIONS_MAP=SECTIONS_MAP
    metadata=base.prepare(image,scratch,(0x31650,));rows=inputs(image);(scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    run=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=180)
    if run.returncode:raise ValueError((run.stdout+run.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());results=[]
    for role in ('native','reference'):
        log=scratch/(role+'.log');digest=hashlib.sha256()
        environment={**os.environ,'HALO2_REFERENCE_LIGHT_FLAGS_31650':'1'if role=='reference'else'','HALO2_NATIVE_LIGHT_FLAGS_STATS':'1'}
        command=[str(args.native.resolve()),str(args.xbe.resolve()),str(args.map.resolve()),str(scratch/(role+'-saves'))]
        with log.open('w+')as errors:
            p=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True,env=environment)
            try:
                for row,gold in zip(rows,proof['cases'],strict=True):
                    if not gold['completed']:raise ValueError('Original World flags input failed')
                    p.stdin.write(json.dumps(row,separators=(',',':'))+'\n');p.stdin.flush();line=p.stdout.readline()
                    if not line:errors.seek(0);raise ValueError(errors.read()[-3500:])
                    actual=json.loads(line);name=role+': '+row['name'];projection=row.get('projection')=='typed-world'
                    for a,b in zip(actual['banks'],gold['banks'],strict=True):
                        if a['address']!=b['address']:raise ValueError(name+' topology differs')
                        start=2*(SP-0x62000000+(8 if projection else 0))if a['address']==0x62000000 else 0
                        if a['after'][start:]!=b['after'][start:]:
                            i=next(i for i,(x,y)in enumerate(zip(a['after'][start:],b['after'][start:]))if x!=y)
                            raise ValueError(name+' bank differs at '+hex(a['address']+start//2+i//2))
                    if not projection:
                        expected=[s for s in gold['writes']if not 0x62000000<=s['address']<SP]
                        stores=[s for s in actual['stores']if not 0x62000000<=s['address']<SP]
                        if stores!=expected:raise ValueError(name+' original store chronology differs')
                        if actual['gp'][3:]!=gold['boundary_gp'][3:]:raise ValueError(name+' callee-save/SP differs')
                        if actual['xmm']!=gold['boundary_xmm']or actual['fp']!=gold['after_fp']:raise ValueError(name+' complete SIMD/FP return differs')
                    if not actual['registered_once']or not actual['queues_restored']:raise ValueError(name+' observer/restoration failed')
                    digest.update(json.dumps(actual,sort_keys=True,separators=(',',':')).encode())
                p.stdin.close()
                if p.wait(timeout=15):errors.seek(0);raise ValueError(errors.read()[-3500:])
            finally:
                if p.poll()is None:p.terminate();p.wait(timeout=10)
        results.append(dict(role=role,passed=True,whole_calls=sum(r.get('projection')!='typed-world'for r in rows),
            sphere_projection_calls=sum(r.get('projection')=='typed-world'for r in rows),observed_sha256=digest.hexdigest(),
            command=command,diagnostic_tail=log.read_text()[-1200:]))
    report=dict(passed=True,physical_cases=len(rows),original_instructions=proof['original_instructions'],
        original_xbe_sha256=base.PIN,owners=metadata['owners'],results=results,instruction_patches=proof['instruction_patches'],
        supplied_original_outcomes=proof['supplied_original_outcomes'],retained_large_artifacts=[],
        helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scope='Unchanged physical31650 versus actual World native/genuine owners, complete consumed memory/ABI/FP/XMM and store PCs. Sphere projection checks retain original queue allocations, restore their bytes and use genuine output. Declared test banks, not a rendered-frame or physical Xbox framebuffer claim.')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS World31650:',len(rows),'histories in native/genuine modes, including original sphere projection/restoration');return 0


if __name__=='__main__':raise SystemExit(main())
