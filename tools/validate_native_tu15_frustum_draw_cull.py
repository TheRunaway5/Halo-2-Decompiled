#!/usr/bin/env python3
"""Whole unchanged TU1.5 frustum cull versus native typed model/mask outputs.

Use run_test.py. All original executable copies, banks and builds stay scratch.
No instruction, pose, frustum predicate or original callee result is replaced.
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
from native_tu15_frustum_draw_cull_cases import cases,SECTIONS_MAP,MASK,SP,STOP

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--genuine-zanzibar',action='store_true');a=parser.parse_args()
    if not a.xbe.is_file():return 77
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest()!=base.PIN:raise ValueError('Wrong original TU1.5 executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);base.SECTIONS_MAP=SECTIONS_MAP
    # Improve only the owned read-only observer's fault diagnostic. The ELF
    # still contains the unchanged complete original text bytes.
    observer_root=scratch/'observer-source';(observer_root/'tools').mkdir(parents=True)
    observer=(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp').read_text()
    before='check(it!=sites.end(),"Original geometry escaped authenticated leaf bodies");'
    after='check(it!=sites.end(),("Original geometry escaped authenticated leaf bodies at PC="+std::to_string(pc)+" case="+std::to_string(c.at("id").get<unsigned>())).c_str());'
    if observer.count(before)!=1:raise ValueError('Read-only observer diagnostic site changed')
    (observer_root/'tools/observe_native_campaign_ai_vector_math_tu15.cpp').write_text(observer.replace(before,after))
    base.ROOT=observer_root
    metadata=base.prepare(image,scratch,(0x44710,0x44680,0x44e80,0x145560,0x167ef0,0x184690,0x32a660))
    metadata['trace_calls']=[dict(entry=0x44e80,arguments=2),dict(entry=0x167ef0,arguments=2,xmm=True,indirect=[dict(argument=0,words=3)])]
    genuine=[]
    if a.genuine_zanzibar:
        from native_tu15_zanzibar_frustum_inputs import inputs
        genuine=inputs()
    (scratch/'metadata.json').write_text(json.dumps(metadata));rows=cases(image,genuine)
    summary=[];instructions=0;digest=hashlib.sha256();original_patches=[]
    with (scratch/'native.log').open('w+')as errors:
        native=subprocess.Popen([str(a.native.resolve())],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for first in range(0,len(rows),12):
                batch=rows[first:first+12];input_path=scratch/'batch-inputs.json';observed_path=scratch/'batch-observed.json'
                input_path.write_text(json.dumps(batch,separators=(',',':')))
                run=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(input_path),str(observed_path)],capture_output=True,text=True,timeout=240)
                if run.returncode:raise ValueError((run.stdout+run.stderr)[-3000:])
                proof=json.loads(observed_path.read_text());instructions+=proof['original_instructions'];original_patches+=proof['instruction_patches']
                if proof['supplied_original_outcomes']or proof['instruction_patches']:raise ValueError('Original frustum proof was substituted')
                for row,gold in zip(batch,proof['cases'],strict=True):
                    if not gold['completed']:raise ValueError(row['name']+' original did not complete: '+str(gold.get('boundary')))
                    def read(at,n):
                        for bank in gold['banks']:
                            size=len(bank['after'])//2
                            if bank['address']<=at and at+n<=bank['address']+size:
                                offset=2*(at-bank['address']);return bytes.fromhex(bank['after'][offset:offset+2*n])
                        raise ValueError(hex(at))
                    units=[list(struct.unpack('<8I',read(0x4c6958+32*i,32)))for i in range(len(row['spec']['units']))]
                    arena=list(struct.unpack('<512I',read(MASK,2048)))
                    if 'genuine_expected'in row:
                        witness=row['genuine_expected']
                        if [u[0]for u in units]!=witness['flags']or arena!=witness['arena']:
                            raise ValueError(row['name']+' physical TU1.5 differs from genuine captured flags/masks')
                    calls=[c['arguments']for c in gold['calls']if c['entry']==0x44e80]
                    points=[[*c['indirect'],c['xmm'][7][0]]for c in gold['calls']if c['entry']==0x167ef0]
                    for observe in (False,True):
                        request=dict(spec=row['spec'],before_fp=row['before_fp'],observe=observe,expect_rejection=row.get('native_rejection',False))
                        native.stdin.write(json.dumps(request,separators=(',',':'))+'\n');native.stdin.flush();line=native.stdout.readline()
                        if not line:errors.seek(0);raise ValueError(row['name']+': '+errors.read()[-3000:])
                        actual=json.loads(line)
                        if row.get('native_rejection'):
                            corrupted=struct.unpack('<I',read(SP,4))[0]
                            if corrupted==STOP or not actual['rejected']:raise ValueError('Original256-node stack-corruption control lost its native rejection')
                            if actual['units']!=[u['words']for u in row['spec']['units']]or actual['arena']!=row['spec']['arena']:
                                raise ValueError('Rejected native matrix count published partial output')
                            digest.update(json.dumps(actual,sort_keys=True,separators=(',',':')).encode());continue
                        if actual['rejected']:raise ValueError('Valid native frustum input rejected')
                        for name,expected in [('units',units),('arena',arena),('calls',calls),('fp',gold['after_fp'])]:
                            if actual[name]!=expected:raise ValueError(row['name']+' '+name+' differs: native='+str(actual[name])[:1800]+' original='+str(expected)[:1800])
                        if actual['arena_used']!=row['spec']['arena_used']:raise ValueError('Frustum cull allocated a mask')
                        if observe and not row['spec']['walk'] and actual['points']!=points:
                            raise ValueError(row['name']+' exact transformed centre/radius words differ: '+str(actual['points'][:4])+' versus '+str(points[:4]))
                        digest.update(json.dumps(actual,sort_keys=True,separators=(',',':')).encode())
                    summary.append(dict(id=row['id'],name=row['name'],passed=True,original_calls=len(calls),bound_tests=len(points),fp=gold['after_fp'],
                        original_fault_prefix=row.get('native_rejection',False),source_return_word=struct.unpack('<I',read(SP,4))[0]if row.get('native_rejection')else None))
                input_path.unlink();observed_path.unlink()
                print('frustum original cases',min(first+12,len(rows)),'/',len(rows),flush=True)
            native.stdin.close()
            if native.wait(timeout=15):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if native.poll()is None:native.terminate();native.wait(timeout=10)
    report=dict(passed=True,physical_cases=len(rows),whole_completed_cases=sum(not r.get('native_rejection',False)for r in rows),fault_prefix_cases=sum(r.get('native_rejection',False)for r in rows),comparisons=2*len(rows),original_instructions=instructions,
        original_xbe_sha256=base.PIN,owners=metadata['owners'],comparison_sha256=digest.hexdigest(),cases=summary,
        genuine_content_cases=len(genuine),genuine_content_witnesses=[r['expected']for r in genuine],
        instruction_patches=original_patches,supplied_original_outcomes=False,retained_large_artifacts=[],
        helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),owned_scratch=str(scratch),
        scope='Whole unchanged TU1.5 44E80 linear/model and44710/44680 walk on declared source banks. Compare every unit word, all512 shared mask words, ordered entry calls, exact transformed point/radius words in leaf calls and full x87/MXCSR. Positive BSP accelerators, live World binding, actual model scheduling and rendered frames remain separate.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');return 0


if __name__=='__main__':raise SystemExit(main())
