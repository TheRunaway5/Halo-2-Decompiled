#!/usr/bin/env python3
"""Persistent retail transparent allocator/producer/suffix/drain comparison.

Executes unchanged1E2D0/1E4E0,41040 gate/final producer slices,40ED2..40EDE,
3FDF0..3FE06,1E370..1E406 and40875..4087F. The35AA0 graphics service
and callback bodies are explicit empty services; callback ordinals are logged.
Callbacks do not mutate the bank. Seeded persistent bytes and explicit generic
producer records are inputs, not inferred original scene state.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x22fff00,0x3000000
COMMANDS,ORDER,SHADER,PART=BASE,BASE+0x30000,BASE+0x40000,BASE+0x41000
def packed(v):return struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def seed_record(seed,index):
    w=[(seed+index*0x1020304+j*0x1234567)&0xffffffff for j in range(28)]
    w[0]=index%4;w[1]=bits(index%19-9);w[2]=index%7;w[3]&=0xffffff00;w[8]=0
    return w

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-damage-shields-20261011/source_transparent_queue_tests')
    p.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/source_transparent_queue_original.tsv')
    p.add_argument('--fp-fixture',type=Path,default=ROOT/'tests/fixtures/shield_fp_suffix_original.tsv')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/damage-shields-goal-20261011/transparent-queue-original.json')
    a=p.parse_args();xbe=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(xbe.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for section in xbe.sections:u.mem_write(section.va,xbe.section_bytes(section))
    u.mem_map(BASE,0x200000);u.mem_map(STOP,0x1000);callbacks=[];entries={}
    def get(at):return words(u.mem_read(at,4))[0]
    def put(at,*v):u.mem_write(at,packed(v))
    def trace(_u,at,_size,_data):
        if at in (0x1e2d0,0x1e4e0,0x41361,0x1e370,0x13df80,0x1e700,0x3fdf0,0x40ed2,0x40875):entries[f'{at:08x}']=entries.get(f'{at:08x}',0)+1
        if at==0x35aa0:
            sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,get(sp));u.reg_write(UC_X86_REG_ESP,sp+4)
        elif at==0x1e3ed:
            callbacks.append((u.reg_read(UC_X86_REG_EAX)-COMMANDS)//112)
            u.reg_write(UC_X86_REG_EIP,0x1e3f3) #Explicit empty callback, before PUSH.
        elif at==0x41477:u.emu_stop()
    u.hook_add(UC_HOOK_CODE,trace)
    def run(at,end=STOP):
        put(STACK,STOP);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(at,end,count=3000000)
        if u.reg_read(UC_X86_REG_EIP) not in (end,0x41477):raise RuntimeError(f'unfinished original {at:x}')
    def original(row):
        seed,n=row[:2];cursor=iter(row[2:]);out=[]
        u.mem_write(COMMANDS,packed(sum((seed_record(seed,i)for i in range(1024)),[])))
        put(0x4b6288,COMMANDS,ORDER);put(0x4b6280,0,0);u.mem_write(0x4b6291,bytes([seed&1]))
        def allocated(pointer):
            out.append((pointer-COMMANDS)//112 if pointer else 0xffffffff)
            if pointer:out.extend(words(u.mem_read(pointer,112)))
        for _ in range(n):
            op=next(cursor)
            if op==0:u.reg_write(UC_X86_REG_EBP,0);run(0x40ed2,0x40ede)
            elif op==1:run(0x1e2d0);allocated(u.reg_read(UC_X86_REG_EAX))
            elif op==2:u.reg_write(UC_X86_REG_EAX,0);run(0x3fdf0,0x3fe06)
            elif op==3:
                u.mem_write(0x4b9da0,packed([next(cursor)for _ in range(3)]));callbacks.clear();run(0x1e370,0x1e406)
                size=get(0x4b6280)-get(0x4b6284)
                order=struct.unpack('<'+'H'*size,u.mem_read(ORDER,size*2))if size else []
                out.extend([size,*order,len(callbacks),*callbacks])
            elif op==4:run(0x40875,0x4087f)
            elif op==5:
                v=[next(cursor)for _ in range(23)];mask,typ,present,view,grouped,flags,shader,token=v[:8]
                put(PART,shader if present else 0xffffffff,*v[8:11]);put(PART+0x10,v[17],v[18],*v[19:23])
                u.mem_write(0x485618,packed(v[11:17]));u.mem_write(SHADER+0x16,struct.pack('<H',flags))
                before=get(0x4b6280);u.reg_write(UC_X86_REG_EAX,typ);u.reg_write(UC_X86_REG_ECX,PART)
                u.reg_write(UC_X86_REG_EFLAGS,0x46 if typ==0 else 2);run(0x41120,0x4113a)
                if u.reg_read(UC_X86_REG_EIP)!=0x41477:
                    u.mem_write(STACK,bytes(0x100));put(STACK+0x14,mask);put(STACK+0x38,PART);put(STACK+0x40,grouped)
                    u.reg_write(UC_X86_REG_ESI,view);u.reg_write(UC_X86_REG_EBX,SHADER);u.reg_write(UC_X86_REG_EDI,shader);u.reg_write(UC_X86_REG_EBP,token)
                    run(0x41361,0x41477)
                allocated(COMMANDS+before*112 if get(0x4b6280)>before else 0)
            elif op==6:
                v=[next(cursor)for _ in range(28)];run(0x1e2d0);pointer=u.reg_read(UC_X86_REG_EAX)
                if pointer:u.mem_write(pointer,packed(v)) #Explicit external producer writes.
                allocated(pointer)
            elif op==7:
                v=[next(cursor)for _ in range(11)];size=v[10];payload=[next(cursor)for _ in range((size+3)//4)]
                u.mem_write(PART,packed(v[:3]));u.mem_write(0x485618,packed(v[3:9]));u.mem_write(SHADER,packed(payload))
                put(STACK+4,v[9],SHADER);u.reg_write(UC_X86_REG_EDX,PART);u.reg_write(UC_X86_REG_ECX,size)
                before=get(0x4b6280);run(0x1e4e0);allocated(COMMANDS+before*112 if get(0x4b6280)>before else 0)
            else:raise RuntimeError('unknown oracle operation')
            out.extend([get(0x4b6280),get(0x4b6284),u.mem_read(0x4b6291,1)[0]])
        for i in (0,1,2,7,31,511,1022,1023):out.extend(words(u.mem_read(COMMANDS+i*112,112)))
        assert next(cursor,None)is None
        return out
    rng=random.Random(0x1e2d03fdf0);rows=[]
    def add(ops,seed=None):rows.append([rng.getrandbits(32)if seed is None else seed,len(ops),*sum(ops,[])])
    def part(typ=1,mask=0x8000,view=0,grouped=0,present=1,plane=0):
        return [5,mask,typ,present,view,grouped,rng.randrange(65536),0xe9f70872,rng.getrandbits(32),
                *[bits(rng.uniform(-10,10))for _ in range(10)],plane,*map(bits,[1,0,0,2])]
    def callback(size=8):return [7,*[bits(rng.uniform(-10,10))for _ in range(9)],0x173c90,size,*[rng.getrandbits(32)for _ in range((size+3)//4)]]
    for count in (0,1,2,7,8,9,31,1022,1023,1024,1025):
        add([[1]]*count+[[3,0,0,0],[2],part(),callback(),[2],part(plane=1),callback(),[3,0,0,0],[4],[1],[0],[1]])
    for size in (0,1,2,3,4,7,8,31,63,64):add([callback(size),[0],callback(size),[3,0,0,0]])
    for _ in range(96):
        ops=[]
        for j in range(rng.randrange(15,60)):
            kind=rng.randrange(8)
            if kind==3:ops.append([3,*[bits(rng.uniform(-10,10))for _ in range(3)]])
            elif kind==5:ops.append(part(rng.choice([0,1,4,5,6]),rng.choice([0,0x8000,0xffffffff]),rng.randrange(3),rng.randrange(2),rng.randrange(2),rng.randrange(2)))
            elif kind==6:
                c=seed_record(rng.getrandbits(32),j);c[8]=0x3010100 if rng.randrange(2)else 0;c[3]|=rng.randrange(2)
                c[4:8]=list(map(bits,[1,0,0,rng.uniform(-10,10)]));c[25:28]=[bits(rng.uniform(-10,10))for _ in range(3)]
                ops.append([6,*c])
            elif kind==7:ops.append(callback(rng.randrange(65)))
            else:ops.append([kind])
        add(ops)
    expected=[original(row)for row in rows]
    encoded=[' '.join(f'{v&0xffffffff:x}'for v in [len(row),*row])for row in rows]
    native=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16)for v in line.split()]for line in native.stdout.splitlines()]
    assert len(actual)==len(expected)
    for i,(want,got)in enumerate(zip(expected,actual)):
        if want!=got:
            at=next((j for j,(x,y)in enumerate(zip(want,got))if x!=y),min(len(want),len(got)))
            raise RuntimeError(f'queue mismatch case{i} word{at}: original{want[at:at+1]} native{got[at:at+1]}')
    a.fixture.write_text('# Unchanged retail persistent queue operation streams. FPCW027F MXCSR1F80.\n'+'\n'.join(e+' '+' '.join(f'{v:x}'for v in w)for e,w in zip(encoded,expected))+'\n')
    trace_path=ROOT/'analysis/damage-shields-goal-20261011/overlay-pass/scheduling/fp-suffix-trace.json'
    traces=json.loads(trace_path.read_text());assert traces['executable_sha256']==PIN
    fp=[]
    for case in traces['results']:
        i=case['input'];ids=[e['ordinal']for e in case['events']if e['event']=='drain_callback']
        fp.append(' '.join(map(str,[i['prefix'],i['part_type'],i['drain'],i['admit'],i['view_type'],int(i['specialized_bank']),
                                    case['final_count'],case['final_first'],case['draw_count'],len(ids),*ids])))
    a.fp_fixture.write_text('# Unchanged2C2AB..2C2D0 FP caller trace; explicit GPU/pose/material/empty-particle services.\n'+'\n'.join(fp)+'\n')
    report=dict(scope=__doc__,executable_sha256=PIN,cases=len(rows),compared_words=sum(map(len,expected)),all_bit_exact=True,
                original_entries=entries,fp_cases=len(fp),fp_source_sha256=hashlib.sha256(trace_path.read_bytes()).hexdigest(),
                source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),native_sha256=hashlib.sha256(a.binary.read_bytes()).hexdigest(),
                fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),fp_fixture_sha256=hashlib.sha256(a.fp_fixture.read_bytes()).hexdigest())
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
