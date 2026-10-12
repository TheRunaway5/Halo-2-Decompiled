#!/usr/bin/env python3
"""Untouched original1E0C80 dual WORLD banks; actual Lockout graph topology.

Explicit finite local-pose/root controls are inputs. No decoder, service or
original instruction is replaced. This tests mirroring/hierarchy arithmetic,
not the complete source animation selector or Xbox framebuffer.
"""
import hashlib
import random
import struct
from audit_world_vertex_declarations import Xbe, ROOT
from inspect_multiplayer import Cache
from weapon_reference_versions import images, unique_entry
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *
PIN='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
assert hashlib.sha256(x.data).hexdigest()==PIN
c=Cache(ROOT/'assets/disc/maps/lockout.map')
versions=[]
for label,image,pin in images():
    entry=unique_entry(image,x.read(0x1e0c80,24))
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    u.mem_map(0x2100000,0x100000);u.mem_map(0x3000000,4096)
    versions.append((label,u,entry,pin))
GRAPH,NODES,LOCALS,ROOTPOSE,OUTPUT=[0x2100000+i*0x10000 for i in range(5)]
STACK,STOP=0x21fff00,0x3000000
def words(data):return struct.unpack('<'+'I'*(len(data)//4),data)
def pack(values):return struct.pack('<'+'I'*len(values),*values)
rng=random.Random(0x169965);rows=[];graphs=set()
for w in c.tags.values():
    if w['group']!='weap' or w['offset'] is None or not c.u(w['offset']+0x12c)&(1<<22):continue
    for fp in c.block(w['offset']+0x2a8,16):
        graph=c.tags[c.u(fp+12)];model=c.tags[c.u(fp+4)]
        if graph['offset'] is None or model['offset'] is None or graph['datum'] in graphs:continue
        graphs.add(graph['datum']);nodes=c.block(graph['offset']+12,32)
        model_nodes=c.block(model['offset']+0x48,0x60)
        name=c.u(model_nodes[0]);selected=next(i for i,p in enumerate(nodes) if c.u(p)==name)
        parent=c.h(nodes[selected]+8)
        if parent<0:continue
        opposite_flag=16 if c.data[nodes[parent]+10]&8 else 8
        opposite=next(i for i,p in enumerate(nodes) if c.data[p+10]&opposite_flag)
        for case in range(16):
            locals_=[]
            for n in nodes:
                q=[rng.uniform(-1,1) for _ in range(4)] if case else [0,0,0,1]
                locals_.extend(q+[rng.uniform(-2,2) for _ in range(3)]+[rng.uniform(.1,2)])
            root=[1,1,0,0,0,1,0,0,0,1,0,0,0]
            if case:root=[rng.uniform(-2,2) for _ in range(13)]
            results=[]
            for label,u,entry,pin in versions:
                u.mem_write(GRAPH,bytes(0x100));u.mem_write(GRAPH+12,pack([len(nodes),NODES]))
                u.mem_write(NODES,b''.join(bytes(c.data[n:n+32]) for n in nodes))
                u.mem_write(LOCALS,struct.pack('<'+'f'*len(locals_),*locals_))
                u.mem_write(ROOTPOSE,struct.pack('<13f',*root));u.mem_write(OUTPUT,bytes(0x10000))
                u.mem_write(STACK,pack([STOP,GRAPH,OUTPUT,LOCALS,ROOTPOSE,selected,opposite]))
                u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
                u.emu_start(entry,STOP,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP
                results.append(bytes(u.mem_read(OUTPUT,len(nodes)*52)))
            assert all(r==results[0] for r in results),f'Original versions differ {graph["name"]} case {case}'
            inputs=[len(nodes),selected,opposite,*words(struct.pack('<13f',*root))]
            for i,n in enumerate(nodes):
                inputs.extend([c.h(n+8)&0xffffffff,*words(struct.pack('<8f',*locals_[i*8:i*8+8]))])
            expected=words(results[0])
            rows.append(' '.join(f'{v:08x}' for v in inputs)+' | '+' '.join(f'{v:08x}' for v in expected))
out=ROOT/'tests/fixtures/first_person_dual_original.tsv'
out.write_text('# Untouched original dual WORLD leaves; actual graph topology; explicit local/root controls\n'+
    ''.join(f'# {label} entry={entry:08x} XBE {pin}\n' for label,u,entry,pin in versions)+
    '# Lockout '+hashlib.sha256(c.data).hexdigest()+'\n'+'\n'.join(rows)+'\n')
print('Wrote',len(rows),'dual WORLD cases from',len(graphs),'actual graphs matching all',len(versions),'original versions')
