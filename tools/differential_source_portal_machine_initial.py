#!/usr/bin/env python3
"""Untouched group initialization and placement binding for all49 automatic machines."""
import hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
BASE,STOP,STACK=0x2100000,0x3000000,0x21fff00
SCENARIO,AUTHORED_GROUPS,GROUP_HEADER,GROUP_DATA,GROUP_BITS,OBJECT_HEADER,OBJECT_DATA,OBJECT,PLACEMENT=[BASE+i*0x1000 for i in range(9)]
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return struct.unpack('<'+'I'*(len(v)//4),v)
x=Xbe(ROOT/'assets/disc/default.xbe');u=Uc(UC_ARCH_X86,UC_MODE_32)
u.mem_map(0x10000,0x600000)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(BASE,0x100000);u.mem_map(STOP,0x1000)
entries={v:0 for v in [0x107a30,0x1076e0,0x16b840,0x1070d0,0x1086e0,0x1087c0]}
def trace(_u,address,_size,_data):
    if address in entries:entries[address]+=1
u.hook_add(UC_HOOK_CODE,trace)
def run(at,args=()):
    u.mem_write(STACK,pack([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK)
    u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
    u.emu_start(at,STOP,count=1000000)
    if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('source device binding failed')
def initialize(groups,power,pos,flags):
    u.mem_write(SCENARIO,bytes(0xb0));u.mem_write(SCENARIO+0xa0,pack([len(groups),AUTHORED_GROUPS]))
    for i,g in enumerate(groups):u.mem_write(AUTHORED_GROUPS+i*40,g)
    u.mem_write(GROUP_HEADER,bytes(0x50));u.mem_write(GROUP_HEADER+0x20,pack([64,12]));u.mem_write(GROUP_HEADER+0x40,pack([1,GROUP_DATA,GROUP_BITS]))
    u.mem_write(GROUP_DATA,bytes(0x1000));u.mem_write(GROUP_BITS,bytes(0x1000))
    u.mem_write(OBJECT_HEADER+0x44,pack([OBJECT_DATA]));u.mem_write(OBJECT_DATA+8,pack([OBJECT]))
    u.mem_write(OBJECT,bytes(0x300));u.mem_write(OBJECT+0x12a,b'\xff\xff')
    u.mem_write(PLACEMENT,bytes(0x48));u.mem_write(PLACEMENT+0x34,struct.pack('<hhI',power,pos,flags))
    for at,ptr in [(0x4e0350,SCENARIO),(0x4e0328,GROUP_HEADER),(0x4e0300,OBJECT_HEADER)]:u.mem_write(at,pack([ptr]))
    run(0x107a30);run(0x1076e0,[0x10000,PLACEMENT])
    power_handle,position_handle=words(u.mem_read(OBJECT+0x130,4))+words(u.mem_read(OBJECT+0x13c,4))
    result=[words(u.mem_read(OBJECT+0x12c,4))[0],*words(u.mem_read(OBJECT+0x134,4)),*words(u.mem_read(OBJECT+0x140,4))]
    for h in [power_handle,position_handle]:
        result.extend([int(h!=0xffffffff),*struct.unpack('<HII',u.mem_read(GROUP_DATA+(h&65535)*12+2,10))])
    return result
rows=[];records=[]
for base,names in [('assets/disc/maps','ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar'.split()),
                   ('assets/map-pack/maps','backwash containment deltatap dune elongation gemini triplicate turf warlock'.split())]:
    for name in names:
        c=Cache(ROOT/base/(name+'.map'));_,s=c.tag(c.scenario,'scnr')
        groups=[bytes(c.data[p:p+40]) for p in c.block(s+0xa0,40)]
        for ordinal,p in enumerate(c.block(s+0xa8,0x48)):
            if c.u32(p+4)&1:continue
            power,pos,flags=struct.unpack_from('<hhI',c.data,p+0x34)
            palette=c.u16(p);pal=c.block(s+0xb0,40)
            datum=c.u32(pal[palette]+4)
            output=initialize(groups,power,pos,flags)
            rows.append(' '.join([name,str(ordinal),f'{datum:x}',f'{flags:x}',str(power),str(pos),*[f'{v:x}' for v in output]]))
            records.append({'map':name,'ordinal':ordinal,'definition':datum,'placement_flags':flags,'power_group':power,'position_group':pos,'source_output':output})
        c.close()
# Synthetic complete no-controller binding exercises every placement bit and
# every named/private combination. This is a group/flags producer proof only.
for flags in range(256):
    for power,pos in [(-1,-1),(0,-1),(-1,1),(0,1)]:
        groups=[bytes(32)+struct.pack('<fI',.25,0),bytes(32)+struct.pack('<fI',.75,1)]
        output=initialize(groups,power,pos,flags)
        expect_flags=0x80|((flags&8)>>3)|((flags&16)>>3)
        if output[0]!=expect_flags:raise RuntimeError('initial device flags differ')
        if output[2]!=struct.unpack('<I',struct.pack('<f',.75 if pos==1 else (1 if flags&1 else 0)))[0]:raise RuntimeError('initial position differs')
assert len(rows)==49
(ROOT/'tests/fixtures/retail_source_portal_machine_initial.tsv').write_text('# Complete original107A30/1076E0 and pure called allocation/lookup; explicit controllerNONE.\n'+'\n'.join(rows)+'\n')
report={'executable_sha256':hashlib.sha256(x.data).hexdigest(),'actual_placements':len(rows),'synthetic_cases':1024,'all_bit_exact':True,
        'routines':{f'{k:08x}':v for k,v in entries.items()},'fpcw':'027F','mxcsr':'1F80',
        'boundary':'Complete device group/placement binding with explicit successful allocation and controllerNONE; not live device motion/door scan or HS scheduler.',
        'records':records}
(ROOT/'analysis/multiplayer-shading/source-portal-machine-initial-original.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='records'},indent=2))
