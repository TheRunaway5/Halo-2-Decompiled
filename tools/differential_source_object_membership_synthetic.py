#!/usr/bin/env python3
"""Untouched14A370/23A160/14A5B0/14A550 PC53 boundary fixtures."""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x24fff00,0x3000000
BSP,COLL,CLUSTERS,PORTALS,PLANES,RAW,POINT,OUTPUT,FORCED,TOTAL=[BASE+x for x in (0,0x1000,0x10000,0x30000,0x40000,0x80000,0x100000,0x101000,0x102000,0x103000)]
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def scalar(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def flat(b,queries):
    out=[len(b['planes'])]
    for p in b['planes']:out.extend(map(word,p))
    out.append(len(b['clusters']))
    for ids in b['clusters']:out.extend([len(ids),*ids])
    out.append(len(b['portals']))
    for p in b['portals']:
        out.extend([p['back']&65535,p['front']&65535,p['plane'],*map(word,p['center']),word(p['radius']),len(p['vertices'])])
        for v in p['vertices']:out.extend(map(word,v))
    out.append(len(queries))
    for q in queries:
        out.extend([q['kind'],q['id']&65535,*map(word,q['center']),word(q['radius'])])
        if q['kind']==1:out.extend([len(q['forced']),*q['forced']])
    return out
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);p.add_argument('--write-fixture',type=Path,required=True);args=p.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    u.mem_map(BASE,0x400000);u.mem_map(STOP,0x1000)
    def put(at,*v):u.mem_write(at,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def run(at,args=(),regs={}):
        for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,regs.get(r,0))
        u.reg_write(UC_X86_REG_ESP,STACK);put(STACK,STOP,*args);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.emu_start(at,STOP,count=10000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
    def original(b,queries):
        u.mem_write(BASE,bytes(0x100000));put(0x4e033c,COLL);put(0x4e0348,BSP);put(BSP+0x18,COLL);put(BSP+0x60,PORTALS);put(BSP+0x9c,len(b['clusters']),CLUSTERS);put(COLL+8,len(b['planes']),PLANES);cursor=RAW
        for i,p in enumerate(b['planes']):put(PLANES+i*16,*map(word,p))
        for i,p in enumerate(b['portals']):
            at=PORTALS+i*36;u.mem_write(at,struct.pack('<hhI4I',p['back'],p['front'],p['plane'],*map(word,p['center']),word(p['radius'])));put(at+0x1c,len(p['vertices']),cursor)
            for v in p['vertices']:put(cursor,*map(word,v));cursor+=12
        for i,ids in enumerate(b['clusters']):
            put(CLUSTERS+i*0xb0+0x8c,len(ids),cursor)
            if ids:u.mem_write(cursor,struct.pack('<'+'H'*len(ids),*ids))
            cursor=(cursor+len(ids)*2+3)&~3
        out=[]
        for i,q in enumerate(queries):
            put(POINT,*map(word,q['center']));put(0x4e7414,i+1);u.mem_write(0x4e7418,bytes(512*4));u.mem_write(OUTPUT,bytes(512))
            if q['kind']==0:
                run(0x14a370,[word(q['radius'])],{UC_X86_REG_EAX:q['id'],UC_X86_REG_ECX:POINT,UC_X86_REG_EDX:BSP});out.append(u.reg_read(UC_X86_REG_EAX)&255)
            else:
                if q['forced']:
                    put(FORCED,*q['forced']);run(0x14a550,[256,OUTPUT],{UC_X86_REG_EBX:FORCED,UC_X86_REG_EDI:TOTAL});total=struct.unpack('<H',u.mem_read(TOTAL,2))[0];stored=u.reg_read(UC_X86_REG_EAX)&65535
                elif q['id']==-1:total=stored=0
                elif q['radius']<=0:total=stored=1;u.mem_write(OUTPUT,struct.pack('<h',q['id']))
                else:
                    run(0x14a5b0,[q['id'],POINT,word(q['radius']),256,OUTPUT]);total=u.reg_read(UC_X86_REG_EAX)&65535;stored=min(total,256)
                out.extend([total,int(total>stored),stored,*struct.unpack('<'+'H'*stored,u.mem_read(OUTPUT,stored*2))])
        return out
    rng=random.Random(0x14a550);cases=[];expected=[];labels=[]
    for i in range(512):
        # Products with opposite signs expose PC53 cancellation. A wider
        # x87 host precision can change admission before float storage.
        scale=scalar(word(10.0**rng.randrange(6,13)));small=scalar(word(10.0**rng.randrange(-13,-4)));center=[scale,-scale,scalar(word(rng.uniform(.5,2)))];a=scalar(word(rng.choice([.707106769,.5,.25,1.])))
        if i==0:scale=1e9;small=1e-10;center=[scale,-scale,1];a=.7071067690849304
        plane=[a,a,small,0];bank=dict(planes=[plane],clusters=[[0],[0]],portals=[dict(back=0,front=1,plane=0,center=center,radius=1,vertices=[])])
        radii=[small,scalar(word(small*1.5)),.001,1.,2.5]
        q=[dict(kind=0,id=0,center=center,radius=r)for r in radii]+[dict(kind=1,id=0,center=center,radius=r,forced=[])for r in radii]
        cases.append(flat(bank,q));expected.append(original(bank,q));labels.append(f'PC53-{i}')
    # Actual fixed source storage limits: DFS keeps counting beyond256, and
    # forced membership enumerates authored cluster IDs in ascending order.
    for count in [1,2,255,256,257,511,512]:
        portals=[dict(back=i,front=i+1,plane=0,center=[0,0,0],radius=2,vertices=[])for i in range(count-1)]
        clusters=[[j for j in [i-1,i]if 0<=j<count-1]for i in range(count)];bank=dict(planes=[[1,0,0,0]],clusters=clusters,portals=portals)
        queries=[dict(kind=1,id=0,center=[0,0,0],radius=1,forced=[]),dict(kind=1,id=-1,center=[0,0,0],radius=1,forced=[0xffffffff]*((count+31)//32)),dict(kind=1,id=-1,center=[0,0,0],radius=1,forced=[]),dict(kind=1,id=count-1,center=[0,0,0],radius=0,forced=[])]
        cases.append(flat(bank,queries));expected.append(original(bank,queries));labels.append(f'capacity-{count}')
    rows=[' '.join(f'{v:x}'for v in c)for c in cases];actual=[list(int(v,16)for v in line.split())for line in subprocess.check_output([str(args.binary.resolve()),'--spatial-probe'],input='\n'.join(rows)+'\n',text=True).splitlines()]
    assert len(actual)==len(expected);failures=[dict(label=labels[i],original=a,native=b)for i,(a,b)in enumerate(zip(expected,actual))if a!=b]
    args.write_fixture.write_text('# Untouched14A370/23A160/14A5B0/14A550 PC53; '+PIN+'\n'+''.join('synthetic '+a+' | '+' '.join(f'{w:x}'for w in b)+'\n'for a,b in zip(rows,expected)))
    report=dict(xbe_sha256=PIN,native_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),cases=len(cases),words=sum(map(len,expected)),exact=len(cases)-len(failures),fpcw='027F',mxcsr='1F80',negative_control_first_expected=expected[0],negative_control_original_pc64='14A370 FPCW037F rejects first query; PC53 admits',failures=failures[:4])
    (ROOT/'analysis/multiplayer-shading/object-membership-synthetic-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));assert not failures
if __name__=='__main__':main()
