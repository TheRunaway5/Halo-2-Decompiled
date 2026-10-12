#!/usr/bin/env python3
"""Execute unchanged base-disc107980; capture189060/176780 requests only.

No sound/effect creation, spatial admission or attached engine is emulated.
The native runtime uses the proven group selection for its own PCM adapter.
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
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BANK, STACK, STOP, TAG_BANK = 0x2100000, 0x21fff00, 0x3000000, 0x2400000
HEADER, POOL, OBJECT, TAGS = BANK, BANK+0x10000, BANK+0x20000, TAG_BANK+0x80000

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--write-fixture',type=Path)
    args=parser.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(x.data).hexdigest()!=PIN:raise RuntimeError('Unsupported original executable')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
    u.mem_map(BANK,0x100000);u.mem_map(TAG_BANK,0x100000);u.mem_map(STOP,0x1000)
    def put(at,value):u.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def ret(pop):
        sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,word(sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
    put(0x4e0300,HEADER);put(HEADER+0x44,POOL);put(0x4e3b44,TAGS)
    observed=[];visited=set()
    def hook(uc,pc,size,unused):
        if pc==0x189060:
            sp=uc.reg_read(UC_X86_REG_ESP)
            observed.extend([1,uc.reg_read(UC_X86_REG_EBX),word(sp+4),word(sp+12),word(sp+8),0,0,0,0,0])
            for address in (uc.reg_read(UC_X86_REG_ECX),uc.reg_read(UC_X86_REG_EAX)):
                observed.extend(struct.unpack('<III',uc.mem_read(address,12)))
            ret(12);return
        if pc==0x176780:
            sp=uc.reg_read(UC_X86_REG_ESP);location=uc.reg_read(UC_X86_REG_EBX)
            observed.extend([2,word(sp+4),uc.reg_read(UC_X86_REG_EDX),word(sp+8),
                uc.reg_read(UC_X86_REG_XMM0)&0xffffffff,word(location),word(location+4),
                struct.unpack('<H',uc.mem_read(location+8,2))[0],word(sp+12),word(sp+16),0,0,0,0,0,0])
            ret(16);return
        if not 0x107980<=pc<0x107a26:raise RuntimeError(f'Unexpected original instruction {pc:08x}')
        if bytes(uc.mem_read(pc,size))!=x.read(pc,size):raise RuntimeError('Original dispatcher instruction changed')
        visited.add(pc)
    u.hook_add(UC_HOOK_CODE,hook)
    def original(row):
        tag,group,owner,location_c2,location_c4,location_c8,power,position=row
        put(POOL+(owner&0xffff)*12+8,OBJECT)
        index=struct.unpack('<h',struct.pack('<H',tag&0xffff))[0]
        put(TAGS+index*16,group)
        u.mem_write(OBJECT+0xc2,struct.pack('<H',location_c2));put(OBJECT+0xc4,location_c4);put(OBJECT+0xc8,location_c8)
        put(OBJECT+0x134,power);put(OBJECT+0x140,position);observed.clear()
        u.reg_write(UC_X86_REG_EAX,tag);u.reg_write(UC_X86_REG_EDX,owner)
        u.reg_write(UC_X86_REG_ESP,STACK);put(STACK,STOP)
        u.emu_start(0x107980,STOP,count=200)
        if u.reg_read(UC_X86_REG_EIP)!=STOP or u.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Original dispatcher did not return cleanly')
        return observed.copy() if observed else [0]*16
    rng=random.Random(0x107980);rows=[]
    for tag in (0xffffffff,0,0x10001,0x12347fff,0x12348000,0x9876fffe):
        for group in (0,0x65666665,0x736e6421,0xffffffff):
            for slot in (0,1,2047):
                rows.append([tag,group,0x12340000|slot,0xabcd,0x12345678,0x87654321,0x3f000000,0x3f800000])
    for _ in range(1536):
        rows.append([rng.getrandbits(32),rng.choice((0,0x65666665,0x736e6421)),
            rng.randrange(1,65536)<<16|rng.randrange(2048),rng.getrandbits(16),*[rng.getrandbits(32) for _ in range(4)]])
    expected=[original(row) for row in rows]
    data=''.join(' '.join(f'{v:x}' for v in row)+'\n' for row in rows)
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input=data,text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in native.stdout.splitlines()]
    mismatches=[i for i,(a,b) in enumerate(zip(expected,actual)) if a!=b]
    passed=len(actual)==len(rows) and not mismatches
    args.output.parent.mkdir(parents=True,exist_ok=True)
    report={'passed':passed,'cases':len(rows),'compared_words':len(rows)*16,'mismatches':mismatches[:20],
        'original_instructions':len(visited),'original_sha256':PIN,'native_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        'boundary':'Complete107980 only, sound189060/effect176780 captured external requests; no creation or spatial admission proof.'}
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    if args.write_fixture and passed:
        args.write_fixture.write_text('# Base-disc107980 inputs8 outputs16; unchanged original bytes, external services captured.\n'+
            ''.join(' '.join(f'{v:x}' for v in row+result)+'\n' for row,result in zip(rows,expected)))
    print(json.dumps(report));raise SystemExit(0 if passed else 1)
if __name__=='__main__':main()
