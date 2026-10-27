#!/usr/bin/env python3
"""Complete unchanged TU1.5 balancing calls versus the compiled native kernel.

Synthetic party/rating arrays are explicit inputs. All original callees execute;
no instructions, allocators, optimizer calls or services are substituted.
Run through tools/run_test.py; compiler outputs live only in its owned scratch.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
from oracle_game_variants import ROOT, VERSIONS, STOP
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, full_tokens


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    if path:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value, indent=2) + "\n")


def correspondence(old, new):
    oldstarts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/retail/functions.tsv').open(),delimiter='\t')}
    newstarts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    oc,nc=_Code(old,'.text'),_Code(new,'.text')
    section=new.section('.text');data=new.section_bytes(section)
    rows=[];entries={}
    for entry in (0x91fc0,0x91e30,0x91f00):
        trace=_trace(oc,entry,oldstarts|{entry});original=[oc.at(p) for p in sorted(trace.code)]
        pattern=[]
        for instruction in original[:16]:
            wild=set()
            if instruction.imm_size and (instruction.mnemonic=='call' or instruction.mnemonic.startswith('j') or
                    any(o.type==X86_OP_IMM and 0x10000<=o.imm<0x810000 for o in instruction.operands)):
                wild.update(range(instruction.imm_offset,instruction.imm_offset+instruction.imm_size))
            if instruction.disp_size and any(o.type==X86_OP_MEM and not o.mem.base and
                    0x10000<=o.mem.disp<0x810000 for o in instruction.operands):
                wild.update(range(instruction.disp_offset,instruction.disp_offset+instruction.disp_size))
            pattern.extend(b'.' if i in wild else re.escape(bytes([v])) for i,v in enumerate(instruction.bytes))
        candidates=[section.va+m.start() for m in re.finditer(b''.join(pattern),data,re.DOTALL)]
        assert len(candidates)==1,(hex(entry),list(map(hex,candidates)))
        target=candidates[0];trace=_trace(nc,target,newstarts|{target});updated=[nc.at(p) for p in sorted(trace.code)]
        equal=full_tokens(original,old)==full_tokens(updated,new)
        rows.append(dict(retail=hex(entry),tu15=hex(target),retail_instructions=len(original),
            tu15_instructions=len(updated),shape_and_internal_branches_equal=equal))
        entries[entry]=target
    return entries,rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tu15',type=Path,default=NEW_DEFAULT)
    parser.add_argument('--fixture',type=Path)
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert sha(args.tu15)==NEW_PIN
    old_path=ROOT/VERSIONS['retail']['path'];assert sha(old_path)==VERSIONS['retail']['pin']
    old,new=Xbe(old_path),Xbe(args.tu15)
    entries,mapping=correspondence(old,new)
    machine=Uc(UC_ARCH_X86,UC_MODE_32);machine.mem_map(0x10000,0x800000)
    for section in new.sections:machine.mem_write(section.va,new.section_bytes(section))
    machine.mem_map(0x1100000,0x10000);machine.mem_map(0x2000000,0x50000);machine.mem_map(STOP,0x1000)
    stack,parties,weights,output,imbalance=0x204ff00,0x1100000,0x1100100,0x1100200,0x1100300
    executed=set();machine.hook_add(UC_HOOK_CODE,lambda g,a,z,u:executed.add(a))
    def put(at,values):machine.mem_write(at,struct.pack('<'+'i'*len(values),*values))
    def call(entry,argv,registers=()):
        machine.mem_write(stack,struct.pack('<'+'I'*(len(argv)+1),STOP,*(v&0xffffffff for v in argv)))
        machine.reg_write(UC_X86_REG_ESP,stack);machine.reg_write(UC_X86_REG_FPCW,0x27f);machine.reg_write(UC_X86_REG_MXCSR,0x1f80)
        for register,value in registers:machine.reg_write(register,value)
        machine.emu_start(entry,STOP,count=5000000)
        assert machine.reg_read(UC_X86_REG_EIP)==STOP,hex(machine.reg_read(UC_X86_REG_EIP))
        return bool(machine.reg_read(UC_X86_REG_EAX)&255)
    rng=random.Random(0x1948b3);cases=[]
    inputs=[]
    for sizes in ([1]*12,[3]*4,[12],[4]*3,[8,4],[5,5,2],[2]*6,[1]*4):
        ids=[i for i,size in enumerate(sizes) for _ in range(size)]
        for preserve in (False,True):
            for optimize in (False,True):
                for values in ([0]*len(ids),list(range(len(ids))),[127 if i%2 else 0 for i in range(len(ids))]):
                    inputs.append((4,3 if len(ids)==12 else 1,False,0,0,preserve,optimize,ids,values,0))
    for _ in range(160):
        teams=rng.randrange(2,5);maximum=rng.randrange(1,9);count=rng.randrange(1,17)
        ids=[];party=0
        for i in range(count):
            if i and rng.randrange(3)==0:party+=1
            ids.append(party)
        inputs.append((teams,maximum,bool(rng.randrange(2)),2,4,bool(rng.randrange(2)),bool(rng.randrange(2)),
            ids,[rng.randrange(-1,128) for _ in ids],rng.randrange(min(2,16-count)+1)))
    for teams,maximum,flagged,minimum,limit,preserve,optimize,ids,values,extra in inputs:
        count=len(ids);put(parties,ids);put(weights,values);put(output,[0x33333333]*16);put(imbalance,[777])
        valid=call(entries[0x91fc0],(teams,maximum,flagged,minimum,limit,preserve,count,parties,weights,optimize,output))
        assignments=list(struct.unpack('<'+'i'*count,machine.mem_read(output,count*4)))
        if not valid:assert assignments==[0x33333333]*count
        count_valid=call(entries[0x91e30],(teams,maximum,flagged,minimum,limit,preserve,imbalance),
            ((UC_X86_REG_EAX,count),(UC_X86_REG_EDX,parties)))
        spread=struct.unpack('<i',machine.mem_read(imbalance,4))[0]
        if not count_valid:assert spread==777
        add_valid=call(entries[0x91f00],(teams,maximum,flagged,minimum,limit,preserve,0),
            ((UC_X86_REG_EAX,count),(UC_X86_REG_ECX,parties),(UC_X86_REG_EDX,extra)))
        cases.append(dict(teams=teams,maximum=maximum,flagged=flagged,minimum=minimum,limit=limit,
            preserve=preserve,optimize=optimize,parties=ids,weights=values,extra=extra,
            assignments=assignments if valid else None,imbalance=spread if count_valid else None,can_add=add_valid))
    native=scratch/'team-balance.rs';binary=scratch/'team-balance'
    native.write_text('#[path='+json.dumps(str(ROOT/'rust/online_protocol/src/team_balance.rs'))+'] mod port;\n'+'''
use std::io::{self,Read};
fn main(){let mut text=String::new();io::stdin().read_to_string(&mut text).unwrap();for line in text.lines(){
let v:Vec<i32>=line.split_whitespace().map(|s|s.parse().unwrap()).collect();let n=v[7]as usize;
let o=port::Options{teams:v[0]as usize,maximum_team_size:v[1]as usize,flag_parties:v[2]!=0,
flag_minimum:v[3]as usize,flag_maximum:v[4]as usize,use_parties:v[5]!=0,optimize:v[6]!=0};
let ids=&v[9..9+n];let weights=&v[9+n..9+n*2];let result=port::balance(o,ids,weights);
print!("{}",i32::from(result.is_some()));if let Some(rows)=result{for row in rows{print!(" {}",row);}}
print!(" | {}",port::by_count(o,ids).unwrap_or(777));println!(" | {}",i32::from(port::can_add(o,ids,v[8]as usize,0)));}}
''')
    subprocess.run(['rustc','--edition=2024','-O',str(native),'-o',str(binary)],check=True)
    lines=[]
    for c in cases:
        lines.append(' '.join(map(str,[c['teams'],c['maximum'],int(c['flagged']),c['minimum'],c['limit'],
            int(c['preserve']),int(c['optimize']),len(c['parties']),c['extra'],*c['parties'],*c['weights']])))
    actual=subprocess.check_output([str(binary)],input=('\n'.join(lines)+'\n').encode()).decode().splitlines()
    assert len(actual)==len(cases)
    for c,line in zip(cases,actual):
        balance,count,add=line.split('|');parts=list(map(int,balance.split()))
        assert parts==([1,*c['assignments']] if c['assignments'] is not None else [0]),('assignment',c,line)
        assert int(count)==(c['imbalance'] if c['imbalance'] is not None else 777),('count',c,line)
        assert bool(int(add))==c['can_add'],('can_add',c,line)
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    for at in executed:
        instruction=next(decoder.disasm(new.read(at,15),at));assert bytes(machine.mem_read(at,instruction.size))==instruction.bytes
    fixture=dict(schema='H2_TU15_TEAM_BALANCE_ORIGINAL_V1',original_sha256=NEW_PIN,mapping=mapping,cases=cases)
    save(args.fixture,fixture)
    report=dict(passed=True,cases=len(cases),whole_original_calls=3*len(cases),source_sha256=NEW_PIN,
        mapping=mapping,original_instructions=len(executed),tool_sha256=sha(__file__),
        native_source_sha256=sha(ROOT/'rust/online_protocol/src/team_balance.rs'),
        fixture_sha256=sha(args.fixture) if args.fixture else None,
        instruction_patches=[],replaced_callees=[],scope=__doc__,scratch=str(scratch),scratch_retained=False)
    save(args.report,report);print(json.dumps(report))


if __name__=='__main__':main()
