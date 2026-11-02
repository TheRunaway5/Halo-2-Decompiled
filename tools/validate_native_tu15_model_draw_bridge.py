#!/usr/bin/env python3
"""Whole original model parents versus the production World's native2CBD0 bridge.

No original instruction or service outcome is replaced. Compare every retained
non-stack bank, caller-visible stack, parent/append stores, consumed ABI and FP.
Reference/native per-call snapshots additionally protect every live stack copy.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base
from native_tu15_model_draw_cases import cases,SECTIONS_MAP,STACK_BASE,STACK_END

ROOT=Path(__file__).resolve().parents[1]


def sha(data):return hashlib.sha256(data).hexdigest()


def selected_stores(stores,implicit_pushes):
    return [s for s in stores if s['instruction']not in implicit_pushes and(not STACK_BASE<=s['address']<STACK_END or
        0x1365c0<=s['instruction']<0x136d50 or s['instruction']in(0x2cddb,0x2cde1,0x2ce13))]


def compare_banks(actual,expected,sp,name):
    for left,right in zip(actual,expected,strict=True):
        if left['address']!=right['address']:raise ValueError(name+' changed bank topology')
        offset=(sp-STACK_BASE)*2 if left['address']==STACK_BASE else 0
        a,b=left['after'][offset:],right['after'][offset:]
        if a!=b:
            first=next(i for i,(x,y)in enumerate(zip(a,b))if x!=y)
            raise ValueError(name+' bank differs at '+hex(left['address']+offset//2+first//2))


def call_contract(call):
    # XMM values are dead; upper lanes0/1 can still affect packed arithmetic's
    # MXCSR status in a later child builder. Compare that exact residue too.
    # Preserve the consumed ABI and original full EAX return word.
    gp=call['gp']
    return dict(before=call['before'],gp=gp if call['before']else[gp[0],*gp[3:]],
        fp=call['fp'],mxcsr_live_lanes=[call['xmm'][r][1:]for r in(0,1)],
        stack_address=call['stack_address'],stack_above=call['stack_above'])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--map',type=Path,required=True)
    parser.add_argument('--native',type=Path);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--physical-only',action='store_true',help='Validate original inputs only; does not qualify the native owner')
    args=parser.parse_args()
    if not args.xbe.is_file()or not args.map.is_file():return 77
    if sha(args.xbe.read_bytes())!=base.PIN:raise ValueError('Wrong original TU1.5 executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);base.SECTIONS_MAP=SECTIONS_MAP
    metadata=base.prepare(image,scratch,(0x1365c0,0x136a80,0x2cbd0,0x44930,0x2cfe0,0x449d0,0x16ac0,
        0x135dc0,0x30270,0xbcdf0,0x130980,0x22a680))
    metadata['trace_calls']=[dict(entry=0x2cbd0,arguments=16,gp=True,xmm=True),dict(entry=0x136ce5,arguments=0,gp=True,xmm=True)]
    from oracle_smg_firing_1_1 import _Code
    code=_Code(image,'.text')
    explicit_writes={s['instruction']for s in metadata['sites']if any(o['write']for o in s['operands'])}
    # Hardware observations describe explicit decoded memory operands.
    # World w() also reports PUSH/CALL's implicit stack writes. Exclude only
    # those authenticated PCs from store traces; stack-byte checks retain them.
    implicit_pushes={s['instruction']for s in metadata['sites']if s['instruction']not in explicit_writes and
        code.at(s['instruction']).mnemonic in('push','call')}
    rows=cases(image)
    # Make the observer's historical default SIMD pattern an explicit input
    # to both executors before comparing upper-lane return residues.
    for row in rows:row.setdefault('xmm',[[0x6b000000+row['id']*37+4*r+lane for lane in range(4)]for r in range(8)])
    (scratch/'metadata.json').write_text(json.dumps(metadata))
    (scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    run=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),
        str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=180)
    if run.returncode:raise ValueError((run.stdout+run.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());references=[];results=[]
    if args.physical_only:
        complete=all(c['completed']for c in proof['cases'])
        report=dict(passed=False,physical_inputs_valid=complete,validation_not_run=True,
            physical_cases=len(rows),original_instructions=proof['original_instructions'],
            observed_sha256=sha((scratch/'observed.json').read_bytes()),original_xbe_sha256=base.PIN,
            owned_scratch=str(scratch),retained_large_artifacts=[])
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
        print('Original-only inputs:',len(rows),'cases,',proof['original_instructions'],'unchanged instructions, complete=',complete)
        return int(not complete)
    if args.native is None:parser.error('--native is required for full validation')
    for role,reference,original_resource in [('reference',True,False),('native',False,False),('native-original-resource',False,True)]:
        log=scratch/(role+'.log');digest=hashlib.sha256();calls_total=0
        environment={**os.environ,'HALO2_REFERENCE_MODEL_DRAW_UNITS':'1'if reference else'',
            'HALO2_REFERENCE_GEOMETRY_RESOURCE':'1'if original_resource else'','HALO2_NATIVE_DRAW_UNIT_STATS':'1'}
        command=[str(args.native.resolve()),str(args.xbe.resolve()),str(args.map.resolve()),str(scratch/(role+'-saves'))]
        with log.open('w+')as errors:
            process=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True,env=environment)
            try:
                for index,(row,gold)in enumerate(zip(rows,proof['cases'],strict=True)):
                    if not gold['completed']:raise ValueError(row['name']+' original parent did not complete')
                    process.stdin.write(json.dumps(row,separators=(',',':'))+'\n');process.stdin.flush()
                    line=process.stdout.readline()
                    if not line:errors.seek(0);raise ValueError(errors.read()[-3500:])
                    actual=json.loads(line);name=role+': '+row['name']
                    compare_banks(actual['banks'],gold['banks'],row['gp'][4],name)
                    if actual['gp'][3:]!=gold['boundary_gp'][3:]:raise ValueError(name+' consumed parent ABI differs')
                    if actual['fp']!=gold['after_fp']:raise ValueError(name+' carried FP differs')
                    unknown={s['instruction']for s in actual['stores']}-explicit_writes-implicit_pushes
                    if unknown:raise ValueError(name+' unqualified native store PCs '+str(sorted(map(hex,unknown))))
                    stores=selected_stores(actual['stores'],implicit_pushes);expected=selected_stores(gold['writes'],implicit_pushes)
                    if stores!=expected:
                        first=next((i for i,(x,y)in enumerate(zip(stores,expected))if x!=y),min(len(stores),len(expected)))
                        raise ValueError(name+' original store history differs at '+str(first)+': '+str(stores[first:first+1])+' vs '+str(expected[first:first+1]))
                    calls=[call_contract(c)for c in actual['calls']];calls_total+=len(calls)//2
                    before=[c for c in calls if c['before']];after=[c for c in calls if not c['before']]
                    original_before=[c for c in gold['calls']if c['entry']==0x2cbd0]
                    original_after=[c for c in gold['calls']if c['entry']==0x136ce5]
                    for c,g in zip(before,original_before,strict=True):
                        arguments=list(struct.unpack('<16I',bytes.fromhex(c['stack_above'])[4:68]))
                        if c['gp']!=g['gp']or arguments!=g['arguments']:raise ValueError(name+' actual2CBD0 inputs differ')
                    for c,g in zip(after,original_after,strict=True):
                        if c['gp']!=[g['gp'][0],*g['gp'][3:]]:raise ValueError(name+' native2CBD0 return contract differs')
                        if c['mxcsr_live_lanes']!=[g['xmm'][r][1:]for r in(0,1)]:raise ValueError(name+' original MXCSR-live XMM lanes differ')
                    if reference:references.append(calls)
                    elif calls!=references[index]:raise ValueError(name+' live per-append stack/ABI/FP differs from genuine2CBD0')
                    digest.update(json.dumps(dict(banks=actual['banks'],stores=stores,calls=calls),sort_keys=True).encode())
                process.stdin.close()
                if process.wait(timeout=20):errors.seek(0);raise ValueError(errors.read()[-3500:])
            finally:
                if process.poll()is None:process.terminate();process.wait(timeout=10)
        results.append(dict(role=role,passed=True,whole_parents=len(rows),model_calls=calls_total,
            observed_sha256=digest.hexdigest(),command=command,environment={k:environment[k]for k in (
                'HALO2_REFERENCE_MODEL_DRAW_UNITS','HALO2_REFERENCE_GEOMETRY_RESOURCE','HALO2_NATIVE_DRAW_UNIT_STATS')},
            diagnostic_tail=log.read_text()[-1800:]))
    report=dict(passed=True,physical_cases=len(rows),original_instructions=proof['original_instructions'],
        original_xbe_sha256=base.PIN,owners=metadata['owners'],results=results,
        instruction_patches=proof['instruction_patches'],supplied_original_outcomes=proof['supplied_original_outcomes'],
        helper_sha256=sha(Path(__file__).read_bytes()),retained_large_artifacts=[],scope=__doc__)
    report['implicit_stack_push_pcs_omitted_from_trace']=sorted(implicit_pushes)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',len(rows),'whole original136A80/1365C0 histories against native/genuine World2CBD0, original stores, live stack copies and FP')
    return 0


if __name__=='__main__':raise SystemExit(main())
