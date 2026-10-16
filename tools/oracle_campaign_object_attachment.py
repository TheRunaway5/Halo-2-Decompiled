#!/usr/bin/env python3
"""Capture TU1.5 BAF10 marker alignment and BB3E0 retained local frames.

Marker lookup supplies explicit completed world marker matrices. B9460 object
position publication is intercepted: its first call retains the aligned root;
its second call captures the local frame before later spatial/physics owners.
The original alignment, inversion, composition, normalization, parent-link and
local-frame arithmetic execute unchanged. This is not a full object lifecycle.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_MXCSR, UC_X86_REG_FPCW)

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--xbe',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
x=a.xbe.read_bytes();pin=hashlib.sha256(x).hexdigest()
if pin!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
    raise ValueError('Expected original Xbox TU1.5 English executable')
if a.output.exists():raise ValueError('Refusing to overwrite evidence')
g=Uc(UC_ARCH_X86,UC_MODE_32)
for address,size in [(0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)]:g.mem_map(address,size)
base=struct.unpack_from('<I',x,0x104)[0];n,table=struct.unpack_from('<II',x,0x11c)
for i in range(n):
    _,address,_,offset,size=struct.unpack_from('<5I',x,table-base+i*56)
    if size:g.mem_write(address,x[offset:offset+size])
def words(address,count):return list(struct.unpack('<'+'I'*count,g.mem_read(address,4*count)))
def put(address,values):g.mem_write(address,struct.pack('<'+'I'*len(values),*values))
def floats(values):return struct.pack('<'+'f'*len(values),*values)
def bits(values):return list(struct.unpack('<'+'I'*len(values),floats(values)))
def return_to_caller(bytes_to_pop):
    sp=g.reg_read(UC_X86_REG_ESP);ret=words(sp,1)[0]
    g.reg_write(UC_X86_REG_ESP,sp+4+bytes_to_pop);g.reg_write(UC_X86_REG_EIP,ret)

parent,child=0x10001,0x20002
parent_body,child_body=0x2102000,0x2103000
put(0x4ecbd0,[0x2100000]);put(0x2100044,[0x2101000])
put(0x2101000+12+8,[parent_body]);put(0x2101000+24+8,[child_body])
put(0x4eabc4,[0])
active={};published=[];lookups=[];executed=set()
def hook(guest,address,size,user):
    executed.add(address)
    sp=guest.reg_read(UC_X86_REG_ESP)
    if address==0xbad60:
        obj,marker,maximum=words(sp+4,3)
        if maximum!=1 or obj not in (parent,child):raise ValueError('Unexpected marker owner')
        out=guest.reg_read(UC_X86_REG_EBX)
        raw=bytearray(0x70);struct.pack_into('<h',raw,0,0)
        # BAD60 publishes both local+4 and world+38 matrices. The unnamed
        # child path consumes the actual identity local marker at +4.
        raw[4:0x38]=floats(active['parent_marker' if obj==parent else 'child_marker'])
        raw[0x38:0x6c]=floats(active['parent_marker' if obj==parent else 'child_marker'])
        guest.mem_write(out,bytes(raw));lookups.append([obj,marker])
        guest.reg_write(UC_X86_REG_EAX,1);return_to_caller(12)
    elif address==0xb9460:
        argv=words(sp+4,9)
        if argv[0]!=child:raise ValueError('Unexpected position owner')
        output=sum((words(ptr,3) for ptr in argv[1:4]),[])
        published.append(output)
        if len(published)==2:
            guest.emu_stop();return
        for offset,ptr in zip((0x64,0x70,0x7c),argv[1:4]):
            guest.mem_write(child_body+offset,bytes(guest.mem_read(ptr,12)))
        return_to_caller(36)
g.hook_add(UC_HOOK_CODE,hook)

identity=[1,1,0,0,0,1,0,0,0,1,0,0,0]
axes=[[1,0,0,0,1,0,0,0,1],[0,1,0,-1,0,0,0,0,1],
      [-1,0,0,0,-1,0,0,0,1],[1,0,0,0,0,1,0,-1,0]]
rng=random.Random(0xbaf10);cases=[]
for named in (False,True):
    for index in range(40):
        current=[rng.uniform(-10,10) for _ in range(3)]+axes[index%4][:3]+axes[index%4][6:9]
        node=[(1,.5,2,1)[index%4],*axes[(index//4)%4],*[rng.uniform(-10,10) for _ in range(3)]]
        parent_marker=[1,*axes[(index//8)%4],*[rng.uniform(-10,10) for _ in range(3)]]
        child_marker=[1,*axes[(index//5)%4],*[rng.uniform(-10,10) for _ in range(3)]] if named else identity
        active=dict(current=current,parent_node=node,parent_marker=parent_marker,child_marker=child_marker)
        g.mem_write(parent_body,bytes(0x400));g.mem_write(child_body,bytes(0x400))
        for body in (parent_body,child_body):
            put(body+0xc,[0xffffffff,0xffffffff,0xffffffff]);put(body+0xb4,[0])
        g.mem_write(parent_body+0x116,struct.pack('<h',0x200));g.mem_write(parent_body+0x200,floats(node))
        g.mem_write(child_body+0x64,floats(current));published.clear();lookups.clear()
        put(0x21fff00,[0x3000000,parent,0,child]);g.reg_write(UC_X86_REG_ESP,0x21fff00)
        g.reg_write(UC_X86_REG_EAX,0x10001c4a if named else 0)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
        g.emu_start(0xbaf10,0x3000000,count=100000)
        if len(published)!=2 or len(lookups)!=2 or words(child_body+0x14,1)[0]!=parent:
            raise ValueError('Incomplete original attachment capture')
        cases.append(dict(named=named,current=bits(current),parent_node=bits(node),
            parent_marker=bits(parent_marker),child_marker=bits(child_marker),
            aligned=published[0].copy(),local=published[1].copy(),lookups=lookups.copy()))
report=dict(xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    entry='000baf10',boundary=__doc__,intercepted_entries=['000bad60','000b9460'],
    executed_instructions=len(executed),cases=cases)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
print(len(cases),'original aligned/local attachment pairs')
