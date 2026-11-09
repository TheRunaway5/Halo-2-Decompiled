#!/usr/bin/env python3
"""Compare whole unchanged physical TU1.5 45CD0 and its native packet owner.

Use run_test.py. Genuine 140CF0 and 1E1A0 populate each declared transparent
plane table before it becomes a 45CD0 input. No instruction/callee is patched.
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
BSP,MODEL,SECTIONS,PART,MATERIAL,CLUST,INST,DEFS,VIS,ROWS,TABLE=(0x1000000+i*0x1000 for i in (*range(10),16))
GEOM=0x100b000
PACKET=base.SP+0x100
BANKS={'constant':(0x46a000,4096),'selector':(0x474000,4096),'palette':(0x494000,0x4000),
    'units':(0x4c6000,0x3000),'bsp':(0x4ec000,4096),'tags':(0x4f0000,4096),
    'visibility':(0x4f9000,4096),'hash':(0x52d000,4096),'data':(0x1000000,0x30000),
    'stack':(0x62000000,0x20000)}


def put(banks,at,value,n=4):
    for b in banks:
        if b['address']<=at and at+n<=b['address']+len(b['bytes']):
            b['bytes'][at-b['address']:at-b['address']+n]=(value&((1<<(8*n))-1)).to_bytes(n,'little');return
    raise ValueError('Unmapped declared45CD0 input '+hex(at))


def encoded(banks):return [dict(address=b['address'],before=b['bytes'].hex())for b in banks]
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]


def original(scratch,rows,name):
    source=scratch/(name+'-input.json');target=scratch/(name+'-output.json')
    source.write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(source),str(target)],capture_output=True,text=True,timeout=240)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-2500:])
    result=json.loads(target.read_text());source.unlink();target.unlink()
    if result['instruction_patches']or result['supplied_original_outcomes']:raise ValueError('Original45CD0 proof was substituted')
    if not all(c['completed']for c in result['cases']):raise ValueError('Original45CD0 did not complete')
    return result


def row(banks,entry=0x45cd0):
    return dict(id=0,name='declared45CD0',entry=entry,gp=[PACKET,0,0,0,base.SP,0,0,TABLE],matrix=0,source=0,
        before_fp=[0x27f,0x220,0x1f80],live_x87=[],max_steps=100000,packet=PACKET,banks=encoded(banks))


def table_inputs(image,scratch):
    result={};instructions=0
    for variant,keys in [('empty',[]),('cluster',[0]),('instance',[1<<16]),
        ('duplicate',[0,0]),('signed-key',[0x8000|(0xffff<<16)])]:
        banks=[dict(address=at,bytes=bytearray(n))for at,n in BANKS.values()]
        banks[0]['bytes'][:]=image.read(0x46a000,4096)
        for at,v in [(0x4ecc18,BSP),(0x52dfd0,TABLE),(BSP+0x158,len(keys)),(BSP+0x15c,ROWS),
            (TABLE+0x20,4096),(TABLE+0x24,2048),(TABLE+0x28,16),(TABLE+0x2c,0x1e8a0),(TABLE+0x30,0x1e8d0)]:put(banks,at,v)
        for i,key in enumerate(keys):
            put(banks,ROWS+20*i,key)
            for k,v in enumerate((.23+i,-.41,.87,1.4)):put(banks,ROWS+20*i+4+4*k,bits(v))
        for entry in (0x140cf0,0x1e1a0):
            r=row(banks,entry);r['gp'][2]=TABLE
            proof=original(scratch,[r],'table-'+variant+'-'+hex(entry));gold=proof['cases'][0];instructions+=proof['original_instructions']
            for b,g in zip(banks,gold['banks'],strict=True):b['bytes'][:]=bytes.fromhex(g['after'])
        result[variant]=banks
    return result,instructions


def cases(tables):
    rows=[]
    def add(kind,variant='empty',lod=0x3f800000,bias=0,rigid=-1,mapping=False,visible=True,fp=(0x27f,0x220,0x1f80),plane_values=None,
            matrix=0,part=0,unit=0,material_index=0,visibility_index=255,selector=0):
        banks=[dict(address=b['address'],bytes=bytearray(b['bytes']))for b in tables[variant]]
        def p(at,v,n=4):put(banks,at,v,n)
        instance=kind=='instance';model=kind.startswith('model');section=255 if kind=='model-ff'else 0
        p(0x4f0414,0x4f0800);p(0x4f0808,MODEL);p(MODEL+0x28,SECTIONS);p(MODEL+0x64,(MATERIAL-32*material_index)&0xffffffff);p(MODEL+4,4 if mapping else 0)
        p(SECTIONS+92*section+0x34,0 if kind=='absent'else GEOM);p(SECTIONS+92*section+0x2c,rigid,2)
        p(BSP+0xa0,CLUST);p(BSP+0xa8,(MATERIAL-32*material_index)&0xffffffff);p(BSP+0x13c,DEFS);p(BSP+0x144,INST)
        p(CLUST+0x50,0 if kind=='absent'else GEOM);p(DEFS+0x50,GEOM);p(INST+0x34,0,2)
        signed_part=struct.unpack('<h',struct.pack('<H',part&65535))[0]
        p(GEOM+4,(PART-72*signed_part)&0xffffffff);p(PART+4,material_index,2);p(PART+0x2c,lod);p(MATERIAL+12,0xaabbccdd)
        p(MATERIAL+0x1c,visibility_index if visible else 0,1)
        p(0x4f9b50,VIS);p(0x474c24,selector,2)
        if visible and visibility_index!=255:p(VIS+(selector*1024+0x201 if instance else selector*32+1)+4*(visibility_index>>5),1<<(visibility_index&31))
        signed_unit=struct.unpack('<h',struct.pack('<H',unit&65535))[0];u=0x4c6958+32*signed_unit
        p(u,0x1000 if instance else 0);p(u+4,0 if model else 0xffffffff);p(u+8,0x2000)
        p(u+12,(section<<9)|(0 if instance else 2047<<18));p(u+0x1b,0xa0|bias,1)
        palette=0x49433c+0x2000
        for node in (-1,0,1):
            for i,v in enumerate((1.7+node,-.8,.37)):p(palette+0x44+48*node+12+16*i,bits(v))
        p(palette+4,-1,2);p(palette+64,1,2)
        for i,v in enumerate((1.3,1,.2,.3,.4,.9,.6,.7,.8,1.1,2.2,-1.3,.55)):p(INST+4*i,bits(v))
        for i,v in enumerate((-.5,.25,1.7)):p(INST+0x3c+4*i,bits(v));p(PART+0x10+4*i,bits(v+1))
        if plane_values is not None:
            # The hash still points at the actual BSP row; its copied plane
            # bytes deliberately differ to catch a read from node+0xC.
            for i,v in enumerate(plane_values):p(ROWS+4+4*i,v)
        for i in range(44):p(PACKET+i,(i*37+19)&255,1)
        r=row(banks);r.update(id=len(rows),name=f'{kind}-{variant}-{len(rows)}',before_fp=list(fp),matrix=part,source=matrix);r['gp'][1]=unit;rows.append(r)
    lods=(0x3f800000,0,0x80000000,0xbf800000,1,0x80000001,0x7fc12345,0x7f812345,0x7f800000,0xff800000)
    fps=((0x27f,0x220,0x1f80),(0x23f,0,0x1fa0),(0x27f,0x220,0x9fc0),
         (0x27f,0,0x3f80),(0x27f,0,0x5f80),(0x27f,0,0x7f80))
    for kind,lod,fp in itertools.product(('model','cluster','instance'),lods,fps):
        add(kind,'instance'if kind=='instance'else'cluster',lod=lod,fp=fp)
    for rigid,mapping,bias in itertools.product((-1,0,1),(False,True),(0,1,4,5,6,15)):
        add('model',rigid=rigid,mapping=mapping,bias=bias)
    for kind,variant in itertools.product(('cluster','instance'),('empty','cluster','instance','duplicate')):add(kind,variant)
    for kind in ('absent','model-ff'):add(kind)
    for values in ((1,0x80000001,0x00800000,0x7f7fffff),(0x7fc12345,0x7f812345,0x7f800000,0xff800000)):
        for fp in fps:add('instance','instance',fp=fp,plane_values=values)
    add('cluster','cluster',visible=False);add('instance','instance',visible=False)
    for matrix,unit,material in itertools.product((0,15,-1),(0,0xbeefffff),(-1,32767)):
        add('model',mapping=True,rigid=1,matrix=matrix,unit=unit,material_index=material)
    for part in (0x8000,0x10000,0xffffffff):
        for kind in ('model','cluster','instance'):add(kind,'instance'if kind=='instance'else'cluster',part=part)
    for kind,index,selector in itertools.product(('cluster','instance'),(0,31,32,127,254),(-1,0,1)):
        add(kind,'instance'if kind=='instance'else'cluster',visibility_index=index,selector=selector)
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--genuine-retained',action='store_true');a=p.parse_args()
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest()!=base.PIN:raise ValueError('Wrong original TU1.5 executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(a.xbe);genuine=[];witnesses=[]
    if a.genuine_retained:
        from native_tu15_command_part_genuine_inputs import collect
        genuine,witnesses,pages=collect(image,base.SP,base.STOP,PACKET)
        for page in sorted(pages):
            if not any(at<=page and page+4096<=at+n for at,n in BANKS.values()):BANKS['retained_'+hex(page)]=(page,4096)
    base.SECTIONS_MAP=BANKS
    meta=base.prepare(image,scratch,(0x45cd0,0x1d690,0x186e70,0x1e260,0x140db0,0x1e8a0,0x1e8d0,0x140cf0,0x1e1a0))
    tables,instructions=table_inputs(image,scratch);rows=cases(tables)+genuine;summary=[];digest=hashlib.sha256()
    for i,r in enumerate(rows):r['id']=i
    with (scratch/'native.log').open('w+')as log:
        native=subprocess.Popen([str(a.native.resolve())],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=log,text=True)
        try:
            for start in range(0,len(rows),10):
                batch=rows[start:start+10];proof=original(scratch,batch,'parts');instructions+=proof['original_instructions']
                for r,gold in zip(batch,proof['cases'],strict=True):
                    native.stdin.write(json.dumps(r,separators=(',',':'))+'\n');native.stdin.flush();line=native.stdout.readline()
                    if not line:log.seek(0);raise ValueError(r['name']+' '+log.read()[-2000:])
                    actual=json.loads(line)
                    stack=next(b for b in gold['banks']if b['address']==0x62000000)
                    packet=stack['after'][2*(PACKET-stack['address']):2*(PACKET-stack['address']+44)]
                    writes=[{k:w[k]for k in ('instruction','address','written')}for w in gold['writes']if PACKET<=w['address']<PACKET+44]
                    expected=dict(accepted=bool(gold['boundary_gp'][0]&255),packet=packet,writes=writes,fp=gold['after_fp'])
                    if actual!=expected:
                        difference={k:dict(actual=actual[k],expected=expected[k])for k in expected if actual[k]!=expected[k]}
                        raise ValueError(r['name']+' differs '+json.dumps(difference)[:2800])
                    before={b['address']:b['before']for b in r['banks']}
                    for b in gold['banks']:
                        if b['address']!=0x62000000 and before[b['address']]!=b['after']:raise ValueError('Original45CD0 wrote a non-stack bank')
                    digest.update(json.dumps(actual,sort_keys=True,separators=(',',':')).encode());summary.append(dict(id=r['id'],name=r['name'],passed=True,fp=actual['fp']))
                print('whole original45CD0 cases',min(start+10,len(rows)),'/',len(rows),flush=True)
            native.stdin.close()
            if native.wait(timeout=10):raise ValueError('Native45CD0 runner failed')
        finally:
            if native.poll()is None:native.terminate();native.wait(timeout=10)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(passed=True,cases=len(rows),case_results=summary,
        original_instructions=instructions,owners=meta['owners'],comparison_sha256=digest.hexdigest(),original_xbe_sha256=base.PIN,
        genuine_retained_cases=len(genuine),genuine_witnesses=witnesses,
        instruction_patches=[],supplied_original_outcomes=False,retained_large_artifacts=[],owned_scratch=str(scratch),
        scope='Declared whole physical45CD0 closure, original-built transparent table, AL, every packet byte, ordered packet stores, unchanged non-stack banks and full FP. Live World integration remains separate.'),indent=2)+'\n');return 0


if __name__=='__main__':raise SystemExit(main())
