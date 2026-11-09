#!/usr/bin/env python3
"""Whole physical TU1.5 1BF30 shader LOD versus native SSE outputs/FP.

Use run_test.py. Virtual shader cases stop at their genuine service entry and
must be rejected natively, without publishing an index or arithmetic changes.
All ordinary cases execute unchanged original instructions to completion.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base

ROOT=Path(__file__).resolve().parents[1]
DEF,REF,TEMPLATE,LOD,ROWS=range(0x1000000,0x1005000,4096)
TAGS=0x1010000;OUTPUT=base.SP+0x100
BANKS={'constant':(0x46a000,4096),'view':(0x492000,4096),'flags':(0x4c6000,4096),
    'tags':(0x4f0000,4096),'signed_group':(0xf10000,4096),'data':(0x1000000,0x12000),
    'stack':(0x62000000,0x20000)}


def signed(v):return struct.unpack('<i',struct.pack('<I',v&0xffffffff))[0]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]


def cases(image):
    result=[]
    def add(count=6,metric=0x3f800000,bias=0,modifier=0,factor=0x3f800000,
            fp=(0x27f,0x220,0x1f80),shader=0xeeee0000,thresholds=None,group=0x73686164):
        banks=[dict(address=at,raw=bytearray(n))for at,n in BANKS.values()]
        banks[0]['raw'][:]=image.read(0x46a000,4096)
        def put(at,value,n=4):
            at&=0xffffffff
            for b in banks:
                if b['address']<=at and at+n<=b['address']+len(b['raw']):
                    b['raw'][at-b['address']:at-b['address']+n]=(value&((1<<(8*n))-1)).to_bytes(n,'little');return
            raise ValueError('Unmapped declared shader input '+hex(at))
        index=shader&65535;tags=(TAGS-16*index)&0xffffffff;template_id=(index+1)&65535
        signed_index=index if index<32768 else index-65536
        put(0x4f0414,tags);put(tags+16*signed_index,group);put(TAGS+8,DEF)
        put(DEF+0x24,REF);put(REF,0xaabb0000|template_id);put(tags+16*template_id+8,TEMPLATE)
        put(TEMPLATE+0x5c,LOD);put(LOD,count);put(LOD+4,ROWS if signed(count)>0 else 0xdeadbeef)
        put(0x4c6839,modifier,1);put(0x492348+4*bias,factor)
        for i,t in enumerate(thresholds or list(map(bits,(4,2,1,.5,0,-1)))):put(ROWS+10*i+6,t)
        put(OUTPUT,0x9b7d5311);put(base.SP+12,OUTPUT)
        row=dict(id=len(result),name=f'shader-lod-{len(result)}',entry=0x1bf30,gp=[bias,0,0,0,base.SP,0,0,0],
            matrix=shader,source=metric,before_fp=list(fp),live_x87=[],max_steps=10000,
            banks=[dict(address=b['address'],before=b['raw'].hex())for b in banks])
        if group in (0x5052544d,0x70727433):row['stop_before_entries']=[0x13a6b0];row['native_rejection']=True
        result.append(row)
    fps=((0x27f,0x220,0x1f80),(0x23f,0,0x1fa0),(0x27f,0,0x9fc0),
         (0x27f,0,0x3f80),(0x27f,0,0x5f80),(0x27f,0,0x7f80))
    for count,metric,bias,modifier,fp in itertools.product((0,1,2,6,0xffffffff,0x80000000),
            map(bits,(.5,1,2,4)),(0,5),(0,1),fps):add(count,metric,bias,modifier,fp=fp)
    special=(0,0x80000000,1,0x80000001,0x00800000,0x7fc12345,0x7f812345,0x7fc54321,0x7f800000,0xff800000)
    for metric,factor,fp in itertools.product(special,special,fps):add(metric=metric,factor=factor,fp=fp)
    for threshold,fp in itertools.product(special,fps):add(thresholds=[threshold]*6,fp=fp)
    for shader,bias in itertools.product((0xcafe0000,0xbeef0001,0xdead7fff,0xabcd8000,0xabcdffff),
            (0,1,5,6,0x40000001,0x80000005,0xffffffff)):add(shader=shader,bias=bias)
    for metric,fp,modifier in itertools.product(special,fps,(0,1)):add(metric=metric,fp=fp,modifier=modifier,bias=5)
    for group in (0x5052544d,0x70727433):add(group=group)
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest()!=base.PIN:raise ValueError('Wrong TU1.5 executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);base.SECTIONS_MAP=BANKS
    meta=base.prepare(image,scratch,(0x1bf30,));rows=cases(image);digest=hashlib.sha256();instructions=0;summary=[]
    with (scratch/'native.log').open('w+')as errors:
        native=subprocess.Popen([str(a.native.resolve())],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for start in range(0,len(rows),15):
                batch=rows[start:start+15];before=scratch/'before.json';after=scratch/'after.json'
                before.write_text(json.dumps(batch,separators=(',',':')))
                run=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(before),str(after)],capture_output=True,text=True,timeout=240)
                if run.returncode:raise ValueError((run.stdout+run.stderr)[-2000:])
                proof=json.loads(after.read_text());before.unlink();after.unlink();instructions+=proof['original_instructions']
                if proof['instruction_patches']or proof['supplied_original_outcomes']:raise ValueError('Original shader proof was substituted')
                for row,gold in zip(batch,proof['cases'],strict=True):
                    if not gold['completed']:raise ValueError('Original shader did not complete')
                    native.stdin.write(json.dumps(row,separators=(',',':'))+'\n');native.stdin.flush();line=native.stdout.readline()
                    if not line:errors.seek(0);raise ValueError(errors.read()[-2000:])
                    actual=json.loads(line)
                    if row.get('native_rejection'):
                        expected=dict(rejected=True,fp=gold['after_fp'])
                    else:
                        stack=next(b for b in gold['banks']if b['address']==0x62000000)
                        index=signed(int.from_bytes(bytes.fromhex(stack['after'][2*(OUTPUT-stack['address']):2*(OUTPUT-stack['address']+4)]),'little'))
                        stores=[signed(int.from_bytes(bytes.fromhex(w['written']),'little'))for w in gold['writes']if w['address']==OUTPUT]
                        expected=dict(rejected=False,index=index,accepted=bool(gold['boundary_gp'][0]&255),stores=stores,fp=gold['after_fp'])
                    if actual!=expected:raise ValueError(row['name']+' differs '+json.dumps(dict(actual=actual,expected=expected)))
                    initial={b['address']:b['before']for b in row['banks']}
                    for bank in gold['banks']:
                        if bank['address']!=0x62000000 and bank['after']!=initial[bank['address']]:raise ValueError('Original shader wrote a non-stack bank')
                    digest.update(json.dumps(actual,sort_keys=True,separators=(',',':')).encode())
                    summary.append(dict(id=row['id'],passed=True,rejected=actual['rejected']))
                print('whole original1BF30 cases',min(start+15,len(rows)),'/',len(rows),flush=True)
            native.stdin.close()
            if native.wait(timeout=10):raise ValueError('Native shader runner failed')
        finally:
            if native.poll()is None:native.terminate();native.wait(timeout=10)
    report=dict(passed=True,cases=len(rows),whole_cases=sum(not r.get('native_rejection')for r in rows),
        original_service_prefixes=sum(bool(r.get('native_rejection'))for r in rows),original_instructions=instructions,
        comparison_sha256=digest.hexdigest(),original_xbe_sha256=base.PIN,owners=meta['owners'],case_results=summary,
        instruction_patches=[],supplied_original_outcomes=False,retained_large_artifacts=[],owned_scratch=str(scratch),
        scope='Whole unchanged ordinary1BF30 physical i386 versus native output index, AL, ordered index stores and complete FP; virtual shader service-entry prefixes retain native rejections. No live World/GPU claim.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');return 0


if __name__=='__main__':raise SystemExit(main())
