#!/usr/bin/env python3
"""Literal source ownership/cold mask plus bounded1152E0 mask-slice differential."""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STOP,STACK=0x2100000,0x3000000,0x22fff00
BSP,HEADER,RECORDS,LIST,MASK,ACOUSTIC,POOL,DESCRIPTORS,GROUPS,GROUP_RECORDS=[BASE+i*0x1000 for i in range(10)]
BODIES,DEFINITIONS=BASE+0x10000,BASE+0x30000

def packed(words):return struct.pack('<'+'I'*len(words),*words)
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def real(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/special-build/source_portal_mask_tests')
    p.add_argument('--count',type=int,default=4096);p.add_argument('--write-fixture',type=Path)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/source-portal-mask-differential.json');a=p.parse_args()
    x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    u.mem_map(BASE,0x200000);u.mem_map(STOP,0x1000)
    entries={v:0 for v in [0x210df0,0x210f00,0x210e80,0x210e20,0xbaeb0,0x115220,0x1157d5]}
    def trace(_u,address,_size,_data):
        if address in entries:entries[address]+=1
    u.hook_add(UC_HOOK_CODE,trace)
    def run(at,args=(),end=STOP):
        u.mem_write(STACK,packed([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(at,end,count=1000000)
        if u.reg_read(UC_X86_REG_EIP)!=end:raise RuntimeError(f'source portal call {at:x} did not return')
    def original(row):
        at=0
        def take(n=1):
            nonlocal at
            v=row[at:at+n];at+=n
            return v[0] if n==1 else v
        portals=take();flags=take(portals) if portals!=1 else [take()]
        if portals==0:flags=[]
        records=[take(6) for _ in range(take())]
        indices_count=take();indices=take(indices_count) if indices_count!=1 else [take()]
        if indices_count==0:indices=[]
        owners=[take(15) for _ in range(take())]
        words=take(16);acoustic=take(128);operations=[take(2) for _ in range(take())]
        assert at==len(row)
        u.mem_write(BSP,bytes(0x300));u.mem_write(BSP+0x5c,packed([portals,BASE+0x50000]));u.mem_write(BSP+0x220,packed([HEADER]))
        for i,flag in enumerate(flags):u.mem_write(BASE+0x50000+i*36+0x18,packed([flag]))
        u.mem_write(HEADER,packed([len(records),RECORDS,len(indices),LIST]))
        for i,r in enumerate(records):u.mem_write(RECORDS+i*12,struct.pack('<IHBBHH',r[0],r[1]&65535,r[2]&255,r[3]&255,r[4]&65535,r[5]&65535))
        u.mem_write(LIST,struct.pack('<'+'H'*len(indices),*[v&65535 for v in indices]))
        u.mem_write(MASK,packed(words));u.mem_write(ACOUSTIC,bytes(0x244));u.mem_write(ACOUSTIC+0x28,struct.pack('<128H',*acoustic))
        u.mem_write(POOL,bytes(0x50));u.mem_write(POOL+0x38,packed([1+max([o[0]&65535 for o in owners],default=0)]));u.mem_write(POOL+0x44,packed([DESCRIPTORS]));u.mem_write(DESCRIPTORS,bytes(0x1000))
        u.mem_write(GROUPS+0x44,packed([GROUP_RECORDS]));u.mem_write(GROUP_RECORDS,bytes(0x1000))
        for i,o in enumerate(owners):
            body=BODIES+i*0x300;definition=DEFINITIONS+i*0x200
            u.mem_write(body,bytes(0x300));u.mem_write(definition,bytes(0x200))
            u.mem_write(DESCRIPTORS+(o[0]&65535)*12,struct.pack('<HBBII',o[0]>>16,0,o[1],0,body))
            u.mem_write(body+0xa4,struct.pack('<IHBB',o[2],o[3]&65535,o[4]&255,o[5]&255))
            u.mem_write(body+0x1da,struct.pack('<HH',o[6]&65535,o[7]&65535))
            u.mem_write(definition+0x11c,struct.pack('<H',o[8]));u.mem_write(definition+0x124,packed(o[9:11]))
            u.mem_write(body+0x140,packed(o[11:12]));u.mem_write(body+0x13c,packed(o[12:13]));u.mem_write(body+0x12c,packed(o[14:15]))
            if o[12]!=0xffffffff:u.mem_write(GROUP_RECORDS+(o[12]&65535)*12+4,packed(o[13:14]))
        for global_at,ptr in [(0x4e0348,BSP),(0x4f93a4,MASK),(0x4ed288,ACOUSTIC),(0x4e0300,POOL),(0x4e0328,GROUPS)]:u.mem_write(global_at,packed([ptr]))
        for op,owner in operations:
            if op==0:run(0x210df0)
            elif op==1:run(0x210f00)
            elif op==2:run(0x210e20)
            elif op==3:
                body=BODIES+owner*0x300;definition=DEFINITIONS+owner*0x200
                u.reg_write(UC_X86_REG_EBP,body);u.mem_write(STACK+0x18,packed([definition]))
                # Untouched pure slice after earlier tick/autoopen work. Original
                # caller body/group state is explicit; no motion service is stubbed.
                run(0x1157d5,end=0x11592d if owners[owner][8]==0 else 0x11599b)
            elif op==4:run(0x115220,[owners[owner][0]])
            elif op==5:run(0x210e80)
            else:raise RuntimeError('unsupported original operation')
        out=list(struct.unpack('<16I',u.mem_read(MASK,64)))+list(struct.unpack('<128H',u.mem_read(ACOUSTIC+0x28,256)))
        for i in range(len(owners)):out.extend(struct.unpack('<HH',u.mem_read(BODIES+i*0x300+0x1da,4)))
        return out
    def encode(flags,records,indices,owners,ops,words=None,acoustic=None):
        return [len(flags),*flags,len(records),*[v&0xffffffff for r in records for v in r],len(indices),*indices,len(owners),
            *[v&0xffffffff for o in owners for v in o],*(words or [0x24681357]*16),*(acoustic or [0x1357]*128),len(ops),*[v for op in ops for v in op]]
    def owner(handle=0x10002,identity=(42,0,7,0),kind=0,current=0,group=0,flags=0,closed=0,opened=1,header=7):
        return [handle,header,*identity,65535,0,kind,bits(closed),bits(opened),bits(current),group,bits(current),flags]
    rows=[];labels=[]
    rows.append(encode([],[],[],[],[[0,0],[1,0],[2,0],[5,0]]));labels.append('empty original ownership banks')
    duplicate_owners=[owner(0x20007),owner(0x10002),owner(0x30003,(43,-2,7,1),current=1,group=2)]
    duplicate_records=[[42,0,7,0,0,1],[42,0,7,0,1,1],[43,99,7,1,0,1]]
    for ops in [[[0,0]],[[0,0],[3,1]],[[0,0],[3,1],[3,2]],[[0,0],[3,1],[3,2],[4,2]],[[0,0],[3,1],[4,1],[5,0],[3,0]]]:
        rows.append(encode([8,4],duplicate_records,[0,1],duplicate_owners,ops));labels.append('first slot/last record/shared overwrite/full identity')
    # Literal Gemini bank and actual typed owner identities. Current group and
    # body positions below remain explicit differential inputs, not live captures.
    identities=[0x30f0,0x318c,0x318e,0x3190,0x4b51,0x4b52]
    records=[[v,0,7,0,f,c] for v,f,c in zip(identities,[0,1,2,4,6,7],[1,1,2,2,1,1])]
    owners=[owner(0x10000+i,(v,0,7,0),group=i) for i,v in enumerate(identities)]
    rows.append(encode([0]*37,records,[33,32,25,36,30,34,35,26],owners,[[0,0]]));labels.append('Gemini source cold ownership')
    rows.append(encode([0]*37,records,[33,32,25,36,30,34,35,26],owners,[[0,0],*[[3,i] for i in range(6)]]));labels.append('Gemini explicit zero-position door steps')
    for current in [0,-0.0,1,-1,real(1),.5]:
        for group in [0xffffffff,0]:
            for flag in [0,0x100]:
                for groupvalue in [0,-0.0,1,-1,real(1)]:
                    o=owner(current=current,group=group,flags=flag);o[13]=bits(groupvalue)
                    for ops in [[[0,0],[3,0]],[[0,0],[3,0],[4,0]]]:
                        rows.append(encode([8,4,0],[[42,0,7,0,0,2]],[0,1],[o],ops));labels.append('door group/current and destroy order')
    # Retain outputs BEFORE destruction as well, including FISTP half-way ties.
    for v in [0,.5,1,1/65535,2/65535,-1,2]:
        for delta in [-2,-1,0,1,2]:
            z=real(max(0,bits(v)+delta)) if v>=0 else v
            o=owner(current=z);rows.append(encode([4],[[42,0,7,0,0,1]],[0],[o],[[0,0],[3,0]]));labels.append('acoustic rounding/clamp adjacent input')
    rng=random.Random(0x210df0)
    for case in range(a.count):
        count=rng.randrange(1,10);slots=rng.sample(range(32),count);owners=[]
        for i in range(count):
            o=owner((rng.randrange(1,65535)<<16)|slots[i],(rng.randrange(4),rng.randrange(3),7,rng.randrange(3)),
                rng.randrange(3),rng.choice([0,real(1),-.25,.1,.5,1,1.5]),i,rng.choice([0,0x100]),
                rng.uniform(-.2,1.2),rng.uniform(-.2,1.2),rng.choice([7,7,7,6]))
            if i%7==0:o[12]=0xffffffff
            o[13]=bits(rng.choice([0,real(1),.5,1]));owners.append(o)
        flags=[rng.choice([0,0,4,8,12]) for _ in range(rng.randrange(1,97))]
        indices=[rng.randrange(len(flags)) for _ in range(rng.randrange(1,12))]
        records=[]
        for i in range(rng.randrange(1,15)):
            o=rng.choice(owners);first=rng.randrange(len(indices));length=rng.randrange(len(indices)-first+1)
            identity=o[2:6].copy()
            if rng.randrange(4)==0:identity[1]=(identity[1]+1)%3
            records.append([*identity,first,length])
        ops=[[0,0]]
        machines=[i for i,o in enumerate(owners) if o[1]==7]
        if machines:
            for i in range(rng.randrange(1,12)):ops.append([rng.choice([1,2,3,4,5]),rng.choice(machines)])
        rows.append(encode(flags,records,indices,owners,ops,acoustic=[rng.randrange(65536) for _ in range(128)]));labels.append('ordered ownership/shared/missing/lifetime')
    expected=[original(row) for row in rows]
    lines=[' '.join(f'{v:x}' for v in [len(row),*row]) for row in rows]
    result=subprocess.run([str(a.binary),'--probe'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(expected):raise RuntimeError('native portal case count differs')
    for i,(x,y) in enumerate(zip(actual,expected)):
        if x!=y:
            j=next(j for j,(xx,yy) in enumerate(zip(x,y)) if xx!=yy)
            raise RuntimeError(f'portal mask case{i} {labels[i]} field{j} native{x[j]:08x} source{y[j]:08x}')
    report={'executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),
        'cases':len(rows),'output_words':sum(map(len,expected)),'routines':{f'{k:08x}':v for k,v in entries.items()},
        'fpcw':'027F','mxcsr':'1F80','bounded_slice':'1157D5..11592D/11599B, actual caller-state inputs; no door-motion implementation',
        'hardware_boundary':'original x86 instructions under Unicorn; no physical Xbox framebuffer claim','all_bit_exact':True}
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    if a.write_fixture:
        selected=range(min(512,len(rows)))
        a.write_fixture.write_text('# Untouched original source portal ownership/mask operations; FPCW027F MXCSR1F80.\n'+'\n'.join(lines[i]+' '+' '.join(f'{v:x}' for v in expected[i]) for i in selected)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
