#!/usr/bin/env python3
"""Original TU1.5 packed sampling and world model-order matrix publication.

Actual opening Pillar metadata/resource bytes feed the standalone2835B0 sampler.
Completed sampled locals and an explicit object root feed unchanged BF501..52B
and BF53C..5B6/BF625. Those matrix intervals follow independent root/type/mask
owners; their declared caller inputs do not establish complete BD090/object birth.
No instructions or callee outputs are replaced. Unicorn reciprocal estimates
remain a software-reference convention, not physical Xbox framebuffer evidence.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_ESI,
    UC_X86_REG_EBP, UC_X86_REG_XMM1, UC_X86_REG_XMM4, UC_X86_REG_MXCSR, UC_X86_REG_FPCW)

NONE=0xffffffff
XBE_PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MAP_PIN='5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28'

def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--map',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists():raise ValueError('Refusing to overwrite original evidence')
    if sha(a.xbe)!=XBE_PIN or sha(a.map)!=MAP_PIN:raise ValueError('Expected pinned original assets')
    x=a.xbe.read_bytes();g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,n in [(0x10000,0x600000),(0x2100000,0x400000),(0x3000000,4096)]:g.mem_map(at,n)
    base=struct.unpack_from('<I',x,0x104)[0];count,table=struct.unpack_from('<II',x,0x11c)
    for i in range(count):
        _,at,_,off,n=struct.unpack_from('<5I',x,table-base+i*56)
        if n:g.mem_write(at,x[off:off+n])
    def put(at,values):g.mem_write(at,struct.pack('<'+'I'*len(values),*values))
    def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    stack=0x24fff00;stop=0x3000000
    executed=set();phase=''
    def observe(vm,pc,size,user):
        executed.add(pc)
        if phase=='root' and pc==0xbf52b:vm.emu_stop()
        if phase=='children' and pc in (0xbf5b6,0xbf625):vm.emu_stop()
    g.hook_add(UC_HOOK_CODE,observe)
    def invalid(vm,access,address,size,value,user):
        print('Original frontier',phase,hex(vm.reg_read(UC_X86_REG_EIP)),hex(address),size,flush=True)
        return False
    g.hook_add(UC_HOOK_MEM_INVALID,invalid)
    with a.map.open('rb') as f,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as data:
        u=lambda at:struct.unpack_from('<I',data,at)[0]
        index=u(16);tag_table=u(index+8)-(u(index)-(index+32))
        magic=u(tag_table+8)-(index+u(20))
        tags={u(tag_table+i*16+4):tag_table+i*16 for i in range(u(index+24))}
        root=lambda tag:u(tags[tag]+8)-magic
        start=(index+magic)&~4095;end=(len(data)+magic+4095)&~4095
        g.mem_map(start,end-start);g.mem_write(index+magic,data[index:])
        put(0x4f0414,[tag_table+magic])
        object_tag,graph_tag=0xe21c00a7,0xe9f1087c
        model_tag=u(root(object_tag)+0x38);mode_tag=u(root(model_tag)+4)
        mode,graph=root(mode_tag),root(graph_tag)
        node_count=u(mode+0x48);node_at=u(mode+0x4c)-magic
        clip_at=u(graph+0x30)-magic;clip=bytes(data[clip_at:clip_at+0x6c])
        if clip[0x10]!=0:raise ValueError('Expected actual type0 Pillar clip')
        sf,af,movement,unknown04,static,unknown08,animated=struct.unpack_from('<bbhhhii',clip,0x3c)
        if movement or unknown04 or unknown08 or u(clip_at+0x64):raise ValueError('Unadmitted resource layout')
        group=u(graph+0xb0)-magic+u(clip_at+0x24)*20;location=u(group+8)
        if location>>30:raise ValueError('Resource requires its actual shared-cache owner')
        resource_at=location+u(clip_at+0x28);size=sf+af+static+animated
        resource=bytes(data[resource_at:resource_at+size])
        resource_buffer=0x2110000;descriptor=0x2100000;locals_at=0x2101000;matrices=0x2103000
        if len(resource)!=size or size>0x100000:raise ValueError('Resource extent differs')
        g.mem_write(resource_buffer,resource)
        frames=struct.unpack_from('<h',clip,0x14)[0]
        g.mem_write(descriptor,struct.pack('<IIBbh',resource_buffer,clip_at+magic+0x3c,clip[0x13],clip[0x11],frames))
        bind=b''.join(data[node_at+i*0x60+24:node_at+i*0x60+40]+
            data[node_at+i*0x60+12:node_at+i*0x60+24]+struct.pack('<f',1) for i in range(node_count))
        parents=[struct.unpack_from('<h',data,node_at+i*0x60+4)[0] for i in range(node_count)]
        cases=[]
        for frame in sorted(set([0,1,2,30,frames//2,frames-1])):
            phase='sample';g.mem_write(locals_at,bind)
            put(stack,[stop,graph+magic,clip_at+magic,node_count,0,0,locals_at,0,0,0,0])
            g.reg_write(UC_X86_REG_ESP,stack);g.reg_write(UC_X86_REG_ESI,descriptor)
            g.reg_write(UC_X86_REG_XMM1,bits(frame));g.reg_write(UC_X86_REG_XMM4,bits(1))
            g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
            g.emu_start(0x2835b0,stop,count=2000000)
            if g.reg_read(UC_X86_REG_EIP)!=stop:raise ValueError('Original sampler did not return')
            sampled=bytes(g.mem_read(locals_at,node_count*32))
            for external in ([1,1,0,0,0,1,0,0,0,1,0,0,0],[.75,0,1,0,-1,0,0,0,0,1,3,-4,5]):
                phase='root';g.mem_write(matrices,bytes([0xa5])*node_count*52)
                scratch=stack-0x800;g.mem_write(scratch,bytes(0x200))
                put(scratch+0x14,[mode+magic]);put(scratch+0x18,[locals_at]);put(scratch+0x1c,[matrices])
                put(scratch+0xc4,[bits(v)for v in external])
                g.reg_write(UC_X86_REG_ESP,scratch);g.emu_start(0xbf501,stop,count=100000)
                if g.reg_read(UC_X86_REG_EIP)!=0xbf52b:raise ValueError('Original root matrix interval did not finish')
                phase='children';g.reg_write(UC_X86_REG_EBP,mode+magic)
                g.emu_start(0xbf53c,stop,count=100000)
                if g.reg_read(UC_X86_REG_EIP)not in(0xbf5b6,0xbf625):raise ValueError('Original model-order matrix interval did not finish')
                cases.append(dict(frame=frame,root=[bits(v)for v in external],locals=words(sampled),
                    world=words(bytes(g.mem_read(matrices,node_count*52)))))
        graph_nodes=u(graph+0x10)-magic
        report=dict(scope=__doc__,xbe_sha256=XBE_PIN,map_sha256=MAP_PIN,tool_sha256=sha(Path(__file__)),
            object=f'{object_tag:08x}',model=f'{model_tag:08x}',mode=f'{mode_tag:08x}',graph=f'{graph_tag:08x}',
            clip_index=0,frame_count=frames,node_count=node_count,parents=parents,
            model_names=[u(node_at+i*0x60)for i in range(node_count)],
            graph_names=[u(graph_nodes+i*32)for i in range(u(graph+0xc))],
            bind=words(bind),resource_sha256=hashlib.sha256(resource).hexdigest(),cases=cases,
            executed_instructions=len(executed),instruction_patches=[],intercepted_callees=[])
        a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
        print(node_count,'model-order nodes;',len(cases),'original packed-sample/world cases;',frames,'clip frames')

if __name__=='__main__':main()
