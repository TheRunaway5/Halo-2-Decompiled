#!/usr/bin/env python3
"""Run complete, unmodified retail39A80 against owned weather initialization.

Resource publication and initial auxiliary RNG chronology are explicit inputs.
No resource, kernel, random, arithmetic, or callees are replaced in the guest.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BANK,GROUP,RNG,RENDER,STATE,STACK,STOP=0x2100000,0x2101000,0x2102000,0x2200000,0x2300000,0x24fff00,0x3000000
def fw(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def words(b):return list(struct.unpack('<'+'I'*(len(b)//4),b))
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--write-fixture',type=Path)
    a=p.parse_args();xbe=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(xbe.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in xbe.sections:guest.mem_write(section.va,xbe.section_bytes(section))
    guest.mem_map(BANK,0x500000);guest.mem_map(STOP,0x1000)
    def put(at,v):guest.mem_write(at,struct.pack('<I',v&0xffffffff))
    put(0x4e7408,RNG)
    addresses=set()
    def verified(uc,at,size,user):
        if not 0x39a80<=at<0x39e45:raise RuntimeError(f'unexpected original instruction {at:08x}')
        if bytes(uc.mem_read(at,size))!=xbe.read(at,size):raise RuntimeError('changed original instruction')
        addresses.add(at)
    guest.hook_add(UC_HOOK_CODE,verified)
    def original(row):
        seed,count,n=row[:3];bank=row[3:38];render=row[38:38+n*5];state=row[38+n*5:]
        put(RNG,0x11223344);put(RNG+4,seed)
        guest.mem_write(BANK,struct.pack('<35I',*bank))
        guest.mem_write(GROUP,struct.pack('<4I',count,RENDER,n,STATE))
        if n:
            guest.mem_write(RENDER,struct.pack('<'+'I'*len(render),*render))
            guest.mem_write(STATE,struct.pack('<'+'I'*len(state),*state))
        guest.reg_write(UC_X86_REG_ECX,BANK);guest.reg_write(UC_X86_REG_ESP,STACK)
        put(STACK,STOP);guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
        guest.emu_start(0x39a80,STOP,count=max(10000,n*400))
        if guest.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original initialization did not return')
        if words(guest.mem_read(RNG,4))!=[0x11223344]:raise RuntimeError('original altered primary RNG')
        return [words(guest.mem_read(RNG+4,4))[0],guest.reg_read(UC_X86_REG_EAX)&255,
            *words(guest.mem_read(BANK,140)),*words(guest.mem_read(RENDER,n*20)),
            *words(guest.mem_read(STATE,n*32))]
    rng=random.Random(0x39a80);rows=[];assets=[]
    # Exact authored definitions and decompressed resource bytes; only published
    # pointer/count fields are explicit owned-resource controls, not cache data.
    for name in ['lockout','warlock','backwash']:
        path=ROOT/('assets/disc/maps' if name=='lockout' else 'assets/map-pack/maps')/(name+'.map')
        c=Cache(path)
        try:
            systems=[]
            for datum,tag in c.tags.items():
                if tag['group']!='weat':continue
                _,p_=c.tag(datum,'weat')
                for ordinal,r in enumerate(c.block(p_,0x8c)):
                    bank=words(c.data[r:r+140]);packed,size,header=bank[16:19]
                    resource=path if packed>>30==0 else path.parent/['','mainmenu.map','shared.map','single_player_shared.map'][packed>>30]
                    with resource.open('rb') as stream:stream.seek(packed&0x3fffffff);raw=stream.read(size)
                    arrays={}
                    for d in c.block(r+0x50,16):
                        kind,_,_,locator,stride,length,offset=struct.unpack_from('<BBHHHII',c.data,d)
                        if kind!=0 or locator not in [0,8]:raise ValueError('unsupported authored resource')
                        arrays[locator]=raw[header+8+offset:header+8+offset+length]
                    n=len(arrays[0])//20
                    if len(arrays[8])!=n*32:raise ValueError('authored array counts differ')
                    bank[25]=1;bank[26]=GROUP
                    for seed in [0,1,0xffffffff,0x12345678]:
                        rows.append([seed,n,n,*bank,*words(arrays[0]),*words(arrays[8])])
                    systems.append({'datum':f'{datum:08x}','ordinal':ordinal,'particles':n,
                        'definition_sha256':hashlib.sha256(c.data[r:r+140]).hexdigest(),
                        'render_sha256':hashlib.sha256(arrays[0]).hexdigest(),
                        'state_sha256':hashlib.sha256(arrays[8]).hexdigest()})
            assets.append({'map':name,'cache_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'systems':systems})
        finally:c.close()
    for i in range(1536):
        n=i%9;bank=[rng.getrandbits(32) for _ in range(35)]
        for j in range(2,16):bank[j]=fw(rng.uniform(-4,4))
        bank[25]=[0,1,0xffffffff,2][i%4];bank[26]=GROUP
        count=[n,0,0xffffffff][(i//4)%3]
        if i%16==0:bank[7:9]=[fw(0),fw(0)]
        if i%16==1:bank[7:9]=[fw(3),fw(1)]
        if i%16==2:bank[12:15]=[fw(0)]*3
        if i%16==3:bank[12:15]=[fw(.00001)]*3
        if i%16==4:bank[2:5]=[fw(.0001)]*3
        render=[rng.getrandbits(32) for _ in range(n*5)]
        state=[rng.getrandbits(32) for _ in range(n*8)]
        rows.append([rng.getrandbits(32),count,n,*bank,*render,*state])
    expected=[original(row) for row in rows]
    encoded=[' '.join(f'{v:x}' for v in row) for row in rows]
    result=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(rows):raise RuntimeError('native probe lost rows')
    failures=[{'case':i,'different_words':[j for j,(x,y) in enumerate(zip(s,t)) if x!=y],
        'source':s[:37],'native':t[:37]} for i,(s,t) in enumerate(zip(expected,actual)) if s!=t]
    if a.write_fixture and not failures:
        a.write_fixture.parent.mkdir(parents=True,exist_ok=True)
        a.write_fixture.write_text('# Complete untouched original39A80. Input: seed,count,owned count,35 bank words,5N render,8N state; output: seed,AL,all banks.\n' +
            '\n'.join(x+' '+' '.join(f'{v:08x}' for v in y) for x,y in zip(encoded,expected))+'\n')
    report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),
        'cases':len(rows),'exact_cases':len(rows)-len(failures),'compared_words':sum(len(x) for x in expected),
        'verified_original_instruction_count':len(addresses),'primary_rng_unchanged':True,'assets':assets,
        'failures':failures[:8],'scope':'Complete39A80 with explicit published resource/count and auxiliary RNG. Visibility, publication, RNG initialization chronology, subsequent particle simulation and live GPU submission remain separate.'}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ['assets']},indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
