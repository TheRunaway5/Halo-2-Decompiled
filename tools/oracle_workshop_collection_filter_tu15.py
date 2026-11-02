#!/usr/bin/env python3
"""Whole unchanged collection filter/leaf execution, verified on host i386.

All parent chains, shape vtables, groups, properties and motion objects are
declared inputs. Every original dependency executes unchanged. No classifier,
resolver, predicate, pointer or result is intercepted or supplied. Output and
compiler products must stay in HALO2_TEST_SCRATCH; only small verified reference
and provenance files should be retained by the enclosing workflow.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import zlib

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace
from oracle_workshop_titles_tu15 import GP, STACK, STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, SIZE, NEG = 0x2100000, 0x4000, 0x90000000
SECOND, FIRST, COLLECTION, CHILDREN = (BASE+n for n in (0x100, 0x200, 0x900, 0xa00))
SHAPES = [BASE+0x1000+n*0x100 for n in range(3)]
TABLES = [BASE+0x2000+n*0x100 for n in range(3)]
BODY, MOTION, MOTION_TABLE, USER = (BASE+n for n in (0x2800,0x2900,0x2a00,0x2b00))
RECEIVER, OUTPUT, CHILD_OUTPUT = (BASE+n for n in (0x300c,0x3f00,0x3d00))
# Complete literal owners, each also referenced by original read-only callable
# pointer data. Their exact bytes are authenticated again below.
TYPES = {0:0xa3850,1:0xad740,4:0x2e8cf0,5:0x31dd70,6:0x31edf0,
         7:0x31ede0,8:0xad390,17:0xad1d0,20:0xadb50,21:0xae690,
         23:0xa9520,24:0xab4b0,25:0xaf940,0xffffffff:0x1cb030}


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def cases():
    chains = [(n,) for n in (0,1,4,5,6,7,8,17,20,24,25,0xffffffff)]
    chains += [(21,1),(23,1),(21,23,1),(21,23,24),(21,21),(23,21)]
    users = [0,1,2,15,16,USER,NEG]
    rows=[]
    def add(label,**changes):
        row=dict(label=label,chain=(1,),user=USER,flag=1,body_type=1,
                 body_present=True,motion=7,first_depth=1,second_depth=1,
                 special=True,index=0,row=2,column=1,mask=0xffffffff)
        row.update(changes);rows.append(row)
    for chain in chains:add('shape_'+'_'.join(map(str,chain)),chain=chain)
    for user in users:
        for flag in (0,1,0xfe):add(f'user_{user:x}_{flag}',user=user,flag=flag)
    for kind in (0,1,2):
        for present in (False,True):
            for motion in (4,6,7,8):add(f'body_{kind}_{int(present)}_{motion}',body_type=kind,body_present=present,motion=motion)
    for a in range(4):
        for b in range(4):
            for index in (0,0xffffffff):add(f'chain_{a}_{b}_{index:x}',first_depth=a,second_depth=b,index=index)
    for row,column in ((1,2),(0x12340001,0x12340002),(0x1234003f,0x56780021),(0xffffffff,0x8000001f)):
        for mask in (0,1,0x80000000,0xffffffff):add(f'group_{row:x}_{column:x}_{mask:x}',special=False,row=row,column=column,mask=mask)
    rng=random.Random(0x1cbe70)
    for n in range(200):
        add(f'declared_{n}',chain=rng.choice(chains),user=rng.choice(users),flag=rng.choice((0,1,0xfe,0xff)),body_type=rng.randrange(3),body_present=bool(rng.randrange(2)),motion=rng.choice((4,6,7,8)),first_depth=rng.randrange(4),second_depth=rng.randrange(4),special=bool(rng.randrange(2)),index=rng.choice((0,1,0xffffffff)),row=rng.getrandbits(32),column=rng.getrandbits(32),mask=rng.getrandbits(32))
    return rows


def inputs(config):
    cells=[]
    def put(at,value,width=4):cells.append([at,width,value&((1<<(width*8))-1)])
    def parents(first,depth,offset,group,kind=0,body=0):
        nodes=[first]+[BASE+offset+n*0x40 for n in range(depth)]
        for n,node in enumerate(nodes):
            put(node,SHAPES[0]);put(node+12,nodes[n+1] if n+1<len(nodes) else 0)
            put(node+24,kind);put(node+28,group);put(node+32,body)
        return nodes[-1]
    parents(FIRST,config['first_depth'],0x600,config['column'])
    body=BODY if config['body_present'] else 0
    parents(SECOND,config['second_depth'],0x300,config['row'],config['body_type'],body)
    put(0x48b860,body if config['special'] else body^0x55aa55aa)
    put(COLLECTION,BASE+0x2300);put(COLLECTION+12,CHILDREN);put(COLLECTION+16,2)
    put(BASE+0x2334,0x306220)
    for n in range(3):put(CHILDREN+n*8,SHAPES[0]);put(CHILDREN+n*8+4,0)
    for n,type_ in enumerate(config['chain']):
        put(SHAPES[n],TABLES[n]);put(TABLES[n]+20,TYPES[type_]);put(SHAPES[n]+8,config['user'])
        if type_==21:put(SHAPES[n]+12,SHAPES[n+1])
        if type_==23:put(SHAPES[n]+48,SHAPES[n+1])
    put(USER+31,config['flag'],1);put(NEG+31,config['flag'],1)
    put(BODY+60,MOTION);put(MOTION,MOTION_TABLE);put(MOTION_TABLE+24,TYPES[config['motion']])
    for n in range(32):
        mask=config['mask'] if n==(config['row']&31) else config['mask']^(0x9e3779b9*(n+1)&0xffffffff)
        put(RECEIVER-12+n*4+28,mask)
    return cells


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    entries=(0x1cbe70,0x185020,0x2e98d0,0x306220,0x3061f0,*TYPES.values())
    bodies={entry:_trace(code,entry,set(entries)) for entry in entries}
    allowed={pc:bytes(code.at(pc).bytes) for body in bodies.values() for pc in body.code}
    literal=[]
    for value,owner in TYPES.items():
        instructions=[code.at(pc)for pc in sorted(bodies[owner].code)]
        raw=b''.join(bytes(ins.bytes)for ins in instructions)
        expected=(b'\x33\xc0\xc3' if value==0 else b'\x83\xc8\xff\xc3' if value==0xffffffff else b'\xb8'+struct.pack('<I',value)+b'\xc3')
        assert raw==expected and len(instructions)==2,(hex(owner),raw.hex())
        literal.append(dict(owner=owner,value=value,bytes=raw.hex()))
    vm=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x800000),(BASE,0x100000),(NEG,4096),(STOP,4096)):vm.mem_map(at,size)
    for section in image.sections:vm.mem_write(section.va,image.section_bytes(section))
    def semantic(at,size):return (BASE<=at and at+size<=BASE+SIZE)or(NEG<=at and at+size<=NEG+4096)or(at==0x48b860 and size==4)
    trace=[];reads=[];stores=[];covered=set()
    def instruction(g,pc,size,_):
        assert pc in allowed and bytes(g.mem_read(pc,size))==allowed[pc],hex(pc)
        trace.append(pc);covered.add(pc)
    def read(g,_kind,at,size,_value,_):
        if semantic(at,size):reads.append([at,size])
    def store(g,_kind,at,size,value,_):
        if semantic(at,size):stores.append(dict(pc=g.reg_read(R.UC_X86_REG_EIP),address=at,before=bytes(g.mem_read(at,size)).hex(),after=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
    vm.hook_add(UC_HOOK_CODE,instruction);vm.hook_add(UC_HOOK_MEM_READ,read);vm.hook_add(UC_HOOK_MEM_WRITE,store)
    recipes=[]
    for n,config in enumerate(cases()):recipes.append(dict(entry=0x1cbe70,receiver=RECEIVER,args=[OUTPUT,0x12345678,SECOND,FIRST,COLLECTION,config['index']],cells=inputs(config),label=config['label'],flags=0x247 if n%2 else 0x202))
    base_config=cases()[0];rng=random.Random(0x3061f0)
    for n,value in enumerate([0,1,4,5,9,0x7fffffff,0x80000000,0xffffffff]+[rng.getrandbits(32)for _ in range(56)]):
        recipes.append(dict(entry=0x3061f0,receiver=COLLECTION,args=[0]*6,cells=inputs(base_config)+[[COLLECTION+16,4,value]],label=f'count_{n}',flags=0x247 if n%2 else 0x202))
    for n,index in enumerate((0,1,2,0x20000000,0x40000001,0xe0000002)):
        recipes.append(dict(entry=0x306220,receiver=COLLECTION,args=[index,CHILD_OUTPUT,0,0,0,0],cells=inputs(base_config),label=f'child_{n}',flags=0x247 if n%2 else 0x202))
    packets=[];rows=[];expected_host=[]
    for recipe in recipes:
        bank=bytearray([0xa5])*SIZE;negative=bytearray([0xa5])*4096;global_=None
        for at,width,value in recipe['cells']:
            raw=value.to_bytes(width,'little')
            if BASE<=at<BASE+SIZE:bank[at-BASE:at-BASE+width]=raw
            elif NEG<=at<NEG+4096:negative[at-NEG:at-NEG+width]=raw
            else:assert at==0x48b860 and width==4;global_=value
        vm.mem_write(BASE,bytes(bank));vm.mem_write(NEG,bytes(negative));vm.mem_write(0x48b860,struct.pack('<I',global_))
        initial=[0x11223344,recipe['receiver'],0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
        for reg,value in zip(GP,initial):vm.reg_write(reg,value)
        for n in range(8):vm.reg_write(R.UC_X86_REG_XMM0+n,int.from_bytes(struct.pack('<4I',*[0x3f000000+n*4+j for j in range(4)]),'little'))
        for reg,value in ((R.UC_X86_REG_EFLAGS,recipe['flags']),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):vm.reg_write(reg,value)
        vm.mem_write(STACK,struct.pack('<7I',STOP,*recipe['args']));trace.clear();reads.clear();stores.clear();vm.emu_start(recipe['entry'],STOP,count=10000)
        assert vm.reg_read(R.UC_X86_REG_EIP)==STOP,recipe['label']
        after=bytes(vm.mem_read(BASE,SIZE));negative_after=bytes(vm.mem_read(NEG,4096))
        regs=[vm.reg_read(reg)for reg in GP];xmm=[vm.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16,'little').hex()for n in range(8)]
        flags=vm.reg_read(R.UC_X86_REG_EFLAGS);fp=[vm.reg_read(R.UC_X86_REG_FPCW),vm.reg_read(R.UC_X86_REG_FPSW),vm.reg_read(R.UC_X86_REG_MXCSR)]
        row={**recipe,'returned':regs[0],'reads':list(reads),'stores':list(stores),'after_bank_sha256':hashlib.sha256(after).hexdigest(),'after_negative_sha256':hashlib.sha256(negative_after).hexdigest(),'fp':fp,'instructions':len(trace),'trace_sha256':hashlib.sha256(struct.pack('<'+'I'*len(trace),*trace)).hexdigest()}
        rows.append(row);expected_host.append((regs,flags,fp,xmm,after,negative_after))
        values=[recipe['entry'],recipe['receiver'],recipe['flags'],global_,*recipe['args']]
        packets.append(' '.join(f'{value:x}'for value in values)+' '+bank.hex()+' '+negative.hex())
    main_body=set(bodies[0x1cbe70].code);missing=sorted(main_body-covered)
    assert not missing,'Unexercised original filter sites: '+','.join(hex(pc)for pc in missing)
    harness=ROOT/'tools/original_workshop_collection_filter_host_x86.cpp';exe=scratch/'original-collection-filter-cpu'
    command=['g++','-m32','-msse2','-O2','-fomit-frame-pointer','-fno-pie','-no-pie',str(harness),'-o',str(exe)]
    subprocess.run(command,check=True)
    result=subprocess.run([str(exe),str(NEW_DEFAULT)],input='\n'.join(packets)+'\n',text=True,capture_output=True,check=True,timeout=120)
    lines=result.stdout.splitlines();assert len(lines)==len(rows)
    for row,line,expected in zip(rows,lines,expected_host):
        values=line.split();assert len(values)==46,(row['label'],len(values))
        numbers=[int(value,16)for value in values[:44]];regs,flags,fp,xmm,after,negative=expected
        assert numbers[:8]==regs and numbers[8]==flags,(row['label'],'GP/flags',numbers[:9],regs+[flags])
        assert numbers[9:12]==fp,(row['label'],'FP')
        assert b''.join(v.to_bytes(4,'little')for v in numbers[12:44])==b''.join(bytes.fromhex(v)for v in xmm),(row['label'],'SIMD')
        assert bytes.fromhex(values[44])==after and bytes.fromhex(values[45])==negative,(row['label'],'complete bank')
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,cases=rows,case_count=len(rows),main_body_sites=len(main_body),main_body_all_sites_executed=True,total_executed_sites=len(covered),literal_owners=literal,host_cpu_cases=len(rows),host_command=command,host_output_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),host_harness_sha256=sha(harness),source_sha256=sha(Path(__file__)),instruction_patches=[],intercepted_calls=[],supplied_original_outcomes=False)
    raw=(json.dumps(report,separators=(',',':'))+'\n').encode();compressed=zlib.compress(raw,9);assert len(compressed)<1024*1024
    args.output.write_bytes(compressed)
    print(json.dumps(dict(cases=len(rows),main_body_sites=len(main_body),all_filter_sites_executed=True,host_cpu_cases=len(rows),compressed_bytes=len(compressed),raw_bytes=len(raw))))


if __name__=='__main__':main()
