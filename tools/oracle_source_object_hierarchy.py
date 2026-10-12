#!/usr/bin/env python3
"""Unchanged TU1.1 attachment-link stores and complete BA780/BF540 detach.

BA3FA..BA43C is entered after actual admission/unregister/backend services.
All owner/header/topology/location bytes here are declared controlled inputs.
No transforms, activation, constructor chronology or live registry is supplied.
"""
import argparse, collections, hashlib, json, random, struct, subprocess, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
BASE,STACK,STOP=0x2100000,0x22fff00,0x3000000
POOL,HEADERS,BODIES=BASE,BASE+0x1000,BASE+0x10000
NONE=0xffffffff
def pack(values):return struct.pack('<'+'I'*len(values),*(v&NONE for v in values))
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    executable=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(executable)==PIN
    xbe=Xbe(executable);u=Uc(UC_ARCH_X86,UC_MODE_32)
    u.mem_map(0x10000,0x600000)
    for section in xbe.sections:u.mem_write(section.va,xbe.section_bytes(section))
    u.mem_map(BASE,0x200000);u.mem_map(STOP,0x1000)
    instructions=collections.Counter();stores=collections.Counter();reached=collections.Counter()
    running=False
    def code(_u,at,_size,_data):
        instructions[at]+=1
        if at in (0xba3fa,0xba780,0xbf540):reached[f'{at:08x}']+=1
    def written(_u,_access,at,size,_value,_data):
        if running and (HEADERS<=at<HEADERS+2048*12 or BODIES<=at<BODIES+2048*0x200):
            stores[f'{u.reg_read(UC_X86_REG_EIP):08x}/{size}']+=1
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,written)
    def put(at,*values):u.mem_write(at,pack(values))
    def read(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def body(handle):return BODIES+(handle&65535)*0x200
    def setup(owners):
        put(0x4e78d0,POOL);put(POOL+0x44,HEADERS)
        for owner in owners:
            h,parent,child,sibling,node,flags,generic,leaf,location,cluster,reference=owner
            header=HEADERS+(h&65535)*12;b=body(h)
            u.mem_write(header,struct.pack('<HBBhHI',h>>16,flags,2,
                (cluster&65535)-65536 if cluster&32768 else cluster&65535,0x180,b))
            u.mem_write(b,bytes([0x5a])*0x180)
            put(b+4,generic);put(b+0xc,sibling,child,parent)
            u.mem_write(b+0x18,bytes([node]));put(b+0x28,leaf,location);put(b+0x60,reference)
    def state(handles):
        result=[]
        for h in handles:
            b=body(h);header=HEADERS+(h&65535)*12
            cluster=struct.unpack('<h',u.mem_read(header+4,2))[0]&NONE
            result.append([h,read(b+0x14),read(b+0x10),read(b+0xc),u.mem_read(b+0x18,1)[0],
                u.mem_read(header+2,1)[0],read(b+4),read(b+0x28),read(b+0x2c),cluster,read(b+0x60)])
        return result
    def run(operation):
        nonlocal running
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
                UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):u.reg_write(register,0xabababab)
        put(STACK,STOP);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_EFLAGS,2)
        if operation[0]==0:
            _,parent,child,node,bsp=operation
            put(STACK+0x18,body(parent));put(STACK+0x78,parent,child,node)
            u.mem_write(0x46fb64,struct.pack('<H',bsp&65535))
            for register,value in ((UC_X86_REG_EBP,body(child)),(UC_X86_REG_EDX,parent),
                    (UC_X86_REG_ECX,POOL),(UC_X86_REG_ESI,(child&65535)*12),
                    (UC_X86_REG_EBX,HEADERS+(child&65535)*12+2),(UC_X86_REG_EDI,body(parent))):
                u.reg_write(register,value)
            start,end=0xba3fa,0xba43c
        else:
            u.reg_write(UC_X86_REG_EAX,operation[1]);start,end=0xba780,STOP
        running=True
        try:u.emu_start(start,end,count=1000000)
        finally:running=False
        assert u.reg_read(UC_X86_REG_EIP)==end,hex(u.reg_read(UC_X86_REG_EIP))
    rng=random.Random(0xba780);cases=[];outputs=[];labels=[];operations_count=0
    def owners(count):
        return [[((0xe100+i)<<16)|i,NONE,NONE,NONE,rng.randrange(256),rng.randrange(256)&~0xc0,
            rng.getrandbits(32)&~0x100,rng.getrandbits(32),rng.getrandbits(32),
            rng.choice([NONE,0,3,31]),NONE] for i in range(count)]
    def capture(label,initial,operations):
        nonlocal operations_count
        setup(initial);handles=[r[0]for r in initial];expected=[]
        for operation in operations:
            run(operation);expected.extend(v for row in state(handles)for v in row)
        cases.append([len(initial),*[v for row in initial for v in row],len(operations),
            *[v for operation in operations for v in operation]])
        outputs.append(expected);labels.append(label);operations_count+=len(operations)
    for index in range(256):
        initial=owners(8);setup(initial);handles=[r[0]for r in initial];operations=[];expected=[]
        for step in range(32):
            current=state(handles);roots=[r[0]for r in current if r[1]==NONE and r[3]==NONE]
            held=[r[0]for r in current if r[1]!=NONE]
            options=[]
            for child in roots:
                for parent in handles:
                    ancestor=parent
                    while ancestor!=NONE and ancestor!=child:ancestor=read(body(ancestor)+0x14)
                    if ancestor!=child:options.append((parent,child))
            if held and (not options or rng.randrange(3)==0):operation=[1,rng.choice(held)]
            else:
                parent,child=rng.choice(options)
                operation=[0,parent,child,rng.choice([0,1,19,20,255,256,32767,65535]),rng.choice([0,1,2,65535])]
            operations.append(operation);run(operation);expected.extend(v for row in state(handles)for v in row)
        cases.append([8,*[v for row in initial for v in row],len(operations),*[v for op in operations for v in op]])
        outputs.append(expected);labels.append(f'persistent-{index}');operations_count+=len(operations)
    for selected in (1,1024,2047):
        initial=owners(2048);handles=[r[0]for r in initial];initial[0][2]=handles[1]
        for i in range(1,2048):initial[i][1]=handles[0];initial[i][3]=handles[i+1]if i<2047 else NONE;initial[i][5]|=0x80
        capture(f'2047-child-splice-{selected}',initial,[[1,handles[selected]]])
    initial=owners(5);h=[r[0]for r in initial];initial[0][2]=h[2]
    initial[1][1]=h[0];initial[1][3]=h[3];initial[1][5]|=0x80
    capture('absent-target-preserves-sibling',initial,[[1,h[1]]])
    text='\n'.join(' '.join(f'{v:x}'for v in case)+' | '+' '.join(f'{v:x}'for v in output)
        for case,output in zip(cases,outputs))+'\n'
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text('# Unchanged TU1.1 BA3FA..BA43C and complete BA780/BF540; '+PIN+'\n'+text)
    failures=[]
    if args.binary:
        result=subprocess.check_output([str(args.binary.resolve()),'--probe'],
            input='\n'.join(' '.join(f'{v:x}'for v in c)for c in cases)+'\n',text=True)
        actual=[[int(v,16)for v in row.split()]for row in result.splitlines()]
        assert len(actual)==len(outputs)
        failures=[labels[i]for i,(a,b)in enumerate(zip(actual,outputs))if a!=b]
    evidence={}
    for entry in ('000ba2a0','000ba780','000bf540'):
        path=ROOT/f'analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/{entry}.asm'
        for line in path.read_text().splitlines():
            at,data,*_=line.split();assert xbe.read(int(at,16),len(bytes.fromhex(data)))==bytes.fromhex(data)
        evidence[str(path.relative_to(ROOT))]=sha(path)
    report=dict(scope=__doc__,xbe_sha256=PIN,fixture_sha256=sha(args.fixture),
        generator_sha256=sha(__file__),streams=len(cases),operations=operations_count,
        compared_words=sum(map(len,outputs)),instruction_count=sum(instructions.values()),
        original_entries=dict(reached),original_stores=dict(stores),instruction_listings_sha256=evidence,
        native_binary_sha256=sha(args.binary)if args.binary else None,all_bit_exact=not failures if args.binary else None,
        failures=failures,labels=labels,
        boundaries=['Explicit controlled owner/header bytes and current BSP word; no captured runtime inputs',
            'Attach starts after real admission/deactivation/unregister/backend service boundary and stops before inverse-node transform',
            'Complete detach BA780/BF540; no BA980/BA7D0 world transform, velocity, bounds, registration or activation providers',
            'Large star and absent-target inputs are declared structural controls, not original live gameplay histories'])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('streams','operations','compared_words','instruction_count','all_bit_exact','fixture_sha256','failures')},indent=2))
    assert not failures
if __name__=='__main__':main()
