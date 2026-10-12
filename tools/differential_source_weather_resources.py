#!/usr/bin/env python3
"""Compare native weather publication with original ready12DE70/223C70.

Actual cache bytes back a declared completed native IO allocation. This checks
publication, linked touches and13D230 age rebasing, not original async IO/LRU.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ECX

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,RAW,PHYSICAL,POOL,BLOCK,BITMAP,CACHE,CACHE_BLOCK,TAGS,STACK,STOP=(
    0x2100000,0x2101000,0x2200000,0x2201000,0x2202000,0x2203000,
    0x2210000,0x2211000,0x2300000,0x24fff00,0x3000000)
def load(path):
    cache=Cache(path)
    try:
        tag=next(t for t in cache.tags.values() if t['group']=='weat')
        _,root=cache.tag(tag['datum'],'weat');p=cache.block(root,0x8c)[0]
        bank=bytes(cache.data[p:p+0x8c]);pointer,size,section=struct.unpack_from('<III',bank,0x40)
        descriptors=[bytes(cache.data[d:d+16]) for d in cache.block(p+0x50,16)]
        files=['','mainmenu.map','shared.map','single_player_shared.map']
        raw_path=path.parent/files[pointer>>30] if pointer>>30 else path
        with raw_path.open('rb') as stream:stream.seek(pointer&0x3fffffff);raw=stream.read(size)
        return {'path':str(path.relative_to(ROOT)),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
            'bank':bank,'raw':raw,'descriptors':descriptors,'section':section,
            'capacity':struct.unpack_from('<I',bank,0x2c)[0]}
    finally:cache.close()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-map-world-20261011/source_weather_world_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/map-world-features/weather-resource-differential.json')
    args=p.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest()==PIN
    assets=[load(ROOT/'assets'/group/'maps'/(name+'.map')) for group,name in
        [('disc','lockout'),('map-pack','warlock'),('map-pack','backwash')]]
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
    u.mem_map(BASE,0x400000);u.mem_map(STOP,4096)
    def put(at,value):u.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    visits=set();writes=set()
    def code(g,pc,size,data):
        if pc==STOP:return
        assert bytes(g.mem_read(pc,size))==image.read(pc,size),f'changed original instruction {pc:08x}'
        visits.add(pc)
    def writing(g,access,address,size,value,data):
        if STACK-4096<=address<STACK+16:return
        writes.update(range(address,address+size))
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
    def call(entry,eax=0,ecx=0):
        put(STACK,STOP);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_EAX,eax);u.reg_write(UC_X86_REG_ECX,ecx)
        u.emu_start(entry,STOP,count=10000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP
        return u.reg_read(UC_X86_REG_EAX)&255
    def original(row):
        map_,count,clock,phase,next_=row;asset=assets[map_];bank=bytearray(asset['bank'])
        struct.pack_into('<I',bank,0x54,BASE+0x100)
        # Completed native allocation is registered in private cache slot0.
        # This is an input to ready publication, not an original IO return.
        struct.pack_into('<I',bank,0x60,0)
        u.mem_write(BASE,bytes(bank));u.mem_write(RAW,asset['raw'])
        for i,source in enumerate(asset['descriptors']):
            descriptor=bytearray(source);stride=struct.unpack_from('<H',descriptor,6)[0]
            struct.pack_into('<I',descriptor,8,count*stride);u.mem_write(BASE+0x100+i*16,bytes(descriptor))
        put(0x4e649c,PHYSICAL);put(0x4e648c,CACHE);put(0x4e6494,0)
        put(PHYSICAL+0x34,12);put(PHYSICAL+0x38,clock);put(PHYSICAL+0x64,POOL)
        put(POOL+0x24,24);put(POOL+0x38,1);put(POOL+0x44,BLOCK);put(POOL+0x48,BITMAP)
        put(BITMAP,1);put(BLOCK+8,RAW>>12);put(BLOCK+20,0xa5a5a5a5)
        put(CACHE+0x44,CACHE_BLOCK);u.mem_write(CACHE_BLOCK+2,b'\1')
        put(0x4e3b44,TAGS);owner=struct.unpack_from('<h',bank,0x58)[0]
        put(TAGS+owner*16,0x77656174);put(0x4e64a0,0);u.mem_write(0x468c4c,b'\0')
        writes.clear();ready=call(0x12de70,3,BASE+0x40)
        if phase==1:put(PHYSICAL+0x38,next_);assert call(0x12de70,3,BASE+0x40)==1
        if phase==2:call(0x13d230,PHYSICAL)
        allowed=set(range(BASE+0x5e,BASE+0x5f))|set(range(BASE+0x64,BASE+0x6c))
        allowed|=set(range(RAW+8,RAW+24))|set(range(BLOCK+20,BLOCK+24))|set(range(PHYSICAL+0x38,PHYSICAL+0x3c))
        assert writes<=allowed,sorted(writes-allowed)[:10]
        assert bytes(u.mem_read(RAW+8+asset['section'],len(asset['raw'])-8-asset['section']))==asset['raw'][8+asset['section']:]
        return [*struct.unpack('<35I',u.mem_read(BASE,140)),
            *struct.unpack('<'+'I'*(asset['section']//4),u.mem_read(RAW+8,asset['section'])),word(BLOCK+20),ready]
    rng=random.Random(0x223c70);rows=[]
    for i,asset in enumerate(assets):
        for n in [asset['capacity'],0,1,asset['capacity']//2]:
            for clock in [0,1,7,0x7ffffefe,0x7ffffeff,0x7fffffff,0xffffffff]:
                for phase in [0,1,2]:rows.append([i,n,clock,phase,rng.randrange(0x80000000)])
        for case in range(256):rows.append([i,rng.randrange(asset['capacity']+1),rng.randrange(0x100000000),case%3,rng.randrange(0x100000000)])
    expected=[original(row) for row in rows]
    native=subprocess.run([str(args.binary.resolve()),'--probe',str(ROOT)],
        input='\n'.join(' '.join(f'{v:x}' for v in row) for row in rows)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in native.stdout.splitlines()]
    assert len(actual)==len(expected)
    failures=[{'case':i,'input':row,'source':source,'native':native} for i,(row,source,native)
        in enumerate(zip(rows,expected,actual)) if source!=native]
    report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        'entries':['0012DE70','00223C70','0013D230'],'cases':len(rows),'bit_exact_cases':len(rows)-len(failures),
        'compared_words':sum(map(len,expected)),'unchanged_original_instruction_addresses':len(visits),
        'services_replaced':[],'original_instruction_patches':[],
        'caller_inputs':'Actual literal cache backing, registered private native slot0 and completed native IO state. No original async IO claim.',
        'payloads_unchanged_by_native_and_original':True,
        'maps':[{k:v for k,v in a.items() if k not in ('bank','raw','descriptors')} for a in assets],
        'failures':failures[:12],'scope':'Weather type0 publication, full system/section-header stores, linked age touches and clock-overflow age rebasing; not original allocation/async/LRU chronology or live snow.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('maps',)},indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
